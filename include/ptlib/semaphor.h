/*
 * semaphor.h
 *
 * Thread synchronisation semaphore class.
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

#ifndef PTLIB_SEMAPHORE_H
#define PTLIB_SEMAPHORE_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <ptlib/psync.h>
#include <limits.h>
#include <ptlib/atomic.h>
#include <semaphore>


/**This class defines a thread synchronisation object. This is in the form of a
   integer semaphore. The semaphore has a count and a maximum value. The
   various combinations of count and usage of the Wait() and
   Signal() functions determine the type of synchronisation mechanism
   to be employed.

   The Wait() operation is that if the semaphore count is > 0,
   decrement the semaphore and return. If it is = 0 then wait (block).

   The Signal() operation is that if there are waiting threads then
   unblock the first one that was blocked. If no waiting threads and the count
   is less than the maximum then increment the semaphore.

</code></pre>
    The first thread will pass through the Wait() function, a second
    thread will block on that function until the first calls the
    Signal() function, releasing the second thread.
 */
template <ptrdiff_t max_value = PTRDIFF_MAX>
class PSemaphoreT : public PSync, public std::counting_semaphore<max_value>
{
  PCLASSINFO(PSemaphoreT, PSync);

public:
/**@name Construction */
//@{
  /**Create a new semaphore with maximum count and initial value specified.
     If the initial value is larger than the maximum value then is is set to
     the maximum value.
   */
  PSemaphoreT(
    unsigned initial = 0         ///< Initial value for semaphore count.
  ) : std::counting_semaphore<max_value>(initial) { }
//@}

/**@name Operations */
//@{
  /**If the semaphore count is > 0, decrement the semaphore and return. If
     if is = 0 then wait (block).
   */
  virtual void Wait() override
  {
    this->acquire();
  }

  /**If the semaphore count is > 0, decrement the semaphore and return. If
     if is = 0 then wait (block) for the specified amount of time.

     @return
     true if semaphore was signalled, false if timed out.
   */
  virtual bool Wait(
    const PTimeInterval & timeout // Amount of time to wait for semaphore.
  ) override
  {
    return this->try_acquire_for(timeout.AsChronoUS());
  }

  /**If there are waiting (blocked) threads then unblock the first one that
     was blocked. If no waiting threads and the count is less than the
     maximum then increment the semaphore.
   */
  virtual void Signal() override
  {
    this->release();
  }
//@}
};

using PSemaphore = PSemaphoreT<>;


/** A named semapore unique cross processes.
*/
class PNamedSemaphore : public PSync
{
  PCLASSINFO(PNamedSemaphore, PSync);

  public:
  /**@name Construction */
  //@{
    /**Create a new system global semaphore with maximum count and initial value specified.
       If the initial value is larger than the maximum value then is is set to
       the maximum value.
     */
    PNamedSemaphore(
      const PString & name,
      unsigned initial = 0,         ///< Initial value for semaphore count.
      unsigned maximum = UINT_MAX   ///< Maximum value for semaphore count.
    ) : m_name(name) { Reset(initial, maximum); }

    /** Create a new semaphore with the same initial and maximum values as the original.
     */
    PNamedSemaphore(const PNamedSemaphore & sem)
      : PSync()
      , m_name(sem.m_name)
    { Reset(sem.m_initial, sem.m_maximum); }

    /**Destroy the semaphore. This will assert if there are still waiting
       threads on the semaphore.
     */
    ~PNamedSemaphore();
  //@}

  /**@name Operations */
  //@{
    /**If the semaphore count is > 0, decrement the semaphore and return. If
       if is = 0 then wait (block).
     */
    virtual void Wait() override;

    /**If the semaphore count is > 0, decrement the semaphore and return. If
       if is = 0 then wait (block) for the specified amount of time.

       @return
       true if semaphore was signalled, false if timed out.
     */
    virtual bool Wait(
      const PTimeInterval & timeout // Amount of time to wait for semaphore.
    ) override;

    /**If there are waiting (blocked) threads then unblock the first one that
       was blocked. If no waiting threads and the count is less than the
       maximum then increment the semaphore.
     */
    virtual void Signal() override;

    /** Reset the semaphore to the specified initial and maximum values.
        Note that the behaviour is undetermined if something is waiting on the
        semaphore when this is called.
      */
    virtual void Reset(
      unsigned initial = 0,         ///< Initial value for semaphore count.
      unsigned maximum = UINT_MAX   ///< Maximum value for semaphore count.
    );

    /** Get the initial value semaphore was created with.
      */
    unsigned GetInitial() const  { return m_initial; }

    /** Get the initial value semaphore was created with.
      */
    unsigned GetMaximum() const { return m_maximum; }
  //@}


  protected:
    PString  m_name;
    unsigned m_maximum;
    unsigned m_initial;


// Include platform dependent part of class
#ifdef _WIN32
  public:
    HANDLE GetHandle() const { return m_handle; }
  protected:
    PWin32Handle m_handle;
#else
protected:
    mutable sem_t m_semaphore;
    #if defined(P_HAS_NAMED_SEMAPHORES)
      mutable struct SemPtr {
        SemPtr() : ptr(NULL) { }
        sem_t * ptr;
      } m_namedSemaphore;
      __inline sem_t * GetSemPtr() const { return m_namedSemaphore.ptr != NULL ? m_namedSemaphore.ptr : &m_semaphore; }
    #else
      __inline sem_t * GetSemPtr() const { return &m_semaphore; }
    #endif
#endif
};


#endif // PTLIB_SEMAPHORE_H


// End Of File ///////////////////////////////////////////////////////////////
