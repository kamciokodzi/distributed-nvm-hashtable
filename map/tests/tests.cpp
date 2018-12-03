#include "../NvmHashMap.hpp"
#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <ctime>

struct root {
    pmem::obj::persistent_ptr<NvmHashMap<int, int> > pmap;
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

TEST(NvmHashMap, InsertGetTest) {
    int tid = 0;
    for(int i = 100; i >= 0; i-10) {
        root_ptr->pmap->insertNew(i+64*tid, i+2);
        int returnValue = root_ptr->pmap->get(i);
        ASSERT_EQ(i+2, returnValue);
    }
}

TEST(NvmHashMap, InsertRemoveTest) {
    int tid = 0;
    for(int i = 100; i >= 0; i-10) {
        root_ptr->pmap->insertNew(i, i+2);
        int returnValue = root_ptr->pmap->remove(i);
        ASSERT_EQ(i+2, returnValue);
    }
}


int main(int argc, char *argv[]) {

    pmem::obj::pool <root> pop;
    std::string path = argv[1];
    std::cout << "path: " << path.c_str() << std::endl;

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

    if (!root_ptr->pmap) {
        pmem::obj::transaction::run(pop, [&] {
            std::cout << "Creating NvmHashMap"<<std::endl;
            root_ptr->pmap = pmem::obj::make_persistent<NvmHashMap<int, int> >();
        });
    }

    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();

}
