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
#include "Peer.h"
#include "BitfieldManFactory.h"
#include "BitfieldMan.h"
#include "Util.h"
#ifdef ENABLE_MESSAGE_DIGEST
# include "MessageDigestHelper.h"
#endif // ENABLE_MESSAGE_DIGEST
#include <cstring>
#include <cassert>
#include <algorithm>

namespace aria2 {

#define BAD_CONDITION_INTERVAL 10

Peer::Peer(std::string ipaddr, uint16_t port):
  ipaddr(ipaddr),
  port(port),
  _bitfield(0),
  sessionUploadLength(0),
  sessionDownloadLength(0),
  active(false),
  _badConditionStartTime(0),
  _seeder(false)
{
  resetStatus();
  std::string idSeed = ipaddr+":"+Util::itos(port);
#ifdef ENABLE_MESSAGE_DIGEST
  id = MessageDigestHelper::digestString("sha1", idSeed);
#else
  id = idSeed;
#endif // ENABLE_MESSAGE_DIGEST
}

Peer::~Peer()
{
  delete _bitfield;
}

bool Peer::operator==(const Peer& p)
{
  return id == p.id;
}
  
bool Peer::operator!=(const Peer& p)
{
  return !(*this == p);
}

void Peer::allocateBitfield(int32_t pieceLength, int64_t totalLength)
{
  delete _bitfield;
  _bitfield = BitfieldManFactory::getFactoryInstance()->createBitfieldMan(pieceLength, totalLength);  
}

void Peer::deallocateBitfield()
{
  delete _bitfield;
  _bitfield = 0;
}

void Peer::updateUploadLength(int32_t bytes)
{
  peerStat.updateUploadLength(bytes);
  sessionUploadLength += bytes;
}

void Peer::updateDownloadLength(int32_t bytes)
{
  peerStat.updateDownloadLength(bytes);
  sessionDownloadLength += bytes;
}

void Peer::updateSeeder()
{
  assert(_bitfield);
  if(_bitfield->isAllBitSet()) {
    _seeder = true;
  }  
}

void Peer::updateBitfield(int32_t index, int32_t operation) {
  assert(_bitfield);
  if(operation == 1) {
    _bitfield->setBit(index);
  } else if(operation == 0) {
    _bitfield->unsetBit(index);
  }
  updateSeeder();
}

int32_t Peer::calculateUploadSpeed()
{
  return peerStat.calculateUploadSpeed();
}

int32_t Peer::calculateUploadSpeed(const struct timeval& now)
{
  return peerStat.calculateUploadSpeed(now);
}

int32_t Peer::calculateDownloadSpeed()
{
  return peerStat.calculateDownloadSpeed();
}

int32_t Peer::calculateDownloadSpeed(const struct timeval& now)
{
  return peerStat.calculateDownloadSpeed(now);
}

int64_t Peer::getSessionUploadLength() const
{
  return sessionUploadLength;
}

int64_t Peer::getSessionDownloadLength() const
{
  return sessionDownloadLength;
}

void Peer::activate()
{
  peerStat.downloadStart();
  active = true;
}

void Peer::deactivate()
{
  peerStat.downloadStop();
  active = false;
}

bool Peer::isActive() const
{
  return active;
}

void Peer::setPeerId(const unsigned char* peerId)
{
  memcpy(this->peerId, peerId, PEER_ID_LENGTH);
}

const unsigned char* Peer::getPeerId() const
{
  return this->peerId;
}
  
void Peer::setBitfield(const unsigned char* bitfield, int32_t bitfieldLength)
{
  assert(_bitfield);
  _bitfield->setBitfield(bitfield, bitfieldLength);
  updateSeeder();
}

const unsigned char* Peer::getBitfield() const
{
  assert(_bitfield);
  return _bitfield->getBitfield();
}

int32_t Peer::getBitfieldLength() const
{
  assert(_bitfield);
  return _bitfield->getBitfieldLength();
}

#define THRESHOLD 1024*1024*2

bool Peer::shouldBeChoking() const {
  if(optUnchoking) {
    return false;
  }
  return chokingRequired;
}

bool Peer::hasPiece(int32_t index) const {
  assert(_bitfield);
  return _bitfield->isBitSet(index);
}

void Peer::resetStatus() {
  tryCount = 0;
  cuid = 0;
  amChoking = true;
  amInterested = false;
  peerChoking = true;
  peerInterested = false;
  chokingRequired = true;
  optUnchoking = false;
  snubbing = false;
  fastExtensionEnabled = false;
  _extendedMessagingEnabled = false;
  _extensions.clear();
  _dhtEnabled = false;
  latency = DEFAULT_LATENCY;
  peerAllowedIndexSet.clear();
  amAllowedIndexSet.clear();
  peerStat.reset();
}

void Peer::setFastExtensionEnabled(bool enabled)
{
  fastExtensionEnabled = enabled;
}

bool Peer::isFastExtensionEnabled() const
{
  return fastExtensionEnabled;
}

int32_t Peer::countPeerAllowedIndexSet() const
{
  return peerAllowedIndexSet.size();
}

const std::deque<int32_t>& Peer::getPeerAllowedIndexSet() const
{
  return peerAllowedIndexSet;
}

bool Peer::isInPeerAllowedIndexSet(int32_t index) const {
  return std::find(peerAllowedIndexSet.begin(), peerAllowedIndexSet.end(),
		   index) != peerAllowedIndexSet.end();
}

void Peer::addPeerAllowedIndex(int32_t index) {
  if(!isInPeerAllowedIndexSet(index)) {
    peerAllowedIndexSet.push_back(index);
  }
}

bool Peer::isInAmAllowedIndexSet(int32_t index) const {
  return std::find(amAllowedIndexSet.begin(), amAllowedIndexSet.end(),
		   index) != amAllowedIndexSet.end();
}

void Peer::addAmAllowedIndex(int32_t index) {
  if(!isInAmAllowedIndexSet(index)) {
    amAllowedIndexSet.push_back(index);
  }
}

void Peer::setAllBitfield() {
  assert(_bitfield);
  _bitfield->setAllBit();
  _seeder = true;
}

void Peer::updateLatency(int32_t latency) {
  this->latency = (this->latency*20+latency*80)/200;
}

void Peer::startBadCondition()
{
  _badConditionStartTime.reset();
}

bool Peer::isGood() const
{
  return _badConditionStartTime.elapsed(BAD_CONDITION_INTERVAL);
}

uint8_t Peer::getExtensionMessageID(const std::string& name)
{
  Extensions::const_iterator itr = _extensions.find(name);
  if(itr == _extensions.end()) {
    return 0;
  } else {
    return (*itr).second;
  }
}

std::string Peer::getExtensionName(uint8_t id)
{
  for(Extensions::const_iterator itr = _extensions.begin();
      itr != _extensions.end(); ++itr) {
    const Extensions::value_type& p = *itr;
    if(p.second == id) {
      return p.first;
    }
  }
  return "";
}

void Peer::setExtension(const std::string& name, uint8_t id)
{
  _extensions[name] = id;
}

} // namespace aria2
