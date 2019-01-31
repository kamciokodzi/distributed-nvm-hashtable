#include "../HashMap.hpp"
#include "constants.hpp"
#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <ctime>
#include <string.h>
#include <fstream>

int threads_count;
HashMap <int, int>* hashMap;
std::ofstream outFile;

void insertIntFromThread(int tid)
{
    for(int i = ELEMENTS_COUNT_PERFORMANCE/threads_count; i >= 0; i--) {
        try {
            hashMap->insertNew(i * threads_count + tid, i * threads_count + tid);
        } catch (...) {
            std::cout << "Not found" << std::endl;
        }
    }
}

void removeIntFromThread(int tid) {
    for(int i = ELEMENTS_COUNT_PERFORMANCE/threads_count; i >= 0; i--) {
        try {
            hashMap->remove(i * threads_count + tid);
        } catch (...) {
            std::cout << "Remove Not found" << std::endl;
        }
    }
}

void getIntFromThread(int tid)
{

    for(int i = ELEMENTS_COUNT_PERFORMANCE/threads_count; i >= 0; i--) {
        try {
            hashMap->get(i * threads_count + tid);
        } catch (...) {
            std::cout << "Get Not found" << std::endl;
        }
    }
}



TEST(HashMapIntParallelPerformance, 1ThreadsInsertGetRemoveTest) {
    threads_count = 1;
    hashMap = new HashMap<int, int>(threads_count);

    auto start = std::chrono::system_clock::now();
    std::thread t1(insertIntFromThread, 0);
    t1.join();
    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_time = end-start;
    outFile << elapsed_time.count() << std::endl;

    start = std::chrono::system_clock::now();
    t1 = std::thread(getIntFromThread, 0);
    t1.join();
    end = std::chrono::system_clock::now();
    elapsed_time = end-start;
    outFile << elapsed_time.count() << std::endl;

    start = std::chrono::system_clock::now();
    t1 = std::thread(removeIntFromThread, 0);
    t1.join();
    end = std::chrono::system_clock::now();
    elapsed_time = end-start;
    outFile << elapsed_time.count() << std::endl;
}

TEST(HashMapIntParallelPerformance, 2ThreadsInsertGetRemoveTest) {
    threads_count = 2;
    hashMap = new HashMap<int, int>(threads_count);

    outFile << std::endl;

    auto start = std::chrono::system_clock::now();
    std::thread t1(insertIntFromThread, 0);
    std::thread t2(insertIntFromThread, 1);
    t1.join();
    t2.join();
    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_time = end-start;
    outFile << elapsed_time.count() << std::endl;

    start = std::chrono::system_clock::now();
    t1 = std::thread(getIntFromThread, 0);
    t2 = std::thread(getIntFromThread, 1);
    t1.join();
    t2.join();
    end = std::chrono::system_clock::now();
    elapsed_time = end-start;
    outFile << elapsed_time.count() << std::endl;

    start = std::chrono::system_clock::now();
    t1 = std::thread(removeIntFromThread, 0);
    t2 = std::thread(removeIntFromThread, 1);
    t1.join();
    t2.join();
    end = std::chrono::system_clock::now();
    elapsed_time = end-start;
    outFile << elapsed_time.count() << std::endl;
}

TEST(HashMapIntParallelPerformance, 4ThreadsInsertGetRemoveTest) {
    threads_count = 4;
    hashMap = new HashMap<int, int>(threads_count);

    outFile << std::endl;
    auto start = std::chrono::system_clock::now();
    std::thread t1(insertIntFromThread, 0);
    std::thread t2(insertIntFromThread, 1);
    std::thread t3(insertIntFromThread, 2);
    std::thread t4(insertIntFromThread, 3);
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_time = end-start;
    outFile << elapsed_time.count() << std::endl;

    start = std::chrono::system_clock::now();
    t1 = std::thread(getIntFromThread, 0);
    t2 = std::thread(getIntFromThread, 1);
    t3 = std::thread(getIntFromThread, 2);
    t4 = std::thread(getIntFromThread, 3);
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    end = std::chrono::system_clock::now();
    elapsed_time = end-start;
    outFile << elapsed_time.count() << std::endl;

    start = std::chrono::system_clock::now();
    t1 = std::thread(removeIntFromThread, 0);
    t2 = std::thread(removeIntFromThread, 1);
    t3 = std::thread(removeIntFromThread, 2);
    t4 = std::thread(removeIntFromThread, 3);
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    end = std::chrono::system_clock::now();
    elapsed_time = end-start;
    outFile << elapsed_time.count() << std::endl;
}

TEST(HashMapIntParallelPerformance, 8ThreadsInsertGetRemoveTest) {
    threads_count = 8;
    hashMap = new HashMap<int, int>(threads_count);

    outFile << std::endl;

    auto start = std::chrono::system_clock::now();
    std::thread t1(insertIntFromThread, 0);
    std::thread t2(insertIntFromThread, 1);
    std::thread t3(insertIntFromThread, 2);
    std::thread t4(insertIntFromThread, 3);
    std::thread t5(insertIntFromThread, 4);
    std::thread t6(insertIntFromThread, 5);
    std::thread t7(insertIntFromThread, 6);
    std::thread t8(insertIntFromThread, 7);
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();
    t6.join();
    t7.join();
    t8.join();
    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_time = end-start;
    outFile << elapsed_time.count() << std::endl;

    start = std::chrono::system_clock::now();
    t1 = std::thread(getIntFromThread, 0);
    t2 = std::thread(getIntFromThread, 1);
    t3 = std::thread(getIntFromThread, 2);
    t4 = std::thread(getIntFromThread, 3);
    t5 = std::thread(getIntFromThread, 4);
    t6 = std::thread(getIntFromThread, 5);
    t7 = std::thread(getIntFromThread, 6);
    t8 = std::thread(getIntFromThread, 7);
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
    t1 = std::thread(removeIntFromThread, 0);
    t2 = std::thread(removeIntFromThread, 1);
    t3 = std::thread(removeIntFromThread, 2);
    t4 = std::thread(removeIntFromThread, 3);
    t5 = std::thread(removeIntFromThread, 4);
    t6 = std::thread(removeIntFromThread, 5);
    t7 = std::thread(removeIntFromThread, 6);
    t8 = std::thread(removeIntFromThread, 7);
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
}

int main(int argc, char *argv[]) {
    outFile.open(HASHMAP_FILE, std::ios_base::out);
    outFile << "HashMap - time [s]" << std::endl;

    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
