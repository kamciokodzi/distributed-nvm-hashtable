#include "../NvmHashMap.hpp"
#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <ctime>
#include <string.h>

#define ELEMENTS_COUNT 10000
#define THREADS_COUNT 8

struct root {
    pmem::obj::persistent_ptr<NvmHashMap<std::string, std::string> > pmapString;
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

void insertFromThread(int tid) {
    for(int i = ELEMENTS_COUNT; i >= 0; i--) {
        root_ptr->pmapString->insertNew(std::to_string(i*THREADS_COUNT+tid), std::to_string(i*THREADS_COUNT+tid));
    }
}

void removeFromThread(int tid) {
    for(int i = 100; i >= 0; i--) {
        root_ptr->pmapString->remove(std::to_string(i*64+tid));
    }
}

TEST(NvmHashMapStr, InsertGetTest) {
    for (int i = ELEMENTS_COUNT; i >= 0; i--) {
        root_ptr->pmapString->insertNew(std::to_string(i), std::to_string(i+2));
    }
    for (int i = ELEMENTS_COUNT; i >= 0; i--) {
        std::string returnValue = root_ptr->pmapString->get(std::to_string(i));
        ASSERT_EQ(std::to_string(i+2), returnValue);
    }
}

TEST(NvmHashMapStr, InsertRemoveTest) {
    for (int i = ELEMENTS_COUNT; i >= 0; i--) {
        root_ptr->pmapString->insertNew(std::to_string(i), std::to_string(i+2));
    }
    for (int i = ELEMENTS_COUNT; i >= 0; i--) {
        std::string returnValue = root_ptr->pmapString->remove(std::to_string(i));
        ASSERT_EQ(std::to_string(i+2), returnValue);
    }
}

TEST(NvmHashMapStrParallel, InsertGetTest) {
    std::thread t1(insertFromThread, 0);
    std::thread t2(insertFromThread, 1);
    std::thread t3(insertFromThread, 2);
    std::thread t4(insertFromThread, 3);
    std::thread t5(insertFromThread, 4);
    std::thread t6(insertFromThread, 5);
    std::thread t7(insertFromThread, 6);
    std::thread t8(insertFromThread, 7);

    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();
    t6.join();
    t7.join();
    t8.join();

    for (int tid = 0; tid < THREADS_COUNT; tid++) {
        for (int i = ELEMENTS_COUNT; i >= 0; i--) {
            std::string returnValue = root_ptr->pmapString->get(std::to_string(i*THREADS_COUNT+tid));
            ASSERT_EQ(std::to_string(i*THREADS_COUNT+tid), returnValue);
        }
    }
}


TEST(NvmHashMapStrParallel, InsertRemoveTest) {
    std::thread t1(insertFromThread, 0);
    std::thread t2(insertFromThread, 1);
    std::thread t3(insertFromThread, 2);
    std::thread t4(insertFromThread, 3);
    std::thread t5(insertFromThread, 4);
    std::thread t6(insertFromThread, 5);
    std::thread t7(insertFromThread, 6);
    std::thread t8(insertFromThread, 7);

    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();
    t6.join();
    t7.join();
    t8.join();

    for (int tid = 0; tid < THREADS_COUNT; tid++) {
        for (int i = ELEMENTS_COUNT; i >= 0; i--) {
            std::string returnValue = root_ptr->pmapString->remove(std::to_string(i*THREADS_COUNT+tid));
            ASSERT_EQ(std::to_string(i*THREADS_COUNT+tid), returnValue);
        }
    }
}



int main(int argc, char *argv[]) {

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

    if (!root_ptr->pmapString) {
        pmem::obj::transaction::run(pop, [&] {
            std::cout << "Creating NvmHashMap"<<std::endl;
            root_ptr->pmapString = pmem::obj::make_persistent<NvmHashMap<std::string, std::string> >();
        });
    }

    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();

}