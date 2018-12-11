cmake_minimum_required(VERSION 2.6)

# Locate GTest
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# Link runTests with what we want to test and the GTest and pthread library
add_executable(runTestsInt testsInt.cpp)
add_executable(runTestsStr testsStr.cpp)
add_executable(runTestsPerformance testsPerformance.cpp)

target_link_libraries(runTestsInt ${GTEST_LIBRARIES} pthread  pmemobj pmem)
target_link_libraries(runTestsStr ${GTEST_LIBRARIES} pthread  pmemobj pmem)
target_link_libraries(runTestsPerformance ${GTEST_LIBRARIES} pthread  pmemobj pmem)

