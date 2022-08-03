#include "byteRingBufferTest.hpp"

using namespace toroni::rmp::unit_tests;
TEST_F(ByteRingBufferTest, Initialized) {
  EXPECT_TRUE(ringBuf->initialized);
  EXPECT_EQ(ringBuf->configBufSizeBytes, ringBufSizeBytes);
  EXPECT_EQ(ringBuf->stats.backPressureCount, 0);
  EXPECT_EQ(ringBuf->stats.notificationCount, 0);
}

TEST_F(ByteRingBufferTest, SetGet) {
  for (int i = 0; i < ringBufSizeBytes; i++) {
    char v = i % 256;
    (*ringBuf)[i] = v;
  }

  for (int i = 0; i < ringBufSizeBytes; i++) {
    char v = i % 256;
    EXPECT_EQ((*ringBuf)[i], v);
  }
}
