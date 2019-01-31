#include "../NvmHashMap.hpp"
#include "constants.hpp"
#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <ctime>
#include <string.h>
#include <fstream>

#define THREADS_COUNT 16

struct root {
    pmem::obj::persistent_ptr<NvmHashMap<int, int> > pmapInt;
};

bool file_exists(const char *fname) {
    FILE *file;
    if ((file = fopen(fname, "r"))) {
        fclose(file);
        return true;
    }
    return false;
}


pmem::obj::persistent_ptr<root> root_ptr;
std::ofstream outFile;

void insertIntFromThread(int tid)
{
    for(int i = ELEMENTS_COUNT_PERFORMANCE/THREADS_COUNT; i >= 0; i--) {
        root_ptr->pmapInt->insertNew(i*THREADS_COUNT+tid, i*THREADS_COUNT+tid);
    }
}

void removeIntFromThread(int tid)
{
    for(int i = ELEMENTS_COUNT_PERFORMANCE/THREADS_COUNT; i >= 0; i--) {
        root_ptr->pmapInt->remove(i*THREADS_COUNT+tid);
    }
}

void getIntFromThread(int tid)
{
    for(int i = ELEMENTS_COUNT_PERFORMANCE/THREADS_COUNT; i >= 0; i--) {
        root_ptr->pmapInt->get(i*THREADS_COUNT+tid);
    }
}


TEST(NvmHashMapIntParallelPerformance, 16ThreadsInsertGetRemoveTest) {
    auto start = std::chrono::system_clock::now();

    std::thread t1(insertIntFromThread, 0);
    std::thread t2(insertIntFromThread, 1);
    std::thread t3(insertIntFromThread, 2);
    std::thread t4(insertIntFromThread, 3);
    std::thread t5(insertIntFromThread, 4);
    std::thread t6(insertIntFromThread, 5);
    std::thread t7(insertIntFromThread, 6);
    std::thread t8(insertIntFromThread, 7);
    std::thread t9(insertIntFromThread, 8);
    std::thread t10(insertIntFromThread, 9);
    std::thread t11(insertIntFromThread, 10);
    std::thread t12(insertIntFromThread, 11);
    std::thread t13(insertIntFromThread, 12);
    std::thread t14(insertIntFromThread, 13);
    std::thread t15(insertIntFromThread, 14);
    std::thread t16(insertIntFromThread, 15);
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
    t9 = std::thread(getIntFromThread, 8);
    t10 = std::thread(getIntFromThread, 9);
    t11 = std::thread(getIntFromThread, 10);
    t12 = std::thread(getIntFromThread, 11);
    t13 = std::thread(getIntFromThread, 12);
    t14 = std::thread(getIntFromThread, 13);
    t15 = std::thread(getIntFromThread, 14);
    t16 = std::thread(getIntFromThread, 15);
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
    
    t1 = std::thread(removeIntFromThread, 0);
    t2 = std::thread(removeIntFromThread, 1);
    t3 = std::thread(removeIntFromThread, 2);
    t4 = std::thread(removeIntFromThread, 3);
    t5 = std::thread(removeIntFromThread, 4);
    t6 = std::thread(removeIntFromThread, 5);
    t7 = std::thread(removeIntFromThread, 6);
    t8 = std::thread(removeIntFromThread, 7);
    t9 = std::thread(removeIntFromThread, 8);
    t10 = std::thread(removeIntFromThread, 9);
    t11 = std::thread(removeIntFromThread, 10);
    t12 = std::thread(removeIntFromThread, 11);
    t13 = std::thread(removeIntFromThread, 12);
    t14 = std::thread(removeIntFromThread, 13);
    t15 = std::thread(removeIntFromThread, 14);
    t16 = std::thread(removeIntFromThread, 15);
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
}


int main(int argc, char *argv[]) {

    outFile.open(NVMHASHMAP_FILE, std::ios_base::app);

    pmem::obj::pool <root> pop;
    std::string path = argv[1];

    try {
        if (!file_exists(path.c_str())) {
            pop = pmem::obj::pool<root>::create(path, "",
                                                PMEMOBJ_MIN_POOL*POOL_SIZE, (S_IWUSR|S_IRUSR));
        } else {
            pop = pmem::obj::pool<root>::open(path, "");
        }
    } catch (pmem::pool_error &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    root_ptr = pop.root();

    if (!root_ptr->pmapInt) {
        pmem::obj::transaction::run(pop, [&] {
            std::cout << "Creating NvmHashMap"<<std::endl;
            root_ptr->pmapInt = pmem::obj::make_persistent<NvmHashMap<int, int> >(THREADS_COUNT);
        });
    }

    testing::InitGoogleTest(&argc, argv);
    outFile << std::endl;
    return RUN_ALL_TESTS();

}
