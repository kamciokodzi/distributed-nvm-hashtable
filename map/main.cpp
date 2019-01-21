#include "NvmHashMap.hpp"
#include <thread>
#include <chrono>
#include <ctime>

#define THREADS_COUNT 1

struct root {
    pmem::obj::persistent_ptr <NvmHashMap<int, int>> pmap;
};

bool file_exists(const char *fname) {
    FILE *file;
    if ((file = fopen(fname, "r"))) {
        fclose(file);
        return true;
    }
    return false;
}

pmem::obj::persistent_ptr <root> root_ptr;

void insertFromThread(int tid) {
    std::cout << "Log iFT, tid=" << tid << std::endl;

 	for(int i = 1000000; i >= 0; i--) {
        root_ptr->pmap->insertNew(i * THREADS_COUNT + tid, i * THREADS_COUNT + tid);
    }
}

void getFromThread(int tid) {
    std::cout << "Log gFT, tid=" << tid << std::endl;

    for(int i = 1; i >= 0; i--) {
        try {
            root_ptr->pmap->get(i * THREADS_COUNT + tid);
        }
        catch (...) {
        }
    }
}

int main(int argc, char *argv[]) {

    pmem::obj::pool <root> pop;
    std::string path = argv[1];
    std::string mode = argv[2];
    bool insertMode = false;

    try {

        if (!file_exists(path.c_str())) {
            std::cout << "File doesn't exists, creating pool" << std::endl;
            pop = pmem::obj::pool<root>::create(path, "",
                                                PMEMOBJ_MIN_POOL * 96, (S_IWUSR | S_IRUSR));
            insertMode = true;
        } else {
            std::cout << "File exists, opening pool" << std::endl;
            pop = pmem::obj::pool<root>::open(path, "");
        }
    } catch (pmem::pool_error &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    root_ptr = pop.root();
    std::cout << "log1" << std::endl;

    if (!root_ptr->pmap) {
        std::cout << "log2" << std::endl;

        pmem::obj::transaction::run(pop, [&] {
            std::cout << "Creating NvmHashMap" << std::endl;
            root_ptr->pmap = pmem::obj::make_persistent<NvmHashMap<int, int> >(THREADS_COUNT);
        });
    }
    std::cout << "log3" << std::endl;

    auto start = std::chrono::system_clock::now();
     if (insertMode) {
         if (mode == "multithread") {
             std::thread t10(insertFromThread, 0);
//             std::thread t20(insertFromThread, 1);
//             std::thread t30(insertFromThread, 2);
//             std::thread t40(insertFromThread, 3);
//             std::thread t50(insertFromThread, 4);
//             std::thread t60(insertFromThread, 5);
//             std::thread t70(insertFromThread, 6);
//             std::thread t80(insertFromThread, 7);


             t10.join();
//             t20.join();
//             t30.join();
//             t40.join();
//             t50.join();
//             t60.join();
//             t70.join();
//             t80.join();
             auto end = std::chrono::system_clock::now();
             std::chrono::duration<double> elapsed_time = end-start;
             std::cout << "Inserting took " << elapsed_time.count() << " seconds." << std::endl;


         }
     } else {
         Iterator<int,int> it(root_ptr->pmap);
         std::cout << "log4" << std::endl;
         root_ptr->pmap->get(1000000);
         int count = 0;
         try {
             std::cout << it.get() << std::endl;
             count += 1;
         }
         catch (...) {
         }

         while (it.next()) {
             try {
                 std::cout << it.get() << std::endl;
                 count += 1;
             }
             catch (...) {
             }
         }

         std::cout << "Nr of elements: " << count << std::endl;

         auto end = std::chrono::system_clock::now();
         std::chrono::duration<double> elapsed_time = end-start;
         std::cout << "Getting took " << elapsed_time.count() << " seconds." << std::endl;
     }
}
