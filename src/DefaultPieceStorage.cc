/* <!-- copyright */
/*
 * aria2 - The high speed download utility
 *
 * Copyright (C) 2006 Tatsuhiro Tsujikawa
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */
/* copyright --> */
#include "DefaultPieceStorage.h"
#include "DownloadContext.h"
#include "Piece.h"
#include "Peer.h"
#include "LogFactory.h"
#include "Logger.h"
#include "prefs.h"
#include "DirectDiskAdaptor.h"
#include "MultiDiskAdaptor.h"
#include "CopyDiskAdaptor.h"
#include "DiskWriter.h"
#include "BitfieldManFactory.h"
#include "BitfieldMan.h"
#include "message.h"
#include "DefaultDiskWriterFactory.h"
#include "FileEntry.h"
#include "DlAbortEx.h"
#include "Util.h"
#include "a2functional.h"
#include "Option.h"
#include <numeric>
#include <algorithm>

namespace aria2 {

DefaultPieceStorage::DefaultPieceStorage(const DownloadContextHandle& downloadContext, const Option* option):
  downloadContext(downloadContext),
  diskAdaptor(0),
  _diskWriterFactory(new DefaultDiskWriterFactory()),
  endGamePieceNum(END_GAME_PIECE_NUM),
  option(option)
{
  bitfieldMan =
    BitfieldManFactory::getFactoryInstance()->
    createBitfieldMan(downloadContext->getPieceLength(),
		      downloadContext->getTotalLength());
  logger = LogFactory::getInstance();
}

DefaultPieceStorage::~DefaultPieceStorage() {
  delete bitfieldMan;
}

bool DefaultPieceStorage::hasMissingPiece(const PeerHandle& peer)
{
  return bitfieldMan->hasMissingPiece(peer->getBitfield(),
				      peer->getBitfieldLength());
}

bool DefaultPieceStorage::isEndGame()
{
  return bitfieldMan->countMissingBlock() <= endGamePieceNum;
}

int32_t DefaultPieceStorage::getMissingPieceIndex(const PeerHandle& peer)
{
  int32_t index = -1;
  if(isEndGame()) {
    index = bitfieldMan->getMissingIndex(peer->getBitfield(),
					 peer->getBitfieldLength());
  } else {
    index = bitfieldMan->getMissingUnusedIndex(peer->getBitfield(),
					       peer->getBitfieldLength());
  }
  return index;
}

PieceHandle DefaultPieceStorage::checkOutPiece(int32_t index)
{
  if(index == -1) {
    return 0;
  }
  bitfieldMan->setUseBit(index);

  PieceHandle piece = findUsedPiece(index);
  if(piece.isNull()) {
    piece = new Piece(index, bitfieldMan->getBlockLength(index));
    addUsedPiece(piece);
    return piece;
  } else {
    return piece;
  }
}

/**
 * Newly instantiated piece is not added to usedPieces.
 * Because it is waste of memory and there is no chance to use them later.
 */
PieceHandle DefaultPieceStorage::getPiece(int32_t index)
{
  if(0 <= index && index <= bitfieldMan->getMaxIndex()) {
    PieceHandle piece = findUsedPiece(index);
    if(piece.isNull()) {
      piece = new Piece(index, bitfieldMan->getBlockLength(index));
      if(hasPiece(index)) {
	piece->setAllBlock();
      }
    }
    return piece;
  } else {
    return 0;
  }
}

void DefaultPieceStorage::addUsedPiece(const PieceHandle& piece)
{
  usedPieces.push_back(piece);
}

class FindPiece {
private:
  int32_t index;
public:
  FindPiece(int32_t index):index(index) {}

  bool operator()(const PieceHandle& piece) {
    return piece->getIndex() == index;
  }
};

PieceHandle DefaultPieceStorage::findUsedPiece(int32_t index) const
{
  Pieces::const_iterator itr = std::find_if(usedPieces.begin(),
					    usedPieces.end(),
					    FindPiece(index));
  if(itr == usedPieces.end()) {
    return 0;
  } else {
    return *itr;
  }
}

PieceHandle DefaultPieceStorage::getMissingPiece(const PeerHandle& peer)
{
  int32_t index = getMissingPieceIndex(peer);
  return checkOutPiece(index);
}

int32_t DefaultPieceStorage::getMissingFastPieceIndex(const PeerHandle& peer)
{
  int32_t index = -1;
  if(peer->isFastExtensionEnabled() && peer->countPeerAllowedIndexSet() > 0) {
    BitfieldMan tempBitfield(bitfieldMan->getBlockLength(),
			     bitfieldMan->getTotalLength());
    for(std::deque<int32_t>::const_iterator itr = peer->getPeerAllowedIndexSet().begin();
	itr != peer->getPeerAllowedIndexSet().end(); itr++) {
      if(!bitfieldMan->isBitSet(index) && peer->hasPiece(*itr)) {
	tempBitfield.setBit(*itr);
      }
    }
    if(isEndGame()) {
      index = bitfieldMan->getMissingIndex(tempBitfield.getBitfield(),
					   tempBitfield.getBitfieldLength());
    } else {
      index = bitfieldMan->getMissingUnusedIndex(tempBitfield.getBitfield(),
						 tempBitfield.getBitfieldLength());
    }
  }
  return index;
}

PieceHandle DefaultPieceStorage::getMissingFastPiece(const PeerHandle& peer)
{
  int32_t index = getMissingFastPieceIndex(peer);
  return checkOutPiece(index);
}

PieceHandle DefaultPieceStorage::getMissingPiece()
{
  return checkOutPiece(bitfieldMan->getSparseMissingUnusedIndex());
}

PieceHandle DefaultPieceStorage::getMissingPiece(const FileEntryHandle& fileEntry)
{
  BitfieldMan temp(*bitfieldMan);
  temp.clearFilter();
  temp.addFilter(fileEntry->getOffset(), fileEntry->getLength());
  temp.enableFilter();

  int32_t firstPieceIndex = START_INDEX(fileEntry->getOffset(), downloadContext->getPieceLength());
  int32_t endPieceIndex = END_INDEX(fileEntry->getOffset(), fileEntry->getLength(), downloadContext->getPieceLength());
  if(!temp.isBitSet(firstPieceIndex) && !temp.isUseBitSet(firstPieceIndex)) {
    PieceHandle piece = findUsedPiece(firstPieceIndex);
    if(!piece.isNull()) {
      if(piece->isRangeComplete(fileEntry->getOffset()-firstPieceIndex*downloadContext->getPieceLength(),
				fileEntry->getLength() > downloadContext->getPieceLength() ?
				downloadContext->getPieceLength():fileEntry->getLength())) {
	temp.setBit(firstPieceIndex);
      }
    }
  }
  if(firstPieceIndex != endPieceIndex && !temp.isBitSet(endPieceIndex) && !temp.isUseBitSet(endPieceIndex)) {
    PieceHandle piece = findUsedPiece(endPieceIndex);
    if(!piece.isNull()) {
      if(piece->isRangeComplete(0, fileEntry->getOffset()+fileEntry->getLength()-endPieceIndex*downloadContext->getPieceLength())) {
	temp.setBit(endPieceIndex);
      }
    }    
  }
  return checkOutPiece(temp.getSparseMissingUnusedIndex());
}

PieceHandle DefaultPieceStorage::getMissingPiece(int32_t index)
{
  if(hasPiece(index) || isPieceUsed(index)) {
    return 0;
  } else {
    return checkOutPiece(index);
  }
}

void DefaultPieceStorage::deleteUsedPiece(const PieceHandle& piece)
{
  if(piece.isNull()) {
    return;
  }
  Pieces::iterator itr = std::find(usedPieces.begin(), usedPieces.end(), piece);
  if(itr != usedPieces.end()) {
    usedPieces.erase(itr);
  }
}

void DefaultPieceStorage::reduceUsedPieces(int32_t delMax)
{
  int32_t toDelete = usedPieces.size()-delMax;
  if(toDelete <= 0) {
    return;
  }
  int32_t fillRate = 10;
  while(fillRate < 50) {
    int32_t deleted = deleteUsedPiecesByFillRate(fillRate, toDelete);
    if(deleted == 0) {
      break;
    }
    toDelete -= deleted;
    fillRate += 10;
  }
}

int32_t DefaultPieceStorage::deleteUsedPiecesByFillRate(int32_t fillRate,
							int32_t toDelete)
{
  int32_t deleted = 0;
  for(Pieces::iterator itr = usedPieces.begin();
      itr != usedPieces.end() && deleted < toDelete;) {
    PieceHandle& piece = *itr;
    if(!bitfieldMan->isUseBitSet(piece->getIndex()) &&
       piece->countCompleteBlock() <= piece->countBlock()*(fillRate/100.0)) {
      logger->debug(MSG_DELETING_USED_PIECE,
		    piece->getIndex(),
		    (piece->countCompleteBlock()*100)/piece->countBlock(),
		    fillRate);
      itr = usedPieces.erase(itr);
      deleted++;
    } else {
      itr++;
    }
  }
  return deleted;
}

void DefaultPieceStorage::completePiece(const PieceHandle& piece)
{
  if(piece.isNull()) {
    return;
  }
  deleteUsedPiece(piece);
  if(!isEndGame()) {
    reduceUsedPieces(100);
  }
  if(allDownloadFinished()) {
    return;
  }
  bitfieldMan->setBit(piece->getIndex());
  bitfieldMan->unsetUseBit(piece->getIndex());
  if(downloadFinished()) {
    diskAdaptor->onDownloadComplete();
    if(isSelectiveDownloadingMode()) {
      logger->notice(MSG_SELECTIVE_DOWNLOAD_COMPLETED);
      // following line was commented out in order to stop sending request
      // message after user-specified files were downloaded.
      //finishSelectiveDownloadingMode();
    } else {
      logger->info(MSG_DOWNLOAD_COMPLETED);
    }
  }
}

bool DefaultPieceStorage::isSelectiveDownloadingMode()
{
  return bitfieldMan->isFilterEnabled();
}

void DefaultPieceStorage::finishSelectiveDownloadingMode()
{
  bitfieldMan->clearFilter();
  diskAdaptor->addAllDownloadEntry();
}

// not unittested
void DefaultPieceStorage::cancelPiece(const PieceHandle& piece)
{
  if(piece.isNull()) {
    return;
  }
  bitfieldMan->unsetUseBit(piece->getIndex());
  if(!isEndGame()) {
    if(piece->getCompletedLength() == 0) {
      deleteUsedPiece(piece);
    }
  }
}

bool DefaultPieceStorage::hasPiece(int32_t index)
{
  return bitfieldMan->isBitSet(index);
}

bool DefaultPieceStorage::isPieceUsed(int32_t index)
{
  return bitfieldMan->isUseBitSet(index);
}

int64_t DefaultPieceStorage::getTotalLength()
{
  return bitfieldMan->getTotalLength();
}

int64_t DefaultPieceStorage::getFilteredTotalLength()
{
  return bitfieldMan->getFilteredTotalLength();
}

int64_t DefaultPieceStorage::getCompletedLength()
{
  return bitfieldMan->getCompletedLength()+getInFlightPieceCompletedLength();
}

int64_t DefaultPieceStorage::getFilteredCompletedLength()
{
  return bitfieldMan->getFilteredCompletedLength()+getInFlightPieceCompletedLength();
}

int32_t DefaultPieceStorage::getInFlightPieceCompletedLength() const
{
  return std::accumulate(usedPieces.begin(), usedPieces.end(), 0, adopt2nd(std::plus<int32_t>(), mem_fun_sh(&Piece::getCompletedLength)));
}

// not unittested
void DefaultPieceStorage::setFileFilter(const std::deque<std::string>& filePaths)
{
  if(downloadContext->getFileMode() != DownloadContext::MULTI || filePaths.empty()) {
    return;
  }
  diskAdaptor->removeAllDownloadEntry();
  for(std::deque<std::string>::const_iterator pitr = filePaths.begin();
      pitr != filePaths.end(); pitr++) {
    if(!diskAdaptor->addDownloadEntry(*pitr)) {
      throw new DlAbortEx(EX_NO_SUCH_FILE_ENTRY, (*pitr).c_str());
    }
    FileEntryHandle fileEntry = diskAdaptor->getFileEntryFromPath(*pitr);
    bitfieldMan->addFilter(fileEntry->getOffset(), fileEntry->getLength());
  }
  bitfieldMan->enableFilter();
}

void DefaultPieceStorage::setFileFilter(IntSequence seq)
{
  std::deque<int32_t> fileIndexes = seq.flush();
  // TODO Is sorting necessary?
  std::sort(fileIndexes.begin(), fileIndexes.end());
  fileIndexes.erase(std::unique(fileIndexes.begin(), fileIndexes.end()), fileIndexes.end());
  std::deque<std::string> filePaths;
  const FileEntries& entries = diskAdaptor->getFileEntries();
  for(int32_t i = 0; i < (int32_t)entries.size(); i++) {
    if(std::find(fileIndexes.begin(), fileIndexes.end(), i+1) != fileIndexes.end()) {
      logger->debug("index=%d is %s", i+1, entries[i]->getPath().c_str());
      filePaths.push_back(entries[i]->getPath());
    }
  }
  setFileFilter(filePaths);
}

// not unittested
void DefaultPieceStorage::clearFileFilter()
{
  bitfieldMan->clearFilter();
  diskAdaptor->addAllDownloadEntry();
}

// not unittested
bool DefaultPieceStorage::downloadFinished()
{
  // TODO iterate all requested FileEntry and Call bitfieldMan->isBitSetOffsetRange()
  return bitfieldMan->isFilteredAllBitSet();
}

// not unittested
bool DefaultPieceStorage::allDownloadFinished()
{
  return bitfieldMan->isAllBitSet();
}

// not unittested
void DefaultPieceStorage::initStorage()
{
  if(downloadContext->getFileMode() == DownloadContext::SINGLE) {
    logger->debug("Instantiating DirectDiskAdaptor");
    DiskWriterHandle writer = _diskWriterFactory->newDiskWriter();
    writer->setDirectIOAllowed(option->getAsBool(PREF_ENABLE_DIRECT_IO));
    DirectDiskAdaptorHandle directDiskAdaptor = new DirectDiskAdaptor();
    directDiskAdaptor->setDiskWriter(writer);
    directDiskAdaptor->setTotalLength(downloadContext->getTotalLength());
    this->diskAdaptor = directDiskAdaptor;
  } else {
    // file mode == DownloadContext::MULTI
    if(option->get(PREF_DIRECT_FILE_MAPPING) == V_TRUE) {
      logger->debug("Instantiating MultiDiskAdaptor");
      MultiDiskAdaptorHandle multiDiskAdaptor = new MultiDiskAdaptor();
      multiDiskAdaptor->setDirectIOAllowed(option->getAsBool(PREF_ENABLE_DIRECT_IO));
      multiDiskAdaptor->setPieceLength(downloadContext->getPieceLength());
      multiDiskAdaptor->setTopDir(downloadContext->getName());
      this->diskAdaptor = multiDiskAdaptor;
    } else {
      logger->debug("Instantiating CopyDiskAdaptor");
      DiskWriterHandle writer = _diskWriterFactory->newDiskWriter();
      writer->setDirectIOAllowed(option->getAsBool(PREF_ENABLE_DIRECT_IO));
      CopyDiskAdaptorHandle copyDiskAdaptor = new CopyDiskAdaptor();
      copyDiskAdaptor->setDiskWriter(writer);
      copyDiskAdaptor->setTempFilename(downloadContext->getName()+".a2tmp");
      copyDiskAdaptor->setTotalLength(downloadContext->getTotalLength());
      if(downloadContext->getFileMode() == DownloadContext::MULTI) {
	copyDiskAdaptor->setTopDir(downloadContext->getName());
      }
      this->diskAdaptor = copyDiskAdaptor;
    }
  }
  diskAdaptor->setStoreDir(downloadContext->getDir());
  diskAdaptor->setFileEntries(downloadContext->getFileEntries());
}

void DefaultPieceStorage::setBitfield(const unsigned char* bitfield,
				      int32_t bitfieldLength)
{
  bitfieldMan->setBitfield(bitfield, bitfieldLength);
}
  
int32_t DefaultPieceStorage::getBitfieldLength()
{
  return bitfieldMan->getBitfieldLength();
}

const unsigned char* DefaultPieceStorage::getBitfield()
{
  return bitfieldMan->getBitfield();
}

DiskAdaptorHandle DefaultPieceStorage::getDiskAdaptor() {
  return diskAdaptor;
}

int32_t DefaultPieceStorage::getPieceLength(int32_t index)
{
  return bitfieldMan->getBlockLength(index);
}

void DefaultPieceStorage::advertisePiece(int32_t cuid, int32_t index)
{
  HaveEntry entry(cuid, index);
  haves.push_front(entry);
}

std::deque<int32_t> DefaultPieceStorage::getAdvertisedPieceIndexes(int32_t myCuid,
							const Time& lastCheckTime)
{
  std::deque<int32_t> indexes;
  for(Haves::const_iterator itr = haves.begin(); itr != haves.end(); itr++) {
    const Haves::value_type& have = *itr;
    if(have.getCuid() == myCuid) {
      continue;
    }
    if(lastCheckTime.isNewer(have.getRegisteredTime())) {
      break;
    }
    indexes.push_back(have.getIndex());
  }
  return indexes;
}

class FindElapsedHave
{
private:
  int32_t elapsed;
public:
  FindElapsedHave(int32_t elapsed):elapsed(elapsed) {}

  bool operator()(const HaveEntry& have) {
    if(have.getRegisteredTime().elapsed(elapsed)) {
      return true;
    } else {
      return false;
    }
  }
};
  
void DefaultPieceStorage::removeAdvertisedPiece(int32_t elapsed)
{
  Haves::iterator itr =
    std::find_if(haves.begin(), haves.end(), FindElapsedHave(elapsed));
  if(itr != haves.end()) {
    logger->debug(MSG_REMOVED_HAVE_ENTRY, haves.end()-itr);
    haves.erase(itr, haves.end());
  }
}

void DefaultPieceStorage::markAllPiecesDone()
{
  bitfieldMan->setAllBit();
}

void DefaultPieceStorage::markPiecesDone(int64_t length)
{
  if(length == bitfieldMan->getTotalLength()) {
    bitfieldMan->setAllBit();
  } else {
    int32_t numPiece = length/bitfieldMan->getBlockLength();
    if(numPiece > 0) {
      bitfieldMan->setBitRange(0, numPiece-1);
    }
    int32_t r = (length%bitfieldMan->getBlockLength())/Piece::BLOCK_LENGTH;
    if(r > 0) {
      PieceHandle p = new Piece(numPiece, bitfieldMan->getBlockLength(numPiece));
      for(int32_t i = 0; i < r; ++i) {
	p->completeBlock(i);
      }
      addUsedPiece(p);
    }
  }
}

void DefaultPieceStorage::markPieceMissing(int32_t index)
{
  bitfieldMan->unsetBit(index);
}

void DefaultPieceStorage::addInFlightPiece(const Pieces& pieces)
{
  copy(pieces.begin(), pieces.end(), back_inserter(usedPieces));
}

int32_t DefaultPieceStorage::countInFlightPiece()
{
  return usedPieces.size();
}

Pieces DefaultPieceStorage::getInFlightPieces()
{
  return usedPieces;
}

void DefaultPieceStorage::setDiskWriterFactory(const DiskWriterFactoryHandle& diskWriterFactory)
{
  _diskWriterFactory = diskWriterFactory;
}

} // namespace aria2
