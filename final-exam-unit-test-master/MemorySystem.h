#pragma once
#include <vector>
#include <iostream>
#include "HeapAllocator.h"
#include "FixedSizeAllocator.h"
using namespace std;


extern vector<FixedSizeAllocator*> vFSA;
extern HeapAllocator* p_HA;
// InitializeMemorySystem - initialize your memory system including your HeapManager and some FixedSizeAllocators
bool InitializeMemorySystem(void * i_pHeapMemory, size_t i_sizeHeapMemory);

// Collect - coalesce free blocks in attempt to create larger blocks
void Collect();

// DestroyMemorySystem - destroy your memory systems
void DestroyMemorySystem();

void* FinalExamMalloc(size_t i_size);

void FinalExamFree(void* i_ptr);
