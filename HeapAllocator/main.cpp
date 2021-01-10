#include "HeapAllocator.h"
#include "HeapAllocator.h"
#include <Windows.h>

#include <assert.h>
#include <algorithm>
#include <vector>


#include<cstdio>
#include<iostream>
#include <ctime>
#define TEST_SINGLE_LARGE_ALLOCATION
#define SUPPORTS_ALIGNMENT
#define SUPPORTS_SHOWFREEBLOCKS
#define RANDOM_DATA

using namespace std;

bool UnitTest()
{
#ifdef RANDOM_DATA
	srand(static_cast<unsigned int>(time(nullptr)));
#endif // RANDOM_DATA

	const size_t sizeHeap = 1024 * 1024;

#ifdef USE_HEAP_ALLOC
	void* pHeapMemory = HeapAlloc(GetProcessHeap(), 0, sizeHeap);
#else
	// Get SYSTEM_INFO, which includes the memory page size
	SYSTEM_INFO SysInfo;
	GetSystemInfo(&SysInfo);
	// round our size to a multiple of memory page size
	assert(SysInfo.dwPageSize > 0);
	size_t sizeHeapInPageMultiples = SysInfo.dwPageSize * ((sizeHeap + SysInfo.dwPageSize) / SysInfo.dwPageSize);
	void* pHeapMemory = VirtualAlloc(NULL, sizeHeapInPageMultiples, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
#endif
	assert(pHeapMemory);

	// Create a heap manager for my test heap.
	auto m_Allocator = Create(pHeapMemory, sizeHeap);
	assert(m_Allocator);


#ifdef TEST_SINGLE_LARGE_ALLOCATION
	// This is a test I wrote to check to see if using the whole block if it was almost consumed by 
	// an allocation worked. Also helped test my ShowFreeBlocks() and ShowOutstandingAllocations().
	{
#ifdef SUPPORTS_SHOWFREEBLOCKS
		m_Allocator->ShowFreeBlocks();
#endif // SUPPORTS_SHOWFREEBLOCKS

		size_t largestBeforeAlloc = m_Allocator->GetLargestFreeBlockSize();
		void* pPtr = m_Allocator->malloc(largestBeforeAlloc - GeMinumumToLeaveForHeap());

		if (pPtr)
		{
#if defined(SUPPORTS_SHOWFREEBLOCKS) || defined(SUPPORTS_SHOWOUTSTANDINGALLOCATIONS)
			printf("After large allocation:\n");
#ifdef SUPPORTS_SHOWFREEBLOCKS
			m_Allocator->ShowFreeBlocks();
#endif // SUPPORTS_SHOWFREEBLOCKS
#ifdef SUPPORTS_SHOWOUTSTANDINGALLOCATIONS
			ShowOutstandingAllocations(pHeapManager);
#endif // SUPPORTS_SHOWOUTSTANDINGALLOCATIONS
			printf("\n");
#endif

			size_t largestAfterAlloc = m_Allocator->GetLargestFreeBlockSize();
			bool success = m_Allocator->Contains(pPtr);
			assert(success);

			success = m_Allocator->free(pPtr);
			assert(success);

			m_Allocator->Collect();

#if defined(SUPPORTS_SHOWFREEBLOCKS) || defined(SUPPORTS_SHOWOUTSTANDINGALLOCATIONS)
			printf("After freeing allocation and garbage collection:\n");
#ifdef SUPPORTS_SHOWFREEBLOCKS
			m_Allocator->ShowFreeBlocks();
#endif // SUPPORTS_SHOWFREEBLOCKS
#ifdef SUPPORTS_SHOWOUTSTANDINGALLOCATIONS
			ShowOutstandingAllocations(pHeapManager);
#endif // SUPPORTS_SHOWOUTSTANDINGALLOCATIONS
			printf("\n");
#endif

			size_t largestAfterCollect = m_Allocator->GetLargestFreeBlockSize();
		}
	}
#endif

	std::vector<void*> AllocatedAddresses;

	long	numAllocs = 0;
	long	numFrees = 0;
	long	numCollects = 0;

	// allocate memory of random sizes up to 1024 bytes from the heap manager
	// until it runs out of memory
	do
	{
		const size_t		maxTestAllocationSize = 4096;

		size_t			sizeAlloc = 1 + (rand() & (maxTestAllocationSize - 1));


#ifdef SUPPORTS_ALIGNMENT
		// pick an alignment
		const unsigned int	alignments[] = { 4, 8, 16, 32, 64 };

		const unsigned int	index = rand() % (sizeof(alignments) / sizeof(alignments[0]));

		const unsigned int	alignment = alignments[index];

		void* pPtr = m_Allocator->malloc(sizeAlloc, alignment);

		// check that the returned address has the requested alignment
		assert((reinterpret_cast<uintptr_t>(pPtr) & (alignment - 1)) == 0);
#else
		void* pPtr = m_Allocator->malloc(sizeAlloc);
#endif // SUPPORT_ALIGNMENT

		// if allocation failed see if garbage collecting will create a large enough block
		if (pPtr == nullptr)
		{
			m_Allocator->Collect();

#ifdef SUPPORTS_ALIGNMENT
			pPtr = m_Allocator->malloc(sizeAlloc, alignment);
#else
			pPtr = m_Allocator->malloc(sizeAlloc);
#endif // SUPPORT_ALIGNMENT

			// if not we're done. go on to cleanup phase of test
			if (pPtr == nullptr)
				break;
		}

		AllocatedAddresses.push_back(pPtr);
		numAllocs++;


		// randomly free and/or garbage collect during allocation phase
		const unsigned int freeAboutEvery = 10;
		const unsigned int garbageCollectAboutEvery = 40;

		if (!AllocatedAddresses.empty() && ((rand() % freeAboutEvery) == 0))
		{
			void* pPtr = AllocatedAddresses.back();
			AllocatedAddresses.pop_back();

			bool success = m_Allocator->Contains(pPtr);
			assert(success);

			success = m_Allocator->free(pPtr);
			assert(success);

			numFrees++;
		}

		if ((rand() % garbageCollectAboutEvery) == 0)
		{
			m_Allocator->Collect();
			numCollects++;
		}
	} while (1);

#if defined(SUPPORTS_SHOWFREEBLOCKS) || defined(SUPPORTS_SHOWOUTSTANDINGALLOCATIONS)
	printf("After exhausting allocations:\n");
#ifdef SUPPORTS_SHOWFREEBLOCKS
	m_Allocator->ShowFreeBlocks();
#endif // SUPPORTS_SHOWFREEBLOCKS
#ifdef SUPPORTS_SHOWOUTSTANDINGALLOCATIONS
	ShowOutstandingAllocations(pHeapManager);
#endif // SUPPORTS_SHOWOUTSTANDINGALLOCATIONS
	printf("\n");
#endif

	// now free those blocks in a random order
	if (!AllocatedAddresses.empty())
	{
		// randomize the addresses
		std::random_shuffle(AllocatedAddresses.begin(), AllocatedAddresses.end());

		// return them back to the heap manager
		while (!AllocatedAddresses.empty())
		{
			void* pPtr = AllocatedAddresses.back();
			AllocatedAddresses.pop_back();

			bool success = m_Allocator->Contains(pPtr);
			assert(success);

			success = m_Allocator->free(pPtr);
			assert(success);
		}

#if defined(SUPPORTS_SHOWFREEBLOCKS) || defined(SUPPORTS_SHOWOUTSTANDINGALLOCATIONS)
		printf("After freeing allocations:\n");
#ifdef SUPPORTS_SHOWFREEBLOCKS
		m_Allocator->ShowFreeBlocks();
#endif // SUPPORTS_SHOWFREEBLOCKS

#ifdef SUPPORTS_SHOWOUTSTANDINGALLOCATIONS
		ShowOutstandingAllocations(pHeapManager);
#endif // SUPPORTS_SHOWOUTSTANDINGALLOCATIONS
		printf("\n");
#endif

		// do garbage collection


		m_Allocator->Collect();

		// our heap should be one single block, all the memory it started with

#if defined(SUPPORTS_SHOWFREEBLOCKS) || defined(SUPPORTS_SHOWOUTSTANDINGALLOCATIONS)
		printf("After garbage collection:\n");
#ifdef SUPPORTS_SHOWFREEBLOCKS
		m_Allocator->ShowFreeBlocks();
#endif // SUPPORTS_SHOWFREEBLOCKS

#ifdef SUPPORTS_SHOWOUTSTANDINGALLOCATIONS
		ShowOutstandingAllocations(pHeapManager);
#endif // SUPPORTS_SHOWOUTSTANDINGALLOCATIONS
		printf("\n");
#endif

		m_Allocator->Collect();

		// do a large test allocation to see if garbage collection worked
		void* pPtr = m_Allocator->malloc(sizeHeap / 2);
		assert(pPtr);

		if (pPtr)
		{
			bool success = m_Allocator->Contains(pPtr);
			assert(success);

			success = m_Allocator->free(pPtr);
			assert(success);

		}
	}

	//Collect(m_Allocator);
	m_Allocator = nullptr;

	if (pHeapMemory)
	{
#ifdef USE_HEAP_ALLOC
		HeapFree(GetProcessHeap(), 0, pHeapMemory);
#else
		VirtualFree(pHeapMemory, 0, MEM_RELEASE);
#endif
	}
	return true;
}


int main()
{

	UnitTest();

	size_t MyHeapSize = 10000u;
	void* InitMem = malloc(MyHeapSize);
	auto m_Allocator = Create(InitMem, MyHeapSize);
	int* IntArray1, * IntArray2;
	char* ch;
	int ArrayLen = 7;
	IntArray1 = static_cast<int*>(m_Allocator->malloc(ArrayLen * sizeof(int)));
	for (int i = 0; i < ArrayLen; i++) *(IntArray1 + i) = i;

	ch = static_cast<char*>(m_Allocator->malloc(sizeof(char)));
	*ch = 'S';
	IntArray2 = static_cast<int*>(m_Allocator->malloc(ArrayLen * sizeof(int)));
	for (int i = 0; i < ArrayLen; i++) *(IntArray2 + i) = ArrayLen - i;

	cout << "IF SUCCESS " << m_Allocator->free(ch) << endl;
	for (int i = 0; i < ArrayLen; i++)
	{
		cout << "First Array[" << i << "] is " << IntArray1[i] << endl;
		cout << "Second Array[" << i << "] is " << IntArray2[i] << endl;
	}
	cout << endl << endl;;
	cout << "IF SUCCESS " << m_Allocator->free(IntArray1) << endl;
	for (int i = 0; i < ArrayLen; i++)
	{
		cout << "First Array[" << i << "] is " << IntArray1[i] << endl;
		cout << "Second Array[" << i << "] is " << IntArray2[i] << endl;
	}
	cout << endl << endl;;
	int NewArrayLen = 10;
	IntArray1 = static_cast<int*>(m_Allocator->malloc(NewArrayLen * sizeof(int)));
	for (int i = 0; i < NewArrayLen; i++) *(IntArray1 + i) = i;

	for (int i = 0; i < ArrayLen; i++)
	{
		cout << "Second Array[" << i << "] is " << IntArray2[i] << endl;
	}
	cout << "IF SUCCESS " << m_Allocator->free(IntArray1) << endl;
	cout << "IF SUCCESS " << m_Allocator->free(IntArray2) << endl;
}