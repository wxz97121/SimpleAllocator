#pragma once
#include<cstdio>
#include<iostream>
#include <algorithm>
#include <assert.h>

#define BLOCK_MAGIC_FLAG 0xF // 1111
#define BLOCK_MAGIC_NUMBER 0xA // 1010
#define PADDING_MAGIC_NUMBER 0xD //1101
// MAGIC_NUMBER is a number to indicate the Memory is outstanding or unused.
// It can also indicate if the memory are malloced by HeapAllocator.

using namespace std;
struct MemBlock
{
	MemBlock* m_Next;
	MemBlock* m_Previous;
	//size_t 
	size_t m_flag;

};
inline void Increment(void*& origin, size_t delta)
{
	bool* tmp = (bool*)origin;
	tmp += delta;
	origin = (void*)tmp;
}
inline void Decrement(void*& origin, size_t delta)
{
	bool* tmp = (bool*)origin;
	tmp -= delta;
	origin = (void*)tmp;
}
inline void SetBlockUnused(MemBlock* pNode)
{
	pNode->m_flag &= ~BLOCK_MAGIC_FLAG;
}
inline bool IsBlockValid(const MemBlock* pNode)
{
	if (pNode != nullptr)
	{
		size_t flag = (pNode->m_flag & BLOCK_MAGIC_FLAG);
		if (flag == BLOCK_MAGIC_NUMBER) return true;
	}
	return false;
}

inline size_t GetBlockSize(const MemBlock* pNode)
{
	return pNode->m_flag >> 4;
}

inline void SetBlockSize(MemBlock* pNode, size_t size)
{
	size_t flag = (size << 4);
	flag |= BLOCK_MAGIC_NUMBER;
	pNode->m_flag = flag;
}

//inline void SetPaddingFill(void* Address)


inline size_t GeMinumumToLeaveForHeap()
{
	return sizeof(MemBlock);
}
inline size_t alignForwardAdjustment(void* address, size_t alignment)
{
	size_t adjustment = alignment - (reinterpret_cast<size_t>(address) & (alignment - 1));

	if (adjustment == alignment) return 0;

	//already aligned 
	return adjustment;
}

class HeapAllocator
{
public:
	inline HeapAllocator();
	inline ~HeapAllocator();
	static friend HeapAllocator* Create(void* pHeapMem, size_t HeapMemSize);
	static friend void Destroy(HeapAllocator* m_HeapAllocator);
	static friend void Renew(HeapAllocator* m_HeapAllocator);
	inline void Collect();
	inline void* malloc(size_t pSize, size_t Align);
	inline bool free(void* start);
	inline const size_t GetLargestFreeBlockSize()
	{
		size_t Res = 0;
		auto CurBlock = m_freelist;
		while (CurBlock)
		{
			Res = max(Res, GetBlockSize(CurBlock));
			CurBlock = CurBlock->m_Next;
		}
		return Res;
	}
	inline const void ShowFreeBlocks()
	{
		cout << "--------Free Block List BEGIN--------" << endl;
		auto CurBlock = m_freelist;
		while (CurBlock)
		{
			cout << "Address: " << CurBlock << "    Size: " << GetBlockSize(CurBlock) << endl;
			CurBlock = CurBlock->m_Next;
		}
		cout << "--------Free Block List END--------" << endl << endl;
		return;
	}
	inline bool Contains(void* p)
	{
		return (p >= this && p < this + this->HeapMemSize);
	}
	
	inline bool DEBUGCheckInFree(MemBlock* BlockToFind)
	{
		if (!BlockToFind) return false;
		auto current = this->m_freelist;
		while (current && current != BlockToFind)
			current = current->m_Next;
		if (current == BlockToFind) return true;
		return false;
	}

private:
	inline MemBlock* FindBlock(size_t pSize);
	MemBlock* m_freelist;
	void* HeapMem;
	size_t HeapMemSize;
	size_t HeapRemainSize;
	inline void Merge(MemBlock* pBlock);
};

HeapAllocator::HeapAllocator()
{
}

HeapAllocator::~HeapAllocator()
{
#ifdef _DEBUG
	cout << "HEAP ALLOCATOR FREE LIST DEBUG" << endl;
	ShowFreeBlocks();
#endif // _DEBUG

}

inline void HeapAllocator::Collect()
{
	auto current = this->m_freelist;
	while (current)
	{
		this->Merge(current);
		current = current->m_Next;
	}
}

void* HeapAllocator::malloc(size_t pSize, size_t Align = 4)
{

	if (pSize > this->HeapRemainSize) return nullptr;
	auto ProperBlock = FindBlock(pSize + Align - 1);
	if (ProperBlock == nullptr) return nullptr;
	//cout << "DEBUG MALLOC" << endl;
	//void* UserAddress = ProperBlock;
	//Increment(UserAddress, sizeof(MemBlock));

	size_t AlignSize = alignForwardAdjustment(ProperBlock + 1, Align);
	size_t OldSize = GetBlockSize(ProperBlock);
	if (OldSize > pSize + AlignSize + sizeof(MemBlock))
	{
		//cout << "DEBUG SPLIT" << endl;
		MemBlock* NewBlockHeader = reinterpret_cast<MemBlock*>(reinterpret_cast<char*>(ProperBlock) + AlignSize + sizeof(MemBlock) + pSize);
		//cout <<"ProperBlock Address "<< ProperBlock << endl;
		//cout << "NewBlockHeader Address " << NewBlockHeader << endl;
		SetBlockSize(NewBlockHeader, OldSize - pSize - AlignSize - sizeof(MemBlock));
		SetBlockSize(ProperBlock, pSize + AlignSize);
		NewBlockHeader->m_Next = ProperBlock->m_Next;
		NewBlockHeader->m_Previous = ProperBlock;

		if (ProperBlock->m_Next)
			ProperBlock->m_Next->m_Previous = NewBlockHeader;
		ProperBlock->m_Next = NewBlockHeader;
		this->HeapRemainSize -= sizeof(MemBlock);
	}

	//cout << "Proper PRE NEXT _1 " << ProperBlock << " " << ProperBlock->m_Previous << " " << ProperBlock->m_Next << endl;
	if (ProperBlock == this->m_freelist)
		this->m_freelist = ProperBlock->m_Next;
	else ProperBlock->m_Previous->m_Next = ProperBlock->m_Next;

	if (ProperBlock->m_Next)
		ProperBlock->m_Next->m_Previous = ProperBlock->m_Previous;

	//cout << "Proper PRE NEXT _2 " << ProperBlock << " " << ProperBlock->m_Previous << " " << ProperBlock->m_Next << endl;


	this->HeapRemainSize -= pSize + AlignSize;
	//assert(!this->DEBUGCheckInFree(ProperBlock));
	//cout << "Debug Malloc Header Address" << ProperBlock << endl;
	void* UserAddress = ProperBlock + 1;
	for (size_t i = 0; i < AlignSize; i++)
	{
		*static_cast<char*>(UserAddress) = PADDING_MAGIC_NUMBER;
		Increment(UserAddress, 1);
	}
	//cout << "Debug User Address" << UserAddress << endl;
	return UserAddress;
}

bool HeapAllocator::free(void* start)
{
	if (start == nullptr) return false;
	char* PaddingHeader = static_cast<char*>(start) - 1;
	while (*PaddingHeader == PADDING_MAGIC_NUMBER)
	{
		PaddingHeader--;
	}PaddingHeader++;
	auto Header = reinterpret_cast<MemBlock*>(PaddingHeader) - 1;
	//cout << "Debug Free Header Address" << Header << endl;
	if (!IsBlockValid(Header)) return false;

	HeapRemainSize += GetBlockSize(Header);
	//Find Pre Node
	auto current = this->m_freelist;
	if (current != nullptr)
	{
		if (current > Header)
		{
			m_freelist = Header;
			Header->m_Next = current;
			Header->m_Previous = nullptr;
			current->m_Previous = Header;
		}
		else
		{
			while (current->m_Next && current->m_Next <= Header)
			{
				//if (current->m_Next == Header) { current->m_Next = Header->m_Next; if (current->m_Next) current->m_Next->m_Previous = current; break; }
				//cout << "Debug " << current << " " << current->m_Next << endl;
				current = current->m_Next;
			}
			if (current->m_Next)
				current->m_Next->m_Previous = Header;
			Header->m_Next = current->m_Next;
			current->m_Next = Header;
			Header->m_Previous = current;
		}

	}
	else
	{
		this->m_freelist = Header;
		Header->m_Next = nullptr;
		Header->m_Previous = nullptr;
	}
	//SetBlockUnused(Header);
	Merge(current);
	return true;
}

MemBlock* HeapAllocator::FindBlock(size_t const pSize)
{
	MemBlock* CurrentBlock = this->m_freelist;
	while (CurrentBlock && (GetBlockSize(CurrentBlock) < pSize))
	{
		//cout << "Debug2" << endl;
		CurrentBlock = CurrentBlock->m_Next;
	}
	return CurrentBlock;

}

void HeapAllocator::Merge(MemBlock* pBlock)
{
	if (!pBlock) return;
	while (pBlock->m_Next && (reinterpret_cast<char*>(pBlock)) + sizeof(MemBlock) + GetBlockSize(pBlock) == reinterpret_cast<char*>(pBlock->m_Next))
	{
		//cout << "Debug3" << endl;
		if (pBlock->m_Next->m_Next)
			pBlock->m_Next->m_Next->m_Previous = pBlock;
		SetBlockSize(pBlock, GetBlockSize(pBlock) + GetBlockSize(pBlock->m_Next) + sizeof(MemBlock));
		pBlock->m_Next = pBlock->m_Next->m_Next;
		HeapRemainSize += sizeof(MemBlock);
	}
}

HeapAllocator* Create(void* pHeapMem, size_t HeapMemSize)
{
	//cout << "Debug HeapSize" << HeapMemSize;
	if (HeapMemSize < sizeof(HeapAllocator) + sizeof(MemBlock)) return nullptr;
	auto m_HeapAllocator = static_cast<HeapAllocator*>(pHeapMem);
	m_HeapAllocator = new (m_HeapAllocator) HeapAllocator();
	m_HeapAllocator->HeapMem = m_HeapAllocator + 1;
	m_HeapAllocator->HeapMemSize = HeapMemSize;
	m_HeapAllocator->HeapRemainSize = HeapMemSize - sizeof(HeapAllocator);


	auto FirstFreeBlock = static_cast<MemBlock*>(m_HeapAllocator->HeapMem);
	SetBlockSize(FirstFreeBlock, m_HeapAllocator->HeapRemainSize);
	FirstFreeBlock->m_Next = nullptr;
	FirstFreeBlock->m_Previous = nullptr;

	Increment(m_HeapAllocator->HeapMem, sizeof(MemBlock));
	m_HeapAllocator->HeapRemainSize -= sizeof(MemBlock);
	m_HeapAllocator->m_freelist = FirstFreeBlock;
	//cout << "Debug Freelist" << m_HeapAllocator->m_freelist;
	return m_HeapAllocator;


}

void Destroy(HeapAllocator* m_HeapAllocator)
{
	m_HeapAllocator->~HeapAllocator();
}

inline void Renew(HeapAllocator* m_HeapAllocator)
{
	m_HeapAllocator->HeapRemainSize = m_HeapAllocator->HeapMemSize - sizeof(HeapAllocator);
	auto FirstFreeBlock = static_cast<MemBlock*>(m_HeapAllocator->HeapMem);
	SetBlockSize(FirstFreeBlock, m_HeapAllocator->HeapRemainSize);
	FirstFreeBlock->m_Next = nullptr;
	FirstFreeBlock->m_Previous = nullptr;
	Increment(m_HeapAllocator->HeapMem, sizeof(MemBlock));
	m_HeapAllocator->HeapRemainSize -= sizeof(MemBlock);
	m_HeapAllocator->m_freelist = FirstFreeBlock;
}

