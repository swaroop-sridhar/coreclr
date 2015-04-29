//
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//
//*****************************************************************************
// UtilCode.h
//
// Utility functions implemented in UtilCode.lib.
//
//*****************************************************************************

#ifndef __Util_h__
#define __Util_h__

//------------------------------------------------------------------------
// BitPosition: Return the position of the single bit that is set in 'value'.
//
// Return Value:
//    The position (0 is LSB) of bit that is set in 'value'
//
// Notes:
//    'value' must have exactly one bit set.
//    The algorithm is as follows:
//    - PRIME is a prime bigger than sizeof(unsigned int), which is not of the
//      form 2^n-1.
//    - Taking the modulo of 'value' with this will produce a unique hash for all
//      powers of 2 (which is what "value" is).
//    - Entries in hashTable[] which are -1 should never be used. There
//      should be PRIME-8*sizeof(value) entries which are -1 .
//

#ifndef _ASSERTE
#define _ASSERTE(expr)
#endif

#define assert(expr)

#define LOG(expr)

inline
unsigned            BitPosition(unsigned value)
{
    _ASSERTE((value != 0) && ((value & (value-1)) == 0));
    const unsigned PRIME = 37;

    static const char hashTable[PRIME] =
    {
        -1,  0,  1, 26,  2, 23, 27, -1,  3, 16,
        24, 30, 28, 11, -1, 13,  4,  7, 17, -1,
        25, 22, 31, 15, 29, 10, 12,  6, -1, 21,
        14,  9,  5, 20,  8, 19, 18
    };

    _ASSERTE(PRIME >= 8*sizeof(value));
    _ASSERTE(sizeof(hashTable) == PRIME);


    unsigned hash   = value % PRIME;
    unsigned index  = hashTable[hash];
    _ASSERTE(index != (unsigned char)-1);

    return index;
}

class IAllocator {
public:
  virtual void* Alloc(size_t sz) = 0;

  // Allocate space for an array of "elems" elements, each of size "elemSize".
  virtual void* ArrayAlloc(size_t elems, size_t elemSize) = 0;

  virtual void  Free(void* p) = 0;
};

template <class T> class CQuickSort {
protected:
  T           *m_pBase;                   // Base of array to sort.
private:
  SSIZE_T     m_iCount;                   // How many items in array.
  SSIZE_T     m_iElemSize;                // Size of one element.
public:
  CQuickSort(
    T           *pBase,                 // Address of first element.
    SSIZE_T     iCount) :               // How many there are.
    m_pBase(pBase),
    m_iCount(iCount),
    m_iElemSize(sizeof(T))
  {
  }

  //*****************************************************************************
  // Call to sort the array.
  //*****************************************************************************
  inline void Sort()
  {
    SortRange(0, m_iCount - 1);
  }

protected:
  //*****************************************************************************
  // Override this function to do the comparison.
  //*****************************************************************************
  virtual FORCEINLINE int Compare(        // -1, 0, or 1
    T           *psFirst,               // First item to compare.
    T           *psSecond)              // Second item to compare.
  {
    return (memcmp(psFirst, psSecond, sizeof(T)));
  }

  virtual FORCEINLINE void Swap(
    SSIZE_T     iFirst,
    SSIZE_T     iSecond)
  {
    if (iFirst == iSecond) return;
    T sTemp(m_pBase[iFirst]);
    m_pBase[iFirst] = m_pBase[iSecond];
    m_pBase[iSecond] = sTemp;
  }

private:
  inline void SortRange(
    SSIZE_T     iLeft,
    SSIZE_T     iRight)
  {
    SSIZE_T     iLast;
    SSIZE_T     i;                      // loop variable.

    for (;;) {
      // if less than two elements you're done.
      if (iLeft >= iRight)
        return;

      // ASSERT that we now have valid indicies.  This is statically provable
      // since this private function is only called with valid indicies,
      // and iLeft and iRight only converge towards eachother.  However,
      // PreFast can't detect this because it doesn't know about our callers.
      assert(iLeft >= 0 && iLeft < m_iCount);
      assert(iRight >= 0 && iRight < m_iCount);

      // The mid-element is the pivot, move it to the left.
      Swap(iLeft, (iLeft + iRight) / 2);
      iLast = iLeft;

      // move everything that is smaller than the pivot to the left.
      for (i = iLeft + 1; i <= iRight; i++) {
        if (Compare(&m_pBase[i], &m_pBase[iLeft]) < 0) {
          Swap(i, ++iLast);
        }
      }

      // Put the pivot to the point where it is in between smaller and larger elements.
      Swap(iLeft, iLast);

      // Sort each partition.
      SSIZE_T iLeftLast = iLast - 1;
      SSIZE_T iRightFirst = iLast + 1;
      if (iLeftLast - iLeft < iRight - iRightFirst) {   // Left partition is smaller, sort it recursively
        SortRange(iLeft, iLeftLast);
        // Tail call to sort the right (bigger) partition
        iLeft = iRightFirst;
        //iRight = iRight;
        continue;
      }
      else {   // Right partition is smaller, sort it recursively
        SortRange(iRightFirst, iRight);
        // Tail call to sort the left (bigger) partition
        //iLeft = iLeft;
        iRight = iLeftLast;
        continue;
      }
    }
  }

}
#endif // __Util_h__
