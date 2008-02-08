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
#ifndef _D_UTIL_H_
#define _D_UTIL_H_

#include "common.h"
#include "SharedHandle.h"
#include "IntSequence.h"
#include <sys/time.h>
#include <string>
#include <utility>
#include <deque>
#include <iosfwd>

namespace aria2 {

class Randomizer;
class BitfieldMan;
class BinaryStream;
class FileEntry;

#define STRTOLL(X) strtoll(X, (char**)NULL, 10)

#define START_INDEX(OFFSET, PIECE_LENGTH) ((OFFSET)/(PIECE_LENGTH))
#define END_INDEX(OFFSET, LENGTH, PIECE_LENGTH) (((OFFSET)+(LENGTH)-1)/(PIECE_LENGTH))

#define DIV_FLOOR(X,Y) ((X)/(Y)+((X)%(Y)? 1:0))

class Util {
public:
  static void split(std::pair<std::string, std::string>& hp,
		    const std::string& src, char delim);

  static std::pair<std::string, std::string>
  split(const std::string& src, const std::string& delims);

  static std::string llitos(int64_t value, bool comma = false);
  static std::string ullitos(uint64_t value, bool comma = false);
  static std::string itos(int32_t value, bool comma = false);
  static std::string uitos(uint32_t value, bool comma = false);
  static std::string itos(int16_t value, bool comma = false);
  static std::string uitos(uint16_t value, bool comma = false);

  /**
   * Computes difference in micro-seconds between tv1 and tv2,
   * assuming tv1 is newer than tv2.
   * If tv1 is older than tv2, then this method returns 0.
   */
  static int64_t difftv(struct timeval tv1, struct timeval tv2);
  static int32_t difftvsec(struct timeval tv1, struct timeval tv2);
  /**
   * Take a string src which is a deliminated list and add its elements
   * into result. result is not cleared before conversion begins.
   */
  static void slice(std::deque<std::string>& result, const std::string& src,
		    char delim, bool trim = false);
  
  static std::string trim(const std::string& src, const std::string& trimCharset = "\r\n\t ");

  static bool startsWith(const std::string& target, const std::string& part);

  static bool endsWith(const std::string& target, const std::string& part);

  static std::string replace(const std::string& target, const std::string& oldstr, const std::string& newstr);

  static std::string urlencode(const unsigned char* target, int32_t len);

  static std::string urlencode(const std::string& target)
  {
    return urlencode((const unsigned char*)target.c_str(), target.size());
  }

  static bool shouldUrlencode(const char c);

  static std::string urldecode(const std::string& target);

  static std::string torrentUrlencode(const unsigned char* target, int32_t len);

  static std::string toHex(const unsigned char* src, int32_t len);

  static std::string toHex(const std::string& src)
  {
    return toHex(reinterpret_cast<const unsigned char*>(src.c_str()), src.size());
  }

  static FILE* openFile(const std::string& filename, const std::string& mode);

  static void fileCopy(const std::string& destFile, const std::string& src);

  static void rangedFileCopy(const std::string& destFile, const std::string& src, int64_t srcOffset, int64_t length);

  static bool isPowerOf(int32_t num, int32_t base);

  static std::string secfmt(int32_t sec);

  static int32_t expandBuffer(char** pbuf, int32_t curLength, int32_t newLength);

  static void unfoldRange(const std::string& src, std::deque<int32_t>& range);

  static int32_t parseInt(const std::string& s, int32_t base = 10);

  static int64_t parseLLInt(const std::string& s, int32_t base = 10);

  static IntSequence parseIntRange(const std::string& src);

  // this function temporarily put here
  static std::string getContentDispositionFilename(const std::string& header);

  static int32_t countBit(uint32_t n);
  
  static std::string randomAlpha(int32_t length,
				 const SharedHandle<Randomizer>& randomizer);
  
  static std::string toUpper(const std::string& src);

  static std::string toLower(const std::string& src);

  static bool isNumbersAndDotsNotation(const std::string& name);

  static void setGlobalSignalHandler(int32_t signal, void (*handler)(int), int32_t flags);

  static void indexRange(int32_t& startIndex, int32_t& endIndex,
			 int64_t offset,
			 int32_t srcLength, int32_t destLength);

  static std::string getHomeDir();

  static int64_t getRealSize(const std::string& sizeWithUnit);

  static std::string abbrevSize(int64_t size);

  static time_t httpGMT(const std::string& httpTimeFormat);

  static void toStream(std::ostream& os,
		       const std::deque<SharedHandle<FileEntry> >& entries);

  static void sleep(long seconds);

  static void usleep(long microseconds);

  static bool isNumber(const std::string& what);

  static bool isLowercase(const std::string& what);

  static bool isUppercase(const std::string& what);

  static int32_t alphaToNum(const std::string& alphabets);

  static void mkdirs(const std::string& dirpath);

  static void convertBitfield(BitfieldMan* dest, const BitfieldMan* src);

  // binaryStream has to be opened before calling this function.
  static std::string toString(const SharedHandle<BinaryStream>& binaryStream);

#ifdef HAVE_POSIX_MEMALIGN
  static void* allocateAlignedMemory(size_t alignment, size_t size);
#endif // HAVE_POSIX_MEMALIGN
};

} // namespace aria2

#endif // _D_UTIL_H_
