#pragma once
#include "BitArray.h"
#include <cstdio>
class FixedSizeAllocator
{
#ifdef WIN32
	typedef uint32_t t_BitData;
#else
	typedef uint64_t t_BitData;
#endif // WIN32
public:
	inline FixedSizeAllocator(void*& io_pMem, size_t i_MemPerBlock, size_t i_BlockNums);
	inline ~FixedSizeAllocator();
	static FixedSizeAllocator* Create(void*& io_pHeapMem, size_t i_MemPerBlock, size_t i_BlockNums);
	static void Destroy(FixedSizeAllocator* m_FixedSizeAllocator);
	inline void* malloc();
	inline void free(void* i_pFree);
	inline bool Contains(void* i_ptr);


private:
	void* m_pMem;
	size_t m_MemPerBlock;
	size_t m_BlockNums;
	BitArray* m_BitArray;
	inline size_t GetIndexOfBitsFromAddress(void* i_Address);
};

inline FixedSizeAllocator::FixedSizeAllocator(void*& i_pMem, size_t i_MemPerBlock, size_t i_BlockNums)
{
	i_pMem = reinterpret_cast<char*>(i_pMem) + sizeof(FixedSizeAllocator);
	m_pMem = i_pMem;
	m_MemPerBlock = i_MemPerBlock;
	m_BlockNums = i_BlockNums;
	i_pMem = reinterpret_cast<char*>(i_pMem) + i_BlockNums * i_MemPerBlock;
	m_BitArray = BitArray::Create(m_BlockNums, false, i_pMem);
}

inline FixedSizeAllocator::~FixedSizeAllocator()
{
	BitArray::Destroy(m_BitArray);
}

inline FixedSizeAllocator* FixedSizeAllocator::Create(void*& io_pHeapMem, size_t i_MemPerBlock, size_t i_BlockNums)
{
	auto p_FSA = reinterpret_cast<FixedSizeAllocator*>(io_pHeapMem);
	new (p_FSA) FixedSizeAllocator(io_pHeapMem, i_MemPerBlock, i_BlockNums);
#ifdef _DEBUG
	//printf("Fixed Size Allocator Created, Start Pos is %zu\n",m_pMem);

#endif // _DEBUG


	return p_FSA;
}


inline void FixedSizeAllocator::Destroy(FixedSizeAllocator* m_FixedSizeAllocator)
{
#ifdef _DEBUG
	if (!m_FixedSizeAllocator->m_BitArray->AreAllBitsClear())
	{
		printf("WARNING: NOT ALL MEMORY HAS BEEN FREED.");
		//DO I really need to free the memory here? Or printing a warning could be enough?

		/*for (size_t i = 0; i < m_FixedSizeAllocator->m_BlockNums; i++)
		{
			//m_FixedSizeAllocator->free(m_FixedSizeAllocator->free)
		}*/
	}
	m_FixedSizeAllocator->~FixedSizeAllocator();
#endif // _DEBUG

}

inline void* FixedSizeAllocator::malloc()
{
	size_t IndexOfFirstClear = 0;
	if (!this->m_BitArray->GetFirstClearBit(IndexOfFirstClear))
		return nullptr;
	this->m_BitArray->SetBit(IndexOfFirstClear);
	//cout << "Debug " << IndexOfFirstClear << endl;
	return(reinterpret_cast<char*>(this->m_pMem) + IndexOfFirstClear * m_MemPerBlock);
}

inline void FixedSizeAllocator::free(void* i_pFree)
{
	assert(Contains(i_pFree));
	size_t IndexOfBit = GetIndexOfBitsFromAddress(i_pFree);
	//cout << "Debug " << IndexOfBit << endl;
	assert(m_BitArray->IsBitSet((IndexOfBit)));
	m_BitArray->ClearBit(IndexOfBit);
	return;
}

inline bool FixedSizeAllocator::Contains(void* i_ptr)
{
	//cout << "DEBUG " << m_pMem << " " << m_BlockNums << " " << m_MemPerBlock << endl;
	return (i_ptr < reinterpret_cast<char*>(m_pMem) + m_BlockNums * m_MemPerBlock) && (i_ptr >= m_pMem);
}

inline size_t FixedSizeAllocator::GetIndexOfBitsFromAddress(void* i_Address)
{
	size_t DistFromStart = reinterpret_cast<char*>(i_Address) - reinterpret_cast<char*>(m_pMem);
	//cout << "FUCK " << DistFromStart << endl;
	assert(DistFromStart % m_MemPerBlock == 0);
	return DistFromStart / m_MemPerBlock;
}
