#include "../NvmHashMap.hpp"
#include "constants.hpp"
#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <ctime>
#include <string.h>
#include <fstream>
#include <unordered_map>

int threads_count = 16;
std::unordered_map<int, int> unorderedMap;
std::ofstream outFile;

void insertIntFromThreadUnordered(int tid)
{
    for(int i = ELEMENTS_COUNT; i >= 0; i--) {
        unorderedMap.insert(std::make_pair(i*threads_count+tid, i*threads_count+tid));
    }
}

void getIntFromThreadUnordered(int tid)
{
    for(int i = ELEMENTS_COUNT; i >= 0; i--) {
        unorderedMap.find(i*threads_count+tid);
    }
}

void removeIntFromThreadUnordered(int tid)
{

    for(int i = ELEMENTS_COUNT; i >= 0; i--) {
        unorderedMap.erase(i*threads_count+tid);
    }
}

TEST(UnorderedIntParallelPerformance, 16ThreadsInsertGetRemoveTest) {
    auto start = std::chrono::system_clock::now();
    std::thread t1(insertIntFromThreadUnordered, 0);
    t1.join();
    std::thread t2(insertIntFromThreadUnordered, 1);
    t2.join();
    std::thread t3(insertIntFromThreadUnordered, 2);
    t3.join();
    std::thread t4(insertIntFromThreadUnordered, 3);
    t4.join();
    std::thread t5(insertIntFromThreadUnordered, 4);
    t5.join();
    std::thread t6(insertIntFromThreadUnordered, 5);
    t6.join();
    std::thread t7(insertIntFromThreadUnordered, 6);
    t7.join();
    std::thread t8(insertIntFromThreadUnordered, 7);
    t8.join();
    std::thread t9(insertIntFromThreadUnordered, 8);
    t9.join();
    std::thread t10(insertIntFromThreadUnordered, 9);
    t10.join();
    std::thread t11(insertIntFromThreadUnordered, 10);
    t11.join();
    std::thread t12(insertIntFromThreadUnordered, 11);
    t12.join();
    std::thread t13(insertIntFromThreadUnordered, 12);
    t13.join();
    std::thread t14(insertIntFromThreadUnordered, 13);
    t14.join();
    std::thread t15(insertIntFromThreadUnordered, 14);
    t15.join();
    std::thread t16(insertIntFromThreadUnordered, 15);
    t16.join();
    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_time = end-start;
    outFile << elapsed_time.count() << std::endl;

    start = std::chrono::system_clock::now();
    t1 = std::thread(getIntFromThreadUnordered, 0);
    t2 = std::thread(getIntFromThreadUnordered, 1);
    t3 = std::thread(getIntFromThreadUnordered, 2);
    t4 = std::thread(getIntFromThreadUnordered, 3);
    t5 = std::thread(getIntFromThreadUnordered, 4);
    t6 = std::thread(getIntFromThreadUnordered, 5);
    t7 = std::thread(getIntFromThreadUnordered, 6);
    t8 = std::thread(getIntFromThreadUnordered, 7);
    t9 = std::thread(getIntFromThreadUnordered, 8);
    t10 = std::thread(getIntFromThreadUnordered, 9);
    t11 = std::thread(getIntFromThreadUnordered, 10);
    t12 = std::thread(getIntFromThreadUnordered, 11);
    t13 = std::thread(getIntFromThreadUnordered, 12);
    t14 = std::thread(getIntFromThreadUnordered, 13);
    t15 = std::thread(getIntFromThreadUnordered, 14);
    t16 = std::thread(getIntFromThreadUnordered, 15);
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();
    t6.join();
    t7.join();
    t8.join();
    t9.join();
    t10.join();
    t11.join();
    t12.join();
    t13.join();
    t14.join();
    t15.join();
    t16.join();
    end = std::chrono::system_clock::now();
    elapsed_time = end-start;
    outFile << elapsed_time.count() << std::endl;

    start = std::chrono::system_clock::now();
    t1 = std::thread(removeIntFromThreadUnordered, 0);
    t1.join();
    t2 = std::thread(removeIntFromThreadUnordered, 1);
    t2.join();
    t3 = std::thread(removeIntFromThreadUnordered, 2);
    t3.join();
    t4 = std::thread(removeIntFromThreadUnordered, 3);
    t4.join();
    t5 = std::thread(removeIntFromThreadUnordered, 4);
    t5.join();
    t6 = std::thread(removeIntFromThreadUnordered, 5);
    t6.join();
    t7 = std::thread(removeIntFromThreadUnordered, 6);
    t7.join();
    t8 = std::thread(removeIntFromThreadUnordered, 7);
    t8.join();
    t9 = std::thread(removeIntFromThreadUnordered, 8);
    t9.join();
    t10 = std::thread(removeIntFromThreadUnordered, 9);
    t10.join();
    t11 = std::thread(removeIntFromThreadUnordered, 10);
    t11.join();
    t12 = std::thread(removeIntFromThreadUnordered, 11);
    t12.join();
    t13 = std::thread(removeIntFromThreadUnordered, 12);
    t13.join();
    t14 = std::thread(removeIntFromThreadUnordered, 13);
    t14.join();
    t15 = std::thread(removeIntFromThreadUnordered, 14);
    t15.join();
    t16 = std::thread(removeIntFromThreadUnordered, 15);
    t16.join();
    end = std::chrono::system_clock::now();
    elapsed_time = end-start;
    outFile << elapsed_time.count() << std::endl;
}

TEST(UnorderedIntParallelPerformance, 8ThreadsInsertGetRemoveTest) {
    outFile << std::endl;
    threads_count = 8;

    auto start = std::chrono::system_clock::now();
    std::thread t1(insertIntFromThreadUnordered, 0);
    t1.join();
    std::thread t2(insertIntFromThreadUnordered, 1);
    t2.join();
    std::thread t3(insertIntFromThreadUnordered, 2);
    t3.join();
    std::thread t4(insertIntFromThreadUnordered, 3);
    t4.join();
    std::thread t5(insertIntFromThreadUnordered, 4);
    t5.join();
    std::thread t6(insertIntFromThreadUnordered, 5);
    t6.join();
    std::thread t7(insertIntFromThreadUnordered, 6);
    t7.join();
    std::thread t8(insertIntFromThreadUnordered, 7);
    t8.join();
    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_time = end-start;
    outFile << elapsed_time.count() << std::endl;

    start = std::chrono::system_clock::now();
    t1 = std::thread(getIntFromThreadUnordered, 0);
    t2 = std::thread(getIntFromThreadUnordered, 1);
    t3 = std::thread(getIntFromThreadUnordered, 2);
    t4 = std::thread(getIntFromThreadUnordered, 3);
    t5 = std::thread(getIntFromThreadUnordered, 4);
    t6 = std::thread(getIntFromThreadUnordered, 5);
    t7 = std::thread(getIntFromThreadUnordered, 6);
    t8 = std::thread(getIntFromThreadUnordered, 7);
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();
    t6.join();
    t7.join();
    t8.join();
    end = std::chrono::system_clock::now();
    elapsed_time = end-start;
    outFile << elapsed_time.count() << std::endl;

    start = std::chrono::system_clock::now();
    t1 = std::thread(removeIntFromThreadUnordered, 0);
    t1.join();
    t2 = std::thread(removeIntFromThreadUnordered, 1);
    t2.join();
    t3 = std::thread(removeIntFromThreadUnordered, 2);
    t3.join();
    t4 = std::thread(removeIntFromThreadUnordered, 3);
    t4.join();
    t5 = std::thread(removeIntFromThreadUnordered, 4);
    t5.join();
    t6 = std::thread(removeIntFromThreadUnordered, 5);
    t6.join();
    t7 = std::thread(removeIntFromThreadUnordered, 6);
    t7.join();
    t8 = std::thread(removeIntFromThreadUnordered, 7);
    t8.join();
    end = std::chrono::system_clock::now();
    elapsed_time = end-start;
    outFile << elapsed_time.count() << std::endl;
}

TEST(UnorderedIntParallelPerformance, 4ThreadsInsertGetRemoveTest) {
    outFile << std::endl;
    threads_count = 4;
    auto start = std::chrono::system_clock::now();
    std::thread t1(insertIntFromThreadUnordered, 0);
    t1.join();
    std::thread t2(insertIntFromThreadUnordered, 1);
    t2.join();
    std::thread t3(insertIntFromThreadUnordered, 2);
    t3.join();
    std::thread t4(insertIntFromThreadUnordered, 3);
    t4.join();
    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_time = end-start;
    outFile << elapsed_time.count() << std::endl;

    start = std::chrono::system_clock::now();
    t1 = std::thread(getIntFromThreadUnordered, 0);
    t2 = std::thread(getIntFromThreadUnordered, 1);
    t3 = std::thread(getIntFromThreadUnordered, 2);
    t4 = std::thread(getIntFromThreadUnordered, 3);
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    end = std::chrono::system_clock::now();
    elapsed_time = end-start;
    outFile << elapsed_time.count() << std::endl;

    start = std::chrono::system_clock::now();
    t1 = std::thread(removeIntFromThreadUnordered, 0);
    t1.join();
    t2 = std::thread(removeIntFromThreadUnordered, 1);
    t2.join();
    t3 = std::thread(removeIntFromThreadUnordered, 2);
    t3.join();
    t4 = std::thread(removeIntFromThreadUnordered, 3);
    t4.join();
    end = std::chrono::system_clock::now();
    elapsed_time = end-start;
    outFile << elapsed_time.count() << std::endl;
}

TEST(UnorderedIntParallelPerformance, 2ThreadsInsertGetRemoveTest) {
    threads_count = 2;
    outFile << std::endl;

    auto start = std::chrono::system_clock::now();
    std::thread t1(insertIntFromThreadUnordered, 0);
    t1.join();
    std::thread t2(insertIntFromThreadUnordered, 1);
    t2.join();
    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_time = end-start;
    outFile << elapsed_time.count() << std::endl;

    start = std::chrono::system_clock::now();
    t1 = std::thread(getIntFromThreadUnordered, 0);
    t2 = std::thread(getIntFromThreadUnordered, 1);
    t1.join();
    t2.join();
    end = std::chrono::system_clock::now();
    elapsed_time = end-start;
    outFile << elapsed_time.count() << std::endl;

    start = std::chrono::system_clock::now();
    t1 = std::thread(removeIntFromThreadUnordered, 0);
    t1.join();
    t2 = std::thread(removeIntFromThreadUnordered, 1);
    t2.join();
    end = std::chrono::system_clock::now();
    elapsed_time = end-start;
    outFile << elapsed_time.count() << std::endl;
}

TEST(UnorderedIntParallelPerformance, 1ThreadsInsertGetRemoveTest) {
    threads_count = 1;
    outFile << std::endl;

    auto start = std::chrono::system_clock::now();
    std::thread t1(insertIntFromThreadUnordered, 0);
    t1.join();
    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_time = end-start;
    outFile << elapsed_time.count() << std::endl;

    start = std::chrono::system_clock::now();
    t1 = std::thread(getIntFromThreadUnordered, 0);
    t1.join();
    end = std::chrono::system_clock::now();
    elapsed_time = end-start;
    outFile << elapsed_time.count() << std::endl;

    start = std::chrono::system_clock::now();
    t1 = std::thread(removeIntFromThreadUnordered, 0);
    t1.join();
    end = std::chrono::system_clock::now();
    elapsed_time = end-start;
    outFile << elapsed_time.count() << std::endl;
}

int main(int argc, char *argv[]) {
    outFile.open(UNORDERED_FILE, std::ios_base::out);

    outFile << "Unordered map - time [s]" << std::endl;
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();

}

