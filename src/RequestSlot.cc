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
#include "RequestSlot.h"
#include "Util.h"

namespace aria2 {

RequestSlot RequestSlot::nullSlot(0, 0, 0, 0);

RequestSlot::RequestSlot(int32_t index, int32_t begin, int32_t length, int32_t blockIndex)
  :index(index), begin(begin), length(length), blockIndex(blockIndex) {}

RequestSlot::RequestSlot(const RequestSlot& requestSlot) {
  copy(requestSlot);
}

void RequestSlot::copy(const RequestSlot& requestSlot) {
  index = requestSlot.index;
  begin = requestSlot.begin;
  length = requestSlot.length;
  blockIndex = requestSlot.blockIndex;
  dispatchedTime = requestSlot.dispatchedTime;
}

RequestSlot& RequestSlot::operator=(const RequestSlot& requestSlot)
{
  if(this != &requestSlot) {
    copy(requestSlot);
  }
  return *this;
}

bool RequestSlot::operator==(const RequestSlot& requestSlot) const
{
  return index == requestSlot.index &&
    begin == requestSlot.begin &&
    length == requestSlot.length;
}

void RequestSlot::setDispatchedTime() {
  dispatchedTime.reset();
}

void RequestSlot::setDispatchedTime(time_t secFromEpoch) {
  dispatchedTime.setTimeInSec(secFromEpoch);
}

bool RequestSlot::isTimeout(time_t timeoutSec) const {
  return dispatchedTime.elapsed(timeoutSec);
}

int32_t RequestSlot::getLatencyInMillis() const {
  return dispatchedTime.differenceInMillis();
}

bool RequestSlot::isNull(const RequestSlot& requestSlot) {
  return requestSlot.index == 0 && requestSlot.begin == 0&&
    requestSlot.length == 0;
}

} // namespace aria2
