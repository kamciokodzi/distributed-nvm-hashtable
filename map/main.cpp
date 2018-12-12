#include "NvmHashMap.hpp"
#include <thread>
#include <chrono>
#include <ctime>

struct root {
//    pmem::obj::persistent_ptr<NvmHashMap<int, std::string> > pmap;
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

 	for(int i = 1000; i >= 0; i--) {
        root_ptr->pmap->insertNew(i * 64 + tid, i * 64 + tid);
    }
}

void getFromThread(int tid) {
    std::cout << "Log gFT, tid=" << tid << std::endl;

    for(int i = 19; i >= 0; i--) {
        root_ptr->pmap->get(i * 64 + tid);
    }
}

int main(int argc, char *argv[]) {

    pmem::obj::pool <root> pop;
    std::string path = argv[1];
    std::string mode = argv[2];
    bool insertMode = false;

    std::cout << "path: " << path.c_str() << std::endl;
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
//            root_ptr->pmap = pmem::obj::make_persistent<NvmHashMap<int, std::string> >();
            root_ptr->pmap = pmem::obj::make_persistent<NvmHashMap<int, int> >();
        });
    }
     auto start = std::chrono::system_clock::now();
     if (insertMode) {
         if (mode == "multithread") {
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
             auto end = std::chrono::system_clock::now();
             std::chrono::duration<double> elapsed_time = end-start;
             std::cout << "Inserting took " << elapsed_time.count() << " seconds." << std::endl;
             for (int tid = 0; tid < 8; tid++) {
                 for (int i = 10000; i >= 0; i--) {
                     int returnValue = root_ptr->pmap->get(i*64+tid);
                 }
             }
         }
         auto end = std::chrono::system_clock::now();
         std::chrono::duration<double> elapsed_time = end-start;
         std::cout << "Getting took " << elapsed_time.count() << " seconds." << std::endl;
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
