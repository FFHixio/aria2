#include "ChunkedEncoding.h"
#include <fstream>
#include <iostream>
#include <string>
#include <cppunit/extensions/HelperMacros.h>

namespace aria2 {

class ChunkedEncodingTest:public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(ChunkedEncodingTest);
  CPPUNIT_TEST(testInflate1);
  CPPUNIT_TEST(testInflateLargeChunk);
  CPPUNIT_TEST_SUITE_END();
private:
  ChunkedEncoding* enc;
public:
  void setUp() {
    enc = new ChunkedEncoding();
    enc->init();
  }

  void testInflate1();
  void testInflateLargeChunk();

};


CPPUNIT_TEST_SUITE_REGISTRATION( ChunkedEncodingTest );

void ChunkedEncodingTest::testInflate1() {
  std::string msg = "a\r\n1234567890\r\n";
  char buf[100];
  int32_t len = sizeof(buf);
  enc->inflate(buf, len, msg.c_str(), msg.size());
  buf[len] = '\0';
  CPPUNIT_ASSERT_EQUAL(std::string("1234567890"), std::string(buf));
  // second pass
  len = sizeof(buf);
  msg = "3;extensionIgnored\r\n123\r\n0\r\n";
  enc->inflate(buf, len, msg.c_str(), msg.size());
  buf[len] = '\0';
  CPPUNIT_ASSERT_EQUAL(std::string("123"), std::string(buf));
  // input is over
  CPPUNIT_ASSERT(enc->finished());
}

void ChunkedEncodingTest::testInflateLargeChunk() {
  // give over 4096 character chunk
  std::fstream is("4096chunk.txt", std::ios::in);
  if(is.fail()) {
    CPPUNIT_FAIL("cannot open file 4096chunk.txt");
  }
  std::string body;
  is >> body;
  char buf[4097];
  int32_t len = sizeof(buf);
  for(int i = 0; i < 2; i++) {
    std::string msg = "1000\r\n"+body+"\r\n";
    len = sizeof(buf);
    enc->inflate(buf, len, msg.c_str(), msg.size());
    buf[len] = '\0';
    CPPUNIT_ASSERT_EQUAL(body, std::string(buf));
  }
  enc->inflate(buf, len, "0\r\n", 3);
  CPPUNIT_ASSERT_EQUAL((int32_t)0, len);
  CPPUNIT_ASSERT(enc->finished());
}

} // namespace aria2
