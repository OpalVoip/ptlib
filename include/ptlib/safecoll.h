/*
 * safecoll.h
 *
 * Thread safe collection classes.
 *
 * Portable Windows Library
 *
 * Copyright (c) 2002 Equivalence Pty. Ltd.
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
 * Contributor(s): ______________________________________.
 */
 
#ifndef PTLIB_SAFE_COLLECTION_H
#define PTLIB_SAFE_COLLECTION_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif


#include <ptlib/syncthrd.h>
#include <ptlib/notifier.h>


#if P_TIMERS
class PTimer;
#endif


/** This class defines a thread-safe object in a collection.

  This is part of a set of classes to solve the general problem of a
  collection (eg a PList or PDictionary) of objects that needs to be a made
  thread safe. Any thread can add, read, write or remove an object with both
  the object and the database of objects itself kept thread safe.

  The act of adding a new object is simple and only requires locking the
  collection itself during the add.

  Locating an object is more complicated. The obvious lock on the collection
  is made for the initial search. But we wish to have the full collection lock
  for as short a period as possible (for performance reasons) so we lock the
  individual object and release the lock on the collection.

  A simple mutex on the object however is very dangerous as it can be (and
  should be able to be!) locked from other threads independently of the
  collection. If one of these threads subsequently needs to get at the
  collection (eg it wants to remove the object) then we will have a deadlock.
  Also, to avoid a race condition with the object begin deleted, the objects
  lock must be made while the collection lock is set. The performance gains
  are then lost as if something has the object locked for a long time, then
  another object wanting that object will actually lock the collection for a
  long time as well.

  So, an object has 4 states: unused, referenced, reading & writing. With the
  additional rider of "being removed". This flag prevents new locks from being
  acquired and waits for all locks to be relinquished before removing the
  object from the system. This prevents numerous race conditions and accesses
  to deleted objects.

  The "unused" state indicates the object exists in the collection but no
  threads anywhere is using it. It may be moved to any state by any thread
  while in this state. An object cannot be deleted (ie memory deallocated)
  until it is unused.

  The "referenced" state indicates that a thread has a reference (eg pointer)
  to the object and it should not be deleted. It may be locked for reading or
  writing at any time thereafter.

  The "reading" state is a form of lock that indicates that a thread is
  reading from the object but not writing. Multiple threads can obtain a read
  lock. Note the read lock has an implicit "reference" state in it.

  The "writing" state is a form of lock where the data in the object may
  be changed. It can only be obtained exclusively and if there are no read
  locks present. Again there is an implicit reference state in this lock.

  Note that threads going to the "referenced" state may do so regardless of
  the read or write locks present.

  Access to safe objects (especially when in a safe collection) is recommended
  to by the PSafePtr<> class which will manage reference counting and the
  automatic unlocking of objects ones the pointer goes out of scope. It may
  also be used to lock each object of a collection in turn.

  The enumeration of a PSafeCollection of PSafeObjects utilises the PSafePtr
  class in a classic "for loop" manner.

  <CODE>
    for (PSafePtr<MyClass> iter(collection, PSafeReadWrite); iter != NULL; ++iter)
      iter->Process();
  </CODE>

  There is one piece if important behaviour in the above. If while enumerating
  a specic object in the collection, that object is "safely deleted", you are
  guaranteed that the object is still usable and has not been phsyically
  deleted, however it will no longer be in the collection, so the enumeration
  will stop as it can no longer determine where in the collection it was.

  What to do in this case is to take a "snapshot" at a point of time that can be safely and completely
  iterated over:

  <CODE>
    PSafeList<MyClass> collCopy = collection;
    for (PSafePtr<MyClass> iter(callCopy, PSafeReadWrite); iter != NULL; ++iter)
      iter->Process();
  </CODE>

 */
class PSafeObject : public PObject, PNonCopyable
{
    PCLASSINFO(PSafeObject, PObject);
  public:
  /**@name Construction */
  //@{
    /**Create a thread safe object.
     */
    PSafeObject();
    PSafeObject(
      const PSafeObject & other ///< Other safe object
    );
    explicit PSafeObject(
      PSafeObject * indirectLock ///< Other safe object to be locked when this is locked
    );
    explicit PSafeObject(
      PReadWriteMutex & mutex ///< Mutex to be locked when this is locked
    );
    ~PSafeObject();
  //@}

  /**@name Operations */
  //@{
    /**Increment the reference count for object.
       This will guarantee that the object is not deleted (ie memory
       deallocated) as the caller thread is using the object, but not
       necessarily at this time locking it.

       If the function returns false, then the object has been flagged for
       deletion and the calling thread should immediately cease using the
       object.

       A typical use of this would be when an entity (eg a thread) has a
       pointer to the object but is not currenty accessing the objects data.
       The LockXXX functions may be called independetly of the reference
       system and the pointer beiong used for the LockXXX call is guaranteed
       to be usable.

       It is recommended that the PSafePtr<> class is used to manage this
       rather than the application calling this function directly.
      */
    PBoolean SafeReference();

    /**Decrement the reference count for object.
       This indicates that the thread no longer has anything to do with the
       object and it may be deleted (ie memory deallocated).

       It is recommended that the PSafePtr<> class is used to manage this
       rather than the application calling this function directly.

       @return true if reference count has reached zero and is not being
               safely deleted elsewhere ie SafeRemove() not called
      */
    PBoolean SafeDereference();

    /**Lock the object for Read Only access.
       This will lock the object in read only mode. Multiple threads may lock
       the object read only, but only one thread can lock for read/write.
       Also, no read only threads can be present for the read/write lock to
       occur and no read/write lock can be present for any read only locks to
       occur.

       If the function returns false, then the object has been flagged for
       deletion and the calling thread should immediately cease use of the
       object, possibly executing the SafeDereference() function to remove
       any references it may have acquired.

       It is expected that the caller had already called the SafeReference()
       function (directly or implicitly) before calling this function. It is
       recommended that the PSafePtr<> class is used to automatically manage
       the reference counting and locking of objects.
      */
    __inline bool LockReadOnly() const { return InternalLockReadOnly(NULL); }
    __inline bool LockReadOnly(const PDebugLocation & location) const { return InternalLockReadOnly(&location); }

    /**Release the read only lock on an object.
       Unlock the read only mutex that a thread had obtained. Multiple threads
       may lock the object read only, but only one thread can lock for
       read/write. Also, no read only threads can be present for the
       read/write lock to occur and no read/write lock can be present for any
       read only locks to occur.

       It is recommended that the PSafePtr<> class is used to automatically
       manage the reference counting and unlocking of objects.
      */
    __inline void UnlockReadOnly() const { InternalUnlockReadOnly(NULL); }
    __inline void UnlockReadOnly(const PDebugLocation & location) const { InternalUnlockReadOnly(&location); }

    /**Lock the object for Read/Write access.
       This will lock the object in read/write mode. Multiple threads may lock
       the object read only, but only one thread can lock for read/write.
       Also no read only threads can be present for the read/write lock to
       occur and no read/write lock can be present for any read only locks to
       occur.

       If the function returns false, then the object has been flagged for
       deletion and the calling thread should immediately cease use of the
       object, possibly executing the SafeDereference() function to remove
       any references it may have acquired.

       It is expected that the caller had already called the SafeReference()
       function (directly or implicitly) before calling this function. It is
       recommended that the PSafePtr<> class is used to automatically manage
       the reference counting and locking of objects.
      */
    __inline bool LockReadWrite() const { return InternalLockReadWrite(NULL); }
    __inline bool LockReadWrite(const PDebugLocation & location) const { return InternalLockReadWrite(&location); }

    /**Release the read/write lock on an object.
       Unlock the read/write mutex that a thread had obtained. Multiple threads
       may lock the object read only, but only one thread can lock for
       read/write. Also, no read only threads can be present for the
       read/write lock to occur and no read/write lock can be present for any
       read only locks to occur.

       It is recommended that the PSafePtr<> class is used to automatically
       manage the reference counting and unlocking of objects.
      */
    __inline void UnlockReadWrite() const { InternalUnlockReadWrite(NULL); }
    __inline void UnlockReadWrite(const PDebugLocation & location) const { InternalUnlockReadWrite(&location); }

    /**Set the removed flag.
       This flags the object as beeing removed but does not physically delete
       the memory being used by it. The SafelyCanBeDeleted() can then be used
       to determine when all references to the object have been released so it
       may be safely deleted.

       This is typically used by the PSafeCollection class and is not expected
       to be used directly by an application.
      */
    void SafeRemove();

    /**Indicate the object is being safely removed.
       Note this returns the value outside of any mutexes, so it could change
       at any moment. Care must be exercised in its use.
      */
    unsigned IsSafelyBeingRemoved() const
    {
      return m_safelyBeingRemoved;
    }

    /**Determine if the object can be safely deleted.
       This determines if the object has been flagged for deletion and all
       references to it have been released.

       This is typically used by the PSafeCollection class and is not expected
       to be used directly by an application.
      */
    PBoolean SafelyCanBeDeleted() const;

    /**Do any garbage collection that may be required by the object so that it
       may be finally deleted. This is especially useful if there a references
       back to this object which this object is in charge of disposing of. This
       reference "glare" is to be resolved by this function being called every
       time the owner collection is cleaning up, causing a cascade of clean ups
       that might need to be required.

       Default implementation simply returns true.

       @return true if object may be deleted.
      */
    virtual bool GarbageCollection();

    /**Get count of references to this object.
       Note this returns the value outside of any mutexes, so it could change
       at any moment. Care must be exercised in its use.
      */
    unsigned GetSafeReferenceCount() const { PWaitAndSignal lock(m_safetyMutex); return m_safeReferenceCount; }
  //@}

#if PTRACING
    static atomic<unsigned> m_traceObjectContext;
    static atomic<unsigned> m_traceObjectLevel;
    static atomic<unsigned> m_traceDetailLevel;
    unsigned GetTraceLogLevel() const { return m_traceContextIdentifier == m_traceObjectContext ? m_traceObjectLevel : m_traceDetailLevel; }
#endif // PTRACING

  private:
    bool InternalLockReadOnly(const PDebugLocation * location) const;
    void InternalUnlockReadOnly(const PDebugLocation * location) const;
    bool InternalLockReadWrite(const PDebugLocation * location) const;
    void InternalUnlockReadWrite(const PDebugLocation * location) const;
    PReadWriteMutex & InternalGetMutex() const;

    mutable PCriticalSection m_safetyMutex;
    unsigned          m_safeReferenceCount;
    bool              m_safelyBeingRemoved;
    bool              m_safeMutexCreated;
    PReadWriteMutex * m_safeInUseMutex;

  friend class PSafeCollection;
  friend class PSafePtrBase;
  friend class PSafeLockReadOnly;
  friend class PSafeLockReadWrite;
  friend class PInstrumentedSafeLockReadOnly;
  friend class PInstrumentedSafeLockReadWrite;
};


class PSafeLockBase
{
  protected:
    typedef bool (PSafeObject:: * LockFn)(const PDebugLocation * location) const;
    typedef void (PSafeObject:: * UnlockFn)(const PDebugLocation * location) const;

    PSafeLockBase(const PSafeObject & object, const PDebugLocation * location, LockFn lock, UnlockFn unlock);

  public:
    ~PSafeLockBase();

    bool Lock();
    void Unlock();

    bool IsLocked() const { return m_locked; }
    bool operator!() const { return !m_locked; }

  protected:
    PSafeObject  & m_safeObject;
    PDebugLocation m_location;
    LockFn         m_lock;
    UnlockFn       m_unlock;
    bool           m_locked;
};


/**Lock a PSafeObject for read only and automatically unlock it when go out of scope.
  */
class PSafeLockReadOnly : public PSafeLockBase
{
  public:
    PSafeLockReadOnly(const PSafeObject & object)
      : PSafeLockBase(object, NULL, &PSafeObject::InternalLockReadOnly, &PSafeObject::InternalUnlockReadOnly)
    { }
};


/**Lock a PSafeObject for read/write and automatically unlock it when go out of scope.
  */
class PSafeLockReadWrite : public PSafeLockBase
{
  public:
    PSafeLockReadWrite(const PSafeObject & object)
      : PSafeLockBase(object, NULL, &PSafeObject::InternalLockReadWrite, &PSafeObject::InternalUnlockReadWrite)
    { }
};


#if PTRACING
  class PInstrumentedSafeLockReadOnly : public PSafeLockBase
  {
    public:
      PInstrumentedSafeLockReadOnly(const PSafeObject & object, const PDebugLocation & location)
        : PSafeLockBase(object, &location, &PSafeObject::InternalLockReadOnly, &PSafeObject::InternalUnlockReadOnly)
      { }
  };

  class PInstrumentedSafeLockReadWrite : public PSafeLockBase
  {
    public:
      PInstrumentedSafeLockReadWrite(const PSafeObject & object, const PDebugLocation & location)
        : PSafeLockBase(object, &location, &PSafeObject::InternalLockReadWrite, &PSafeObject::InternalUnlockReadWrite)
      { }
  };

  #define P_INSTRUMENTED_LOCK_READ_ONLY2(var, obj)  PInstrumentedSafeLockReadOnly  var((obj), P_DEBUG_LOCATION)
  #define P_INSTRUMENTED_LOCK_READ_WRITE2(var, obj) PInstrumentedSafeLockReadWrite var((obj), P_DEBUG_LOCATION)
#else // P_TRACING
  #define P_INSTRUMENTED_LOCK_READ_ONLY2(var, obj)  PSafeLockReadOnly  var((obj))
  #define P_INSTRUMENTED_LOCK_READ_WRITE2(var, obj) PSafeLockReadWrite var((obj))
#endif // P_TRACING

#define P_MAKE_UNIQUE_VAR(base) P_MAKE_UNIQUE_VAR1(base, __LINE__)
#define P_MAKE_UNIQUE_VAR1(base, line) P_MAKE_UNIQUE_VAR2(base, line)
#define P_MAKE_UNIQUE_VAR2(base, line)  base##line

#define P_READ_WRITE_RETURN_ARG_0()
#define P_READ_WRITE_RETURN_ARG_1(arg) ; if (!P_MAKE_UNIQUE_VAR(lock).IsLocked()) arg
#define P_READ_WRITE_RETURN_PART1(narg, args) P_READ_WRITE_RETURN_PART2(narg, args)
#define P_READ_WRITE_RETURN_PART2(narg, args) P_READ_WRITE_RETURN_ARG_##narg args

#define P_INSTRUMENTED_LOCK_READ_ONLY(...)  P_INSTRUMENTED_LOCK_READ_ONLY2(P_MAKE_UNIQUE_VAR(lock),*this) P_READ_WRITE_RETURN_PART1(PARG_COUNT(__VA_ARGS__), (__VA_ARGS__))
#define P_INSTRUMENTED_LOCK_READ_WRITE(...) P_INSTRUMENTED_LOCK_READ_WRITE2(P_MAKE_UNIQUE_VAR(lock),*this) P_READ_WRITE_RETURN_PART1(PARG_COUNT(__VA_ARGS__), (__VA_ARGS__))


/** This class defines a thread-safe collection of objects.
  This class is a wrapper around a standard PCollection class which allows
  only safe, mutexed, access to the collection.

  This is part of a set of classes to solve the general problem of a
  collection (eg a PList or PDictionary) of objects that needs to be a made
  thread safe. Any thread can add, read, write or remove an object with both
  the object and the database of objects itself kept thread safe.

  See the PSafeObject class for more details. Especially in regard to
  enumeration of collections.
 */
class PSafeCollection : public PSmartObject, PNonCopyable
{
    PCLASSINFO(PSafeCollection, PSmartObject);
  public:
  /**@name Construction */
  //@{
    /**Create a thread safe collection of objects.
       Note the collection is automatically deleted on destruction.
     */
    PSafeCollection(
      PCollection * collection    ///< Actual collection of objects
     );

    /**Destroy the thread safe collection.
       The will delete the collection object provided in the constructor.
      */
    ~PSafeCollection();
  //@}

  /**@name Operations */
  //@{
  protected:
    /**Remove an object to the collection.
       This function removes the object from the collection itself, but does
       not actually delete the object. It simply moves the object to a list
       of objects to be garbage collected at a later time.

       As for Append() full mutual exclusion locking on the collection itself
       is maintained.
      */
    virtual PBoolean SafeRemove(
      PSafeObject * obj   ///< Object to remove from collection
    );

    /**Remove an object to the collection.
       This function removes the object from the collection itself, but does
       not actually delete the object. It simply moves the object to a list
       of objects to be garbage collected at a later time.

       As for Append() full mutual exclusion locking on the collection itself
       is maintained.
      */
    virtual PBoolean SafeRemoveAt(
      PINDEX idx    ///< Object index to remove
    );

  public:
    /** Output the contents of the object to the stream. The exact output is
       dependent on the exact semantics of the descendent class. This is
       primarily used by the standard <code>#operator<<</code> function.

       The default behaviour is to print the class name.
     */
    virtual void PrintOn(
      ostream &strm   // Stream to print the object into.
    ) const override;

    /**Remove all objects in collection.
      */
    virtual void RemoveAll(
      PBoolean synchronous = false  ///< Wait till objects are deleted before returning
    );

    /**Disallow the automatic delete any objects that have been removed.
       Objects are simply removed from the collection and not marked for
       deletion using PSafeObject::SafeRemove() and DeleteObject().
      */
    void AllowDeleteObjects(
      PBoolean yes = true   ///< New value for flag for deleting objects
    ) { m_deleteObjects = yes; }

    /**Disallow the automatic delete any objects that have been removed.
       Objects are simply removed from the collection and not marked for
       deletion using PSafeObject::SafeRemove() and DeleteObject().
      */
    void DisallowDeleteObjects() { m_deleteObjects = false; }

    /**Delete any objects that have been removed.
       Returns true if all objects in the collection have been removed and
       their pending deletions carried out.
      */
    virtual PBoolean DeleteObjectsToBeRemoved();

    /**Delete an objects that has been removed.
      */
    virtual void DeleteObject(PObject * object) const;

    /**Start a timer to automatically call DeleteObjectsToBeRemoved().
      */
    virtual void SetAutoDeleteObjects();

    /**Get the current size of the collection.
       Note that usefulness of this function is limited as it is merely an
       instantaneous snapshot of the state of the collection.
      */
    PINDEX GetSize() const;

    /**Determine if the collection is empty.
       Note that usefulness of this function is limited as it is merely an
       instantaneous snapshot of the state of the collection.
      */
    PBoolean IsEmpty() const { return GetSize() == 0; }

    /**Get the mutex for the collection.
      */
    const PMutex & GetMutex() const { return m_collectionMutex; }
          PMutex & GetMutex()       { return m_collectionMutex; }
  //@}

  protected:
    void CopySafeCollection(PCollection * other);
    void CopySafeDictionary(PAbstractDictionary * other);
    bool SafeAddObject(PSafeObject * obj, PSafeObject * old);
    void SafeRemoveObject(PSafeObject * obj);

    PCollection      * m_collection;
    mutable PMutex     m_collectionMutex;
    bool               m_deleteObjects;
    PList<PSafeObject> m_toBeRemoved;
    PMutex             m_removalMutex;

#if P_TIMERS
    PDECLARE_NOTIFIER(PTimer, PSafeCollection, DeleteObjectsTimeout);
    PTimer           * m_deleteObjectsTimer;
#endif

  friend class PSafePtrBase;
};


enum PSafetyMode {
  PSafeReference,
  PSafeReadOnly,
  PSafeReadWrite
};

/** This class defines a base class for thread-safe pointer to an object.

  This is part of a set of classes to solve the general problem of a
  collection (eg a PList or PDictionary) of objects that needs to be a made
  thread safe. Any thread can add, read, write or remove an object with both
  the object and the database of objects itself kept thread safe.

  NOTE: the PSafePtr will allow safe and mutexed access to objects but is not
  thread safe itself! You should not share PSafePtr instances across threads.

  See the PSafeObject class for more details.
 */
class PSafePtrBase : public PObject
{
    PCLASSINFO(PSafePtrBase, PObject);

  /**@name Construction */
  //@{
  protected:
    /**Create a new pointer to a PSafeObject.
       An optional locking mode may be provided to lock the object for reading
       or writing and automatically unlock it on destruction.

       Note that this version is not associated with a collection so the ++
       and -- operators will not work.
     */
    PSafePtrBase(
      PSafeObject * obj = NULL,         ///< Physical object to point to.
      PSafetyMode mode = PSafeReference ///< Locking mode for the object
    );

    /**Copy the pointer to the PSafeObject.
       This will create a copy of the pointer with the same locking mode and
       lock on the PSafeObject. It will also increment the reference count on
       the PSafeObject as well.
      */
    PSafePtrBase(
      const PSafePtrBase & enumerator   ///< Pointer to copy
    );

  public:
    /**Unlock and dereference the PSafeObject this is pointing to.
      */
    ~PSafePtrBase();
  //@}

  /**@name Overrides from class PObject */
  //@{
    /**Compare the pointers.
       Note this is not a value comparison and will only return EqualTo if the
       two PSafePtrBase instances are pointing to the same instance.
      */
    virtual Comparison Compare(
      const PObject & obj   ///< Other instance to compare against
    ) const override;

    /** Output the contents of the object to the stream. The exact output is
       dependent on the exact semantics of the descendent class. This is
       primarily used by the standard <code>#operator<<</code> function.

       The default behaviour is to print the class name.
     */
    virtual void PrintOn(
      ostream &strm   // Stream to print the object into.
    ) const override;
  //@}

  /**@name Operations */
  //@{
    /**Set the pointer to NULL, unlocking/dereferencing existing pointer value.
      */
    virtual void SetNULL();

    /**Return true if pointer is NULL.
      */
    bool operator!() const { return m_currentObject == NULL; }

    /**Return pointer to safe object.
      */
    PSafeObject * GetObject() const { return m_currentObject; }

    /**Return pointer to safe object.
      */
    template <class T>
    T * GetObjectAs() const { return dynamic_cast<T *>(m_currentObject); }

    /**Get the locking mode used by this pointer.
      */
    PSafetyMode GetSafetyMode() const { return m_lockMode; }

    /**Change the locking mode used by this pointer.

       If the function returns false, then the object has been flagged for
       deletion and the calling thread should immediately cease use of the
       object. This instance pointer will be set to NULL.
      */
    virtual PBoolean SetSafetyMode(
      PSafetyMode mode  ///< New locking mode
    );
  //@}

    virtual void Assign(const PSafePtrBase & ptr);
    virtual void Assign(PSafeObject * obj);

  protected:
    virtual void DeleteObject(PSafeObject * obj);

    enum EnterSafetyModeOption {
      WithReference,
      AlreadyReferenced
    };
    PBoolean EnterSafetyMode(EnterSafetyModeOption ref);

    enum ExitSafetyModeOption {
      WithDereference,
      NoDereference
    };
    void ExitSafetyMode(ExitSafetyModeOption ref);

    virtual void LockPtr() { }
    virtual void UnlockPtr() { }

  protected:
    PSafeObject * m_currentObject;
    PSafetyMode   m_lockMode;
};


/** This class defines a base class for thread-safe pointer to an object.

  This is part of a set of classes to solve the general problem of a
  collection (eg a PList or PDictionary) of objects that needs to be a made
  thread safe. Any thread can add, read, write or remove an object with both
  the object and the database of objects itself kept thread safe.

  NOTE: unlikel PSafePtrBase, pointers based on this class are thread safe
  themseleves, at the expense of performance on every operation.

  See the PSafeObject class for more details.
 */
class PSafePtrMultiThreaded : public PSafePtrBase
{
    PCLASSINFO(PSafePtrMultiThreaded, PSafePtrBase);

  /**@name Construction */
  //@{
  protected:
    /**Create a new pointer to a PSafeObject.
       An optional locking mode may be provided to lock the object for reading
       or writing and automatically unlock it on destruction.

       Note that this version is not associated with a collection so the ++
       and -- operators will not work.
     */
    PSafePtrMultiThreaded(
      PSafeObject * obj = NULL,         ///< Physical object to point to.
      PSafetyMode mode = PSafeReference ///< Locking mode for the object
    );

    /**Copy the pointer to the PSafeObject.
       This will create a copy of the pointer with the same locking mode and
       lock on the PSafeObject. It will also increment the reference count on
       the PSafeObject as well.
      */
    PSafePtrMultiThreaded(
      const PSafePtrMultiThreaded & enumerator   ///< Pointer to copy
    );

  public:
    /**Unlock and dereference the PSafeObject this is pointing to.
      */
    ~PSafePtrMultiThreaded();
  //@}

  /**@name Overrides from class PObject */
  //@{
    /**Compare the pointers.
       Note this is not a value comparison and will only return EqualTo if the
       two PSafePtrBase instances are pointing to the same instance.
      */
    virtual Comparison Compare(
      const PObject & obj   ///< Other instance to compare against
    ) const override;
  //@}

  /**@name Operations */
  //@{
    /**Set the pointer to NULL, unlocking/dereferencing existing pointer value.
      */
    virtual void SetNULL() override;

    /**Change the locking mode used by this pointer.

       If the function returns false, then the object has been flagged for
       deletion and the calling thread should immediately cease use of the
       object. This instance pointer will be set to NULL.
      */
    virtual PBoolean SetSafetyMode(
      PSafetyMode mode  ///< New locking mode
    ) override;
  //@}

    virtual void Assign(const PSafePtrMultiThreaded & ptr);
    virtual void Assign(const PSafePtrBase & ptr) override;
    virtual void Assign(PSafeObject * obj) override;

  protected:
    virtual void DeleteObject(PSafeObject * obj) override;

    virtual void LockPtr() override { m_mutex.Wait(); }
    virtual void UnlockPtr() override;

  protected:
    mutable PMutex m_mutex;
    PSafeObject  * m_objectToDelete;
};


/** This class defines a thread-safe enumeration of object in a collection.

  This is part of a set of classes to solve the general problem of a
  collection (eg a PList or PDictionary) of objects that needs to be a made
  thread safe. Any thread can add, read, write or remove an object with both
  the object and the database of objects itself kept thread safe.

  There are two modes of safe pointer: one that is enumerating a collection;
  and one that is independent of the collection that the safe object is in.
  There are subtle semantics that must be observed in each of these two
  modes especially when switching from one to the other.

  NOTE: the PSafePtr will allow safe and mutexed access to objects but may not
  be thread safe itself! This depends on the base class being used. If the more
  efficient PSafePtrBase class is used you should not share PSafePtr instances
  across threads.

  See the PSafeObject class for more details, especially in regards to
  enumeration of collections.
 */
template <class T, class BaseClass = PSafePtrBase> class PSafePtr : public BaseClass
{
  public:
  /**@name Construction */
  //@{
    /**Create a new pointer to a PSafeObject.
       An optional locking mode may be provided to lock the object for reading
       or writing and automatically unlock it on destruction.

       Note that this version is not associated with a collection so the ++
       and -- operators will not work.
     */
    PSafePtr(
      T * obj = NULL,                   ///< Physical object to point to.
      PSafetyMode mode = PSafeReference ///< Locking mode for the object
    ) : BaseClass(obj, mode) { }

    /**Copy the pointer to the PSafeObject.
       This will create a copy of the pointer with the same locking mode and
       lock on the PSafeObject. It will also increment the reference count on
       the PSafeObject as well.
      */
    PSafePtr(
      const PSafePtr & ptr   ///< Pointer to copy
    ) : BaseClass(ptr) { }

    /**Copy the pointer to the PSafeObject.
       This will create a copy of the pointer with the same locking mode and
       lock on the PSafeObject. It will also increment the reference count on
       the PSafeObject as well.
      */
    PSafePtr & operator=(const PSafePtr & ptr)
      {
        this->Assign(ptr);
        return *this;
      }

    /**Set the new pointer to a PSafeObject.
       This will set the pointer to the new object. The old object pointed to
       will be unlocked and dereferenced and the new object referenced.

       If the safe pointer has an associated collection and the new object is
       in that collection, then the object is set to the same locking mode as
       the previous pointer value. This, in effect, jumps the enumeration of a
       collection to the specifed object.

       If the safe pointer has no associated collection or the object is not
       in the associated collection, then the object is always only referenced
       and there is no read only or read/write lock done. In addition any
       associated collection is removed so this becomes a non enumerating
       safe pointer.
     */
    PSafePtr & operator=(T * obj)
      {
        this->Assign(obj);
        return *this;
      }

    /**Set the safe pointer to the specified object.
       This will return a PSafePtr for the previous value of this. It does
       this in a thread safe manner so "test and set" semantics are obeyed.
      */
    PSafePtr Set(T * obj)
      {
        this->LockPtr();
        PSafePtr oldPtr = *this;
        this->Assign(obj);
        this->UnlockPtr();
        return oldPtr;
      }
  //@}

  /**@name Operations */
  //@{
    /**Return the physical pointer to the object.
      */
    operator T*()    const { return  dynamic_cast<T *>(this->m_currentObject); }

    /**Return the physical pointer to the object.
      */
    T & operator*()  const { return *dynamic_cast<T *>(PAssertNULL(this->m_currentObject)); }

    /**Allow access to the physical object the pointer is pointing to.
      */
    T * operator->() const { return  dynamic_cast<T *>(PAssertNULL(this->m_currentObject)); }
  //@}
};


/**Cast the pointer to a different type. The pointer being cast to MUST
    be a derived class or NULL is returned.
  */
template <class Base, class Derived>
PSafePtr<Derived> PSafePtrCast(const PSafePtr<Base> & oldPtr)
{
    PSafePtr<Derived> newPtr;
    if (dynamic_cast<Derived *>(oldPtr.GetObject()) != NULL)
      newPtr.Assign(oldPtr);
    return newPtr;
}


/** This class defines a thread-safe collection of objects.

  This is part of a set of classes to solve the general problem of a
  collection (eg a PList or PDictionary) of objects that needs to be a made
  thread safe. Any thread can add, read, write or remove an object with both
  the object and the database of objects itself kept thread safe.

  See the PSafeObject class for more details. Especially in regard to
  enumeration of collections.
 */
template <class Collection> class PSafeColl : public PSafeCollection
{
    PCLASSINFO_WITH_CLONE(PSafeColl, PSafeCollection);
  public:
    typedef Collection coll_type;
    typedef typename coll_type::iterator coll_iter;
    typedef typename coll_type::value_type value_type;
    typedef PSafePtr<value_type> ptr_type;

  /**@name Construction */
  //@{
    /**Create a safe list collection wrapper around the real collection.
      */
    PSafeColl()
      : PSafeCollection(new coll_type)
      { }

    /**Copy constructor for safe collection.
       Note the left hand side will always have DisallowDeleteObjects() set.
      */
    PSafeColl(const PSafeColl & other)
      : PSafeCollection(new coll_type)
    {
      PWaitAndSignal lock2(other.m_collectionMutex);
      this->CopySafeCollection(other.m_collection);
    }

    /**Assign one safe collection to another.
       Note the left hand side will always have DisallowDeleteObjects() set.
      */
    PSafeColl & operator=(const PSafeColl & other)
    {
      if (&other != this) {
        RemoveAll(true);
        PWaitAndSignal lock1(this->m_collectionMutex);
        PWaitAndSignal lock2(other.m_collectionMutex);
        CopySafeCollection(other.m_collection);
      }
      return *this;
    }
  //@}

  /**@name Operations */
  //@{
    /**Add an object to the collection.
       This uses the PCollection::Append() function to add the object to the
       collection, with full mutual exclusion locking on the collection.
      */
    virtual ptr_type Append(
      value_type * obj,       ///< Object to add to safe collection.
      PSafetyMode mode = PSafeReference   ///< Safety mode for returned locked PSafePtr
    ) {
        PWaitAndSignal mutex(this->m_collectionMutex);
        if (!SafeAddObject(obj, NULL))
          return NULL;
        this->m_collection->Append(obj);
        return ptr_type(obj, mode);
      }

    /**Remove an object to the collection.
       This function removes the object from the collection itself, but does
       not actually delete the object. It simply moves the object to a list
       of objects to be garbage collected at a later time.

       As for Append() full mutual exclusion locking on the collection itself
       is maintained.
      */
    virtual PBoolean Remove(
      value_type * obj          ///< Object to remove from safe collection
    ) {
        return SafeRemove(obj);
      }

    /**Remove an object to the collection.
       This function removes the object from the collection itself, but does
       not actually delete the object. It simply moves the object to a list
       of objects to be garbage collected at a later time.

       As for Append() full mutual exclusion locking on the collection itself
       is maintained.
      */
    virtual PBoolean RemoveAt(
      PINDEX idx     ///< Index to remove
    ) {
        return SafeRemoveAt(idx);
      }

    /**Get the instance in the collection of the index.
       The returned safe pointer will increment the reference count on the
       PSafeObject and lock to the object in the mode specified. The lock
       will remain until the PSafePtr goes out of scope.
      */
    virtual ptr_type GetAt(
      PINDEX idx,
      PSafetyMode mode = PSafeReadWrite
    ) const {
        ptr_type ptr;
        this->m_collectionMutex.Wait();
        if (idx < this->GetCollectionPtr()->GetSize()) {
          coll_iter it = this->GetCollectionPtr()->begin();
          std::advance(it, idx);
          ptr = ptr_type(&*it, PSafeReference);
        }
        this->m_collectionMutex.Signal();
        ptr.SetSafetyMode(mode);
        return ptr;
      }

    /**Find the instance in the collection of an object with the same value.
       The returned safe pointer will increment the reference count on the
       PSafeObject and lock to the object in the mode specified. The lock
       will remain until the PSafePtr goes out of scope.
      */
    virtual ptr_type FindWithLock(
      const value_type & value,
      PSafetyMode mode = PSafeReadWrite
    ) const {
        this->m_collectionMutex.Wait();
        coll_iter it = std::find(this->GetCollectionPtr()->begin(), this->GetCollectionPtr()->end(), value);
        ptr_type ptr(it != this->GetCollectionPtr()->end() ? &*it : NULL, PSafeReference);
        this->m_collectionMutex.Signal();
        ptr.SetSafetyMode(mode);
        return ptr;
      }
  //@}

    class iterator_base
    {
      public:
        typedef std::forward_iterator_tag iterator_category;
        typedef typename coll_type::value_type value_type;
        typedef ptrdiff_t difference_type;
        typedef value_type * pointer;
        typedef value_type & reference;

      protected:
        PSmartPtr<PSafeColl> m_collection;
        coll_iter            m_iterator;
        ptr_type             m_pointer;

        iterator_base(const PSafeColl & coll)
          : m_collection(new PSafeColl(coll))
          , m_iterator(m_collection->GetCollectionPtr()->begin())
          , m_pointer(m_iterator != m_collection->GetCollectionPtr()->end() ? &*m_iterator : NULL, PSafeReference)
        {
        }

        void Next()
        {
          while (++this->m_iterator != this->m_collection->GetCollectionPtr()->end()) {
            m_pointer = ptr_type(&*m_iterator, PSafeReference);
            if (m_pointer != NULL)
              return;
          }
          *this = iterator();
        }

      public:
        iterator_base() { }

        value_type * operator->() const { return &*this->m_pointer; }
        value_type & operator* () const { return  *this->m_pointer; }

        bool operator==(const iterator_base & it) const { return this->m_iterator == it.m_iterator; }
        bool operator!=(const iterator_base & it) const { return !operator==(it); }

        bool operator==(const value_type * ptr) const { return this->m_iterator != this->m_collection->GetCollectionPtr()->end() ? &*this->m_iterator == ptr : (ptr == NULL); }
        bool operator!=(const value_type * ptr) const { return !operator==(ptr); }

        bool SetSafetyMode(PSafetyMode mode) { return this->m_pointer.SetSafetyMode(mode); }
    };

    class iterator : public iterator_base
    {
      protected:
        iterator(const PSafeColl & coll) : iterator_base(coll) { }

      public:
        iterator() { }

        iterator & operator++()    {                      this->Next(); return *this; }
        iterator   operator++(int) { iterator it = *this; this->Next(); return it;    }

        friend class PSafeColl;
    };

    iterator begin() { return iterator(*this); }
    iterator end()   { return iterator(); }
    void erase(const iterator& it) { if (PAssert(it != end(), PLogicError)) this->SafeRemove(&*it.m_iterator); }

    class const_iterator : public iterator_base
    {
      protected:
        const_iterator(const PSafeColl & coll) : iterator_base(coll) { }

      public:
        const_iterator() { }
        const_iterator(const typename iterator & it) : iterator_base(it) { }

        const_iterator & operator++()    {                      this->Next(); return *this; }
        const_iterator   operator++(int) { iterator it = *this; this->Next(); return it;    }

        friend class PSafeColl;
    };

    const_iterator begin() const { return const_iterator(*this); }
    const_iterator end()   const { return const_iterator(); }

  protected:
    coll_type * GetCollectionPtr() const { return dynamic_cast<coll_type *>(this->m_collection); }
};


/** This class defines a thread-safe array of objects.
  See the PSafeObject class for more details. Especially in regard to
  enumeration of collections.
 */
template <class Base> class PSafeArray : public PSafeColl< PArray<Base> >
{
  public:
    typedef PSafePtr<Base> value_type;
};


/** This class defines a thread-safe list of objects.
  See the PSafeObject class for more details. Especially in regard to
  enumeration of collections.
 */
template <class Base> class PSafeList : public PSafeColl< PList<Base> >
{
  public:
    typedef PSafePtr<Base> value_type;
};


/** This class defines a thread-safe sorted array of objects.
  See the PSafeObject class for more details. Especially in regard to
  enumeration of collections.
 */
template <class Base> class PSafeSortedList : public PSafeColl< PSortedList<Base> >
{
  public:
    typedef PSafePtr<Base> value_type;
};


/** This class defines a thread-safe dictionary of objects.

  This is part of a set of classes to solve the general problem of a
  collection (eg a PList or PDictionary) of objects that needs to be a made
  thread safe. Any thread can add, read, write or remove an object with both
  the object and the database of objects itself kept thread safe.

  See the PSafeObject class for more details. Especially in regard to
  enumeration of collections.
 */
template <class K, class D> class PSafeDictionary : public PSafeCollection
{
    PCLASSINFO_WITH_CLONE(PSafeDictionary, PSafeCollection);
  public:
    typedef K key_type;
    typedef D data_type;
    typedef PDictionary<K, D> dict_type;
    typedef typename dict_type::iterator dict_iter;
    typedef PSafePtr<D> ptr_type;

  /**@name Construction */
  //@{
    /**Create a safe dictionary wrapper around the real collection.
      */
    PSafeDictionary()
      : PSafeCollection(new dict_type) { }

    /**Copy constructor for safe collection.
       Note the left hand side will always have DisallowDeleteObjects() set.
      */
    PSafeDictionary(const PSafeDictionary & other)
      : PSafeCollection(new dict_type)
    {
      PWaitAndSignal lock2(other.m_collectionMutex);
      CopySafeDictionary(other.GetDictionaryPtr());
    }

    /**Assign one safe collection to another.
       Note the left hand side will always have DisallowDeleteObjects() set.
      */
    PSafeDictionary & operator=(const PSafeDictionary & other)
    {
      if (&other != this) {
        RemoveAll(true);
        PWaitAndSignal lock1(m_collectionMutex);
        PWaitAndSignal lock2(other.m_collectionMutex);
        CopySafeDictionary(other.GetDictionaryPtr());
      }
      return *this;
    }
  //@}

  /**@name Operations */
  //@{
    /**Add an object to the collection.
       This uses the PCollection::Append() function to add the object to the
       collection, with full mutual exclusion locking on the collection.
      */
    virtual void SetAt(const key_type & key, data_type * obj)
      {
        this->m_collectionMutex.Wait();
        if (SafeAddObject(obj, this->GetDictionaryPtr()->GetAt(key)))
          this->GetDictionaryPtr()->SetAt(key, obj);
        this->m_collectionMutex.Signal();
      }

    /**Remove an object to the collection.
       This function removes the object from the collection itself, but does
       not actually delete the object. It simply moves the object to a list
       of objects to be garbage collected at a later time.

       As for Append() full mutual exclusion locking on the collection itself
       is maintained.
      */
    virtual PBoolean RemoveAt(
      const key_type & key   ///< Key to find object to delete
    ) {
        PWaitAndSignal lock(this->m_collectionMutex);
        return SafeRemove(this->GetDictionaryPtr()->GetAt(key));
      }

    /**Determine of the dictionary contains an entry for the key.
      */
    virtual PBoolean Contains(
      const key_type & key
    ) {
        PWaitAndSignal lock(this->m_collectionMutex);
        return this->GetDictionaryPtr()->Contains(key);
      }

    /**Find the instance in the collection of an object with the same value.
       The returned safe pointer will increment the reference count on the
       PSafeObject and lock to the object in the mode specified. The lock
       will remain until the PSafePtr goes out of scope.
      */
    virtual ptr_type Find(
      const key_type & key,
      PSafetyMode mode = PSafeReadWrite
    ) const {
        this->m_collectionMutex.Wait();
        ptr_type ptr(this->GetDictionaryPtr()->GetAt(key), PSafeReference);
        this->m_collectionMutex.Signal();
        ptr.SetSafetyMode(mode);
        return ptr;
      }

    /** Move an object from one key location to another.
      */
    virtual bool Move(
      const key_type & from,   ///< Key to find object to move
      const key_type & to      ///< Key to place found object
    ) {
      PWaitAndSignal mutex(this->m_collectionMutex);
      if (this->GetDictionaryPtr()->GetAt(to) != NULL)
        return false;
      this->GetDictionaryPtr()->SetAt(to, this->GetDictionaryPtr()->RemoveAt(from));
      return true;
    }

    /** Move all objects from other dictionary to this one.
        This will delete all the objects from the other dictionary, but does
        not delete the objects themselves.
      */
    void MoveFrom(PSafeDictionary & other)
    {
      if (this == &other)
        return;

      *this = other;
      this->AllowDeleteObjects(); // We now own the objects, need to put this back on

      // Remove from other without deleting them
      bool del = other.m_deleteObjects;
      other.DisallowDeleteObjects();
      other.RemoveAll();
      other.AllowDeleteObjects(del);
    }

    /**Get an array containing all the keys for the dictionary.
      */
    PArray<key_type> GetKeys() const
    {
      PArray<key_type> keys;
      this->m_collectionMutex.Wait();
      this->GetDictionaryPtr()->AbstractGetKeys(keys);
      this->m_collectionMutex.Signal();
      return keys;
    }
  //@}

  /**@name Iterators */
  //@{
    class iterator;
    class const_iterator;

    class iterator_pair
    {
    public:
      const key_type & first;
      ptr_type       & second;

    private:
      iterator_pair() : first(reinterpret_cast<const key_type &>(0)) { }
    };

    class iterator_base {
      public:
        typedef std::forward_iterator_tag iterator_category;
        typedef iterator_pair value_type;
        typedef ptrdiff_t difference_type;
        typedef value_type * pointer;
        typedef value_type & reference;

      protected:
        const key_type * m_internal_first;  // Must be first two members
        ptr_type       * m_internal_second;

        PSmartPtr<PSafeDictionary> m_collection;
        dict_iter                  m_iterator;
        ptr_type                   m_pointer;

        iterator_base()
          : m_internal_first(NULL)
          , m_internal_second(&m_pointer)
        {
        }

        iterator_base(const iterator_base & iter)
          : m_internal_first(NULL)
          , m_internal_second(&m_pointer)
          , m_collection(iter.m_collection)
        {
          if (iter.m_iterator != iter.m_collection->GetDictionaryPtr()->end()) {
            dict_iter it = m_collection->GetDictionaryPtr()->begin();
            std::advance(it, std::distance(iter.m_iterator, iter.m_collection->GetDictionaryPtr()->begin()));
            SetIterator(it);
          }
        }

        iterator_base(const PSafeDictionary & coll)
          : m_internal_first(NULL)
          , m_internal_second(&m_pointer)
          , m_collection(new PSafeDictionary(coll))
        {
          dict_iter it = this->m_collection->GetDictionaryPtr()->begin();
          while (this->SetIterator(it))
            ++it;
        }

        iterator_base(const PSafeDictionary & coll, const key_type & key)
          : m_internal_first(NULL)
          , m_internal_second(&m_pointer)
          , m_collection(new PSafeDictionary(coll))
        {
          this->SetIterator(this->m_collection->GetDictionaryPtr()->find(key));
        }

        bool SetIterator(const dict_iter & it)
        {
          this->m_internal_first = NULL;
          this->m_pointer.SetNULL();
          this->m_iterator = it;
          if (it == this->m_collection->GetDictionaryPtr()->end())
            return false;
          if (it->second.IsSafelyBeingRemoved())
            return true;
          this->m_internal_first = &this->m_iterator->first;
          this->m_pointer = const_cast<data_type *>(&this->m_iterator->second);
          return false;
        }

        void Next()
        {
          dict_iter it = this->m_iterator;
          do ++it; while (this->SetIterator(it));
        }

      public:
        bool operator==(const iterator_base & it) const { return this->m_iterator == it.m_iterator; }
        bool operator!=(const iterator_base & it) const { return !operator==(it); }
    };

    class iterator : public iterator_base {
      protected:
        iterator(const PSafeDictionary & owner) : iterator_base(owner) { }
        iterator(const PSafeDictionary & owner, const key_type & key) : iterator_base(owner, key) { }

      public:
        iterator() { }

        iterator & operator++()    {                      this->Next(); return *this; }
        iterator   operator++(int) { iterator it = *this; this->Next(); return it;    }

        const iterator_pair * operator->() const { return  reinterpret_cast<const iterator_pair *>(this); }
        const iterator_pair & operator* () const { return *reinterpret_cast<const iterator_pair *>(this); }

        friend class PSafeDictionary;
    };

    iterator begin()             { return iterator(*this); }
    iterator end()               { return iterator(); }
    iterator find(const K & key) { return iterator(*this, key); }


    class const_iterator : public iterator_base {
      protected:
        const_iterator(const PSafeDictionary & owner) : iterator_base(owner) { }
        const_iterator(const PSafeDictionary & owner, const key_type & key) : iterator_base(owner, key) { }

      public:
        const_iterator() { }
        const_iterator(const PSafeDictionary::iterator & iter) : iterator_base(iter) { }

        const_iterator & operator++()    {                            this->Next(); return *this; }
        const_iterator   operator++(int) { const_iterator it = *this; this->Next(); return it;    }

        const iterator_pair * operator->() const { return  reinterpret_cast<const iterator_pair *>(this); }
        const iterator_pair & operator* () const { return *reinterpret_cast<const iterator_pair *>(this); }

        friend class PSafeDictionary;
    };

    const_iterator begin()                    const { return const_iterator(*this); }
    const_iterator end()                      const { return const_iterator(); }
    const_iterator find(const key_type & key) const { return const_iterator(*this, key); }

    void erase(const       iterator & it) { this->RemoveAt(it->first); }
    void erase(const const_iterator & it) { this->RemoveAt(it->first); }
  //@}

  protected:
    dict_type* GetDictionaryPtr() const { return PAssertNULL(dynamic_cast<dict_type*>(this->m_collection)); }
};


#endif // PTLIB_SAFE_COLLECTION_H


// End Of File ///////////////////////////////////////////////////////////////
