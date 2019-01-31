#include "../NvmHashMap.hpp"
#include "constants.hpp"
#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <ctime>
#include <string.h>
#include <algorithm>

int sum = 0;

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

void insertFromThread(int tid)
{
    std::vector<int> values;
    for (int i = 0; i < ELEMENTS_COUNT_CORRECTNESS; i++) {
        values.push_back(i);
    }

    std::random_shuffle(values.begin(), values.end());

    for(int i = 0; i < ELEMENTS_COUNT_CORRECTNESS/2; i++) {
        root_ptr->pmap->insertNew(values[i]*THREADS_COUNT+tid, values[i]*THREADS_COUNT+tid);
        sum += values[i]*THREADS_COUNT+tid;
    }
}

TEST(NvmHashMapInt, InsertIterateCheckSumTest) {
    std::thread t1(insertFromThread, 0);
    t1.join();
    int count = 0;
    int sumIterate = 0;
    Iterator<int,int> it(root_ptr->pmap);
    sumIterate += it.get();
    count += 1;

    while (it.next()) {
        sumIterate += it.get();
        count += 1;
    }

    ASSERT_EQ(sum, sumIterate);
    ASSERT_EQ(ELEMENTS_COUNT_CORRECTNESS/2, count);
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
            root_ptr->pmap = pmem::obj::make_persistent<NvmHashMap<int, int> >();
        });
    }

    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();

}
