#include "NvmHashMap.hpp"

struct root {
    pmem::obj::persistent_ptr<NvmHashMap<int> > pmap;
};

int main(int argc, char *argv[]) {

    std::string path = argv[1];

    pmem::obj::pool <root> pop;

    //pop = pmem::obj::pool<root>::create(path, "", PMEMOBJ_MIN_POOL, (S_IWUSR|S_IRUSR));

    std::cout << "Log 1"<<std::endl;
    pop = pmem::obj::pool<root>::open(path, "");

    std::cout << "Log 2"<<std::endl;
    pmem::obj::persistent_ptr <root> root_ptr;

    std::cout << "Log 3"<<std::endl;
    root_ptr = pop.root();

    std::cout << "Log 4"<<std::endl;
    if (!root_ptr->pmap) {
        pmem::obj::transaction::run(pop, [&] {

            std::cout << "Log 4a"<<std::endl;
            root_ptr->pmap = pmem::obj::make_persistent<NvmHashMap<int> >();
        });

        std::cout << "Log 5"<<std::endl;
    }

    //root_ptr->pmap->insert(10, 30);

    std::cout << "Log 6"<<std::endl;
    std::cout << root_ptr->pmap->get(10);
}
