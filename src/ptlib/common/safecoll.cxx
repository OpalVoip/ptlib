/*
 * safecoll.cxx
 *
 * Thread safe collection classes.
 *
 * Portable Tools Library
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
 * The Original Code is Portable Tools Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 */

#ifdef P_USE_PRAGMA
#pragma implementation "safecoll.h"
#endif

#include <ptlib.h>
#include <ptlib/safecoll.h>


#define PTraceModule() "SafeColl"
#define new PNEW


/////////////////////////////////////////////////////////////////////////////

#if PTRACING
atomic<unsigned> PSafeObject::m_traceObjectContext(1234567890);
atomic<unsigned> PSafeObject::m_traceObjectLevel(3);
atomic<unsigned> PSafeObject::m_traceDetailLevel(7);
#endif // PTRACING

PSafeObject::PSafeObject()
  : m_safeReferenceCount(0)
  , m_safelyBeingRemoved(false)
  , m_safeMutexCreated(true)
  , m_safeInUseMutex(NULL)
{
}


PSafeObject::PSafeObject(const PSafeObject & other)
  : PObject(other)
  , m_safeReferenceCount(0)
  , m_safelyBeingRemoved(false)
  , m_safeMutexCreated(true)
  , m_safeInUseMutex(NULL)
{
}


PSafeObject::PSafeObject(PSafeObject * indirectLock)
  : m_safeReferenceCount(0)
  , m_safelyBeingRemoved(false)
  , m_safeMutexCreated(false)
{
  if (PAssert(indirectLock != NULL, PNullPointerReference))
    m_safeInUseMutex = &indirectLock->InternalGetMutex();
  else
    m_safeInUseMutex = NULL;
}


PSafeObject::PSafeObject(PReadWriteMutex & mutex)
  : m_safeReferenceCount(0)
  , m_safelyBeingRemoved(false)
  , m_safeMutexCreated(false)
  , m_safeInUseMutex(&mutex)
{
}


PSafeObject::~PSafeObject()
{
  if (m_safeMutexCreated)
    delete m_safeInUseMutex;
}


bool PSafeObject::SafeReference()
{
#if PTRACING
  unsigned count = 0;
  {
    PWaitAndSignal mutex(m_safetyMutex);
    if (!m_safelyBeingRemoved)
      count = ++m_safeReferenceCount;
  }

  unsigned level = count == 0 ? 2 : GetTraceLogLevel();
  if (PTrace::CanTrace(level)) {
    ostream & trace = PTRACE_BEGIN(level);
    trace << GetClassName() << ' ' << (void *)this;
    if (count != 0)
      trace << " incremented reference count to " << count;
    else {
      trace << " removed, cannot reference";
      if (PTrace::CanTrace(6))
        PTrace::WalkStack(trace);
    }
    trace << PTrace::End;
  }
  return count > 0;
#else
  PWaitAndSignal mutex(m_safetyMutex);
  if (m_safelyBeingRemoved)
    return false;
  ++m_safeReferenceCount;
  return true;
#endif
}


bool PSafeObject::SafeDereference()
{
  bool mayBeDeleted = false;

  m_safetyMutex.Wait();
  if (PAssert(m_safeReferenceCount > 0, PLogicError)) {
    m_safeReferenceCount--;
    mayBeDeleted = m_safeReferenceCount == 0 && !m_safelyBeingRemoved;
  }
#if PTRACING
  unsigned count = m_safeReferenceCount;
  unsigned level = GetTraceLogLevel();
#endif
  m_safetyMutex.Signal();

  PTRACE(level, GetClassName() << ' ' << (void *)this << " decremented reference count to " << count);

  // At this point the object could be deleted in another thread, do not use it anymore!
  return mayBeDeleted;
}


PReadWriteMutex & PSafeObject::InternalGetMutex() const
{
  PWaitAndSignal lock(m_safetyMutex);
  if (m_safeInUseMutex == NULL)
    const_cast<PSafeObject *>(this)->m_safeInUseMutex = new PReadWriteMutex(typeid(*this).name());
  return *m_safeInUseMutex;
}


bool PSafeObject::InternalLockReadOnly(const PDebugLocation * location) const
{
  PTRACE(GetTraceLogLevel(), "Waiting read ("<<(void *)this<<")");
  m_safetyMutex.Wait();

  if (m_safelyBeingRemoved) {
    m_safetyMutex.Signal();
    PTRACE(GetTraceLogLevel(), "Being removed while waiting read ("<<(void *)this<<")");
    return false;
  }

  m_safetyMutex.Signal();

  InternalGetMutex().StartRead(location);
  PTRACE(GetTraceLogLevel(), "Locked read ("<<(void *)this<<")");
  return true;
}


void PSafeObject::InternalUnlockReadOnly(const PDebugLocation * location) const
{
  PTRACE(GetTraceLogLevel(), "Unlocked read ("<<(void *)this<<")");
  InternalGetMutex().EndRead(location);
}


bool PSafeObject::InternalLockReadWrite(const PDebugLocation * location) const
{
  PTRACE(GetTraceLogLevel(), "Waiting readWrite ("<<(void *)this<<")");
  m_safetyMutex.Wait();

  if (m_safelyBeingRemoved) {
    m_safetyMutex.Signal();
    PTRACE(GetTraceLogLevel(), "Being removed while waiting readWrite ("<<(void *)this<<")");
    return false;
  }

  m_safetyMutex.Signal();

  InternalGetMutex().StartWrite(location);
  PTRACE(GetTraceLogLevel(), "Locked readWrite ("<<(void *)this<<")");
  return true;
}


void PSafeObject::InternalUnlockReadWrite(const PDebugLocation * location) const
{
  PTRACE(GetTraceLogLevel(), "Unlocked readWrite ("<<(void *)this<<")");
  InternalGetMutex().EndWrite(location);
}


void PSafeObject::SafeRemove()
{
  m_safetyMutex.Wait();
  m_safelyBeingRemoved = true;
  m_safetyMutex.Signal();
}


bool PSafeObject::SafelyCanBeDeleted() const
{
  PWaitAndSignal mutex(m_safetyMutex);
  return m_safelyBeingRemoved && m_safeReferenceCount == 0;
}


bool PSafeObject::GarbageCollection()
{
  return true;
}


/////////////////////////////////////////////////////////////////////////////

PSafeLockBase::PSafeLockBase(const PSafeObject & object, const PDebugLocation * location, LockFn lock, UnlockFn unlock)
  : m_safeObject(const_cast<PSafeObject &>(object))
  , m_location(location)
  , m_lock(lock)
  , m_unlock(unlock)
  , m_locked((m_safeObject.*lock)(&m_location))
{
}

PSafeLockBase::~PSafeLockBase()
{
  if (m_locked)
    (m_safeObject.*m_unlock)(&m_location);
}

bool PSafeLockBase::Lock()
{
  if (!m_locked)
    m_locked = (m_safeObject.*m_lock)(&m_location);
  return m_locked;
}

void PSafeLockBase::Unlock()
{
  if (m_locked) {
    (m_safeObject.*m_unlock)(&m_location);
    m_locked = false;
  }
}


/////////////////////////////////////////////////////////////////////////////

PSafeCollection::PSafeCollection()
  : m_collectionMutex(PDebugLocation(__FILE__, __LINE__, "SafeCollection"))
  , m_removalMutex(PDebugLocation(__FILE__, __LINE__, "SafeRemoval"))
{
}


PSafeCollection::~PSafeCollection()
{
  /* Delete objects moved to deleted list in RemoveAll(), we don't use
     DeleteObjectsToBeRemoved() as that will do a garbage collection which might
     prevent deletion. Need to be a bit more forceful here. */
  for (auto obj : m_toBeRemoved) {
    obj->GarbageCollection();
    if (obj->SafelyCanBeDeleted())
      delete obj;
    else {
      // If anything still has a PSafePtr .. "detach" it from the collection so
      // will be deleted when that PSafePtr finally goes out of scope.
      obj->m_safelyBeingRemoved = false;
    }
  }
}


void PSafeCollection::SafeRemoveObject(PSafeObject * obj)
{
  if (obj == NULL)
    return;

  obj->SafeRemove();

  m_removalMutex.Wait();
  m_toBeRemoved.push_back(obj);
  m_removalMutex.Signal();
}


bool PSafeCollection::DeleteObjectsToBeRemoved()
{
  {
    PWaitAndSignal lock(m_removalMutex);

    auto it = m_toBeRemoved.begin();
    while (it != m_toBeRemoved.end()) {
      PSafeObject * obj = *it;
      if (obj->GarbageCollection() && obj->SafelyCanBeDeleted()) {
        m_toBeRemoved.erase(it);

        m_removalMutex.Signal();
        DeleteObject(obj);
        m_removalMutex.Wait();

        it = m_toBeRemoved.begin(); // Restart looking through list
      }
      else {
        PTRACE(obj->GetTraceLogLevel(), obj->GetClassName() << ' ' << (void *)obj
               << " awaiting delete: references=" << obj->GetSafeReferenceCount());
        ++it;
      }
    }

    if (!m_toBeRemoved.empty())
      return false;
  }

  PWaitAndSignal lock(m_collectionMutex);
  return IsEmpty();
}


void PSafeCollection::DeleteObject(PObject * object) const
{
  delete object;
}


void PSafeCollection::SetAutoDeleteObjects()
{
#if P_TIMERS
  if (m_deleteObjectsTimer != NULL)
    return;

  m_deleteObjectsTimer = std::make_unique<PTimer>();
  m_deleteObjectsTimer->SetNotifier(PCREATE_NOTIFIER(DeleteObjectsTimeout), "SafeDelObj");
  m_deleteObjectsTimer->RunContinuous(1000); // Every second
#endif
}


#if P_TIMERS
void PSafeCollection::DeleteObjectsTimeout(PTimer &, intptr_t)
{
  DeleteObjectsToBeRemoved();
}
#endif


/////////////////////////////////////////////////////////////////////////////

PSafePtrBase::PSafePtrBase(PSafeObject * obj, PSafetyMode mode)
  : m_currentObject(obj)
  , m_lockMode(mode)
{
  EnterSafetyMode(WithReference);
}


PSafePtrBase::PSafePtrBase(const PSafePtrBase & enumerator)
  : m_currentObject(enumerator.m_currentObject)
  , m_lockMode(enumerator.m_lockMode)
{
  EnterSafetyMode(WithReference);
}


PSafePtrBase::~PSafePtrBase()
{
  ExitSafetyMode(WithDereference);
}


PObject::Comparison PSafePtrBase::Compare(const PObject & obj) const
{
  const PSafePtrBase * other = PDownCast(const PSafePtrBase, &obj);
  if (other == NULL)
    return GreaterThan;

  if (m_currentObject < other->m_currentObject)
    return LessThan;
  if (m_currentObject > other->m_currentObject)
    return GreaterThan;
  return EqualTo;
}


void PSafePtrBase::PrintOn(ostream &strm) const
{
  strm << m_currentObject;
}


void PSafePtrBase::Assign(const PSafePtrBase & enumerator)
{
  if (this == &enumerator)
    return;

  // lockCount ends up zero after this
  ExitSafetyMode(WithDereference);

  m_currentObject = enumerator.m_currentObject;
  m_lockMode = enumerator.m_lockMode;

  EnterSafetyMode(WithReference);
}


void PSafePtrBase::Assign(PSafeObject * newObj)
{
  ExitSafetyMode(WithDereference);

  m_currentObject = newObj;

  if (newObj == NULL)
    return;

  m_lockMode = PSafeReference;
  if (!EnterSafetyMode(WithReference))
    m_currentObject = NULL;
}


void PSafePtrBase::SetNULL()
{
  // lockCount ends up zero after this
  ExitSafetyMode(WithDereference);

  m_currentObject = NULL;
  m_lockMode = PSafeReference;
}


bool PSafePtrBase::SetSafetyMode(PSafetyMode mode)
{
  if (m_lockMode == mode)
    return true;

  ExitSafetyMode(NoDereference);
  m_lockMode = mode;
  return EnterSafetyMode(AlreadyReferenced);
}


bool PSafePtrBase::EnterSafetyMode(EnterSafetyModeOption ref)
{
  if (m_currentObject == NULL)
    return false;

  if (ref == WithReference && !m_currentObject->SafeReference()) {
    m_currentObject = NULL;
    return false;
  }

  switch (m_lockMode) {
    case PSafeReadOnly :
      if (m_currentObject->LockReadOnly())
        return true;
      break;

    case PSafeReadWrite :
      if (m_currentObject->LockReadWrite())
        return true;
      break;

    case PSafeReference :
      return true;
  }

  m_currentObject->SafeDereference();
  m_currentObject = NULL;
  return false;
}


void PSafePtrBase::ExitSafetyMode(ExitSafetyModeOption ref)
{
  if (m_currentObject == NULL)
    return;

  switch (m_lockMode) {
    case PSafeReadOnly :
      m_currentObject->UnlockReadOnly();
      break;

    case PSafeReadWrite :
      m_currentObject->UnlockReadWrite();
      break;

    case PSafeReference :
      break;
  }

  if (ref == WithDereference && m_currentObject->SafeDereference()) {
    PSafeObject * objectToDelete = m_currentObject;
    m_currentObject = NULL;
    DeleteObject(objectToDelete);
  }
}


void PSafePtrBase::DeleteObject(PSafeObject * obj)
{
  if (obj == NULL)
    return;

  PTRACE(obj->GetTraceLogLevel(), "Deleting object (" << obj << ')');
  obj->GarbageCollection();
  delete obj;
}


/////////////////////////////////////////////////////////////////////////////

PSafePtrMultiThreaded::PSafePtrMultiThreaded(PSafeObject * obj, PSafetyMode mode)
  : PSafePtrBase(NULL, mode)
  , m_mutex(PDebugLocation(__FILE__, __LINE__, "PSafePtrMultiThreaded"))
  , m_objectToDelete(NULL)
{
  LockPtr();

  m_currentObject = obj;
  EnterSafetyMode(WithReference);

  UnlockPtr();
}


PSafePtrMultiThreaded::PSafePtrMultiThreaded(const PSafePtrMultiThreaded & enumerator)
  : m_mutex(PDebugLocation(__FILE__, __LINE__, "PSafePtrMultiThreaded"))
  , m_objectToDelete(NULL)
{
  LockPtr();
  enumerator.m_mutex.Wait();

  m_currentObject = enumerator.m_currentObject;
  m_lockMode = enumerator.m_lockMode;

  EnterSafetyMode(WithReference);

  enumerator.m_mutex.Signal();
  UnlockPtr();
}


PSafePtrMultiThreaded::~PSafePtrMultiThreaded()
{
  LockPtr();
  ExitSafetyMode(WithDereference);
  m_currentObject = NULL;
  UnlockPtr();
}


PObject::Comparison PSafePtrMultiThreaded::Compare(const PObject & obj) const
{
  PWaitAndSignal mutex(m_mutex);
  return PSafePtrBase::Compare(obj);
}


void PSafePtrMultiThreaded::SetNULL()
{
  LockPtr();
  PSafePtrBase::SetNULL();
  UnlockPtr();
}


bool PSafePtrMultiThreaded::SetSafetyMode(PSafetyMode mode)
{
  PWaitAndSignal mutex(m_mutex);
  return PSafePtrBase::SetSafetyMode(mode);
}


void PSafePtrMultiThreaded::Assign(const PSafePtrMultiThreaded & ptr)
{
  LockPtr();
  ptr.m_mutex.Wait();
  PSafePtrBase::Assign(ptr);
  ptr.m_mutex.Signal();
  UnlockPtr();
}


void PSafePtrMultiThreaded::Assign(const PSafePtrBase & ptr)
{
  LockPtr();
  PSafePtrBase::Assign(ptr);
  UnlockPtr();
}


void PSafePtrMultiThreaded::Assign(PSafeObject * obj)
{
  LockPtr();
  PSafePtrBase::Assign(obj);
  UnlockPtr();
}


void PSafePtrMultiThreaded::DeleteObject(PSafeObject * obj)
{
  m_objectToDelete = obj;
}


void PSafePtrMultiThreaded::UnlockPtr()
{
  PSafeObject * obj = m_objectToDelete;
  m_objectToDelete = NULL;
  m_mutex.Signal();
  PSafePtrBase::DeleteObject(obj);
}


// End of File ///////////////////////////////////////////////////////////////
