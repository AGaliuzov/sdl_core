#include "gmock/gmock.h"

extern "C" void __gcov_flush();

int main(int argc, char** argv) {
   testing::InitGoogleMock(&argc, argv);
   bool result = RUN_ALL_TESTS();
  __gcov_flush();
   return result;
}

