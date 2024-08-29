/*
 * array.h
 *
 * Linear Array Container classes.
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
 * The Original Code is Portable Tools Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Portions are Copyright (C) 1993 Free Software Foundation, Inc.
 * All Rights Reserved.
 *
 * Contributor(s): ______________________________________.
 */

#ifndef PTLIB_ARRAY_H
#define PTLIB_ARRAY_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <vector>
#include <span>

#include <ptlib/contain.h>

///////////////////////////////////////////////////////////////////////////////
// An array of some base type

/**This template class maps the <code>PAbstractArray</code> to a specific element type. The
   functions in this class primarily do all the appropriate casting of types.

   The following classes are instantiated automatically for the basic scalar
   types:
   \li <code>PCharArray</code>
   \li <code>PBYTEArray</code>
   \li <code>PShortArray</code>
   \li <code>PWORDArray</code>
   \li <code>PIntArray</code>
   \li <code>PUnsignedArray</code>
   \li <code>PLongArray</code>
   \li <code>PDWORDArray</code>
 */
template <typename T> class PScalarArray : public PObject, public std::vector<T>
{
    PCLASSINFO(PScalarArray, PObject);
  public:
    typedef T value_type;
    typedef std::vector<value_type> vector_type;

  /**@name Construction */
  //@{
    /**Construct a new dynamic array of elements of the specified type. The
       array is initialised to all zero bytes. Note that this may not be
       logically equivalent to the zero value for the type, though this would
       be very rare.
     */
    PScalarArray(
      PINDEX initialSize = 0  ///< Initial number of elements in the array.
    ) : vector_type(initialSize) { }

    /**Construct a new dynamic array of elements of the specified type.
     */
    PScalarArray(
      T const * buffer,   ///< Pointer to an array of the elements of type \b T.
      PINDEX length       ///< Number of elements pointed to by \p buffer.
    ) : vector_type(buffer, buffer+length) { }

    /**Construct a new dynamic array of elements of the specified type.
     */
    PScalarArray(
      std::initializer_list<T> init  ///< Initial value(s)
    ) : vector_type(init)
    { }
//@}

  /**@name Overrides from class PObject */
  //@{
    /** Clone the object.
     */
    /**Get the relative rank of the two arrays. The following algorithm is
       employed for the comparison:

       \arg \c EqualTo if the two arrays are identical in length
       and each objects values, not pointer, are equal.

       \arg \c LessThan if the instances object value at an
       ordinal position is less than the corresponding objects value in the
       <code>obj</code> parameters list.

       This is also returned if all objects are equal and the instances list
       length is less than the <code>obj</code> parameters list length.

       \arg \c GreaterThan if the instances object value at an
       ordinal position is greater than the corresponding objects value in the
       <code>obj</code> parameters list.

       This is also returned if all objects are equal and the instances list
       length is greater than the <code>obj</code> parameters list length.

       @return
       comparison of the two objects, <code>EqualTo</code> for same,
       <code>LessThan</code> for <code>obj</code> logically less than the
       object and <code>GreaterThan</code> for <code>obj</code> logically
       greater than the object.
     */
    virtual Comparison Compare(
      const PObject & obj   ///< Object being compared to
    ) const override
    {
      const PScalarArray & t1 = *this;
      const PScalarArray & t2 = dynamic_cast<const PScalarArray &>(obj);
      const auto & it1 = t1.begin();
      const auto & it2 = t2.begin();
      while (it1 != t1.end() && it2 != t2.end()) {
        if (*it1 < *it2)
          return PObject::LessThan;
        if (*it1 > *it2)
          return PObject::GreaterThan;
      }
      if (it2 != t2.end())
        return PObject::LessThan;
      if (it1 != t1.end())
        return PObject::GreaterThan;
      return PObject::EqualTo;
    }

    virtual PObject * Clone() const override
    {
      return PNEW PScalarArray(*this);
    }
  //@}

  /**@name New functions for class */
  //@{
    bool IsEmpty() const { return this->empty(); }
    PINDEX GetSize() const { return this->size(); }
    PINDEX GetLength() const { return this->size(); }
    virtual bool SetSize(PINDEX newSize) { this->resize(newSize); return true; }

    bool SetMinSize(PINDEX newSize)
    {
      if (this->size() < newSize)
        this->resize(newSize);
      return true;
    }

    /**Set the specific element in the array. The array will automatically
       expand, if necessary, to fit the new element in.

       @return
       <code>true</code> if new memory for the array was successfully allocated.
     */
    bool SetAt(
      PINDEX index,   ///< Position in the array to set the new value.
      T val           ///< Value to set in the array.
    ) {
      if (!SetMinSize(index + 1))
        return false;
      GetPointer()[index] = val;
      return true;
    }

    /**Get a value from the array. If the \p index is beyond the end
       of the allocated array then a zero value is returned.

       @return
       Value at the array position.
     */
    T GetAt(
      PINDEX index  ///< Position on the array to get value from.
    ) const {
      PASSERTINDEX(index);
      return index < GetSize() ? GetPointer()[index] : T();
    }

    /**Get a pointer to the internal array and assure that it is of at least
       the specified size. This is useful when the array contents are being
       set by some external or system function eg file read.

       It is unsafe to assume that the pointer is valid for very long after
       return from this function. The array may be resized or otherwise
       changed and the pointer returned invalidated. It should be used for
       simple calls to atomic functions, or very careful examination of the
       program logic must be performed.

       @return
       Pointer to the array memory.
     */
    T * GetPointer(
      PINDEX minSize = 0    ///< Minimum size for returned buffer pointer.
    ) {
      SetMinSize(minSize);
      return const_cast<T *>(this->data());
    }
    const T * GetPointer() const {
      return this->data();
    }

    /**Get a pointer to the internal array. The user may not modify the
       contents of this pointer. This is useful when the array contents are
       required by some external or system function eg file write.

       It is unsafe to assume that the pointer is valid for very long after
       return from this function. The array may be resized or otherwise
       changed and the pointer returned invalidated. It should be used for
       simple calls to atomic functions, or very careful examination of the
       program logic must be performed.

       @return
       Constant pointer to the array memory.
     */
    operator T const *() const {
      return this->data();
    }

    /**Concatenate one array to the end of this array.
       This function will allocate a new array large enough for the existing
       contents and the contents of the parameter. The paramters contents is then
       copied to the end of the existing array.

       Note this does nothing and returns <code>false</code> if the target array is not
       dynamically allocated.

       @return
       <code>true</code> if the memory allocation succeeded.
     */
    void Concatenate(
      const PScalarArray & array  ///< Other array to concatenate
    ) {
      this->insert(this->end(), array.begin(), array.end());
    }
    void Concatenate(
      const T * data,  ///< Other array to concatenate
      const PINDEX length
    )
    {
      this->insert(this->end(), data, data+length);
    }
  //@}
};

template <class T> class PScalarArrayWithPrint : public PScalarArray<T>
{
public:
  typedef PScalarArray<T> base_type;
  PScalarArrayWithPrint(PINDEX initialSize = 0) : base_type(initialSize) { }
  PScalarArrayWithPrint(T const * buffer, PINDEX length) : base_type(buffer, length) { }
  PScalarArrayWithPrint(std::initializer_list<T> init) : base_type(init) { }

  /// Print the array
  virtual void PrintOn(ostream & strm) const override
  {
    for (const auto it : *this)
      strm << it;
  }

  /// Read the array
  virtual void ReadFrom(istream & strm) override
  {
    for (;;) {
      T v;
      strm >> v;
      if (strm.fail())
        break;
      this->push_back(v);
    }
  }
};


/// Array of characters.
class PCharArray : public PScalarArray<char>
{
    typedef PScalarArray<char> base_type;
    PCLASSINFO(PCharArray, base_type);
  public:
  /**@name Construction */
  //@{
    /**Construct a new dynamic array of char.
       The array is initialised to all zero bytes.
     */
    PCharArray(
      PINDEX initialSize = 0  ///< Initial number of elements in the array.
    ) : base_type(initialSize) { }

    /**Construct a new dynamic array of char.
     */
    PCharArray(
      char const * buffer,   ///< Pointer to an array of chars.
      PINDEX length       ///< Number of elements pointed to by \p buffer.
    ) : base_type(buffer, length) { }
  //@}

  /**@name Overrides from class PObject */
  //@{
    /// Print the array
    virtual void PrintOn(
      ostream & strm ///< Stream to output to.
    ) const override;
    /// Read the array
    virtual void ReadFrom(
      istream &strm   // Stream to read the objects contents from.
    ) override;
  //@}
};

/// Array of short integers.
typedef PScalarArrayWithPrint<short> PShortArray;

/// Array of integers.
typedef PScalarArrayWithPrint<int> PIntArray;

/// Array of long integers.
typedef PScalarArrayWithPrint<long> PLongArray;

/// Array of unsigned characters.
class PBYTEArray : public PScalarArray<uint8_t>
{
    typedef PScalarArray<uint8_t> base_type;
    PCLASSINFO(PCharArray, base_type);
  public:
  /**@name Construction */
  //@{
    /**Construct a new dynamic array of unsigned chars.
       The array is initialised to all zeros.
     */
    PBYTEArray(
      PINDEX initialSize = 0  ///< Initial number of elements in the array.
    ) : base_type(initialSize) { }

    /**Construct a new dynamic array of unsigned chars.
     */
    PBYTEArray(
      uint8_t const * buffer,   ///< Pointer to an array of BYTEs.
      PINDEX length       ///< Number of elements pointed to by \p buffer.
    ) : base_type(buffer, length) { }

    /**Construct a new dynamic array of unsigned chars.
     */
    PBYTEArray(
      const std::string & str  ///< String to copy to BYTE array
    ) : base_type(reinterpret_cast<const BYTE *>(str.data()), str.length()) { }
  //@}

  /**@name Overrides from class PObject */
  //@{
    /** Clone the object.
     */
    virtual PObject * Clone() const override
    {
      return PNEW PBYTEArray(*this);
    }
    /// Print the array
    virtual void PrintOn(
      ostream & strm ///< Stream to output to.
    ) const override;
    /// Read the array
    virtual void ReadFrom(
      istream &strm   ///< Stream to read the objects contents from.
    ) override;
  //@}

    /**Function to cast block of memory in PBYTEArray to another structure.
      */
    template <typename T> const T & GetAs(PINDEX offset = 0) const
    {
      PAssert(offset+(PINDEX)sizeof(T) <= GetSize(), PInvalidParameter);
      return *(const T *)(GetPointer()+offset);
    }
    template <typename T> void SetAs(const T & value) const
    {
      if (PAssert((PINDEX)sizeof(T) <= GetSize(), PInvalidParameter))
        *(T *)GetPointer() = value;
    }
    template <typename T> void SetAs(PINDEX offset, const T & value) const
    {
      if (PAssert(offset+(PINDEX)sizeof(T) <= GetSize(), PInvalidParameter))
        *(T *)(GetPointer()+offset) = value;
    }
};


class PBYTEView : public PObject, public std::span<const uint8_t>
{
    typedef std::span<const uint8_t> span_type;
    PCLASSINFO(PBYTEView, PObject)
  public:
    PBYTEView() : span_type() { }
    PBYTEView(const uint8_t * data, PINDEX length)
      : span_type(data, length)
    { }
    PBYTEView(const PBYTEArray & bytes)
      : span_type(bytes.data(), bytes.size())
    { }

    PINDEX GetSize() const { return size(); }
    PINDEX GetLength() const { return size(); }
    PINDEX IsEmpty() const { return empty(); }
    const uint8_t * GetPointer() const { return data(); }
    operator const uint8_t * () const { return data(); }

    void Attach(const uint8_t * data, PINDEX length)
    {
      *this = PBYTEView(data, length);
    }

    virtual void PrintOn(ostream & strm) const override;

      /**Function to cast block of memory in PBYTEArray to another structure.
        */
    template <typename T> const T & GetAs(PINDEX offset = 0) const
    {
      PAssert(offset+(PINDEX)sizeof(T) <= GetSize(), PInvalidParameter);
      return *(const T *)(data()+offset);
    }
};


/**Helper class for outputting a block of memory in hex
  */
class PHexDump : public PBYTEView
{
    PCLASSINFO(PHexDump, PBYTEView)
  public:
    PHexDump(const void * data, size_t length, bool compact = true)
      : PBYTEView(static_cast<const uint8_t *>(data), length)
      , m_compact(compact)
    { }

    explicit PHexDump(const PBYTEArray & data, bool compact = true)
      : PBYTEView(data)
      , m_compact(compact)
    { }

    virtual void PrintOn(ostream & strm) const override;

  protected:
    bool m_compact;
};


/// Array of unsigned short integers.
typedef PScalarArrayWithPrint<uint16_t> PWORDArray;

/// Array of unsigned integers.
typedef PScalarArrayWithPrint<unsigned> PUnsignedArray;

/// Array of unsigned long integers.
typedef PScalarArrayWithPrint<uint32_t> PDWORDArray;


///////////////////////////////////////////////////////////////////////////////
// Linear array of objects

/** An array of objects.
This class is a collection of objects which are descendents of the
#PObject class. It is implemeted as a dynamic, linear array of
pointers to the objects.

The implementation of an array allows very fast random access to items in
the collection, but has severe penalties for inserting and deleting objects
as all other objects must be moved to accommodate the change.

An array of objects may have "gaps" in it. These are array entries that
contain NULL as the object pointer.
*/
template <class T> class PArray : public PCollection<T, std::vector<std::shared_ptr<T>>>
{
    typedef T value_type;
    typedef std::shared_ptr<value_type> pointer_type;
    typedef std::vector<pointer_type> vector_type;
    typedef PCollection<value_type, vector_type> base_type;
    PCLASSINFO(PArray, base_type);
  public:
  /**@name Construction */
  //@{
    /**Create a new array of objects. The array is initially set to the
       specified size with each entry having NULL as is pointer value.

       Note that by default, objects placed into the list will be deleted when
       removed or when all references to the list are destroyed.
     */
    __inline PArray(
      PINDEX initialSize = 0  ///< Initial number of objects in the array.
    )
    {
      this->m_collection.resize(initialSize);
    }
  //@}

  /**@name Overrides from class PObject */
  //@{
    /**Make a complete duplicate of the array. Note that all objects in the
       array are also cloned, so this will make a complete copy of the array.
     */
    virtual PObject * Clone() const override
    {
      return this->CloneContents(PNEW PArray());
    }
  //@}

  /**@name Overrides from class PCollection */
  //@{
    bool SetSize(PINDEX newSize)
    {
      this->m_collection.resize(newSize);
      return true;
    }

    bool SetMinSize(PINDEX newSize)
    {
      const auto & c = this->m_collection;
      if (c.size() < newSize)
        c.resize(newSize);
    }

    /**Append a new object to the collection. This will increase the size of
       the array by one and place the new object at that position.

       @return
       Index of the newly added object.
     */
    virtual void Append(
      value_type * obj   ///< New object to place into the collection.
    )
    {
      this->m_collection.emplace_back(pointer_type(obj));
    }
    virtual void Append(
      pointer_type obj   ///< New object to place into the collection.
    )
    {
      this->m_collection.emplace_back(obj);
    }

    /**Insert a new object immediately before the specified object. If the
       object to insert before is not in the collection then the equivalent of
       the <code>Append()</code> function is performed.

       All objects, including the \p before object are shifted up
       one in the array.

       Note that the object values are compared for the search of the
       <code>before</code> parameter, not the pointers. So the objects in the
       collection must correctly implement the PObject::Compare()
       function.

       @return
       index of the newly inserted object.
     */
    virtual PINDEX Insert(
      const value_type & before,   ///< Object value to insert before.
      value_type * obj             ///< New object to place into the collection.
    )
    {
      PINDEX index = 0;
      auto & c = this->m_collection;
      for (auto it = c.begin(); it != c.end(); ++it, ++index) {
        if (**it >= before) {
          c.emplace(it, obj);
          return index;
        }
      }
      c.emplace_back(obj);
      return c.size();
    }

    /** Insert a new object at the specified ordinal index. If the index is
       greater than the number of objects in the collection then the
       equivalent of the <code>Append()</code> function is performed.

       All objects, including the \p index position object are
       shifted up one in the array.

       @return
       Index of the newly inserted object.
     */
    virtual PINDEX InsertAt(
      PINDEX index,   ///< Index position in collection to place the object.
      value_type * obj   ///< New object to place into the collection.
    )
    {
      auto & c = this->m_collection;
      if (index >= c.size()) {
        c.resize(index);
        c.emplace_back(obj);
        return c.size();
      }
      auto it = c.begin();
      std::advance(it, index);
      c.emplace(it, obj);
      return index;
    }

    /**Remove the specific object from the collection.
       All objects are shifted down to fill the vacated position.
     */
    virtual void Remove(
      value_type * obj  ///< Object to remove
    )
    {
      auto & c = this->m_collection;
      for (auto it = c.begin(); it != c.end(); ++it) {
        if (it->get() == obj) {
          c.erase(it);
          break;
        }
      }
    }

    /**Remove the object at the specified ordinal index from the collection.
       All objects are shifted down to fill the vacated position.

       Note if the \p index is beyond the size of the collection then the
       function will assert.
     */
    virtual void RemoveAt(
      PINDEX index   ///< Index position in collection of object to remove.
    )
    {
      auto & c = this->m_collection;
      c.erase(c.begin() + index);
    }

    /**Set the object at the specified ordinal position to the new value. This
       will overwrite the existing entry.

       @return
       <code>true</code> if the object was successfully added.
     */
    virtual pointer_type SetAt(
      PINDEX index,   ///< Index position in collection to set.
      value_type * val   ///< New value to place into the collection.
    )
    {
      auto & c = this->m_collection;
      if (index >= c.size())
        c.resize(index+1);
      auto & p = c[index];
      p.reset(val);
      return p;
    }

    virtual void SetAt(
      PINDEX index,   ///< Index position in collection to set.
      pointer_type val   ///< New value to place into the collection.
    )
    {
      auto & c = this->m_collection;
      if (index >= c.size())
        c.resize(index+1);
      c[index] = val;
    }

    /**Get the object at the specified ordinal position. If the index was
       greater than the size of the collection then NULL is returned.

       @return
       Pointer to object at the specified index.
     */
    virtual pointer_type GetAt(
      PINDEX index  ///< Index position in the collection of the object.
    ) const
    {
      return this->m_collection[index];
    }

    /**Retrieve a reference  to the object in the array. If there was not an
       object at that ordinal position or the index was beyond the size of the
       array then the function asserts.

       @return
       Reference to the object at \p index position.
     */
    T & operator[](
      PINDEX index  ///< Index position in the collection of the object.
    ) const {
      auto obj = this->GetAt(index);
      if (obj)
        return *obj;
      PAssertAlways(PInvalidArrayElement);
      std::abort();
    }
  //@}
};


template <class T> class PVector : public PObject, public std::vector<T>
{
  PCLASSINFO(PVector, PObject);
public:
  PVector() { }

  virtual void PrintOn(std::ostream & strm) const
  {
    strm << PPrintValues(*this);
  }

  __inline PINDEX GetSize() const { return this->size(); }
};


#endif // PTLIB_ARRAY_H


// End Of File ///////////////////////////////////////////////////////////////
