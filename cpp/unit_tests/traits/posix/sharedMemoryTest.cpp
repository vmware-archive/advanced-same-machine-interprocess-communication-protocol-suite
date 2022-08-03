#include "toroni/traits/posix/sharedMemory.hpp"

#include <gtest/gtest.h>

using namespace toroni::traits;

TEST(SharedMemory, OpenNonExistingThrows) {
  EXPECT_ANY_THROW(SharedMemory::Open("OpenNonExistingThrows", 10));
  EXPECT_ANY_THROW(SharedMemory::OpenReadOnly("OpenNonExistingThrows", 10));
}

TEST(SharedMemory, CreateOrOpenReadOnlyThrows) {
  EXPECT_ANY_THROW(SharedMemory::CreateOrOpenReadOnly(
                       "OpenOrCreateReadOnlyThrows", 16, 0600););
}

TEST(SharedMemory, CreateOrOpenUnlinkDoesNotThrow) {
  EXPECT_NO_THROW({
    SharedMemory shm =
        SharedMemory::CreateOrOpen("OpenOrCreateReadOnlyThrows", 16, 0600);
    shm.Unlink();
  });
}

TEST(SharedMemory, CreateOrOpenOpenUnlinkOpen) {
  SharedMemory shm =
      SharedMemory::CreateOrOpen("CreateOrOpenOpenUnlinkOpen", 16, 0600);

  // Open existing
  EXPECT_NO_THROW(SharedMemory::Open("CreateOrOpenOpenUnlinkOpen", 10));

  shm.Unlink();

  // Open non-existing
  EXPECT_ANY_THROW(SharedMemory::Open("CreateOrOpenOpenUnlinkOpen", 10));
}

TEST(SharedMemory, TransferData) {
  SharedMemory shm1 = SharedMemory::CreateOrOpen("TransferData", 16, 0600);
  SharedMemory shm2 =
      SharedMemory::CreateOrOpenReadOnly("TransferData", 16, 0600);

  *static_cast<int *>(shm1.Ptr()) = 20;
  EXPECT_EQ(*static_cast<int *>(shm2.Ptr()), 20);

  shm1.Unlink();
  EXPECT_ANY_THROW(shm2.Unlink());
}

TEST(SharedMemoryDeathTest, WriteToReadOnlyDies) {
  SharedMemory shm1 =
      SharedMemory::CreateOrOpen("WriteToReadOnlyDies", 16, 0600);
  SharedMemory shm2 =
      SharedMemory::CreateOrOpenReadOnly("WriteToReadOnlyDies", 16, 0600);

  EXPECT_DEATH(*static_cast<int *>(shm2.Ptr()) = 20, "");

  shm1.Unlink();
}

TEST(SharedMemoryDeathTest, WriteToUnmappedDies) {
  SharedMemory shm =
      SharedMemory::CreateOrOpen("WriteToUnmappedDies", 16, 0600);
  shm.Unmap();
  EXPECT_DEATH(*static_cast<int *>(shm.Ptr()) = 42, "");
  shm.Unlink();
}

TEST(SharedMemory, WriteBiggerSizeDies) {
  SharedMemory shm =
      SharedMemory::CreateOrOpen("WriteBiggerSizeDies", 16, 0600);
  EXPECT_NO_FATAL_FAILURE(memset(shm.Ptr(), 42, 16));
  shm.Unlink();
}

TEST(SharedMemory, IsCreator) {
  SharedMemory shm1 = SharedMemory::CreateOrOpen("IsCreator", 16, 0600);
  SharedMemory shm2 = SharedMemory::CreateOrOpen("IsCreator", 16, 0600);
  SharedMemory shm3 = SharedMemory::CreateOrOpen("IsCreator", 16, 0600);
  EXPECT_EQ(shm1.IsCreator(), true);
  EXPECT_EQ(shm2.IsCreator(), false);
  EXPECT_EQ(shm3.IsCreator(), false);

  shm1.Unlink();
}