#include "NvmHashMap.hpp"

struct root {
    pmem::obj::persistent_ptr<NvmHashMap<int> > pmap;
};

bool file_exists(const char *fname) {
    FILE *file;
    if ((file = fopen(fname, "r"))) {
        fclose(file);
        return true;
    }
    return false;
}

int main(int argc, char *argv[]) {

    pmem::obj::pool <root> pop;
    std::string path = argv[1];
    bool insertMode = false;

    try {
        if (!file_exists(path.c_str())) {
            std::cout << "File doesn't exists, creating pool"<<std::endl;
            pop = pmem::obj::pool<root>::create(path, "",
                    PMEMOBJ_MIN_POOL, (S_IWUSR|S_IRUSR));
            insertMode = true;
        } else {
            std::cout << "File exists, opening pool"<<std::endl;
            pop = pmem::obj::pool<root>::open(path, "");
        }
    } catch (pmem::pool_error &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    pmem::obj::persistent_ptr <root> root_ptr;

    std::cout << "Log 3"<<std::endl;
    root_ptr = pop.root();

    std::cout << "Log 4"<<std::endl;
    if (!root_ptr->pmap) {
        pmem::obj::transaction::run(pop, [&] {

            std::cout << "Log 4a"<<std::endl;
            root_ptr->pmap = pmem::obj::make_persistent<NvmHashMap<int> >();
        });
        std::cout << "Log 5a"<<std::endl;
    } /*else {
        pmem::obj::transaction::run(pop, [&] {

            std::cout << "Log 4b"<<std::endl;
            root_ptr->pmap->initialize();

        });
        std::cout << "Log 5b"<<std::endl;
    }*/

    if (insertMode) {
        for (int i = 0; i < 64; i++) {
            root_ptr->pmap->insert(i, 30+i);
        }
        std::cout << "Inserting values to array" << std::endl;
    }
    for (int i = 0; i < 64; i++) {
        std::cout << root_ptr->pmap->get(i) << std::endl;
    }

    std::cout << "Log 6"<<std::endl;
}
