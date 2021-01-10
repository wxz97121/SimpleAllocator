#pragma once
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <intrin.h>
#include <assert.h>

class BitArray
{
#ifdef WIN32
	typedef uint32_t t_BitData;
	typedef long* BitTestOutType;
#define t_BitDataMax (UINT32_MAX)
#define BitTest _bittest
#define BitScanReverse _BitScanReverse
#define BitScanForward _BitScanForward
#define BitTestAndSet _bittestandset
#define BitTestAndReset _bittestandreset
#else
	typedef uint64_t t_BitData;
	typedef __int64* BitTestOutType;
#define t_BitDataMax (UINT64_MAX);
#define BitTest _bittest64
#define BitScanReverse _BitScanReverse64
#define BitScanForward _BitScanForward64
#define BitTestAndSet _bittestandset64
#define BitTestAndReset _bittestandreset64
#endif // WIN32



private:
	t_BitData* m_pBits;
	size_t m_numElements = 0;
	size_t m_numBits = 0;
	size_t m_numSetBits = 0;
	static const size_t bitsPerElement = sizeof(t_BitData) * 8;
public:
	inline BitArray(size_t i_numBits, bool i_bInitToOne, void*& i_pMem);
	inline BitArray(size_t i_numBits, bool i_bInitToOne);
	static BitArray* Create(size_t i_numBits, bool i_bInitToOne, void*& i_pMem);
	static void Destroy(BitArray* i_pBitArray);
	~BitArray();

	void ClearAll(void);
	void SetAll(void);

	bool AreAllBitsClear(void) const;
	bool AreAllBitsSet(void) const;

	inline bool IsBitSet(size_t i_bitNumber) const;
	inline bool IsBitClear(size_t i_bitNumber) const;

	void SetBit(size_t i_bitNumber);
	void ClearBit(size_t i_bitNumber);

	bool GetFirstClearBit(size_t& o_bitNumber) const;
	bool GetFirstSetBit(size_t& o_bitNumber) const;

	bool operator[](size_t i_index) const;


};
//I hope BitArray could be used in either Allocator or normal situation. 
BitArray::BitArray(size_t i_numBits, bool i_bInitToOne)
{
	m_numElements = (i_numBits / bitsPerElement) + ((i_numBits % bitsPerElement) != 0);
	m_numBits = i_numBits;
	m_numSetBits = i_bInitToOne ? i_numBits : 0;

	//Construct the array depends on different ways, depending on are we using Placement New.
	m_pBits = new t_BitData[m_numElements];

	memset(m_pBits, i_bInitToOne ? 1 : 0, m_numElements);
}

BitArray::BitArray(size_t i_numBits, bool i_bInitToOne, void*& i_pMem)
{
	m_numElements = (i_numBits / bitsPerElement) + ((i_numBits % bitsPerElement) != 0);
	m_numBits = i_numBits;
	m_numSetBits = i_bInitToOne ? i_numBits : 0;

	//Construct the array depends on different ways, depending on are we using Placement New.
	m_pBits = reinterpret_cast<t_BitData*>(i_pMem);
	for (size_t i = 0; i < m_numElements; i++)
		new (m_pBits + i) t_BitData();
	i_pMem = (m_pBits + m_numElements);
	memset(m_pBits, i_bInitToOne ? 1 : 0, m_numElements);
}

inline BitArray* BitArray::Create(size_t i_numBits, bool i_bInitToOne, void*& i_pMem)
{
	if (!i_pMem) return nullptr;
	auto pBitArray = static_cast<BitArray*>(i_pMem);
	i_pMem = reinterpret_cast<char*>(i_pMem) + sizeof(BitArray);
	pBitArray = new (pBitArray) BitArray(i_numBits, i_bInitToOne, i_pMem);
	return pBitArray;
}

inline void BitArray::Destroy(BitArray* i_pBitArray)
{
	return;
}

inline BitArray::~BitArray()
{
	delete(m_pBits);
}

inline void BitArray::ClearAll(void)
{
	memset(m_pBits, 0, m_numElements);
	m_numSetBits = 0;
}

inline void BitArray::SetAll(void)
{
	memset(m_pBits, 1, m_numElements);
	m_numSetBits = 1;
}

inline bool BitArray::AreAllBitsClear(void) const
{
	return m_numSetBits == 0;
}

inline bool BitArray::AreAllBitsSet(void) const
{
	return m_numSetBits == m_numBits;
}

inline bool BitArray::IsBitSet(size_t i_bitNumber) const
{
	assert(i_bitNumber < m_numBits);
	size_t IndexOfElement = i_bitNumber / bitsPerElement;
	size_t IndexOfBit = i_bitNumber % bitsPerElement;
	return BitTest(reinterpret_cast<BitTestOutType>(&m_pBits[IndexOfElement]), static_cast<t_BitData>(IndexOfBit));
}

inline bool BitArray::IsBitClear(size_t i_bitNumber) const
{
	return !IsBitSet(i_bitNumber);
}

inline void BitArray::SetBit(size_t i_bitNumber)
{
	assert(i_bitNumber < m_numBits);
	size_t IndexOfElement = i_bitNumber / bitsPerElement;
	size_t IndexOfBit = i_bitNumber % bitsPerElement;
	auto tmp = BitTestAndSet(reinterpret_cast<BitTestOutType>(&m_pBits[IndexOfElement]), static_cast<t_BitData>(IndexOfBit));
	if (!tmp) m_numSetBits++;
}

inline void BitArray::ClearBit(size_t i_bitNumber)
{
	assert(i_bitNumber < m_numBits);
	size_t IndexOfElement = i_bitNumber / bitsPerElement;
	size_t IndexOfBit = i_bitNumber % bitsPerElement;
	auto tmp = BitTestAndReset(reinterpret_cast<BitTestOutType>(&m_pBits[IndexOfElement]), static_cast<t_BitData>(IndexOfBit));
	if (tmp) m_numSetBits--;
}

inline bool BitArray::GetFirstClearBit(size_t& o_bitNumber) const
{
	if (AreAllBitsSet()) return false;
	size_t index = 0;
	for (size_t i = 0; i < m_numElements; i++)
	{
		if (BitScanForward(reinterpret_cast<unsigned long*>(&index), ~m_pBits[i]))
		{
			index = index + i * bitsPerElement;
			o_bitNumber = index;
			return true;
		}
	}
	return false;

}

inline bool BitArray::GetFirstSetBit(size_t& o_bitNumber) const
{
	if (AreAllBitsClear()) return false;
	size_t index = 0;
	for (size_t i = 0; i < m_numElements; i++)
	{
		if (BitScanForward(reinterpret_cast<unsigned long*>(&index), m_pBits[i]))
		{
			index = index + i * bitsPerElement;
			o_bitNumber = index;
			return true;
		}
	}
	return false;
}

inline bool BitArray::operator[](size_t i_index) const
{
	assert(i_index < m_numBits);
	size_t IndexOfElement = i_index / bitsPerElement;
	size_t IndexOfBit = i_index % bitsPerElement;
	return BitTest(reinterpret_cast<BitTestOutType>(&m_pBits[IndexOfElement]), static_cast<t_BitData>(IndexOfBit));
}
