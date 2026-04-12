/*
 * memchk.h
 *
 * Mother of all ancestor classes.
 *
 * Portable Tools Library
 *
 * Copyright (c) 1993-1998 Equivalence Pty. Ltd.
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Portable Windows Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Portions are Copyright (C) 1993 Free Software Foundation, Inc.
 * All Rights Reserved.
 *
 * Contributor(s): ______________________________________.
 */

#ifndef PTLIB_MEMCHK_H
#define PTLIB_MEMCHK_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

///////////////////////////////////////////////////////////////////////////////
// Memory management

#if PMEMORY_CHECK || (defined(_MSC_VER) && defined(_DEBUG)) 

#define PMEMORY_HEAP 1

/** Memory heap checking class.
This class implements the memory heap checking and validation functions. It
maintains lists of allocated block so that memory leaks can be detected. It
also initialises memory on allocation and deallocation to help catch errors
involving the use of dangling pointers.
*/
class PMemoryHeap {
  protected:
    /// Initialise the memory checking subsystem.
    PMemoryHeap();

  public:
    // Clear up the memory checking subsystem, dumping memory leaks.
    ~PMemoryHeap();

    /** Allocate a memory block.
       This allocates a new memory block and keeps track of it. The memory
       block is filled with the value in the <code>allocFillChar</code> member variable
       to help detect uninitialised structures.
       @return pointer to newly allocated memory block.
     */
    static void * Allocate(
      size_t nSize,           ///< Number of bytes to allocate.
      const char * file,      ///< Source file name for allocating function.
      int line,               ///< Source file line for allocating function.
      const char * className  ///< Class name for allocating function.
    );

    /** Allocate a memory block.
       This allocates a new memory block and keeps track of it. The memory
       block is filled with the value in the <code>allocFillChar</code> member variable
       to help detect uninitialised structures.
       @return pointer to newly allocated memory block.
     */
    static void * Allocate(
      size_t count,       ///< Number of items to allocate.
      size_t iSize,       ///< Size in bytes of each item.
      const char * file,  ///< Source file name for allocating function.
      int line            ///< Source file line for allocating function.
    );

    /** Change the size of an allocated memory block.
       This allocates a new memory block and keeps track of it. The memory
       block is filled with the value in the <code>allocFillChar</code> member variable
       to help detect uninitialised structures.
      @return pointer to reallocated memory block. Note this may
      {\em not} be the same as the pointer passed into the function.
     */
    static void * Reallocate(
      void * ptr,         ///< Pointer to memory block to reallocate.
      size_t nSize,       ///< New number of bytes to allocate.
      const char * file,  ///< Source file name for allocating function.
      int line            ///< Source file line for allocating function.
    );

    /** Free a memory block.
      The memory is deallocated, a warning is displayed if it was never
      allocated. The block of memory is filled with the value in the
      <code>freeFillChar</code> member variable.
     */
    static void Deallocate(
      void * ptr,             ///< Pointer to memory block to deallocate.
      const char * className  ///< Class name for deallocating function.
    );

    /** Validation result.
     */
    enum Validation {
      Ok,         // All good.
      Bad,        // Bad pointer, not allocated by us, double deletion or just random address.
      Trashed,    // Heap is trashed, linked list failed.
      Corrupt,    // Corrupted guard bytes or object.
      Inactive    // Heap checking disabled or after final destruction.
    };
    /** Validate the memory pointer.
        The <code>ptr</code> parameter is validated as a currently allocated heap
        variable.
        @return Ok for pointer is in heap, Bad for pointer is not in the heap
        or Trashed if the pointer is in the heap but has overwritten the guard
        bytes before or after the actual data part of the memory block.
     */
    static Validation Validate(
      const void * ptr,       ///< Pointer to memory block to check
      const char * className, ///< Class name it should be.
      ostream * error         ///< Stream to receive error message (may be NULL)
    );

    /** Validate all objects in memory.
       This effectively calls Validate() on every object in the heap.
        @return true if every object in heap is Ok.
     */
    static PBoolean ValidateHeap(
      ostream * error = NULL  ///< Stream to output, use default if NULL
    );

    /** Ignore/Monitor allocations.
       Set internal flag so that allocations are not included in the memory
       leak check on program termination.
       Returns the previous state.
     */
    static PBoolean SetIgnoreAllocations(
      PBoolean ignore  ///< New flag for allocation ignoring.
    );

    /** Get memory check system statistics.
        Dump statistics output to the default stream.
     */
    static void DumpStatistics();
    /** Get memory check system statistics.
        Dump statistics output to the specified stream.
     */
    static void DumpStatistics(ostream & strm /** Stream to output to */);

    typedef unsigned alloc_t;

#if PMEMORY_CHECK
    struct State {
      alloc_t allocationNumber;
    };
#else
	typedef _CrtMemState State;
#endif

    /* Get memory state.
      This returns a state that may be used to determine where to start dumping
      objects from.
     */
    static void GetState(
      State & state  ///< Memory state
    );

    /** Dump allocated objects.
       Dump ojects allocated and not deallocated since the specified object
       number. This would be a value returned by the <code>GetAllocationRequest()</code>
       function.

       Output is to the default stream.
     */
    static void DumpObjectsSince(
      const State & when    ///< Memory state to begin dump from.
    );

    /** Dump allocated objects.
       Dump ojects allocated and not deallocated since the specified object
       number. This would be a value returned by the <code>GetAllocationRequest()</code>
       function.
     */
    static void DumpObjectsSince(
      const State & when,   ///< Memory state to begin dump from.
      ostream & strm        ///< Stream to output dump
    );

    /** Set break point allocation number.
      Set the allocation request number to cause an assert. This allows a
      developer to cause a halt in a debugger on a certain allocation allowing
      them to determine memory leaks allocation point.
     */
    static void SetAllocationBreakpoint(
      alloc_t point   ///< Allocation number to stop at.
    );

#if PMEMORY_CHECK

  protected:
    static PMemoryHeap & GetInstance();

    void * InternalAllocate(
      size_t nSize,           // Number of bytes to allocate.
      const char * file,      // Source file name for allocating function.
      int line,               // Source file line for allocating function.
      const char * className, // Class name for allocating function.
      bool zeroFill           // FIll with zeros
    );
    void * InternalReallocate(
      void * ptr,             // Pointer to memory block to reallocate
      size_t nSize,           // Number of bytes to allocate.
      const char * file,      // Source file name for allocating function.
      int line                // Source file line for allocating function.
    );
    void InternalDeallocate(
      void * ptr,
      const char * className
    );
    Validation InternalValidate(
      const void * ptr,       // Pointer to memory block to check
      const char * className, // Class name it should be.
      ostream * error         // Stream to receive error message (may be NULL)
    );
    bool InternalValidateHeap(ostream * error);
    void InternalDumpStatistics(ostream & strm);
    void InternalDumpObjectsSince(DWORD objectNumber, ostream & strm);

    enum Flags {
      NoLeakPrint = 1
    };

#pragma pack(1)
    struct Header {
      enum {
        // Assure that the Header struct is aligned to 8 byte boundary
        NumGuardBytes = 16 - (sizeof(Header *) +
                              sizeof(Header *) +
                              sizeof(const char *) +
                              sizeof(const char *) +
                              sizeof(size_t) +
                              sizeof(alloc_t) +
                              sizeof(uint16_t) +
                              sizeof(uint8_t) +
                              sizeof(PThreadIdentifier)
                              )%8
      };

      Header     * m_prev;
      Header     * m_next;
      const char * m_className;
      const char * m_fileName;
      size_t       m_size;
      alloc_t      m_request;
      uint16_t     m_line;
      uint8_t      m_flags;
      PThreadIdentifier m_threadId;
      char         m_guard[NumGuardBytes];

      static char GuardBytes[NumGuardBytes];
    };
#pragma pack()

    enum {
      e_Destroyed =-1,
      e_Disabled,
      e_TrackOnly,
      e_Active
    } m_state;

    Header * m_listHead;
    Header * m_listTail;

    alloc_t m_allocationRequest;
    alloc_t m_allocationBreakpoint;
    alloc_t m_firstRealObject;
    uint8_t m_flags;

    char  m_allocFillChar;
    char  m_freeFillChar;

    size_t  m_currentMemoryUsage;
    size_t  m_peakMemoryUsage;
    alloc_t m_currentObjects;
    alloc_t m_peakObjects;
    alloc_t m_totalObjects;

    ostream * m_leakDumpStream;

    std::mutex m_mutex;
    void Lock() { m_mutex.lock(); }
    void Unlock() { m_mutex.unlock(); }
#else

    static void CreateInstance();
#define P_CLIENT_BLOCK (_CLIENT_BLOCK|(0x61<<16)) // This identifies a PObject derived class
    _CrtMemState initialState;

#endif // PMEMORY_CHECK
};


/** Allocate memory for the run time library.
This version of free is used for data that is not to be allocated using the
memory check system, ie will be free'ed inside the C run time library.
*/
inline void * runtime_malloc(size_t bytes /** Size of block to allocate */ ) { return malloc(bytes); }

/** Free memory allocated by run time library.
This version of free is used for data that is not allocated using the
memory check system, ie was malloc'ed inside the C run time library.
*/
inline void runtime_free(void * ptr /** Memory block to free */ ) { free(ptr); }


/** Override of system call for memory check system.
This macro is used to allocate memory via the memory check system selected
with the <code>PMEMORY_CHECK</code> compile time option. It will include the source file
and line into the memory allocation to allow the PMemoryHeap class to keep
track of the memory block.
*/
#define malloc(s) PMemoryHeap::Allocate(s, __FILE__, __LINE__, NULL)

/** Override of system call for memory check system.
This macro is used to allocate memory via the memory check system selected
with the <code>PMEMORY_CHECK</code> compile time option. It will include the source file
and line into the memory allocation to allow the PMemoryHeap class to keep
track of the memory block.
*/
#define calloc(n,s) PMemoryHeap::Allocate(n, s, __FILE__, __LINE__)

/** Override of system call for memory check system.
This macro is used to allocate memory via the memory check system selected
with the <code>PMEMORY_CHECK</code> compile time option. It will include the source file
and line into the memory allocation to allow the PMemoryHeap class to keep
track of the memory block.
*/
#define realloc(p,s) PMemoryHeap::Reallocate(p, s, __FILE__, __LINE__)


/** Override of system call for memory check system.
This macro is used to deallocate memory via the memory check system selected
with the <code>PMEMORY_CHECK</code> compile time option. It will include the source file
and line into the memory allocation to allow the PMemoryHeap class to keep
track of the memory block.
*/
#define free(p) PMemoryHeap::Deallocate(p, NULL)


/** Override of system call for memory check system.
This macro is used to deallocate memory via the memory check system selected
with the <code>PMEMORY_CHECK</code> compile time option. It will include the source file
and line into the memory allocation to allow the PMemoryHeap class to keep
track of the memory block.
*/
#define cfree(p) PMemoryHeap::Deallocate(p, NULL)


/** Macro for overriding system default <code>new</code> operator.
This macro is used to allocate memory via the memory check system selected
with the PMEMORY_CHECK compile time option. It will include the source file
and line into the memory allocation to allow the PMemoryHeap class to keep
track of the memory block.

This macro could be used instead of the system <code>new</code> operator. Or you can place
the line
<pre><code>
  #define new PNEW
</code></pre>
at the begining of the source file, after all declarations that use the
<code>#PCLASSINFO</code> macro.
*/
#define PNEW  new (__FILE__, __LINE__)

#if !defined(_MSC_VER) || _MSC_VER<1200
#define PSPECIAL_DELETE_FUNCTION
#else
#define PSPECIAL_DELETE_FUNCTION \
    void operator delete(void * ptr, const char *, int) \
      { PMemoryHeap::Deallocate(ptr, Class()); } \
    void operator delete[](void * ptr, const char *, int) \
      { PMemoryHeap::Deallocate(ptr, Class()); }
#endif

#define PNEW_AND_DELETE_FUNCTIONS(align) \
    void * operator new(size_t nSize, const char * file, int line) \
      { return PMemoryHeap::Allocate(nSize, file, line, Class()); } \
    void * operator new(size_t nSize) \
      { return PMemoryHeap::Allocate(nSize, NULL, 0, Class()); } \
    void operator delete(void * ptr) \
      { PMemoryHeap::Deallocate(ptr, Class()); } \
    void * operator new(size_t, void * placement) \
      { return placement; } \
    void operator delete(void *, void *) \
      { } \
    void * operator new[](size_t nSize, const char * file, int line) \
      { return PMemoryHeap::Allocate(nSize, file, line, Class()); } \
    void * operator new[](size_t nSize) \
      { return PMemoryHeap::Allocate(nSize, NULL, 0, Class()); } \
    void operator delete[](void * ptr) \
      { PMemoryHeap::Deallocate(ptr, Class()); } \
    PSPECIAL_DELETE_FUNCTION


inline void * operator new(size_t nSize, const char * file, int line)
  { return PMemoryHeap::Allocate(nSize, file, line, NULL); }

inline void * operator new[](size_t nSize, const char * file, int line)
  { return PMemoryHeap::Allocate(nSize, file, line, NULL); }

#ifndef __GNUC__
P_PUSH_MSVC_WARNINGS(28251)
void * operator new(size_t nSize);
void * operator new[](size_t nSize);
P_POP_MSVC_WARNINGS()

void operator delete(void * ptr);
void operator delete[](void * ptr);

#if defined(_MSC_VER) && _MSC_VER>=1200
inline void operator delete(void * ptr, const char *, int)
  { PMemoryHeap::Deallocate(ptr, NULL); }

inline void operator delete[](void * ptr, const char *, int)
  { PMemoryHeap::Deallocate(ptr, NULL); }
#endif
#endif


class PMemoryHeapIgnoreAllocationsForScope {
public:
  PMemoryHeapIgnoreAllocationsForScope() : previousIgnoreAllocations(PMemoryHeap::SetIgnoreAllocations(true)) { }
  ~PMemoryHeapIgnoreAllocationsForScope() { PMemoryHeap::SetIgnoreAllocations(previousIgnoreAllocations); }
private:
  PBoolean previousIgnoreAllocations;
};

#define PMEMORY_IGNORE_ALLOCATIONS_FOR_SCOPE PMemoryHeapIgnoreAllocationsForScope instance_PMemoryHeapIgnoreAllocationsForScope

class PMemoryAllocationBreakpoint {
public:
  PMemoryAllocationBreakpoint(DWORD point)
  {
    PMemoryHeap::SetAllocationBreakpoint(point);
  }
};

#define PMEMORY_ALLOCATION_BREAKPOINT(point) PMemoryAllocationBreakpoint PMemoryAllocationBreakpointInstance(point)


#else // PMEMORY_CHECK || (defined(_MSC_VER) && defined(_DEBUG))

#define PMEMORY_HEAP 0

#define PNEW new

#if _MSC_VER < 1800
  #define PNEW_AND_DELETE_FUNCTIONS(align)
#else
  #define PNEW_AND_DELETE_FUNCTIONS_ALIGNED(align) \
      void * operator new(size_t nSize) \
        { return _aligned_malloc(nSize, align); } \
      void operator delete(void * ptr) \
        { _aligned_free(ptr); } \
      void * operator new(size_t, void * placement) \
        { return placement; } \
      void operator delete(void *, void *) \
        { } \
      void * operator new[](size_t nSize) \
        { return _aligned_malloc(nSize, align); } \
      void operator delete[](void * ptr) \
        { _aligned_free(ptr); }

  #define PNEW_AND_DELETE_FUNCTIONS64 PNEW_AND_DELETE_FUNCTIONS_ALIGNED(64)
  #define PNEW_AND_DELETE_FUNCTIONS32 PNEW_AND_DELETE_FUNCTIONS_ALIGNED(32)
  #define PNEW_AND_DELETE_FUNCTIONS16 PNEW_AND_DELETE_FUNCTIONS_ALIGNED(16)
  #define PNEW_AND_DELETE_FUNCTIONS8  PNEW_AND_DELETE_FUNCTIONS_ALIGNED(8)
  #define PNEW_AND_DELETE_FUNCTIONS4  PNEW_AND_DELETE_FUNCTIONS_ALIGNED(4)
  #define PNEW_AND_DELETE_FUNCTIONS2  PNEW_AND_DELETE_FUNCTIONS_ALIGNED(2)
  #define PNEW_AND_DELETE_FUNCTIONS0
  #define PNEW_AND_DELETE_FUNCTIONS(align) PNEW_AND_DELETE_FUNCTIONS##align
#endif

#define runtime_malloc(s) malloc(s)
#define runtime_free(p) free(p)

#define PMEMORY_IGNORE_ALLOCATIONS_FOR_SCOPE
#define PMEMORY_ALLOCATION_BREAKPOINT(point)

#endif // PMEMORY_CHECK || (defined(_MSC_VER) && defined(_DEBUG))


#endif // PTLIB_MEMCHK_H


// End Of File ///////////////////////////////////////////////////////////////
