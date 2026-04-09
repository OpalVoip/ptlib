/*
 * thread.cxx
 *
 * Sample program to test PWLib threads.
 *
 * Portable Windows Library
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
 * The Original Code is Portable Windows Library.
 *
 * The Initial Developer of the Original Code is Roger Hardiman
 *
 * Contributor(s): Robert Jongbloed (robertj@voxlucida.com.au).
 */

#include <ptlib.h>
#include <ptlib/pprocess.h>
#include <thread>


class MyNoAutoDeleteThread : public PThread
{
  PCLASSINFO(MyNoAutoDeleteThread, PThread);
  public:
    MyNoAutoDeleteThread()
      : PThread(0, NoAutoDeleteThread)
    {
      Resume();
    }

    virtual void Main() override
    {
      while (!shutdown.Wait(100))
        std::cout << "N " << std::flush;
    }

    void Stop() {
      // signal the shutdown PSyncPoint. On the next iteration, the thread's
      // Main() function will exit cleanly.
      shutdown.Signal();
    }

    protected:
      PSyncPoint shutdown;
};


class MyAutoDeleteThread : public PThread
{
  PCLASSINFO(MyAutoDeleteThread, PThread);
  public:
    MyAutoDeleteThread(std::atomic<bool> & exitedFlag)
      : PThread(0, AutoDeleteThread)
      , m_exitedFlag(exitedFlag)
    {
    }

    ~MyAutoDeleteThread()
    {
      m_exitedFlag = true;
    }

    virtual void Main() override
    {
      for (int i = 1; i <= 20; ++i) {
        std::cout << i << ' ' << std::flush;
        Sleep(100);
      }
    }

    protected:
      std::atomic<bool> & m_exitedFlag;
};


void External()
{
  PAssert(PThread::Current() != NULL, "Failed!");
  for (int i = 0; i < 25; ++i) {
    std::cout << "E " << std::flush;
    Sleep(100);
  }
}


void LockAthenB(PMutex & a, PMutex & b)
{
  PTRACE(1, "Locking A then B");
  a.Wait();
  b.Wait();
  PTRACE(1, "Locked A then B");
  PThread::Sleep(60000);
}


void LockBthenA(PMutex & a, PMutex & b)
{
  PTRACE(1, "Locking B then A");
  b.Wait();
  a.Wait();
  PTRACE(1, "Locked B then A");
  PThread::Sleep(60000);
}


/*
 * The main program class
 */
class ThreadTest : public PProcess
{
  PCLASSINFO(ThreadTest, PProcess)
  public:
    virtual void Main() override;
};

PCREATE_PROCESS(ThreadTest);

// The main program
void ThreadTest::Main()
{
  cout << "Thread Test Program" << endl;

  PTRACE_INITIALISE(5, "debugstream");

  PArgList & args = GetArguments();
  args.Parse("d-deadlock. Test deadlock detection");

  if (args.HasOption('d')) {
    cout << "Testing deadlock detection." << endl;
    PTimedMutex a, b;
    PThread * th1 = new PThread2Arg<PMutex &, PMutex &>(a, b, LockAthenB);
    PThread * th2 = new PThread2Arg<PMutex &, PMutex &>(a, b, LockBthenA);
    Sleep(30000);
    delete th1;
    delete th2;
    return;
  }

  // Create the threads
  std::thread external(External);

  auto * thread1 = new MyNoAutoDeleteThread();
  std::atomic<bool> exitedFlag(false);
  MyAutoDeleteThread * thread2 = new MyAutoDeleteThread(exitedFlag);
  // Display about 10 N's
  PThread::Sleep(1000);
  thread2->Resume();
  // Display about 10 more N's and number 1 to 10
  PThread::Sleep(1000);
  thread1->Stop();
  thread1->WaitForTermination();
  delete thread1;

  // Display roughly number 11 to 20
  while (!exitedFlag)
    PThread::Sleep(100);

  std::cout << "\nThread 2 has exited and auto deleted." << std::endl;
  external.join();
}

