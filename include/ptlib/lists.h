/*
 * lists.h
 *
 * List Container Classes
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

#ifndef PTLIB_LISTS_H
#define PTLIB_LISTS_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif


#include <set>
#include <queue>
#include <stack>


///////////////////////////////////////////////////////////////////////////////
// PList container class

/**This class is a collection of objects which are descendents of the
   <code>PObject</code> class. It is implemeted as a doubly linked list.

   The implementation of a list allows very fast inserting and deleting of
   objects in the collection, but has severe penalties for random access. All
   object access should be done sequentially to avoid these speed penalties.
 */
template <class T> class PList : public PCollection<T, std::list<std::shared_ptr<T>>>
{
    typedef T value_type;
    typedef std::shared_ptr<value_type> pointer_type;
    typedef std::list<pointer_type> list_type;
    typedef PCollection<value_type, list_type> base_type;
    PCLASSINFO(PList, base_type);
  public:
  /**@name Construction */
  //@{
    /**Create a new, empty, list.

       Note that by default, objects placed into the list will be deleted when
       removed or when all references to the list are destroyed.
     */
    PList()
    { }
  //@}

  /**@name Overrides from class PObject */
    /**Make a complete duplicate of the list. Note that all objects in the
       array are also cloned, so this will make a complete copy of the list.
     */
    virtual PObject * Clone() const override
      { return this->CloneContents(PNEW PList()); }
  //@}

  /**@name New functions for class */
  //@{
    /**Append a new object to the collection. This places a new link at the
       "tail" of the list.
    
       @return
       index of the newly added object.
     */
    virtual void Append(
      value_type * obj   ///< New object to place into the collection.
    ) override
    {
      this->Append(pointer_type(obj));
    }
    void Append(
      pointer_type obj   ///< New object to place into the collection.
    )
    {
      this->m_collection.emplace_back(obj);
    }

    /**Append a new object to the collection. This places a new link at the
       "tail" of the list.
    
       @return
       index of the newly added object.
     */
    void Prepend(
      value_type * obj   ///< New object to place into the collection.
    )
    {
      this->Prepend(pointer_type(obj));
    }
    void Prepend(
      pointer_type obj   ///< New object to place into the collection.
    )
    {
      this->m_collection.emplace_front(obj);
    }

    /**Insert a new object immediately before the specified object. If the
       object to insert before is not in the collection then the equivalent of
       the <code>Append()</code> function is performed.
       
       Note that the object values are compared for the search of the
       <code>before</code> parameter, not the pointers. So the objects in the
       collection must correctly implement the <code>PObject::Compare()</code>
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

    /**Remove the object from the collection.

       @return
       true if the object was in the collection.
     */
    virtual bool Remove(
      const value_type * obj   ///< Existing object to remove from the collection.
    )
    {
      return this->m_collection.remove_if([obj](const auto & e) {return e.get() == obj; }) > 0;
    }

    /**Remove the head object from the list.
     */
    virtual void RemoveHead()
    {
      this->m_collection.pop_front();
    }

    /**Remove the tail object from the list.
     */
    virtual void RemoveTail()
    {
      this->m_collection.pop_back();
    }

    // See if list contains object
    bool Contains(const value_type & v) const
    {
      return std::find(this->begin(), this->end(), v) != this->end();
    }
  //@}
};


/**This is a wrapper around std::queue<> to be derieved from PObject.
 */
template <class T> class PQueue : public PObject, public std::queue<T *>
{
    PCLASSINFO(PQueue, PObject);
  public:
    ~PQueue() { PAssert(this->empty(), PLogicError); }

    /* Cloning a queue does not make sense */
    virtual PObject * Clone() const override
    {
      PAssertAlways(PUnsupportedFeature);
      return PNEW PQueue();
    }

    P_DEPRECATED virtual void Enqueue(T * obj) { this->push(obj); }
    P_DEPRECATED virtual T * Dequeue() { T* p = this->front(); this->pop(); return p; }
    P_DEPRECATED virtual bool IsEmpty() const { return this->empty(); }
    P_DEPRECATED virtual PINDEX GetSize() const { return this->size(); }
};


/**This is a wrapper around std::stack<> to be derieved from PObject.
 */
template <class T> class PStack : public PObject, public std::stack<T *>
{
  PCLASSINFO(PStack, PObject);

  public:
    ~PStack() { PAssert(this->empty(), PLogicError); }

    /* Cloning a queue does not make sense */
    virtual PObject * Clone() const
    {
      PAssertAlways(PUnsupportedFeature);
      return PNEW PStack();
    }

    P_DEPRECATED virtual void Push(T * obj) { this->push(obj); }
    P_DEPRECATED virtual T * Pop() { T* p = this->front(); this->pop(); return p; }
    P_DEPRECATED virtual T & Top() { return this->front(); }
    P_DEPRECATED virtual bool IsEmpty() const { return this->empty(); }
    P_DEPRECATED virtual PINDEX GetSize() const { return this->size(); }
};


///////////////////////////////////////////////////////////////////////////////
// Sorted List of PObjects

template <class T> struct PSortedListCompare
{
  using is_transparent = void;  // important

  bool operator() (std::shared_ptr<T> const & l, std::shared_ptr<T> const & r) const { return *l < *r; }
  bool operator() (std::shared_ptr<T> const & l,                 T  const & r) const { return *l < r; }
  bool operator() (                T  const & l, std::shared_ptr<T> const & r) const { return l < *r; }
};


/**This class is a collection of objects which are descendents of the
   <code>PObject</code> class. It is implemeted as a Red-Black binary tree to
   maintain the objects in rank order. Note that this requires that the
   <code>PObject::Compare()</code> function be fully implemented oin objects
   contained in the collection.

   The implementation of a sorted list allows fast inserting and deleting as
   well as random access of objects in the collection. As the objects are being
   kept sorted, "fast" is a relative term. All operations take o(lg n) unless
   a particular object is repeatedly accessed.

   The class remembers the last accessed element. This state information is
   used to optimise access by the "virtual array" model of collections. If
   repeated access via ordinal index is made there is little overhead. All
   other access incurs a minimum overhead, but not insignificant.
 */
template <class T> class PSortedList : public PCollection<T, std::multiset<std::shared_ptr<T>, PSortedListCompare<T>>>
{
    typedef T value_type;
    typedef std::shared_ptr<value_type> pointer_type;
    typedef std::multiset<pointer_type> set_type;
    typedef PCollection<value_type, set_type> base_type;
    PCLASSINFO(PSortedList, base_type);
  public:
  /**@name Construction */
  //@{
    /**Create a new, empty, sorted list.

       Note that by default, objects placed into the list will be deleted when
       removed or when all references to the list are destroyed.
     */
    PSortedList() { }
  //@}

    /**Make a complete duplicate of the list. Note that all objects in the
       array are also cloned, so this will make a complete copy of the list.
     */
    virtual PObject * Clone() const override
    {
      return this->CloneContents(PNEW PSortedList());
    }
//@}

  /**@name Overrides from class PCollection */
  //@{
    /**Add a new object to the collection. The object is always placed in the
       correct ordinal position in the list. It is not placed at the "end".

       @return
       index of the newly added object.
     */
    virtual void Append(
      value_type * obj   // New object to place into the collection.
    ) override
    {
      this->m_collection.emplace(obj);
    }

    /**Remove the object from the collection.

       Note that the comparison for searching for the object in collection is
       made by pointer, not by value. Thus the parameter must point to the
       same instance of the object that is in the collection.

       @return
       true if the object was in the collection.
     */
    virtual bool Remove(
      const value_type * obj   // Existing object to remove from the collection.
    )
    {
      auto & c = this->m_collection;
      auto it = c.find(pointer_type(const_cast<value_type *>(obj)));
      if (it == c.end())
        return false;
      c.erase(it);
      return true;
    }

    // See if list contains object
    bool Contains(const value_type & v) const
    {
      return this->find(v) != this->end();
    }
  //@}
};


#endif // PTLIB_LISTS_H


// End Of File ///////////////////////////////////////////////////////////////
