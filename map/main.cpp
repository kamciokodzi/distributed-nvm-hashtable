#include "NvmHashMap.hpp"
#include <thread>
#include <chrono>
#include <ctime>

#define THREADS_COUNT 8

struct root {
//    pmem::obj::persistent_ptr<NvmHashMap<std::string, std::string> > pmap;
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

 	for(int i = 100000; i >= 0; i--) {
        root_ptr->pmap->insertNew(i * THREADS_COUNT + tid, i * THREADS_COUNT + tid);
//        root_ptr->pmap->insertNew(std::to_string(i * THREADS_COUNT + tid), std::to_string(i * THREADS_COUNT + tid));
    }
}

void getFromThread(int tid) {
    std::cout << "Log gFT, tid=" << tid << std::endl;

    for(int i = 10000; i >= 0; i--) {
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
                                                PMEMOBJ_MIN_POOL * 16, (S_IWUSR | S_IRUSR));
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

    if (!root_ptr->pmap) {
        pmem::obj::transaction::run(pop, [&] {
            std::cout << "Creating NvmHashMap" << std::endl;
//            root_ptr->pmap = pmem::obj::make_persistent<NvmHashMap<std::string, std::string> >();
            root_ptr->pmap = pmem::obj::make_persistent<NvmHashMap<int, int> >(THREADS_COUNT);
        });
    }
     auto start = std::chrono::system_clock::now();
     if (insertMode) {
         if (mode == "multithread") {
             auto start = std::chrono::system_clock::now();

             std::thread t10(getFromThread, 0);
             std::thread t20(getFromThread, 1);
             std::thread t30(getFromThread, 2);
             std::thread t40(getFromThread, 3);
             std::thread t50(getFromThread, 4);
             std::thread t60(getFromThread, 5);
             std::thread t70(getFromThread, 6);
             std::thread t80(getFromThread, 7);


             t10.join();
             t20.join();
             t30.join();
             t40.join();
             t50.join();
             t60.join();
             t70.join();
             t80.join();

             auto end = std::chrono::system_clock::now();
             std::chrono::duration<double> elapsed_time = end-start;
             std::cout << "Getting took " << elapsed_time.count() << " seconds." << std::endl;


         }
//         Iterator<int,int> it(root_ptr->pmap);
//         std::cout << it.get() << std::endl;
//         while (it.next()) {
//            std::cout << it.get() << std::endl;
//         }
     } else {
         std::cout << "Getting values from array" << std::endl;
         std::thread t1(getFromThread, 0);
         t1.join();
         auto end = std::chrono::system_clock::now();
         std::chrono::duration<double> elapsed_time = end-start;
         std::cout << "Getting took " << elapsed_time.count() << " seconds." << std::endl;
     }
}
