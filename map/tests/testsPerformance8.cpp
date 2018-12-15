#include "../NvmHashMap.hpp"
#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <ctime>
#include <string.h>
#include <fstream>

#define ELEMENTS_COUNT 10000
#define THREADS_COUNT 8

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
    for(int i = ELEMENTS_COUNT; i >= 0; i--) {
        root_ptr->pmapInt->insertNew(i*THREADS_COUNT+tid, i*THREADS_COUNT+tid);
    }
}

void removeIntFromThread(int tid)
{
    for(int i = ELEMENTS_COUNT; i >= 0; i--) {
        root_ptr->pmapInt->remove(i*THREADS_COUNT+tid);
    }
}

void getIntFromThread(int tid)
{
    for(int i = ELEMENTS_COUNT; i >= 0; i--) {
        root_ptr->pmapInt->get(i*THREADS_COUNT+tid);
    }
}


TEST(NvmHashMapIntParallelPerformance, 8ThreadsInsertGetRemoveTest) {
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
    outFile << THREADS_COUNT <<" threads inserting " << ELEMENTS_COUNT << " ints per each " << elapsed_time.count() << " seconds." << std::endl;


    start = std::chrono::system_clock::now();

    t1 = std::thread(getIntFromThread, 0);
    t2 = std::thread(getIntFromThread, 1);
    t3 = std::thread(getIntFromThread, 2);
    t4 = std::thread(getIntFromThread, 3);
    t5 = std::thread(getIntFromThread, 4);
    t6 = std::thread(getIntFromThread, 5);
    t7 = std::thread(getIntFromThread, 6);
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();
    t6.join();
    t7.join();

    end = std::chrono::system_clock::now();
    elapsed_time = end-start;
    outFile << THREADS_COUNT << " threads getting " << ELEMENTS_COUNT << " ints per each: " << elapsed_time.count() << " seconds." << std::endl;


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
    outFile << THREADS_COUNT << " threads removing " << ELEMENTS_COUNT << " ints per each: " << elapsed_time.count() << " seconds." << std::endl;
}


int main(int argc, char *argv[]) {

    outFile.open("performance.txt", std::ios_base::app);

    pmem::obj::pool <root> pop;
    std::string path = argv[1];

    try {
        if (!file_exists(path.c_str())) {
            pop = pmem::obj::pool<root>::create(path, "",
                                                PMEMOBJ_MIN_POOL*16, (S_IWUSR|S_IRUSR));
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
