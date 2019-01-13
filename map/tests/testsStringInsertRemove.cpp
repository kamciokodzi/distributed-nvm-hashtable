#include "../NvmHashMap.hpp"
#include "constants.hpp"
#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <ctime>
#include <string.h>

#define THREADS_COUNT 8

struct root {
    pmem::obj::persistent_ptr<NvmHashMap<std::string, std::string> > pmap;
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
    for(int i = ELEMENTS_COUNT_CORRECTNESS; i >= 0; i--) {
        root_ptr->pmap->insertNew(std::to_string(i*THREADS_COUNT+tid), std::to_string(i*THREADS_COUNT+tid));
    }
}

void removeFromThread(int tid) {
    for(int i = 100; i >= 0; i--) {
        root_ptr->pmap->remove(std::to_string(i*64+tid));
    }
}

TEST(NvmHashMapStr, InsertRemoveTest) {
    for (int i = ELEMENTS_COUNT_CORRECTNESS; i >= 0; i--) {
        root_ptr->pmap->insertNew(std::to_string(i), std::to_string(i+2));
    }
    for (int i = ELEMENTS_COUNT_CORRECTNESS; i >= 0; i--) {
        std::string returnValue = root_ptr->pmap->remove(std::to_string(i));
        ASSERT_EQ(std::to_string(i+2), returnValue);
    }
}

int main(int argc, char *argv[]) {

    pmem::obj::pool <root> pop;
    std::string path = argv[1];

    try {
        if (!file_exists(path.c_str())) {
            std::cout << "File doesn't exists, creating pool"<<std::endl;
            pop = pmem::obj::pool<root>::create(path, "",
                                                PMEMOBJ_MIN_POOL*192, (S_IWUSR|S_IRUSR));
        } else {
            std::cout << "File exists, opening pool"<<std::endl;
            pop = pmem::obj::pool<root>::open(path, "");
        }
    } catch (pmem::pool_error &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    root_ptr = pop.root();

    if (!root_ptr->pmap) {
        pmem::obj::transaction::run(pop, [&] {
            std::cout << "Creating NvmHashMap"<<std::endl;
            root_ptr->pmap = pmem::obj::make_persistent<NvmHashMap<std::string, std::string> >();
        });
    }

    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();

}
