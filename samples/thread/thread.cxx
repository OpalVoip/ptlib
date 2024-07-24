/*
 * thread.cxx
 *
 * Sample program to test PWLib threads.
 *
 * Portable Tools Library
 *
 * Copyright (c) 2001,2002 Roger Hardiman
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
 * The Initial Developer of the Original Code is Roger Hardiman
 *
 */

/*
 * This sample program tests threads is PWLib. It creates two threads,
 * one which display the number '1' and one which displays the number '2'.
 * It also demonstrates starting a thread with Resume(), using
 * Suspend() and Resume() to suspend a running thread and two different
 * ways to make a thread terminate.
 */

#include <ptlib.h>
#include <ptlib/pprocess.h>

/*
 * Thread #1 displays the number 1 every 10ms.
 * When it is created, Main() starts executing immediatly.
 * The thread is terminated by calling Stop() which uses a PSyncPoint with a
 * 10ms timeout.
 */
class MyThread1 : public PThread
{
  PCLASSINFO(MyThread1, PThread);
  public:
    MyThread1() : PThread(NoAutoDeleteThread)
    {
      Start(); // start running this thread when it is created.
    }

    virtual void Main() override {
      while (!shutdown.Wait(10)) { // 10ms delay
        printf("1 ");
        fflush(stdout);
	Sleep(10);
      }
    }

    void Stop() {
      // signal the shutdown PSyncPoint. On the next iteration, the thread's
      // Main() function will exit cleanly.
      shutdown.Signal();
    }

    protected:
      PSyncPoint shutdown;
};


/*
 * Thread #2 displays the number 2 every 10 ms.
 * This thread will not start automatically. We must call
 * Resume() after creating the thread.
 * The thread is terminated by calling Stop() which sets a local variable.
 */
class MyThread2 : public PThread
{
  PCLASSINFO(MyThread2, PThread);
  public:
    MyThread2()
      : PThread(NoAutoDeleteThread)
      , m_exitFlag(false)
    {
    }

    void Main() override {
      for (;;) {
        // Check if we need to exit
        if (m_exitFlag.load()) {
          break;
        }

        // Display the number 2, then sleep for a short time
        printf("2 ");
        fflush(stdout);
	Sleep(10); // sleep 10ms
      }
    }

    void Stop()
    {
      m_exitFlag.store(true);
    }

    protected:
      atomic<bool> m_exitFlag;
};


void ExternalThread()
{
  PThread::Sleep(1000);
  PThread * thread = PThread::Current();
  cout << "External thread: " << thread << ' ' << thread->GetThreadName().ToLiteral() << endl;
  PThread::Sleep(1000);
}


void LockAthenB(PMutex * a, PMutex * b)
{
  cout << "Locking A then B" << endl;
  a->Wait();
  b->Wait();
  cout << "Locked A then B" << endl;
  PThread::Sleep(16000);
  b->Signal();
  a->Signal();
  cout << "Unocked A then B" << endl;
}


void LockBthenA(PMutex * a, PMutex * b)
{
  cout << "Locking B then A" << endl;
  b->Wait();
  a->Wait();
  cout << "Locked B then A" << endl;
  PThread::Sleep(1000);
  b->Signal();
  a->Signal();
  cout << "Unocked B then A" << endl;
}


/*
 * The main program class
 */
class ThreadTest : public PProcess
{
  PCLASSINFO(ThreadTest, PProcess)
public:
  void Main() override
  {
    cout << "Thread Test Program" << endl;

    PArgList & args = GetArguments();
    args.Parse(PTRACE_ARGLIST);
    PTRACE_INITIALISE(args);

    SimpleTest();
    ExternalTest();
    DeadlockTest();
  }

  void SimpleTest()
  {
    cout << "This program will display the following:" << endl;
    cout << "             2 seconds of 1 1 1 1 1..." << endl;
    cout << " followed by 2 seconds of 1 2 1 2 1 2 1 2 1 2..." << endl;
    cout << " followed by 2 seconds of 2 2 2 2 2..." << endl;
    cout << endl;

    // Create the threads
    auto mythread1 = std::make_unique<MyThread1>();
    auto mythread2 = std::make_unique<MyThread2>();

    // Thread 1 should now be running, as there is a Start() function
    // in the thread constructor.
    // Thread 2 should be suspended.
    // Sleep for three seconds. Only thread 1 will be running.
    // Display will show "1 1 1 1 1 1 1..."
    Sleep(2000);


    // Start the second thread.
    // Both threads should be running
    // Sleep for 3 seconds, allowing the threads to run.
    // Display will show "1 2 1 2 1 2 1 2 1 2..."
    mythread2->Start();
    Sleep(2000);

    // Clean up
    mythread1->Stop();
    mythread1->WaitForTermination();

    Sleep(2000);

    mythread2->Stop();
    mythread2->WaitForTermination();

    cout << endl;
  }

  void ExternalTest()
  {
    cout << "\nTesting external thread." << endl;
    auto ext = std::make_unique<std::thread>(&ExternalThread);
    ext->join();
  }

  void DeadlockTest()
  {
    #if PTRACING
    PTrace::SetLevel(std::max(PTrace::GetLevel(), 1U));
    #endif
    cout << "\nTesting deadlock detection." << endl;
    PTimedMutex a, b;
    auto th1 = std::make_unique<PThread>(std::bind(LockAthenB, &a, &b));
    auto th2 = std::make_unique<PThread>(std::bind(LockBthenA, &a, &b));
    Sleep(20000);
    cout << "Deleting deadlock threads." << endl;
  }
};

PCREATE_PROCESS(ThreadTest);
