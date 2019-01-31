cmake_minimum_required(VERSION 2.6)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Locate GTest
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

add_executable(runTestsIntIterate testsIntIterate.cpp)
add_executable(runTestsIntInsertGet testsIntInsertGet.cpp)
add_executable(runTestsIntInsertRemove testsIntInsertRemove.cpp)
add_executable(runTestsIntInsertGetParallel testsIntInsertGetParallel.cpp)
add_executable(runTestsIntInsertRemoveParallel testsIntInsertRemoveParallel.cpp)

add_executable(runTestsStringIterate testsStringIterate.cpp)
add_executable(runTestsStringInsertGet testsStringInsertGet.cpp)
add_executable(runTestsStringInsertRemove testsStringInsertRemove.cpp)
add_executable(runTestsStringInsertGetParallel testsStringInsertGetParallel.cpp)
add_executable(runTestsStringInsertRemoveParallel testsStringInsertRemoveParallel.cpp)

add_executable(runTestsPerformance1 testsPerformance1.cpp)
add_executable(runTestsPerformance2 testsPerformance2.cpp)
add_executable(runTestsPerformance4 testsPerformance4.cpp)
add_executable(runTestsPerformance8 testsPerformance8.cpp)
add_executable(runTestsPerformance16 testsPerformance16.cpp)
add_executable(runTestsPerformanceHashMap testsPerformanceHashMap.cpp)

# Link runTests with what we want to test and the libraries
target_link_libraries(runTestsIntIterate ${GTEST_LIBRARIES} pthread  pmemobj pmem)
target_link_libraries(runTestsIntInsertGet ${GTEST_LIBRARIES} pthread  pmemobj pmem)
target_link_libraries(runTestsIntInsertRemove ${GTEST_LIBRARIES} pthread  pmemobj pmem)
target_link_libraries(runTestsIntInsertGetParallel ${GTEST_LIBRARIES} pthread  pmemobj pmem)
target_link_libraries(runTestsIntInsertRemoveParallel ${GTEST_LIBRARIES} pthread  pmemobj pmem)

target_link_libraries(runTestsStringIterate ${GTEST_LIBRARIES} pthread  pmemobj pmem)
target_link_libraries(runTestsStringInsertGet ${GTEST_LIBRARIES} pthread  pmemobj pmem)
target_link_libraries(runTestsStringInsertRemove ${GTEST_LIBRARIES} pthread  pmemobj pmem)
target_link_libraries(runTestsStringInsertGetParallel ${GTEST_LIBRARIES} pthread  pmemobj pmem)
target_link_libraries(runTestsStringInsertRemoveParallel ${GTEST_LIBRARIES} pthread  pmemobj pmem)

target_link_libraries(runTestsPerformance1 ${GTEST_LIBRARIES} pthread  pmemobj pmem)
target_link_libraries(runTestsPerformance2 ${GTEST_LIBRARIES} pthread  pmemobj pmem)
target_link_libraries(runTestsPerformance4 ${GTEST_LIBRARIES} pthread  pmemobj pmem)
target_link_libraries(runTestsPerformance8 ${GTEST_LIBRARIES} pthread  pmemobj pmem)
target_link_libraries(runTestsPerformance16 ${GTEST_LIBRARIES} pthread  pmemobj pmem)
target_link_libraries(runTestsPerformanceHashMap ${GTEST_LIBRARIES} pthread)