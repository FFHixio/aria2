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
#ifndef _D_METALINK_PARSER_STATE_MACHINE_H_
#define _D_METALINK_PARSER_STATE_MACHINE_H_

#include "common.h"
#include "SharedHandle.h"
#include <string>
#include <map>

namespace aria2 {

class MetalinkParserController;
class MetalinkParserState;
class SkipTagMetalinkParserState;
class Metalinker;

class MetalinkParserStateMachine {
private:
  SharedHandle<MetalinkParserController> _ctrl;

  MetalinkParserState* _state;

  static MetalinkParserState* _initialState;
  static MetalinkParserState* _metalinkState;
  static MetalinkParserState* _filesState;
  static MetalinkParserState* _fileState;
  static MetalinkParserState* _sizeState;
  static MetalinkParserState* _versionState;
  static MetalinkParserState* _languageState;
  static MetalinkParserState* _osState;
  static MetalinkParserState* _verificationState;
  static MetalinkParserState* _hashState;
  static MetalinkParserState* _piecesState;
  static MetalinkParserState* _pieceHashState;
  static MetalinkParserState* _resourcesState;
  static MetalinkParserState* _urlState;
  static MetalinkParserState* _finState;

  SkipTagMetalinkParserState* _skipTagState;
public:
  MetalinkParserStateMachine();

  ~MetalinkParserStateMachine();

  void setMetalinkState();

  void setFilesState();

  void setFileState();

  void setSizeState();

  void setVersionState();

  void setLanguageState();
  
  void setOSState();

  void setVerificationState();

  void setHashState();

  void setPiecesState();

  void setPieceHashState();

  void setResourcesState();

  void setURLState();

  void setFinState();

  void setSkipTagState(MetalinkParserState* prevSate);

  void restoreSavedState();
  
  bool finished() const;

  bool error() const;

  void beginElement(const std::string& name, const std::map<std::string, std::string>& attrs);
  
  void endElement(const std::string& name, const std::string& characters);

  void newEntryTransaction();

  void setFileNameOfEntry(const std::string& filename);

  void setFileLengthOfEntry(int64_t length);

  void setVersionOfEntry(const std::string& version);

  void setLanguageOfEntry(const std::string& language);

  void setOSOfEntry(const std::string& os);

  void setMaxConnectionsOfEntry(int32_t maxConnections);

  void commitEntryTransaction();

  void newResourceTransaction();

  void setURLOfResource(const std::string& url);

  void setTypeOfResource(const std::string& type);

  void setLocationOfResource(const std::string& location);

  void setPreferenceOfResource(int32_t preference);

  void setMaxConnectionsOfResource(int32_t maxConnections);

  void commitResourceTransaction();

  void cancelResourceTransaction();

  void newChecksumTransaction();

  void setTypeOfChecksum(const std::string& type);

  void setHashOfChecksum(const std::string& md);

  void commitChecksumTransaction();

  void cancelChecksumTransaction();

  void newChunkChecksumTransaction();

  void setLengthOfChunkChecksum(int32_t length);

  void setTypeOfChunkChecksum(const std::string& type);

  void createNewHashOfChunkChecksum(int32_t order);

  void setMessageDigestOfChunkChecksum(const std::string& md);

  void addHashOfChunkChecksum();

  void commitChunkChecksumTransaction();

  void cancelChunkChecksumTransaction();

  SharedHandle<Metalinker> getResult() const;
};

} //  namespace aria2

#endif // _D_METALINK_PARSER_STATE_MACHINE_H_
