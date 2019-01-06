cmake_minimum_required(VERSION 2.6)

# Locate GTest
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

add_executable(runTestsInt testsInt.cpp)
add_executable(runTestsStr testsStr.cpp)
add_executable(runTestsPerformance16 testsPerformance16.cpp)
add_executable(runTestsPerformance8 testsPerformance8.cpp)
add_executable(runTestsPerformance4 testsPerformance4.cpp)
add_executable(runTestsPerformance2 testsPerformance2.cpp)
add_executable(runTestsPerformance1 testsPerformance1.cpp)
add_executable(runTestsPerformanceUnordered testsPerformanceUnordered.cpp)

# Link runTests with what we want to test and the GTest and pthread library
target_link_libraries(runTestsInt ${GTEST_LIBRARIES} pthread  pmemobj pmem)
target_link_libraries(runTestsStr ${GTEST_LIBRARIES} pthread  pmemobj pmem)
target_link_libraries(runTestsPerformance16 ${GTEST_LIBRARIES} pthread  pmemobj pmem)
target_link_libraries(runTestsPerformance8 ${GTEST_LIBRARIES} pthread  pmemobj pmem)
target_link_libraries(runTestsPerformance4 ${GTEST_LIBRARIES} pthread  pmemobj pmem)
target_link_libraries(runTestsPerformance2 ${GTEST_LIBRARIES} pthread  pmemobj pmem)
target_link_libraries(runTestsPerformance1 ${GTEST_LIBRARIES} pthread  pmemobj pmem)
target_link_libraries(runTestsPerformanceUnordered ${GTEST_LIBRARIES} pthread)
