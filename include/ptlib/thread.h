/*
 * thread.h
 *
 * Executable thread encapsulation class (pre-emptive if OS allows).
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

#ifndef PTLIB_THREAD_H
#define PTLIB_THREAD_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#ifdef Priority
#undef Priority
#endif

#include <ptlib/mutex.h>
#include <ptlib/notifier.h>
#include <set>


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

    /// Codes for thread autodelete flag
    enum AutoDeleteFlag {
      /// Automatically delete thread object on termination.
      AutoDeleteThread,   

      /// Don't delete thread as it may not be on heap.
      NoAutoDeleteThread  
    };

    /** Create a new thread instance. Unless the <code>startSuspended</code>
       parameter is <code>true</code>, the threads <code>Main()</code> function is called to
       execute the code for the thread.
       
       Note that the exact timing of the execution of code in threads can
       never be predicted. Thus you you can get a race condition on
       intialising a descendent class. To avoid this problem a thread is
       always started suspended. You must call the <code>Resume()</code> function after
       your descendent class construction is complete.

       If synchronisation is required between threads then the use of
       semaphores is essential.

       If the <code>deletion</code> is set to <code>AutoDeleteThread</code>
       then the <code>PThread</code> is assumed to be allocated with the new operator and
       may be freed using the delete operator as soon as the thread is
       terminated or executes to completion (usually the latter).
     */
    PThread(
      AutoDeleteFlag deletion = AutoDeleteThread,   ///< Automatically delete PThread instance on termination of thread.
      Priority priorityLevel = NormalPriority,      ///< Initial priority of thread.
      PString threadName = PString::Empty()         ///< The name of the thread (for Debug/Trace)
    );
    PThread(
      std::function<void()> mainFunction,           ///< Function to execute
      PString threadName = PString::Empty(),        ///< The name of the thread (for Debug/Trace)
      bool startImmediate = true,                   ///< Start execution immediatelty
      AutoDeleteFlag deletion = NoAutoDeleteThread, ///< Automatically delete PThread instance on termination of thread.
      Priority priorityLevel = NormalPriority       ///< Initial priority of thread.
    );
    P_DEPRECATED PThread(
      PINDEX,  ///< Stack size, no longer used
      AutoDeleteFlag deletion = AutoDeleteThread,  ///< Automatically delete PThread instance on termination of thread.
      Priority priorityLevel = NormalPriority,     ///< Initial priority of thread.
      PString threadName = PString::Empty()        ///< The name of the thread (for Debug/Trace)
    );

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
    /** Start a thread.
        If the thread had run to completion, it is restarted.
        If the thread is running then this function is ignored.
     */
    virtual bool Start();
    P_DEPRECATED void Restart() { Start(); }
    P_DEPRECATED void Resume() { Start(); }

    /** Terminate the thread. It is highly recommended that this is not used
       except in abnormal abort situations as not all clean up of resources
       allocated to the thread will be executed. This is especially true in
       C++ as the destructors of objects that are automatic variables are not
       called causing at the very least the possiblity of memory leaks.

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
    virtual bool IsTerminated() const;

    /** Block and wait for the thread to terminate.
     */
    void WaitForTermination() const;

    /** Block and wait for the thread to terminate.

       @return
       <code>false</code> if the thread has not terminated and the timeout has expired, <code>true</code> otherwise.
     */
    bool WaitForTermination(
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

    /** Set the flag indicating thread object is to be automatically deleted
        when the thread ends.
     */
    virtual void SetAutoDelete(
      AutoDeleteFlag deletion = AutoDeleteThread  ///< New auto delete setting.
    );

    /** Reset the flag indicating thread object is to be automatically deleted
        when the thread ends.
     */
    void SetNoAutoDelete() { SetAutoDelete(NoAutoDeleteThread); }

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

    /**For outputting thread ID's to a stream.
       For platforms where unique thread id is different from the "normal" thread id,
       then the output will append it to the thread id string, e.g. "0x12345678 (54321)".
      */
    class Identifiers
    {
        PThreadIdentifier m_tid;
        PUniqueThreadIdentifier m_uid;
      public:
        Identifiers(PThreadIdentifier tid, PUniqueThreadIdentifier uid) : m_tid(tid), m_uid(uid) { }
        friend ostream & operator<<(ostream& strm, const Identifiers & ids);
    };

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
    virtual void Main();

    /** Get the currently running thread object instance. It is possible, even
       likely, that the same code may be executed in the context of different
       threads. Under some circumstances it may be necessary to know what the
       current codes thread is and this static function provides that
       information.

       @return
       pointer to current thread.
     */
    static PThread* Current();

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
      intptr_t parameter = 0,         ///< Parameter value to pass to notifier.
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
  
    bool IsAutoDelete() const { return m_type == Type::IsAutoDelete; }

  private:
    void InternalThreadMain();
    void PlatformConstruct();
    void PlatformDestroy();
    void PlatformSetThreadName(const char * name);

    friend class PProcess;
    friend class PExternalThread;
    // So a PProcess can get at PThread() constructor but nothing else.

    PThread & operator=(const PThread &) { return *this; }
    // Empty assignment operator to prevent copying of thread instances.

  protected:
    void InternalThreadStarted();

    enum class Type { IsAutoDelete, IsManualDelete, IsProcess, IsExternal } m_type;
    PThread(Type type, PString name, Priority priorityLevel = NormalPriority, std::function<void()> mainFunction = {});

    std::function<void()>           m_mainFunction;
    const Priority                  m_initialPriority;
    PString                         m_threadName; // Give the thread a name for debugging purposes.
    mutable std::mutex              m_threadNameMutex;
    mutable std::timed_mutex        m_threadRunning;
    std::atomic<std::thread::native_handle_type> m_nativeHandle;
    std::atomic<PThreadIdentifier>  m_threadId;
    PUniqueThreadIdentifier         m_uniqueId;
    PTime                           m_threadStartTime;
    PTime                           m_threadEndTime;

    static thread_local PThread * sm_currentThread;

    // Include platform dependent part of class
#ifdef _WIN32
  #if defined(P_WIN_COM)
    public:
      bool CoInitialise();
    private:
      bool m_comInitialised;
  #endif
#else
  friend class PSocket;
  public:
    int PXBlockOnIO(
      int handle,
      int type,
      const PTimeInterval & timeout
    );

    void PXAbortBlock() const;
    static bool PlatformKill(PThreadIdentifier tid, PUniqueThreadIdentifier uid, int sig);

  protected:
    int m_threadUnblockPipe[2];
#endif
};


/** Define some templates to simplify the declaration
  * of simple <code>PThread</code> descendants with one or two paramaters 
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
      : PThread(autoDel ? PThread::AutoDeleteThread : PThread::NoAutoDeleteThread, priority, name)
      , m_funct(funct)
    {
      this->Start();
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
      this->Start();
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
      this->Start();
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
      this->Start();
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
      this->Start();
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
      this->Start();
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
      this->Start();
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


///////////////////////////////////////////////////////////////////////////////
//
// PThreadLocalStorage
//

template <class T>
class P_DEPRECATED PThreadLocalStorage
{
  public:
    T * Get() const        { return &m_data; }
    operator T *() const   { return &m_data; }
    T * operator->() const { return &m_data; }
    T & operator*() const  { return  m_data; }

  protected:
    static thread_local T m_data;
};

template <class T> T thread_local PThreadLocalStorage<T>::m_data;


#endif // PTLIB_THREAD_H

// End Of File ///////////////////////////////////////////////////////////////
