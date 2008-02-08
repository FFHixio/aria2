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
#ifndef _D_FTP_CONNECTION_H_
#define _D_FTP_CONNECTION_H_

#include "common.h"
#include "SharedHandle.h"
#include <utility>
#include <string>

namespace aria2 {

class Option;
class Logger;
class Segment;
class Request;
class SocketCore;

class FtpConnection {
private:
  int32_t cuid;
  SharedHandle<SocketCore> socket;
  SharedHandle<Request> req;
  const Option* option;
  const Logger* logger;

  std::string strbuf;

  int32_t getStatus(const std::string& response) const;
  bool isEndOfResponse(int32_t status, const std::string& response) const;
  bool bulkReceiveResponse(std::pair<int32_t, std::string>& response);
public:
  FtpConnection(int32_t cuid, const SharedHandle<SocketCore>& socket,
		const SharedHandle<Request>& req, const Option* op);
  ~FtpConnection();
  void sendUser() const;
  void sendPass() const;
  void sendType() const;
  void sendCwd() const;
  void sendSize() const;
  void sendPasv() const;
  SharedHandle<SocketCore> sendPort() const;
  void sendRest(const SharedHandle<Segment>& segment) const;
  void sendRetr() const;

  int32_t receiveResponse();
  int32_t receiveSizeResponse(int64_t& size);
  int32_t receivePasvResponse(std::pair<std::string, int32_t>& dest);
};

} // namespace aria2

#endif // _D_FTP_CONNECTION_H_
