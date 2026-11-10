/*
 * synctest - verify PTLib sync primitives against documented semantics.
 */

#include <ptlib.h>
#include <ptlib/pprocess.h>
#include <ptlib/syncthrd.h>
#include <atomic>
#include <iostream>


class SyncTest : public PProcess
{
  PCLASSINFO(SyncTest, PProcess);
public:
  SyncTest()
    : PProcess("PTLib", "synctest", 1, 0, ReleaseCode, 0)
  {
  }

  virtual void Main() override;

private:
  unsigned m_failures = 0;
  unsigned m_checks = 0;

  void Check(bool ok, const char * name)
  {
    ++m_checks;
    if (ok)
      cout << "  PASS  " << name << endl;
    else {
      cout << "  FAIL  " << name << endl;
      ++m_failures;
    }
  }

  void Section(const char * title)
  {
    cout << endl << title << endl;
  }
};


PCREATE_PROCESS(SyncTest)


namespace {

class BoolCondMutex : public PCondMutex
{
  PCLASSINFO(BoolCondMutex, PCondMutex);
public:
  explicit BoolCondMutex(bool ready = false)
    : m_ready(ready)
  {
  }

  virtual PBoolean Condition() override { return m_ready; }

  void SetReady(bool ready)
  {
    Wait();
    m_ready = ready;
    Signal();
  }

private:
  bool m_ready;
};

} // namespace


void SyncTest::Main()
{
  cout << "synctest: PTLib sync semantics" << endl;

  ///////////////////////////////////////////////////////////////////////////
  Section("PTimedMutex / PMutex");
  {
    PMutex mutex;
    Check(mutex.Wait(0), "mutex free: Wait(0) acquires");
    Check(mutex.Try(), "mutex recursive: Try while held");
    mutex.Signal();
    mutex.Signal();

    std::atomic<bool> otherGot{false};
    mutex.Wait();
    class Other : public PThread
    {
      PCLASSINFO(Other, PThread);
    public:
      Other(PMutex & m, std::atomic<bool> & g)
        : PThread(0, NoAutoDeleteThread), mutex(m), got(g) { Resume(); }
      virtual void Main() override { got = mutex.Wait(0); if (got) mutex.Signal(); }
      PMutex & mutex;
      std::atomic<bool> & got;
    };
    Other other(mutex, otherGot);
    other.WaitForTermination(1000);
    Check(!otherGot.load(), "mutex excludes other thread");
    mutex.Signal();

    {
      PWaitAndSignal lock(mutex);
      otherGot = false;
      Other under(mutex, otherGot);
      under.WaitForTermination(500);
      Check(!otherGot.load(), "PWaitAndSignal excludes other thread");
    }
    Check(mutex.Wait(0), "mutex free after PWaitAndSignal");
    mutex.Signal();

    // Timed wait: other thread holds lock
    PSyncPoint holderReady, releaseHold;
    class Holder : public PThread
    {
      PCLASSINFO(Holder, PThread);
    public:
      Holder(PMutex & m, PSyncPoint & ready, PSyncPoint & release)
        : PThread(0, NoAutoDeleteThread), mutex(m), ready(ready), release(release)
      { Resume(); }
      virtual void Main() override
      {
        mutex.Wait();
        ready.Signal();
        release.Wait();
        mutex.Signal();
      }
      PMutex & mutex;
      PSyncPoint & ready;
      PSyncPoint & release;
    };
    Holder holder(mutex, holderReady, releaseHold);
    holderReady.Wait();
    PTime start;
    bool timedOut = !mutex.Wait(50);
    Check(timedOut && (PTime() - start) >= 40, "mutex timed Wait times out");
    releaseHold.Signal();
    holder.WaitForTermination(2000);
    Check(mutex.Wait(100), "mutex timed Wait succeeds when free");
    mutex.Signal();

    // Copy constructs a distinct mutex
    PMutex a;
    a.Wait();
    PMutex b(a);
    Check(b.Wait(0), "mutex copy ctor allocates independent lock");
    b.Signal();
    a.Signal();

    // Assignment is a no-op (still two distinct locks)
    PMutex c, d;
    c.Wait();
    d = c;
    Check(d.Wait(0), "mutex assignment does not share lock");
    d.Signal();
    c.Signal();

    // Contention counter
    std::atomic<unsigned> counter{0};
    std::atomic<bool> ready{false};
    class Worker : public PThread
    {
      PCLASSINFO(Worker, PThread);
    public:
      Worker(PMutex & m, std::atomic<unsigned> & c, std::atomic<bool> & r)
        : PThread(0, NoAutoDeleteThread), mutex(m), counter(c), ready(r) { Resume(); }
      virtual void Main() override
      {
        while (!ready.load())
          Sleep(1);
        for (unsigned i = 0; i < 1000; ++i) {
          PWaitAndSignal lock(mutex);
          unsigned v = counter.load();
          Sleep(0);
          counter.store(v + 1);
        }
      }
      PMutex & mutex;
      std::atomic<unsigned> & counter;
      std::atomic<bool> & ready;
    };
    Worker w1(mutex, counter, ready), w2(mutex, counter, ready);
    ready = true;
    w1.WaitForTermination();
    w2.WaitForTermination();
    Check(counter.load() == 2000, "mutex serialises contended increments");
  }

  ///////////////////////////////////////////////////////////////////////////
  Section("PCriticalSection");
  {
    PCriticalSection cs;
    Check(cs.Try(), "CS Try acquires when free");
    Check(cs.Try(), "CS Try recursive while held");
    std::atomic<bool> otherGot{false};
    class OtherCS : public PThread
    {
      PCLASSINFO(OtherCS, PThread);
    public:
      OtherCS(PCriticalSection & c, std::atomic<bool> & g)
        : PThread(0, NoAutoDeleteThread), cs(c), got(g) { Resume(); }
      virtual void Main() override { got = cs.Try(); if (got) cs.Signal(); }
      PCriticalSection & cs;
      std::atomic<bool> & got;
    };
    OtherCS other(cs, otherGot);
    other.WaitForTermination(1000);
    Check(!otherGot.load(), "CS Try false for other thread while held");
    cs.Leave();
    cs.Leave();

    cs.Enter();
    cs.Enter();
    cs.Leave();
    cs.Leave();
    Check(cs.Wait(0), "CS Wait(0)/Enter/Leave recursion");
    cs.Signal();

    // Copy constructs distinct CS
    PCriticalSection a;
    a.Wait();
    PCriticalSection b(a);
    Check(b.Try(), "CS copy ctor allocates independent lock");
    b.Signal();
    a.Signal();

    // Timed Wait polls Try
    PSyncPoint ready, release;
    class Holder : public PThread
    {
      PCLASSINFO(Holder, PThread);
    public:
      Holder(PCriticalSection & c, PSyncPoint & r, PSyncPoint & rel)
        : PThread(0, NoAutoDeleteThread), cs(c), ready(r), release(rel) { Resume(); }
      virtual void Main() override
      {
        cs.Wait();
        ready.Signal();
        release.Wait();
        cs.Signal();
      }
      PCriticalSection & cs;
      PSyncPoint & ready;
      PSyncPoint & release;
    };
    Holder holder(cs, ready, release);
    ready.Wait();
    Check(!cs.Wait(30), "CS Wait(timeout) fails while held by other");
    release.Signal();
    holder.WaitForTermination(2000);
    Check(cs.Wait(100), "CS Wait(timeout) succeeds when free");
    cs.Signal();
  }

  ///////////////////////////////////////////////////////////////////////////
  Section("PSemaphore");
  {
    PSemaphore empty(0, 5);
    Check(empty.GetInitial() == 0 && empty.GetMaximum() == 5, "semaphore GetInitial/GetMaximum");
    Check(!empty.Wait(0), "semaphore initial 0: Wait(0) fails");

    PSemaphore clamped(10, 3);
    Check(clamped.GetInitial() == 3 && clamped.GetMaximum() == 3,
          "semaphore initial clamped to maximum");
    Check(clamped.Wait(0) && clamped.Wait(0) && clamped.Wait(0), "semaphore drains initial count");
    Check(!clamped.Wait(0), "semaphore empty after drain");

    PSemaphore sem(0, 2);
    sem.Signal();
    sem.Signal();
    sem.Signal(); // third Signal must not exceed max
    Check(sem.Wait(0) && sem.Wait(0), "semaphore Signal respects maximum");
    Check(!sem.Wait(0), "semaphore max-2 cannot Signal a third permit");

    sem.Reset(1, 1);
    Check(sem.GetInitial() == 1 && sem.Wait(0), "semaphore Reset restores count");
    Check(!sem.Wait(0), "semaphore Reset max-1 is empty after one Wait");

    PSemaphore copySrc(2, 4);
    PSemaphore copy(copySrc);
    Check(copy.GetInitial() == 2 && copy.GetMaximum() == 4, "semaphore copy ctor copies initial/max");
    Check(copy.Wait(0) && copy.Wait(0) && !copy.Wait(0), "semaphore copy is independent instance");

    PSemaphore named("synctest-sem", 1, 1);
    Check(named.Wait(0) && !named.Wait(0), "named semaphore ctor is process-local");

    // Blocking Wait + timed Wait
    PSemaphore gate(0, 1);
    std::atomic<bool> waiterDone{false};
    class SemWaiter : public PThread
    {
      PCLASSINFO(SemWaiter, PThread);
    public:
      SemWaiter(PSemaphore & s, std::atomic<bool> & d)
        : PThread(0, NoAutoDeleteThread), sem(s), done(d) { Resume(); }
      virtual void Main() override { sem.Wait(); done = true; }
      PSemaphore & sem;
      std::atomic<bool> & done;
    };
    SemWaiter waiter(gate, waiterDone);
    PThread::Sleep(40);
    Check(!waiterDone.load(), "semaphore Wait blocks until Signal");
    Check(!gate.Wait(20), "semaphore timed Wait times out when empty");
    gate.Signal();
    waiter.WaitForTermination(2000);
    Check(waiterDone.load(), "semaphore Wait unblocked by Signal");

    // Mutex-style 1,1
    PSemaphore mutexSem(1, 1);
    mutexSem.Wait();
    std::atomic<bool> blocked{true};
    class SemOther : public PThread
    {
      PCLASSINFO(SemOther, PThread);
    public:
      SemOther(PSemaphore & s, std::atomic<bool> & b)
        : PThread(0, NoAutoDeleteThread), sem(s), blocked(b) { Resume(); }
      virtual void Main() override { blocked = !sem.Wait(30); if (!blocked) sem.Signal(); }
      PSemaphore & sem;
      std::atomic<bool> & blocked;
    };
    SemOther so(mutexSem, blocked);
    so.WaitForTermination(1000);
    Check(blocked.load(), "semaphore(1,1) excludes other thread");
    mutexSem.Signal();
  }

  ///////////////////////////////////////////////////////////////////////////
  Section("PSyncPoint");
  {
    PSyncPoint sp;
    Check(!sp.Wait(0), "syncpoint initially unsignalled");
    sp.Signal();
    sp.Signal();
    Check(sp.Wait(0), "syncpoint double Signal coalesces to one Wait");
    Check(!sp.Wait(0), "syncpoint consumed after single Wait");

    Check(!sp.Wait(30), "syncpoint timed Wait times out");

    std::atomic<bool> woke{false};
    PSyncPoint start; // unused pattern - waiter blocks on sp
    class SPWaiter : public PThread
    {
      PCLASSINFO(SPWaiter, PThread);
    public:
      SPWaiter(PSyncPoint & s, std::atomic<bool> & w)
        : PThread(0, NoAutoDeleteThread), sp(s), woke(w) { Resume(); }
      virtual void Main() override { sp.Wait(); woke = true; }
      PSyncPoint & sp;
      std::atomic<bool> & woke;
    };
    SPWaiter waiter(sp, woke);
    PThread::Sleep(40);
    Check(!woke.load(), "syncpoint Wait blocks");
    sp.Signal();
    waiter.WaitForTermination(2000);
    Check(woke.load(), "syncpoint Signal releases one waiter");

    // Copy is independent
    PSyncPoint a;
    a.Signal();
    PSyncPoint b(a);
    Check(!b.Wait(0), "syncpoint copy ctor starts unsignalled");
    Check(a.Wait(0), "syncpoint original still signalled after copy");
  }

  ///////////////////////////////////////////////////////////////////////////
  Section("PSyncPointAck");
  {
    PSyncPointAck ack;
    std::atomic<int> stage{0};
    class Worker : public PThread
    {
      PCLASSINFO(Worker, PThread);
    public:
      Worker(PSyncPointAck & a, std::atomic<int> & s)
        : PThread(0, NoAutoDeleteThread), ack(a), stage(s) { Resume(); }
      virtual void Main() override
      {
        ack.Wait();
        stage = 1;
        PThread::Sleep(40);
        stage = 2;
        ack.Acknowledge();
      }
      PSyncPointAck & ack;
      std::atomic<int> & stage;
    };
    Worker worker(ack, stage);
    PThread::Sleep(20);
    Check(stage.load() == 0, "SyncPointAck waiter blocked before Signal");
    ack.Signal(); // blocks until Acknowledge
    Check(stage.load() == 2, "SyncPointAck Signal waits for Acknowledge");
    worker.WaitForTermination(2000);
  }

  ///////////////////////////////////////////////////////////////////////////
  Section("PReadWriteMutex");
  {
    PReadWriteMutex rw;

    // Nested reads
    rw.StartRead();
    rw.StartRead();
    rw.EndRead();
    rw.EndRead();
    Check(true, "RW nested StartRead/EndRead balanced");

    // Nested writes
    rw.StartWrite();
    rw.StartWrite();
    rw.EndWrite();
    rw.EndWrite();
    Check(true, "RW nested StartWrite/EndWrite balanced");

    // Multiple concurrent readers
    rw.StartRead();
    std::atomic<bool> reader2In{false};
    class Reader : public PThread
    {
      PCLASSINFO(Reader, PThread);
    public:
      Reader(PReadWriteMutex & m, std::atomic<bool> & in)
        : PThread(0, NoAutoDeleteThread), rw(m), in(in) { Resume(); }
      virtual void Main() override
      {
        rw.StartRead();
        in = true;
        PThread::Sleep(30);
        rw.EndRead();
      }
      PReadWriteMutex & rw;
      std::atomic<bool> & in;
    };
    Reader r2(rw, reader2In);
    PThread::Sleep(20);
    Check(reader2In.load(), "RW concurrent readers allowed");
    rw.EndRead();
    r2.WaitForTermination(2000);

    // Writer blocked by reader
    rw.StartRead();
    std::atomic<bool> writerIn{false};
    class Writer : public PThread
    {
      PCLASSINFO(Writer, PThread);
    public:
      Writer(PReadWriteMutex & m, std::atomic<bool> & in)
        : PThread(0, NoAutoDeleteThread), rw(m), in(in) { Resume(); }
      virtual void Main() override
      {
        rw.StartWrite();
        in = true;
        rw.EndWrite();
      }
      PReadWriteMutex & rw;
      std::atomic<bool> & in;
    };
    Writer w(rw, writerIn);
    PThread::Sleep(40);
    Check(!writerIn.load(), "RW writer waits while read held");
    rw.EndRead();
    w.WaitForTermination(2000);
    Check(writerIn.load(), "RW writer proceeds after EndRead");

    // Reader blocked by writer
    rw.StartWrite();
    std::atomic<bool> readerIn{false};
    class BlockedReader : public PThread
    {
      PCLASSINFO(BlockedReader, PThread);
    public:
      BlockedReader(PReadWriteMutex & m, std::atomic<bool> & in)
        : PThread(0, NoAutoDeleteThread), rw(m), in(in) { Resume(); }
      virtual void Main() override
      {
        rw.StartRead();
        in = true;
        rw.EndRead();
      }
      PReadWriteMutex & rw;
      std::atomic<bool> & in;
    };
    BlockedReader br(rw, readerIn);
    PThread::Sleep(40);
    Check(!readerIn.load(), "RW reader waits while write held");
    rw.EndWrite();
    br.WaitForTermination(2000);
    Check(readerIn.load(), "RW reader proceeds after EndWrite");

    // Read -> write upgrade (NestMap allows StartWrite while holding read)
    {
      rw.StartRead();
      rw.StartWrite();
      rw.EndWrite();
      rw.EndRead();
      Check(true, "RW StartWrite while holding read does not deadlock");
    }

    // RAII helpers
    {
      PReadWaitAndSignal rlock(rw);
      Check(true, "PReadWaitAndSignal acquires read");
    }
    {
      PWriteWaitAndSignal wlock(rw);
      Check(true, "PWriteWaitAndSignal acquires write");
    }

    PDECLARE_READ_WRITE_MUTEX(declaredRw, SyncTestRW);
    {
      PReadWaitAndSignal lock(declaredRw);
      Check(true, "PDECLARE_READ_WRITE_MUTEX usable");
    }
  }

  ///////////////////////////////////////////////////////////////////////////
  Section("PCondMutex / PIntCondMutex");
  {
    BoolCondMutex bcm(false);
    std::atomic<bool> waited{false};
    class CondWaiter : public PThread
    {
      PCLASSINFO(CondWaiter, PThread);
    public:
      CondWaiter(BoolCondMutex & c, std::atomic<bool> & w)
        : PThread(0, NoAutoDeleteThread), cond(c), waited(w) { Resume(); }
      virtual void Main() override
      {
        cond.WaitCondition();
        waited = true;
        cond.Signal();
      }
      BoolCondMutex & cond;
      std::atomic<bool> & waited;
    };
    CondWaiter cw(bcm, waited);
    PThread::Sleep(40);
    Check(!waited.load(), "PCondMutex WaitCondition blocks while false");
    bcm.SetReady(true);
    cw.WaitForTermination(2000);
    Check(waited.load(), "PCondMutex WaitCondition released when true");

    // PIntCondMutex GE
    {
      PIntCondMutex cond(0, 2, PIntCondMutex::GE);
      Check((int)cond == 0, "PIntCondMutex initial value");
      std::atomic<bool> done{false};
      class W : public PThread
      {
        PCLASSINFO(W, PThread);
      public:
        W(PIntCondMutex & c, std::atomic<bool> & d)
          : PThread(0, NoAutoDeleteThread), cond(c), done(d) { Resume(); }
        virtual void Main() override { cond.WaitCondition(); done = true; cond.Signal(); }
        PIntCondMutex & cond;
        std::atomic<bool> & done;
      };
      W w(cond, done);
      PThread::Sleep(30);
      Check(!done.load(), "PIntCondMutex GE blocks at 0 < 2");
      ++cond;
      PThread::Sleep(30);
      Check(!done.load(), "PIntCondMutex GE still blocks at 1 < 2");
      cond += 1;
      w.WaitForTermination(2000);
      Check(done.load() && (int)cond == 2, "PIntCondMutex GE releases at 2");
    }

    // Operators and EQ
    {
      PIntCondMutex cond(5, 3, PIntCondMutex::EQ);
      cond = 3;
      Check((int)cond == 3 && cond.Condition(), "PIntCondMutex operator= and EQ Condition");
      --cond;
      Check((int)cond == 2 && !cond.Condition(), "PIntCondMutex operator--");
      cond -= -1;
      Check((int)cond == 3 && cond.Condition(), "PIntCondMutex operator-=");
    }

    // LT / GT smoke via Condition() under lock
    {
      PIntCondMutex lt(1, 5, PIntCondMutex::LT);
      lt.Wait();
      Check(lt.Condition(), "PIntCondMutex LT true when value < target");
      lt.Signal();
      PIntCondMutex gt(9, 5, PIntCondMutex::GT);
      gt.Wait();
      Check(gt.Condition(), "PIntCondMutex GT true when value > target");
      gt.Signal();
      PIntCondMutex le(5, 5, PIntCondMutex::LE);
      le.Wait();
      Check(le.Condition(), "PIntCondMutex LE true when value == target");
      le.Signal();
    }
  }

  ///////////////////////////////////////////////////////////////////////////
  Section("PInstrumentedMutex");
  {
#if PTRACING
    PDECLARE_INSTRUMENTED_MUTEX(imtx, SyncTestImtx, 5000, 5000);
    Check(imtx.Wait(0), "PInstrumentedMutex Wait(0)");
    Check(imtx.Try(), "PInstrumentedMutex recursive Try");
    imtx.Signal();
    imtx.Signal();
    {
      PWaitAndSignal lock(imtx);
      Check(true, "PInstrumentedMutex with PWaitAndSignal");
    }
#else
    PDECLARE_MUTEX(imtx, SyncTestImtx);
    imtx.Wait();
    imtx.Signal();
    Check(true, "PMutex stand-in (PTRACING off)");
#endif
  }

  ///////////////////////////////////////////////////////////////////////////
  Section("PSyncQueue (composition)");
  {
    PSyncQueue<int> q;
    Check(q.IsOpen() && q.empty(), "PSyncQueue starts open and empty");
    Check(q.Enqueue(7), "PSyncQueue Enqueue");
    int value = 0;
    Check(q.Dequeue(value, 100) && value == 7, "PSyncQueue Dequeue");
    Check(!q.Dequeue(value, 30), "PSyncQueue Dequeue times out when empty");

    std::atomic<bool> got{false};
    class Consumer : public PThread
    {
      PCLASSINFO(Consumer, PThread);
    public:
      Consumer(PSyncQueue<int> & queue, std::atomic<bool> & g, int & out)
        : PThread(0, NoAutoDeleteThread), q(queue), got(g), out(out) { Resume(); }
      virtual void Main() override
      {
        int v = 0;
        if (q.Dequeue(v, 2000)) {
          out = v;
          got = true;
        }
      }
      PSyncQueue<int> & q;
      std::atomic<bool> & got;
      int & out;
    };
    int out = 0;
    Consumer c(q, got, out);
    PThread::Sleep(30);
    Check(q.Enqueue(42), "PSyncQueue Enqueue wakes consumer");
    c.WaitForTermination(2000);
    Check(got.load() && out == 42, "PSyncQueue blocked Dequeue receives item");

    q.Close(false);
    Check(!q.IsOpen() && !q.Enqueue(1), "PSyncQueue Close rejects Enqueue");
  }

  cout << endl
       << (m_failures == 0 ? "ALL PASSED" : "FAILURES")
       << " (" << m_failures << " failed / " << m_checks << " checks)" << endl;
  SetTerminationValue(m_failures == 0 ? 0 : 1);
}
