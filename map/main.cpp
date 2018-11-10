#include "NvmHashMap.hpp"

struct root
{
	put::NvmHashMap<int> pmap;
}

int main(int argc, char* argv[])
{

	std::string path = argv[1];
	
	pool<root> pop;

	pop = pool<root>::create(path, "", PMEMOBJ_MIN_POOL, CREATE_MODE_RW);

	//pop = pool<root>::open(path, "");

	persistent_ptr<root> root_ptr;

	root_ptr = pop.root();

	if(!q->pmap)
	{
		transaction::run(pop, [&] {
			q->pmap = make_persistent<put::NvmHashMap<int> >();
		});
	}
	
	q->pmap->insert(10, 30);

	// std::cout << q->pmap->get(10);
}
