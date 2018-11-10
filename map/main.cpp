#include "NvmHashMap.hpp"

struct root {
    pmem::obj::persistent_ptr<NvmHashMap<int> > pmap;
};

int main(int argc, char *argv[]) {

    std::string path = argv[1];

    pmem::obj::pool <root> pop;

    pop = pmem::obj::pool<root>::create(path, "", PMEMOBJ_MIN_POOL, (S_IWUSR|S_IRUSR));

    //pop = pool<root>::open(path, "");

    pmem::obj::persistent_ptr <root> root_ptr;

    root_ptr = pop.root();

    if (root_ptr->pmap != nullptr) {
        pmem::obj::transaction::run(pop, [&] {
            root_ptr->pmap = pmem::obj::make_persistent<NvmHashMap<int> >();
        });
    }

    root_ptr->pmap->insert(10, 30);

    // std::cout << q->pmap->get(10);
}
