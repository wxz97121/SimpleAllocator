#include "MemorySystem.h"



bool InitializeMemorySystem(void* i_pHeapMemory, size_t i_sizeHeapMemory)
{
	// create your HeapManager and FixedSizeAllocators
	vFSA.push_back(FixedSizeAllocator::Create(i_pHeapMemory, 16, 100));
	i_sizeHeapMemory -= 16 * 100;
	vFSA.push_back(FixedSizeAllocator::Create(i_pHeapMemory, 32, 200));
	i_sizeHeapMemory -= 32 * 200;
	vFSA.push_back(FixedSizeAllocator::Create(i_pHeapMemory, 96, 400));
	i_sizeHeapMemory -= 96 * 400;

	p_HA = Create(i_pHeapMemory, i_sizeHeapMemory);
	return true;
}

void Collect()
{
	// coalesce free blocks
	// you may or may not need to do this depending on how you've implemented your HeapManager
}

void DestroyMemorySystem()
{
	cout << "BEGIN DESTROY" << endl;
	Destroy(p_HA);
	cout << "HEAPALLOCATOR DESTROYED" << endl;
	for (auto& p_FSA : vFSA)
	{
		FixedSizeAllocator::Destroy(p_FSA);
		cout << "ONE FIXEDSIZEALLOCATOR DESTROYED" << endl;
	}
	cout << "END DESTROYED" << endl;
	// Destroy your HeapManager and FixedSizeAllocators
}

void* FinalExamMalloc(size_t i_size)
{
	cout << "malloc " << i_size << " ";
	void* res = nullptr;
	if (i_size <= 16) res = vFSA[0]->malloc();
	if (res) { cout << res << " FSA1" << endl; return res; }
	if (i_size <= 32) res = vFSA[1]->malloc();
	if (res) { cout << res << " FSA2" << endl; return res; }
	if (i_size <= 96) res = vFSA[2]->malloc();
	if (res) { cout << res << " FSA3" << endl; return res; }

	res = p_HA->malloc(i_size);
	{ cout << res << " HA" << endl; return res; }
}

void FinalExamFree(void* i_ptr)
{
	cout << "free " << i_ptr << endl;
	for (auto& p_FSA : vFSA)
	{
		if (p_FSA->Contains(i_ptr))
		{
			p_FSA->free(i_ptr);
			return;
		}
	}
	assert(p_HA->Contains(i_ptr));
	p_HA->free(i_ptr);
}

