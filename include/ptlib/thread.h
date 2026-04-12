/*
 * thread.h
 *
 * Executable thread encapsulation class
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

#ifndef PTLIB_THREAD_H
#define PTLIB_THREAD_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#ifdef Priority
#undef Priority
#endif

#include <ptlib/mutex.h>
#include <ptlib/syncpoint.h>
#include <ptlib/notifier.h>
#include <set>
#include <thread>


class PSemaphore;
class PSyncPoint;


///////////////////////////////////////////////////////////////////////////////
// PThread

/** This class defines a thread of execution in the system. A <i>thread</i> is
   an independent flow of processor instructions. This differs from a
   <i>process</i> which also embodies a program address space and resource
   allocation. So threads can share memory and resources as they run in the
   context of a given process. A process always contains at least one thread.
   This is reflected in this library by the <code>PProcess</code> class being
   descended from the <code>PThread</code> class.

   The implementation of a thread is platform dependent, but it is
   assumed that the platform has some support for native threads.
   Previous versions of PTLib/PWLib have some support for co-operative
   threads, but this has been removed.
 */
class PThread : public PObject, PProfiling::HighWaterMark<PThread>, PNonCopyable
{
  PCLASSINFO(PThread, PObject);

  public:
  /**@name Construction */
  //@{
    /// Codes for thread priorities.
    P_DECLARE_TRACED_ENUM(Priority,
      LowestPriority,  ///< Corresponds to Win32 THREAD_PRIORITY_LOWEST and posix SCHED_BATCH.
      LowPriority,     ///< Corresponds to Win32 THREAD_PRIORITY_BELOW_NORMAL and posix SCHED_BATCH.
      NormalPriority,  ///< Corresponds to Win32 THREAD_PRIORITY_NORMAL and posix SCHED_OTHER.
      HighPriority,    ///< Corresponds to Win32 THREAD_PRIORITY_ABOVE_NORMAL and posix SCHED_RR with sched_priority at 50%.
      HighestPriority  ///< Corresponds to Win32 THREAD_PRIORITY_HIGHEST and posix SCHED_RR with sched_priority at max.
    );

    /// Codes for thread auto-delete flag
    enum AutoDeleteFlag {
      /// Automatically delete thread object on termination.
      AutoDeleteThread,   

      /// Don't delete thread as it may not be on heap.
      NoAutoDeleteThread  
    };

    /** Create a new thread instance.
       
       Note that while the thread is started in this constructor it is
       immediately paused to avoid race conditions on initialising a
       descendent class. To Main() function is called for proceeding with
       thread once Start() is executed.

       If the <code>deletion</code> is set to <code>AutoDeleteThread</code>
       then the <code>PThread</code> is assumed to be allocated with the new operator and
       may be freed using the delete operator as soon as the thread is
       terminated or executes to completion (usually the latter).
     */
    PThread(
      AutoDeleteFlag deletion = AutoDeleteThread,
        ///< Automatically delete PThread instance on termination of thread.
      Priority priorityLevel = NormalPriority,  ///< Initial priority of thread.
      const PString & threadName = PString::Empty() ///< The name of the thread (for Debug/Trace)
    );
    P_DEPRECATED PThread(
      unsigned,
      AutoDeleteFlag deletion = AutoDeleteThread,
        ///< Automatically delete PThread instance on termination of thread.
      Priority priorityLevel = NormalPriority,  ///< Initial priority of thread.
      const PString & threadName = PString::Empty() ///< The name of the thread (for Debug/Trace)
    ) : PThread(deletion, priorityLevel, threadName)
    { }

    /** Destroy the thread, this simply calls the <code>Terminate()</code> function
       with all its restrictions and penalties. See that function for more
       information.

       Note that the correct way for a thread to terminate is to return from
       the <code>Main()</code> function.
     */
    ~PThread();
  //@}

  /**@name Overrides from <code>PObject</code> */
  //@{
    /**Standard stream print function.
       The <code>PObject</code> class has a << operator defined that calls this function
       polymorphically.
      */
    void PrintOn(
      ostream & strm    ///< Stream to output text representation
    ) const override;
  //@}

  /**@name Control functions */
  //@{
    /** Terminate the thread. It is highly recommended that this is not used
       except in abnormal abort situations as not all clean up of resources
       allocated to the thread will be executed. This is especially true in
       C++ as the destructors of objects that are automatic variables are not
       called causing at the very least the possibility of memory leaks.

       Note that the correct way for a thread to terminate is to return from
       the <code>Main()</code> function or self terminate by calling
       <code>Terminate()</code> within the context of the thread which can then
       assure that all resources are cleaned up.
     */
    virtual void Terminate();

    /** Determine if the thread has been terminated or ran to completion.

       @return
       <code>true</code> if the thread has been terminated.
     */
    virtual PBoolean IsTerminated() const;

    /** Block and wait for the thread to terminate.
     */
    void WaitForTermination() const;

    /** Block and wait for the thread to terminate.

       @return
       <code>false</code> if the thread has not terminated and the timeout has expired, <code>true</code> otherwise.
     */
    PBoolean WaitForTermination(
      const PTimeInterval & maxWait  ///< Maximum time to wait for termination.
    ) const;

    /**Wait for thread termination and delete object.
       The thread pointer is set to NULL within an optional mutex. The mutex is
       released before waiting for thread termination, which avoids deadlock
       scenarios. The \p lock parameter indicates if the mutex is already locked
       on entry and should not be locked again, preventing the unlock from
       occurring due to nested mutex semantics.

       If the \p maxTime timeout occurs, the thread is preemptively destroyed
       which is very likely to cause many issues, see PThread::Terminate() so
       a PAssert is raised when this condition occurs.

       @return
       <code>true</code> if the thread forcibly terminated, <code>false</code> if orderly exit.
      */
    static bool WaitAndDelete(
      PThread * & thread,                    ///< Thread to delete
      const PTimeInterval & maxWait = 10000, ///< Maximum time to wait for termination.
      PMutex * mutex = NULL,                 ///< Optional mutex to protect setting of thread variable
      bool lock = true                       ///< Mutex should be locked.
    );

    /** Start thread execution.
      * In practice, the thread is actually started in the constructor, but it
      * is suspended until this function is called. This allows the descendent
      * class to complete its initialisation before the thread begins
      * executing the Main() function.
     */
    void Start() { m_waitForStart.Signal(); }
    P_DEPRECATED void Resume() { Start(); }

    /// Suspend the current thread for the specified amount of time.
    static void Sleep(
      const PTimeInterval & delay   ///< Time interval to sleep for.
    );

    /** Set the priority of the thread relative to other threads in the current
       process.
     */
    virtual void SetPriority(
      Priority priorityLevel    ///< New priority for thread.
    );

    /** Get the current priority of the thread in the current process.

       @return
       current thread priority.
     */
    virtual Priority GetPriority() const;

    /** Get the name of the thread. Thread names are a optional debugging aid.

       @return
       current thread name.
     */
    virtual PString GetThreadName() const;

    /** Get the name of the thread. Thread names are a optional debugging aid.

       @return
       current thread name.
     */
    static PString GetThreadName(PThreadIdentifier id);

    /**Get the current threads name.
      */
    static PString GetCurrentThreadName() { return GetThreadName(GetCurrentThreadId()); }

    /**Convert to thread identifiers as a string.
       For platforms where unique thread id is different from the "normal" thread id,
       then the output will append it to the thread id string, e.g. "0x12345678 (54321)".
      */
    static PString GetIdentifiersAsString(PThreadIdentifier tid, PUniqueThreadIdentifier uid);

    /** Change the name of the thread. Thread names are a optional debugging aid.

       @return
       current thread name.
     */
    virtual void SetThreadName(
      const PString & name        ///< New name for the thread.
    );
  //@}

  /**@name Miscellaneous */
  //@{
    PPROFILE_EXCLUDE(
      /** Get operating system specific thread identifier for this thread.
      */
      PThreadIdentifier GetThreadId() const
    );

    PPROFILE_EXCLUDE(
    /** Get operating system specific thread identifier for the current thread.
      */
      static PThreadIdentifier GetCurrentThreadId()
    );

    PPROFILE_EXCLUDE(
    /** This returns a unique number for the thread.
        For most platforms this is identical to GetThreadId(), but others,
        e.g. Linux, it is different and GetThreadId() return values get
        re-used during the run of the process.
      */
      PUniqueThreadIdentifier GetUniqueIdentifier() const
    );

    PPROFILE_EXCLUDE(
    /** This returns a unique number for the current thread.
      */
      static PUniqueThreadIdentifier GetCurrentUniqueIdentifier()
    );

    PPROFILE_EXCLUDE(
    /** Get the total number of threads.
     */
      static PINDEX GetTotalCount()
    );

    /// Times for execution of the thread.
    struct Times
    {
      PPROFILE_EXCLUDE(
        Times()
      );
      PPROFILE_EXCLUDE(
        friend ostream & operator<<(ostream & strm, const Times & times)
      );
      PPROFILE_EXCLUDE(
        float AsPercentage() const
      );
      PPROFILE_EXCLUDE(
        Times operator-(const Times & rhs) const
      );
      PPROFILE_EXCLUDE(
        Times & operator-=(const Times & rhs)
      );
      PPROFILE_EXCLUDE(
        bool operator<(const Times & rhs) const { return m_uniqueId < rhs.m_uniqueId; }
      );

      PString                 m_name;     ///< Name of thread
      PThreadIdentifier       m_threadId; ///< Operating system thread ID
      PUniqueThreadIdentifier m_uniqueId; ///< Unique thread identifier
      PTimeInterval           m_real;     ///< Total real time since thread start in milliseconds.
      PTimeInterval           m_kernel;   ///< Total kernel CPU time in milliseconds.
      PTimeInterval           m_user;     ///< Total user CPU time in milliseconds.
      PTime                   m_sample;   ///< Time of sample
    };

    PPROFILE_EXCLUDE(
    /** Get the thread execution times.
     */
    bool GetTimes(
      Times & times   ///< Times for thread execution.
    ));

    PPROFILE_EXCLUDE(
    /** Get the thread execution times.
     */
    static bool GetTimes(
      PThreadIdentifier id, ///< Thread identifier to get times for
      Times & times         ///< Times for thread execution.
    ));

    PPROFILE_EXCLUDE(
    /** Get the thread execution times for all threads.
     */
    static void GetTimes(
      std::vector<Times> & times    ///< Times for thread execution.
    ));
    static void GetTimes(
      std::list<Times> & times      ///< Times for thread execution.
    );
    static void GetTimes(
      std::set<Times> & times      ///< Times for thread execution.
    );

    /** Calculate the percentage CPU used over a period of time.
        Usage:
           PThread::Times previous;
           for (;;) {
             DoStuff();
             unsigned percentage;
             if (PThread::GetPercentageCPU(previous, percentage, 2000) && percentage > 90)
               cout << "Excessive CPU used!" << end;
           }

        @returns percentage value if calculated, -1 if thread id incorrect or -2 if
                 it is not yet time to calculate the percentage based on \p period.
      */
    static int GetPercentageCPU(
      Times & previousTimes,                             ///< Previous CPU times, contecxt for calculations
      const PTimeInterval & period = PTimeInterval(0,1), ///< Time over which to integrate the average CPU usage.
      PThreadIdentifier id = PNullThreadIdentifier       ///< Thread ID to get CPU usage, null means current thread
    );

    /**Get number of processors, or processor cores, this machine has available.
      */
    static unsigned GetNumProcessors();

    /** User override function for the main execution routine of the thread. A
       descendent class must provide the code that will be executed in the
       thread within this function.
       
       Note that the correct way for a thread to terminate is to return from
       this function.
     */
    virtual void Main() = 0;

    /** Get the currently running thread object instance. It is possible, even
       likely, that the smae code may be executed in the context of differenct
       threads. Under some circumstances it may be necessary to know what the
       current codes thread is and this static function provides that
       information.

       @return
       pointer to current thread.
     */
    static PThread * Current();

    /** Yield to another thread without blocking.
        This duplicates the implicit thread yield that may occur on some
        I/O operations or system calls.

        This may not be implemented on all platforms.
     */
    static void Yield();

    /**Create a simple thread executing the specified notifier.
       This creates a simple <code>PThread</code> class that automatically executes the
       function defined by the <code>PNotifier</code> in the context of a new thread.
      */
    static PThread * Create(
      const PNotifier & notifier,     ///< Function to execute in thread.
      INT parameter = 0,              ///< Parameter value to pass to notifier.
      AutoDeleteFlag deletion = AutoDeleteThread,
        ///< Automatically delete PThread instance on termination of thread.
      Priority priorityLevel = NormalPriority,  ///< Initial priority of thread.
      const PString & threadName = PString::Empty() ///< The name of the thread (for Debug/Trace)
    );
    static PThread * Create(
      const PNotifier & notifier,     ///< Function to execute in thread.
      const PString & threadName      ///< The name of the thread (for Debug/Trace)
    ) { return Create(notifier, 0, NoAutoDeleteThread, NormalPriority, threadName); }
  //@}
  
  protected:
    enum Type { e_IsAutoDelete, e_IsManualDelete, e_IsProcess, e_IsExternal } m_type;

    std::atomic<Priority> m_priority;
    PTime m_threadStartTime{ 0 };
    PTime m_threadEndTime{ 0 };

    PString m_threadName; // Give the thread a name for debugging purposes.
    PCriticalSection m_threadNameMutex;

    PSyncPoint              m_waitForStart;
    PThreadIdentifier       m_threadId = PNullThreadIdentifier;
    PUniqueThreadIdentifier m_uniqueId = 0;

// Include platform dependent part of class
#ifdef _WIN32
    PWin32Handle m_threadHandle;
#ifdef P_WIN_COM
    struct WinComInit {
      WinComInit();
      ~WinComInit();
    };
#endif
#else
    struct PipeHandles : std::array<int, 2>
    {
      PipeHandles() :std::array<int, 2>{-1,-1} { PAssertOS(::pipe(data()) == 0); }
      ~PipeHandles() { for (auto h : *this) ::close(h); }
    } m_unblockPipeHandles;
    friend class PSocket;
  public:
    int PXBlockOnChildTerminate(int pid, const PTimeInterval & timeout);
    int PXBlockOnIO(int handle, int type, const PTimeInterval & timeout);
    void PXAbortBlock() const;
    static bool PX_kill(PThreadIdentifier tid, PUniqueThreadIdentifier uid, int sig);
#endif

  private:
    PThread(Type type, Priority priority, const PString & name);
    void InternalThreadMain();
    void InternalThreadStarted();

  friend class PProcess;
  friend class PExternalThread;
};


/** Define some templates to simplify the declaration
  * of simple <code>PThread</code> descendants with one or two parameters 
  */

/*
   This class automates calling a global function with no arguments within it's own thread.
   It is used as follows:

   void GlobalFunction()
   {
   }

   ...
   PString arg;
   new PThreadMain(&GlobalFunction)
 */
class PThreadMain : public PThread
{
    PCLASSINFO(PThreadMain, PThread);
  public:
    typedef void (*FnType)(); 
    PThreadMain(FnType function, bool autoDel = false)
      : PThread(autoDel ? PThread::AutoDeleteThread : PThread::NoAutoDeleteThread)
      , m_function(function)
    {
      Start();
    }

    ~PThreadMain()
    {
        WaitForTermination();
    }

    virtual void Main() override
    {
      (*m_function)();
    }

  protected:
    FnType m_function;
};


/*
   This template automates calling a global function using a functor
   It is used as follows:

   struct Functor {
     void operator()(PThread & thread) { ... code in here }
   }

   ...
   Functor arg;
   new PThreadFunctor<Functor>(arg)
 */
template<typename Functor>
class PThreadFunctor : public PThread
{
    PCLASSINFO(PThreadFunctor, PThread);
  public:
    PThreadFunctor(
      Functor & funct,
      bool autoDel = false,
      const char * name = NULL,
      PThread::Priority priority = PThread::NormalPriority
    )
      : PThread(10000, autoDel ? PThread::AutoDeleteThread : PThread::NoAutoDeleteThread, priority, name)
      , m_funct(funct)
    {
      Start();
    }

    ~PThreadFunctor()
    {
        WaitForTermination();
    }

    virtual void Main() override
    {
      m_funct(*this);
    }

  protected:
    Functor & m_funct;
};


/*
   This template automates calling a global function with one argument within it's own thread.
   It is used as follows:

   void GlobalFunction(PString arg)
   {
   }

   ...
   PString arg;
   new PThread1Arg<PString>(arg, &GlobalFunction)
 */
template<typename Arg1Type>
class PThread1Arg : public PThread
{
    PCLASSINFO(PThread1Arg, PThread);
  public:
    typedef void (*FnType)(Arg1Type arg1);

    PThread1Arg(
      Arg1Type arg1,
      FnType function,
      bool autoDel = false,
      const char * name = NULL,
      PThread::Priority priority = PThread::NormalPriority
    ) : PThread(autoDel ? PThread::AutoDeleteThread : PThread::NoAutoDeleteThread, priority, name)
      , m_function(function)
      , m_arg1(arg1)
    {
      Start();
    }

    ~PThread1Arg()
    {
        WaitForTermination();
    }

    virtual void Main() override
    {
      (*m_function)(m_arg1);
    }

  protected:
    FnType   m_function;
    Arg1Type m_arg1;
};


/*
   This template automates calling a global function with two arguments within it's own thread.
   It is used as follows:

   void GlobalFunction(PString arg1, int arg2)
   {
   }

   ...
   PString arg;
   new PThread2Arg<PString, int>(arg1, arg2, &GlobalFunction)
 */
template<typename Arg1Type, typename Arg2Type>
class PThread2Arg : public PThread
{
    PCLASSINFO(PThread2Arg, PThread);
  public:
    typedef void (*FnType)(Arg1Type arg1, Arg2Type arg2); 
    PThread2Arg(
      Arg1Type arg1,
      Arg2Type arg2,
      FnType function,
      bool autoDel = false,
      const char * name = NULL,
      PThread::Priority priority = PThread::NormalPriority
    ) : PThread(autoDel ? PThread::AutoDeleteThread : PThread::NoAutoDeleteThread, priority, name)
      , m_function(function)
      , m_arg1(arg1)
      , m_arg2(arg2)
    {
      Start();
    }
    
    ~PThread2Arg()
    {
        WaitForTermination();
    }

    virtual void Main() override
    {
      (*m_function)(m_arg1, m_arg2);
    }

  protected:
    FnType   m_function;
    Arg1Type m_arg1;
    Arg2Type m_arg2;
};

/*
   This template automates calling a global function with three arguments within it's own thread.
   It is used as follows:

   void GlobalFunction(PString arg1, int arg2, int arg3)
   {
   }

   ...
   PString arg;
   new PThread3Arg<PString, int, int>(arg1, arg2, arg3, &GlobalFunction)
 */
template<typename Arg1Type, typename Arg2Type, typename Arg3Type>
class PThread3Arg : public PThread
{
  PCLASSINFO(PThread3Arg, PThread);
  public:
    typedef void (*FnType)(Arg1Type arg1, Arg2Type arg2, Arg3Type arg3); 
    PThread3Arg(
      Arg1Type arg1,
      Arg2Type arg2,
      Arg3Type arg3,
      FnType function,
      bool autoDel = false,
      const char * name = NULL,
      PThread::Priority priority = PThread::NormalPriority
    ) : PThread(autoDel ? PThread::AutoDeleteThread : PThread::NoAutoDeleteThread, priority, name)
      , m_function(function)
      , m_arg1(arg1)
      , m_arg2(arg2)
      , m_arg3(arg3)
    {
      Start();
    }
    
    ~PThread3Arg()
    {
        WaitForTermination();
    }

    virtual void Main() override
    {
      (*m_function)(m_arg1, m_arg2, m_arg3);
    }

  protected:
    FnType   m_function;
    Arg1Type m_arg1;
    Arg2Type m_arg2;
    Arg3Type m_arg3;
};

/*
   This template automates calling a member function with no arguments within it's own thread.
   It is used as follows:

   class Example {
     public:
      void Function()
      {
      }
   };

   ...
   Example ex;
   new PThreadObj<Example>(ex, &Example::Function)
 */

template <typename ObjType>
class PThreadObj : public PThread
{
    PCLASSINFO_ALIGNED(PThreadObj, PThread, 16);
  public:
    typedef void (ObjType::*ObjTypeFn)(); 

    PThreadObj(
      ObjType & obj,
      ObjTypeFn function,
      bool autoDel = false,
      const char * name = NULL,
      PThread::Priority priority = PThread::NormalPriority
    ) : PThread(autoDel ? PThread::AutoDeleteThread : PThread::NoAutoDeleteThread, priority, name)
      , m_object(obj)
      , m_function(function)
    {
      Start();
    }

    ~PThreadObj()
    {
        WaitForTermination();
    }

    void Main() override
    {
      (m_object.*m_function)();
    }

  protected:
    ObjType & m_object;
    P_ALIGN_FIELD(ObjTypeFn,m_function,16);
};


/*
   This template automates calling a member function with one argument within it's own thread.
   It is used as follows:

   class Example {
     public:
      void Function(PString arg)
      {
      }
   };

   ...
   Example ex;
   PString str;
   new PThreadObj1Arg<Example>(ex, str, &Example::Function)
 */
template <class ObjType, typename Arg1Type>
class PThreadObj1Arg : public PThread
{
    PCLASSINFO_ALIGNED(PThreadObj1Arg, PThread, 16);
  public:
    typedef void (ObjType::*ObjTypeFn)(Arg1Type); 

    PThreadObj1Arg(
      ObjType & obj,
      Arg1Type arg1,
      ObjTypeFn function,
      bool autoDel = false,
      const char * name = NULL,
      PThread::Priority priority = PThread::NormalPriority
    ) : PThread(autoDel ? PThread::AutoDeleteThread : PThread::NoAutoDeleteThread, priority, name)
      , m_object(obj)
      , m_function(function)
      , m_arg1(arg1)
    {
      Start();
    }

    ~PThreadObj1Arg()
    {
        WaitForTermination();
    }

    void Main() override
    {
      (m_object.*m_function)(m_arg1);
    }

  protected:
    ObjType & m_object;
    P_ALIGN_FIELD(ObjTypeFn,m_function,16);
    Arg1Type  m_arg1;
};

template <class ObjType, typename Arg1Type, typename Arg2Type>
class PThreadObj2Arg : public PThread
{
    PCLASSINFO_ALIGNED(PThreadObj2Arg, PThread, 16);
  public:
    typedef void (ObjType::*ObjTypeFn)(Arg1Type, Arg2Type);

    PThreadObj2Arg(
      ObjType & obj,
      Arg1Type arg1,
      Arg2Type arg2,
      ObjTypeFn function,
      bool autoDel = false,
      const char * name = NULL,
      PThread::Priority priority = PThread::NormalPriority
    ) : PThread(autoDel ? PThread::AutoDeleteThread : PThread::NoAutoDeleteThread, priority, name)
      , m_object(obj)
      , m_function(function)
      , m_arg1(arg1)
      , m_arg2(arg2)
    {
      Start();
    }

    ~PThreadObj2Arg()
    {
        WaitForTermination();
    }

    void Main() override
    {
      (m_object.*m_function)(m_arg1, m_arg2);
    }

  protected:
    ObjType & m_object;
    P_ALIGN_FIELD(ObjTypeFn,m_function,16);
    Arg1Type  m_arg1;
    Arg2Type  m_arg2;
};


#endif // PTLIB_THREAD_H

// End Of File ///////////////////////////////////////////////////////////////
