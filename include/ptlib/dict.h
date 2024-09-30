/*
 * dict.h
 *
 * Dictionary (hash table) Container classes.
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


#ifndef PTLIB_DICT_H
#define PTLIB_DICT_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <ptlib/array.h>

#include <unordered_set>


///////////////////////////////////////////////////////////////////////////////
// PDictionary classes

/**This class is used when an ordinal index value is the key for <code>PSet</code>
   and <code>PDictionary</code> classes.
 */
template <typename T>
class PKey : public PObject
{
    PCLASSINFO(PKey, PObject);
  public:
    typedef T value_type;
    typedef PKey<T> my_type;

  /**@name Construction */
  //@{
    /** Create a new key for ordinal index values.
     */
    __inline PKey(
      value_type newKey = 0   ///< Ordinal index value to use as a key.
    ) : m_key(newKey) { }

    /**Operator to assign the ordinal.
      */
    __inline PKey & operator=(value_type newKey)
    { this->m_key = newKey; return *this; }
  //@}

  /**@name Overrides from class PObject */
  //@{
    /// Create a duplicate of the PKey.
    virtual PObject * Clone() const override { return new PKey(this->m_key); }

    /* Get the relative rank of the ordinal index. This is a simpel comparison
       of the objects PINDEX values.

       @return
       comparison of the two objects, <code>EqualTo</code> for same,
       <code>LessThan</code> for \p obj logically less than the
       object and <code>GreaterThan</code> for \p obj logically
       greater than the object.
     */
    virtual Comparison Compare(const PObject & obj) const override
    {
      return Compare2(this->m_key, dynamic_cast<const my_type &>(obj).m_key);
    }

    /**This function calculates a hash table index value for the implementation
       of <code>PSet</code> and <code>PDictionary</code> classes.

       @return
       hash table bucket number.
     */
    virtual PINDEX HashFunction() const override
    {
#if PINDEX_SIGNED
      return std::abs((PINDEX)this->m_key)%23;
#else
      return ((PINDEX)this->m_key)%23;
#endif
    }

    /**Output the ordinal index to the specified stream. This is identical to
       outputting the PINDEX, i.e. integer, value.

       @return
       stream that the index was output to.
     */
    virtual void PrintOn(ostream & strm) const override { strm << this->m_key; }
  //@}

  /**@name New functions for class */
  //@{
    /** Operator so that a PKey can be used as a PINDEX value.
     */
    __inline operator value_type() const  { return this->m_key; }

    /**Operator to pre-increment the ordinal.
      */
    __inline value_type operator++() { return ++this->m_key; }

    /**Operator to post-increment the ordinal.
      */
    __inline value_type operator++(int) { return this->m_key++; }

    /**Operator to pre-decrement the ordinal.
      */
    __inline value_type operator--() { return --this->m_key; }

    /**Operator to post-decrement the ordinal.
      */
    __inline value_type operator--(int) { return this->m_key--; }

    /**Operator to add the ordinal.
      */
    __inline PKey & operator+=(value_type add) { this->m_key += add; return *this; }

    /**Operator to subtract from the ordinal.
      */
    __inline PKey & operator-=(value_type minus) { this->m_key -= minus; return *this; }
  //@}

  private:
    value_type m_key;
};

typedef PKey<PINDEX> POrdinalKey;


//////////////////////////////////////////////////////////////////////////////

template <class T> using PSetBase = std::unordered_set<std::shared_ptr<T>, std::hash<std::shared_ptr<PObject>>>;

/** Abstract set of PObjects.
 */
template <class T> class PSet : public PCollection<T, PSetBase<T>>
{
    typedef T value_type;
    typedef std::shared_ptr<value_type> pointer_type;
    typedef PSetBase<value_type> set_type;
    typedef PCollection<value_type, set_type> base_type;
    PCLASSINFO(PSet, base_type);
  public:
  /**@name Construction */
  //@{
    /**Create a new, empty, set.

       Note that by default, objects placed into the list will be deleted when
       removed or when all references to the list are destroyed.
     */
    __inline PSet() { }
  //@}

  /**@name Overrides from class PObject */
  //@{
    /**Make a complete duplicate of the set. Note that all objects in the
       array are also cloned, so this will make a complete copy of the set.
     */
    virtual PObject * Clone() const override
    {
      return this->CloneContents(PNEW PSet());
    }
  //@}

  /**@name Overrides from class PCollection */
  //@{
    /**Add a new object to the collection. If the objects value is already in
       the set then the object is \b not included.

       @return
       hash function value of the newly added object.
     */
    virtual void Append(
      value_type * obj   ///< New object to place into the collection.
    ) override
    {
      this->m_collection.emplace(obj);
    }
  //@}

  /**@name New functions for class */
  //@{
    /**Include the specified object into the set. If the objects value is
       already in the set then the object is \b not included.

       The object values are compared, not the pointers.  So the objects in
       the collection must correctly implement the <code>PObject::Compare()</code>
       function. The hash table is used to locate the entry.
     */
    void Include(
      const value_type * obj   // New object to include in the set.
    ) { this->m_collection.emplace(obj); }

    /**Include the specified objects value into the set. If the objects value
       is already in the set then the object is \b not included.

       The object values are compared, not the pointers.  So the objects in
       the collection must correctly implement the <code>PObject::Compare()</code>
       function. The hash table is used to locate the entry.
     */
    PSet & operator+=(
      const value_type & obj   // New object to include in the set.
    ) { this->m_collection.emplace(obj.Clone()); return *this; }

    /**Remove the object from the set.

       The object values are compared, not the pointers.  So the objects in
       the collection must correctly implement the <code>PObject::Compare()</code>
       function. The hash table is used to locate the entry.
     */
    void Exclude(
      const value_type * obj   // New object to exclude in the set.
    ) { this->m_collection.erase(this->m_collection.find(obj)); }

    /**Remove the objects value from the set.

       The object values are compared, not the pointers.  So the objects in
       the collection must correctly implement the <code>PObject::Compare()</code>
       function. The hash table is used to locate the entry.
     */
    PSet & operator-=(
      const value_type & obj   // New object to exclude in the set.
      )
    {
      this->m_collection.erase(this->m_collection.find(pointer_type(const_cast<value_type *>(&obj))));
      return *this;
    }

    /**Determine if the value of the object is contained in the set. The
       object values are compared, not the pointers.  So the objects in the
       collection must correctly implement the <code>PObject::Compare()</code>
       function. The hash table is used to locate the entry.

       @return
       true if the object value is in the set.
     */
    bool Contains(
      const value_type & key  ///< Key to look for in the set.
    ) const
    {
      return this->m_collection.contains(pointer_type(const_cast<value_type *>(&key)));
    }

    /**Determine if the value of the object is contained in the set. The
       object values are compared, not the pointers.  So the objects in the
       collection must correctly implement the <code>PObject::Compare()</code>
       function. The hash table is used to locate the entry.

       @return
       true if the object value is in the set.
     */
    bool operator[](
      const value_type & key  ///< Key to look for in the set.
    ) const { return this->Contains(key); }

    /**Calculate union of sets.
       Returns true if any new elements were added.
      */
    bool Union(
      const PSet & set
    );

    /**Calculate intersection of sets.
       Returns true if there is an intersection.
      */
    static bool Intersection(
      const PSet & set1,
      const PSet & set2,
      PSet * intersection = NULL
    );
  //@}
};



//////////////////////////////////////////////////////////////////////////////

template <typename K, class D> using PUnorderedMap = std::unordered_map<K, D, std::hash<PObject>>;

/**An abstract dictionary container.
*/
template <typename K, class D> class PDictionary : public PObject
{
    PCLASSINFO(PDictionary, PObject);
  public:
    typedef K key_type;
    typedef D data_type;
    typedef std::shared_ptr<data_type> pointer_type;
    typedef std::unordered_map<key_type, pointer_type, std::hash<PObject>> map_type;

  /**@name Construction */
  //@{
    /**Create a new, empty, dictionary.

       Note that by default, objects placed into the dictionary will be deleted
       when removed or when all references to the dictionary are destroyed.
     */
    __inline PDictionary() { }
  //@}

  /**@name Overrides from class PObject */
  //@{
    /**Make a complete duplicate of the dictionary. Note that all objects in
       the array are also cloned, so this will make a complete copy of the
       dictionary.
     */
    virtual PObject * Clone() const override
    {
      return this->CloneContents(PNEW PDictionary());
    }

  /**Output the contents of the object to the stream. The exact output is
       dependent on the exact semantics of the descendent class. This is
       primarily used by the standard <code>operator<<</code> function.

       The default behaviour is to print the class name.
     */
    virtual void PrintOn(
      ostream &strm   ///< Stream to print the object into.
    ) const override
    {
      char separator = '\0';

      for (const auto & it : this->m_map) {
        if (separator != '\0')
          strm << separator;
        else {
          separator = strm.fill();
          if (separator == ' ')
            separator = '\n';
        }
        strm << it.first<< '=' << *it.second;
      }

      if (separator == '\n')
        strm << separator;
    }
  //@}

  /**@name New functions for class */
  //@{
    bool IsEmpty() const { return this->m_map.empty(); }
    bool empty() const { return this->m_map.empty(); }

    PINDEX GetSize() const { return this->m_map.size(); }
    size_t size() const { return this->m_map.size(); }

    /**Add a new object to the collection. If the objects value is already in
       the dictionary then the object is overrides the previous value.

       The object is placed in the an ordinal position dependent on the keys
       hash function. Subsequent searches use the hash function to speed access
       to the data item.

       @return
       true if the object was successfully added.
     */
    virtual pointer_type SetAt(
      const key_type & key,  ///< Key for position in dictionary to add object.
      data_type * obj         ///< New object to put into the dictionary.
    )
    {
      auto result = this->m_map.emplace(key, obj);
      if (!result.second)
        result.first->second.reset(obj);
      return result.first->second;
    }
    virtual pointer_type SetAt(
      const key_type & key,  ///< Key for position in dictionary to add object.
      pointer_type obj         ///< New object to put into the dictionary.
    )
    {
      auto result = this->m_map.emplace(key, obj);
      if (!result.second)
        result.first->second = obj;
      return result.first->second;
    }

    /**Get the object at the specified key position. If the key was not in the
       collection then NULL is returned.

       @return
       pointer to object at the specified key.
     */
    virtual pointer_type GetAt(
      const key_type & key   ///< Key for position in dictionary to get object.
    ) const
    {
      const auto & m = this->m_map;
      const auto it = m.find(key);
      return it != m.end() ? it->second : nullptr;
    }

    /**Get the object contained in the dictionary at the \p key
       position. The hash table is used to locate the data quickly via the
       hash function provided by the \p key.

       The last key/data pair is remembered by the class so that subseqent
       access is very fast.

       @return
       reference to the object indexed by the key.
     */
    const D & operator[](
      const key_type & key   ///< Key to look for in the dictionary.
    ) const
    {
      return *this->m_map[key];
    }
    D & operator[](
      const key_type & key   ///< Key to look for in the dictionary.
    )
    {
      return *this->m_map[key];
    }

/**Determine if the value of the object is contained in the hash table. The
   object values are compared, not the pointers.  So the objects in the
   collection must correctly implement the <code>PObject::Compare()</code>
   function. The hash table is used to locate the entry.

   @return
   true if the object value is in the dictionary.
 */
    bool Contains(
      const K & key   ///< Key to look for in the dictionary.
    ) const
    {
      return this->m_map.contains(key);
    }

/**Remove an object at the specified \p key. The returned pointer is then
   removed using the <code>SetAt()</code> function to set that key value to
   NULL.

   @return
   pointer to the object being removed, or NULL if the key was not
   present in the dictionary or the object was deleted automatically.
 */
    virtual void RemoveAt(
      const K & key   ///< Key for position in dictionary to get object.
    )
    {
      this->m_map.erase(key);
    }
    void erase(const K & key) { this->RemoveAt(key); }

    void RemoveAll()
    {
      this->m_map.clear();
    }

/**Get an array containing all the keys for the dictionary.
      */
    virtual std::vector<K> GetKeys() const
    {
      std::vector<K> keys;
      keys.reserve(this->m_map.size());
      for (const auto & it : this->m_map)
        keys.push_back(it.first);
      return keys;
    }
  //@}

  /**@name Iterators */
  //@{
    class iterator;
    class const_iterator;
    class iterator_base
    {
    protected:
      mutable key_type const * m_internal_first;  // Must be first two members
      mutable data_type * m_internal_second;
      mutable pointer_type m_internal_pointer;

      iterator_base()
        : m_internal_first(nullptr)
        , m_internal_second(nullptr)
      { }

      template <typename P> void internal_set(const P & pair) const
      {
        this->m_internal_first = &pair.first;
        this->m_internal_second = pair.second.get();
        this->m_internal_pointer = pair.second;
      }
    };

    template<class CK, class CD>
    class iterator_pair
    {
    public:
      CK & first;
      CD & second;

    private:
      iterator_pair() = delete;
    };

    class iterator : public iterator_base, public map_type::iterator
    {
      friend class PDictionary;
      iterator(const map_type::iterator & it): map_type::iterator(it) { }
    public:
      iterator() : map_type::iterator() { }
      typedef iterator_pair<K, D> pair;
      const pair * operator->() const {
        this->internal_set(this->map_type::iterator::operator*());
        return  reinterpret_cast<const pair *>(this);
      }
      const pair & operator* () const {
        this->internal_set(this->map_type::iterator::operator*());
        return *reinterpret_cast<const pair *>(this);
      }
    };

    iterator begin() { return this->m_map.begin(); }
    iterator end() { return this->m_map.end(); }
    iterator find(const key_type & key) { return this->m_map.find(key); }


    class const_iterator : public iterator_base, public map_type::const_iterator
    {
      friend class PDictionary;
      const_iterator(const map_type::const_iterator & it) : map_type::const_iterator(it) { }
    public:
      const_iterator() : map_type::iterator() { }
      typedef iterator_pair<const K, const D> pair;
      const pair * operator->() const
      {
        this->internal_set(this->map_type::const_iterator::operator*());
        return  reinterpret_cast<const pair *>(this);
      }
      const pair & operator* () const
      {
        this->internal_set(this->map_type::const_iterator::operator*());
        return *reinterpret_cast<const pair *>(this);
      }
    };

    const_iterator begin() const { return this->m_map.begin(); }
    const_iterator end()   const { return this->m_map.end(); }
    const_iterator find(const key_type & key) const { return this->m_map.find(key); }

    void erase(const       iterator & it) { this->m_map.erase(it); }
    void erase(const const_iterator & it) { this->m_map.erase(it); }
    //@}

  protected:
    map_type m_map;

    virtual PObject * CloneContents(PDictionary * clone) const
    {
      for (const auto & it : this->m_map) {
        clone->m_map.emplace(it.first, it.second->template CloneAs<D>());
      }
      return clone;
    }
};

#endif // PTLIB_DICT_H

// End Of File ///////////////////////////////////////////////////////////////
