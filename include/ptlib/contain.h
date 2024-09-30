/*
 * contain.h
 *
 * Umbrella include for Container Classes.
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

#ifndef PTLIB_CONTAIN_H
#define PTLIB_CONTAIN_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif


#include <ptlib/object.h>


///////////////////////////////////////////////////////////////////////////////
// Helper template functions

template <typename T> class PPrintValuesWrapper
{
  const T & m_container;
public:
  PPrintValuesWrapper(const T & container) : m_container(container) { }

  friend std::ostream& operator<<(std::ostream& strm, const PPrintValuesWrapper & wrap)
  {
    auto fill = strm.fill();
    bool first = true;
    for (const auto & it : wrap.m_container) {
      if (fill != ' ' && std::exchange(first, true))
        strm << fill;
      strm << it;
    }
    return strm;
  }
};

template <typename T> PPrintValuesWrapper<T> PPrintValues(const T & container)
{
  return PPrintValuesWrapper<T>{container};
}

template <class T, class C> class PCollection : public PObject
{
  PCLASSINFO(PCollection, PObject);
public:
  typedef T value_type;
  typedef std::shared_ptr<T> pointer_type;
  typedef C collection_type;

  /**@name Overrides from class PObject */
  //@{
    /**Get the relative rank of the two lists. The following algorithm is
       employed for the comparison:

       \arg \c EqualTo if the two lists are identical in length
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
  virtual PObject::Comparison Compare(const PObject & obj) const override
  {
    const collection_type & t1 = this->m_collection;
    const collection_type & t2 = dynamic_cast<const PCollection &>(obj).m_collection;
    const auto & it1 = t1.begin();
    const auto & it2 = t2.begin();
    while (it1 != t1.end() && it2 != t2.end()) {
      auto c = (*it1)->Compare(**it2);
      if (c != PObject::EqualTo)
        return c;
    }
    if (it1 != t1.end())
      return PObject::GreaterThan;
    if (it2 != t2.end())
      return PObject::LessThan;
    return PObject::EqualTo;
  }
  //@}

    bool IsEmpty() const { return this->m_collection.empty(); }
    bool empty() const { return this->m_collection.empty(); }

    /** Get the number of elements in the collection.
     */
    PINDEX GetSize() const { return this->m_collection.size(); }
    size_t size() const { return this->m_collection.size(); }

    /**Search the collection for the specific instance of the object. The
       object pointers are compared, not the values. A simple linear search
       from ordinal position zero is performed.

       @return
       Ordinal index position of the object, or <code>P_MAX_INDEX</code>.
     */
  virtual PINDEX GetObjectsIndex(
    const value_type * obj  ///< Object to find.
  ) const
  {
    PINDEX index = 0;
    for (const auto & it : m_collection) {
      if (it.get() == obj)
        return index;
    }
    return P_MAX_INDEX;
  }

  /**Search the collection for the specified value of the object. The object
     values are compared, not the pointers.  So the objects in the
     collection must correctly implement the PObject::Compare()
     function. A simple linear search from ordinal position zero is
     performed.

     @return
     Ordinal index position of the object, or <code>P_MAX_INDEX</code>.
   */
  virtual PINDEX GetValuesIndex(
    const value_type & obj   // Object to find equal of.
  ) const
  {
    PINDEX index = 0;
    for (const auto & it : m_collection) {
      if (it->Compare(obj) == PObject::EqualTo)
        return index;
    }
    return P_MAX_INDEX;
  }

    /**Remove all of the elements in the collection. This operates by
       continually calling <code>RemoveAt()</code> until there are no objects left.

       The objects are removed from the last, at index
       (GetSize()-1) toward the first at index zero.
     */
  virtual void RemoveAll()
  {
    this->m_collection.clear();
  }

  /**Add a new object to the collection. The object is always placed in the
       correct ordinal position in the list. It is not placed at the "end".

       @return
       index of the newly added object.
     */
  virtual void Append(
    value_type * obj   // New object to place into the collection.
  ) = 0;

  /**@name Iterators */
  //@{
  class iterator
  {
    friend class PCollection;
    typedef typename collection_type::iterator base_iterator;
    base_iterator m_iterator;
    iterator(const base_iterator & it) : m_iterator(it) { }
  public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = T;

    iterator() { }
    value_type * operator->() const { return   this->m_iterator->get(); }
    value_type & operator* () const { return **this->m_iterator; }
    iterator & operator++() { ++this->m_iterator; return *this; }
    iterator operator++(int) { iterator prev = *this; ++this->m_iterator; return prev; }
    iterator & operator--() { --this->m_iterator; return *this; }
    iterator operator--(int) { iterator prev = *this; --this->m_iterator; return prev; }
    bool operator==(const iterator & it) const { return this->m_iterator == it.m_iterator; }
    bool operator!=(const iterator & it) const { return this->m_iterator != it.m_iterator; }
  };

  iterator begin() { return this->m_collection.begin(); }
  iterator end() { return this->m_collection.end(); }
  iterator rbegin() { return this->m_collection.rbegin(); }
  iterator rend() { return this->m_collection.rend(); }
  void insert(const iterator & it, value_type * obj) { this->m_collection.emplace(it, obj); }
  void insert(const iterator & it, const value_type & obj) { this->m_collection.emplace(it, obj.Clone()); }

  class const_iterator
  {
    friend class PCollection;
    typedef typename collection_type::const_iterator base_iterator;
    base_iterator m_iterator;
    const_iterator(const base_iterator & it) : m_iterator(it) { }
  public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = const T;

    const_iterator() { }
    const_iterator(const iterator & it) : m_iterator(it.m_iterator) { }
    const value_type * operator->() const { return   this->m_iterator->get(); }
    const value_type & operator* () const { return **this->m_iterator; }
    const_iterator & operator++() { ++this->m_iterator; return *this; }
    const_iterator operator++(int) { const_iterator prev = *this; ++this->m_iterator; return prev; }
    const_iterator & operator--() { --this->m_iterator; return *this; }
    const_iterator operator--(int) { const_iterator prev = *this; --this->m_iterator; return prev; }
    bool operator==(const const_iterator & it) const { return this->m_iterator == it.m_iterator; }
    bool operator!=(const const_iterator & it) const { return this->m_iterator != it.m_iterator; }
  };

  const_iterator begin()  const { return this->m_collection.begin(); }
  const_iterator end()    const { return this->m_collection.end(); }
  const_iterator rbegin() const { return this->m_collection.rbegin(); }
  const_iterator rend()   const { return this->m_collection.rend(); }

  iterator find(const value_type & obj) { return this->m_collection.find(obj); }
  const_iterator find(const value_type & obj) const { return this->m_collection.find(obj); }
  value_type & front() { return *this->m_collection.front(); }
  value_type & back() { return *this->m_collection.back(); }
  const value_type & front() const { return *this->m_collection.front(); }
  const value_type & back() const { return *this->m_collection.back(); }
  void erase(const iterator & it) { this->m_collection.erase(it.m_iterator); }
  void erase(const const_iterator & it) { this->m_collection.erase(it.m_iterator); }
  void push_front(const value_type & value) { this->m_collection.push_front(value); }
  void push_back(const value_type & value) { this->m_collection.push_back(value); }
  void pop_front() { this->m_collection.pop_front(); }
  void pop_back() { this->m_collection.pop_back(); }
  void clear() { this->m_collection.clear(); }
//@}

protected:
  collection_type m_collection;

  virtual PObject * CloneContents(PCollection * clone) const
  {
    for (const auto & it : *this)
      clone->Append(it.template CloneAs<value_type>());
    return clone;
  }
};


///////////////////////////////////////////////////////////////////////////////
// The abstract array class

#include <ptlib/array.h>

///////////////////////////////////////////////////////////////////////////////
// The abstract array class

#include <ptlib/lists.h>

///////////////////////////////////////////////////////////////////////////////
// Dictionary (map) class

#include <ptlib/dict.h>


///////////////////////////////////////////////////////////////////////////////
// PString class

#include <ptlib/pstring.h>


#endif // PTLIB_CONTAIN_H


// End Of File ///////////////////////////////////////////////////////////////
