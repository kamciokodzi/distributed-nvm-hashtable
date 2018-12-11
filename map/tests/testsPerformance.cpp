#include "../NvmHashMap.hpp"
#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <ctime>
#include <string.h>
#include <fstream>


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
    for(int i = 100; i >= 0; i--) {
        root_ptr->pmapInt->insertNew(i*64+tid, i*64+tid);
    }
}

void removeIntFromThread(int tid)
{
    for(int i = 100; i >= 0; i--) {
        root_ptr->pmapInt->remove(i*64+tid);
    }
}


TEST(NvmHashMapIntParallelPerformance, Insert100Get100IntTest) {
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
    outFile << "8 threads inserting 100 ints per each " << elapsed_time.count() << " seconds." << std::endl;

    start = std::chrono::system_clock::now();

    for (int tid = 0; tid < 8; tid++) {
        for (int i = 100; i >= 0; i--) {
            int returnValue = root_ptr->pmapInt->get(i*64+tid);
            ASSERT_EQ(i*64+tid, returnValue);
        }
    }
    end = std::chrono::system_clock::now();
    elapsed_time = end-start;
    outFile << "8 threads getting 100 ints per each: " << elapsed_time.count() << " seconds." << std::endl;
}


int main(int argc, char *argv[]) {

    outFile.open("../performance.txt", std::ios_base::app);


    pmem::obj::pool <root> pop;
    std::string path = argv[1];

    try {
        if (!file_exists(path.c_str())) {
            std::cout << "File doesn't exists, creating pool"<<std::endl;
            pop = pmem::obj::pool<root>::create(path, "",
                                                PMEMOBJ_MIN_POOL*16, (S_IWUSR|S_IRUSR));
        } else {
            std::cout << "File exists, opening pool"<<std::endl;
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
            root_ptr->pmapInt = pmem::obj::make_persistent<NvmHashMap<int, int> >();
        });
    }

    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();

}
