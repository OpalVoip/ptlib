/*
 * assert.cxx
 *
 * Assert function implementation.
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


#include <ptlib.h>

#include <ctype.h>
#include <signal.h>
#include <fstream>
#include <stdexcept>
#include <ptlib/pprocess.h>


#if P_HAS_BACKTRACE && PTRACING

  #define DEBUG_CERR(arg) // std::cerr << arg << std::endl

  #include <chrono>
  #include <execinfo.h>
  #include <dlfcn.h>
  #if P_HAS_DEMANGLE
    #include <cxxabi.h>
  #endif

  #if PTRACING
    #define InternalMaxStackWalk PTrace::MaxStackWalk
  #else
    #define InternalMaxStackWalk 20
  #endif // PTRACING


#ifndef P_MACOSX
  static bool fgetstr(std::string & str, FILE * fp)
  {
    for (;;) {
      struct pollfd pfd;
      pfd.fd = fileno(fp);
      pfd.events = POLLIN;
      pfd.revents = 0;

      int pollStatus = ::poll(&pfd, 1, 10000);

      if (pollStatus < 0) {
        if (errno == EINTR)
          continue;
        str = strerror(errno);
        return false;
      }

      if (pollStatus == 0) {
        str = "Timed out";
        return false;
      }

      char c;
      if (read(pfd.fd, &c, 1) <= 0) {
        str += " - fgetc - ";
        str += strerror(errno);
        return false;
      }

      if (c == '\n')
        return true;

      str += (char)c;
    }
  }
#endif // P_MACOSX


  typedef vector<void *> StackAddresses;

  static void InternalWalkStack(ostream & strm, unsigned framesToSkip, const StackAddresses addresses, bool noSymbols)
  {
    DEBUG_CERR("InternalWalkStack: count=" << addresses.size() << ", framesToSkip=" << framesToSkip);
    if (addresses.size() <= framesToSkip) {
      strm << "\tStack back trace empty, possibly corrupt.";
      return;
    }

    if (noSymbols) {
      for (unsigned i = framesToSkip; i < addresses.size(); ++i) {
        if (i > framesToSkip)
          strm << ' ';
        strm << addresses[i];
      }
      return;
    }

    std::vector<std::string> lines(addresses.size());

#ifndef P_MACOSX
    {
      FILE * pipe;
      {
        std::stringstream cmd;
        PString exeFile = PProcess::Current().GetFile();
        cmd << "addr2line '--exe=" << exeFile << '\'';

        uint64_t textSegmentOffset = 0;
        if (::getenv("PTLIB_BACKTRACE_MAPPING")) {
          std::ifstream maps("/proc/self/maps");
          while (maps.good()) {
            std::string line;
            std::getline(maps, line);
            DEBUG_CERR("/proc/self/maps: \"" << line << '"');
            size_t pos = line.find("r-xp");
            if (pos != std::string::npos && line.find(exeFile.c_str()) != std::string::npos) {
              textSegmentOffset = strtoull(line.c_str(), NULL, 16);
              DEBUG_CERR("textSegmentOffset=0x" << hex << textSegmentOffset << dec);
              if (textSegmentOffset != 0)
                cmd << " --section=.text";
              break;
            }
          }
        }

        for (unsigned i = framesToSkip; i < addresses.size(); ++i)
          cmd << ' ' << (const void *)(((const char *)addresses[i]) - textSegmentOffset);
        std::string cmdstr = cmd.str();
        DEBUG_CERR("InternalWalkStack: before " << cmdstr);
        pipe = popen(cmdstr.c_str(), "r");
      }

      DEBUG_CERR("InternalWalkStack: addr2line pipe=" << pipe);
      if (pipe != NULL) {
        for (unsigned i = framesToSkip; i < addresses.size(); ++i) {
          std::string & line = lines[i];
          if (!fgetstr(line, pipe)) {
            DEBUG_CERR("InternalWalkStack: fgetstr error: " << line);
            if (!line.empty())
              strm << "\n\tStack trace error with addr2line: " << line;
            break;
          }
          DEBUG_CERR("InternalWalkStack: line=\"" << line << '"');
          if (line == "??:0" || line == "??:?")
            line.clear();
        }
        DEBUG_CERR("InternalWalkStack: closing pipe=" << pipe);
        pclose(pipe);
      }
    }
#endif // P_MACOSX


    DEBUG_CERR("InternalWalkStack: before backtrace_symbols");
    char ** symbols = backtrace_symbols(addresses.data(), addresses.size());
    DEBUG_CERR("InternalWalkStack: after backtrace_symbols");
    for (unsigned i = framesToSkip; i < addresses.size(); ++i) {
      strm << "\n\t";

      if (symbols[i] == NULL || symbols[i][0] == '\0') {
        strm << addresses[i] << ' ' << lines[i];
        continue;
      }

      #if P_HAS_DEMANGLE
        char * mangled = strchr(symbols[i], '(');
        if (mangled != NULL) {
          ++mangled;
          char * offset = mangled + strcspn(mangled, "+)");
          if (offset != mangled) {
            char separator = *offset;
            *offset++ = '\0';

    	    int status = -1;
	    char * demangled = abi::__cxa_demangle(mangled, NULL, NULL, &status);
            if (status == 0) {
              *mangled = '\0';
              strm << symbols[i] << demangled << separator << offset << ' ' << lines[i];
              runtime_free(demangled);
              continue;
            }
            if (demangled != NULL)
              runtime_free(demangled);
            *--offset = separator;
          }
        }
      #endif // P_HAS_DEMANGLE

      strm << symbols[i] << ' ' << lines[i];
    }
    runtime_free(symbols);

    DEBUG_CERR("InternalWalkStack: done");
  }


    struct PWalkStackInfo
    {
      enum { OtherThreadSkip = 6 };
      std::timed_mutex m_mainMutex;
      PThreadIdentifier m_threadId;
      PUniqueThreadIdentifier m_uniqueId;
      StackAddresses    m_addresses;
      int               m_addressCount;
      std::mutex        m_condMutex;
      std::condition_variable m_condVar;
      PTime             m_signalSentTime;

      PWalkStackInfo()
        : m_uniqueId(0)
        , m_addressCount(-1)
        , m_signalSentTime(0)
      {
      }

      void WalkOther(ostream & strm, PThreadIdentifier tid, PUniqueThreadIdentifier uid, bool noSymbols)
      {
        DEBUG_CERR("WalkOther: " << PThread::Identifiers(tid, uid));

        if (tid == PNullThreadIdentifier) {
          strm << "\n\tStack trace WalkOther with zero thread ID.";
          return;
        }

        auto when = std::chrono::steady_clock::now() + std::chrono::seconds(2);

        if (!m_mainMutex.try_lock_until(when)) {
          strm << "\n\tStack trace system is too busy to WalkOther";
          DEBUG_CERR("WalkOther: mutex timeout");
          return;
        }

        m_threadId = tid;
        m_uniqueId = uid;
        m_addressCount = -1;
        m_addresses.resize(InternalMaxStackWalk+OtherThreadSkip);
        m_signalSentTime.SetCurrentTime();
        if (!PThread::PlatformKill(tid, uid, PProcess::WalkStackSignal)) {
          strm << "\n\tThread " << PThread::Identifiers(tid, uid) << " is no longer running";
          m_mainMutex.unlock();
          return;
        }

        try {
          std::unique_lock<std::mutex> lock(m_condMutex);
          if (!m_condVar.wait_until(lock, when, [this]() { return m_addressCount >= 0; }))
            strm << "\n\tNo response getting stack trace for " << PThread::Identifiers(tid, uid);
          else
            InternalWalkStack(strm, OtherThreadSkip, m_addresses, noSymbols);
        }
        catch (...) {
          strm << "\n\tError getting stack trace for " << PThread::Identifiers(tid, uid);
        }

        m_threadId = PNullThreadIdentifier;
        m_uniqueId = 0;

        m_mainMutex.unlock();
        DEBUG_CERR("WalkOther: done");
      }

      void OthersWalk()
      {
        PThreadIdentifier tid = PThread::GetCurrentThreadId();
        PUniqueThreadIdentifier uid = PThread::GetCurrentUniqueIdentifier();

        if (!m_signalSentTime.IsValid()) {
          PTRACE(0, "StackWalk", "No stack walk in operation, unexpected signal " << PProcess::WalkStackSignal);
          return;
        }

        if (m_threadId == PNullThreadIdentifier) {
          PTRACE(0, "StackWalk", "Thread took too long (" << m_signalSentTime.GetElapsed() << "s) to respond to signal.");
          return;
        }

        if (m_threadId != tid || (m_uniqueId != 0 && m_uniqueId != uid)) {
          PTRACE(0, "StackWalk", "Signal received on " << PThread::Identifiers(tid, uid) <<
                                  " but expected " << PThread::Identifiers(m_threadId, m_uniqueId));
          return;
        }

        int addressCount = backtrace(m_addresses.data(), m_addresses.size());
        m_addresses.resize(addressCount < 0 ? 0 : addressCount);

        m_condMutex.lock();
        m_addressCount = m_addresses.size();
        m_condVar.notify_one();
        m_condMutex.unlock();
      }
    };

    static PWalkStackInfo s_otherThreadStack;


    void PPlatformWalkStack(ostream & strm, PThreadIdentifier id, PUniqueThreadIdentifier uid, unsigned framesToSkip, bool noSymbols)
    {
      if (!PProcess::IsInitialised())
        return;

      if (id != PNullThreadIdentifier && id != PThread::GetCurrentThreadId())
        s_otherThreadStack.WalkOther(strm, id, uid, noSymbols);
      else {
        DEBUG_CERR("PPlatformWalkStack: id=0x" << hex << id << dec);
        // Allow for some bizarre optimisation when called from PTrace::WalkStack()
        if (framesToSkip == 1)
          framesToSkip = 0;
        const size_t maxStackWalk = InternalMaxStackWalk + framesToSkip;
        StackAddresses addresses(maxStackWalk);
        addresses.resize(backtrace(addresses.data(), maxStackWalk));
        InternalWalkStack(strm, framesToSkip, addresses, noSymbols);
      }
    }

    void PProcess::InternalWalkStackSignaled()
    {
      if (IsInitialised())
        s_otherThreadStack.OthersWalk();
    }

#else // P_HAS_BACKTRACE && PTRACING

  void PPlatformWalkStack(ostream & strm, PThreadIdentifier id, PUniqueThreadIdentifier uid, unsigned skip, bool noSymbols)
  {
  }

#endif // P_HAS_BACKTRACE && PTRACING


#if PTRACING
  #define TRACE_MESSAGE() \
    PTrace::Begin(0, location.m_file, location.m_line, NULL, "PAssert") << msg << PTrace::End

  #define OUTPUT_MESSAGE() \
    TRACE_MESSAGE(); \
    if (PTrace::GetStream() != &PError) \
      PError << msg << endl
#else
  #define TRACE_MESSAGE()
  #define OUTPUT_MESSAGE() PError << msg << endl
#endif


#if defined(P_ANDROID)

  #include <android/log.h>

  #undef  OUTPUT_MESSAGE
  #define OUTPUT_MESSAGE() \
      TRACE_MESSAGE(); \
      __android_log_assert("", PProcess::Current().GetName(), "%s", msg.c_str());

  static const char ActionMessage[] = "Ignoring";

  static bool AssertAction(int, const char *)
  {
    return false;
  }

#else

  static const char ActionMessage[] = "<A>bort, <C>ore dump, "
  #ifdef _DEBUG
                                      "<D>ebug, "
  #endif
                                      "<I>gnore";

  static bool AssertAction(int c, const char * msg)
  {
    switch (c) {
      case 'a' :
      case 'A' :
        PError << "\nAborting.\n";
        abort();
        return true;

  #ifdef _DEBUG
      case 'd' :
      case 'D' :
        {
          PString cmd = ::getenv("PTLIB_ASSERT_DEBUGGER");
          if (cmd.IsEmpty())
            cmd = "gdb";
          cmd &= PProcess::Current().GetFile();
          cmd.sprintf(" %d", getpid());
          PError << "\nStarting debugger \"" << cmd << '"' << endl;
          if (system((const char *)cmd) < 0)
            PError << "Could not execute debugger!" << endl;
        }
        return false;
  #endif

      case 'c' :
      case 'C' :
        PError << "\nDumping core.\n";
        abort();
        return false;

      case 'i' :
      case 'I' :
      case EOF :
        PError << "\nIgnoring.\n";
        return false;

      default :
        return true;
    }
  }

#endif


void PPlatformAssertFunc(const PDebugLocation & location, const char * msg, char defaultAction)
{
  OUTPUT_MESSAGE();

  // DO default action is specified
  if (defaultAction != '\0' && !AssertAction(defaultAction, msg))
    return;

  // Check for if stdin is not a TTY and just ignore the assert if so.
  if (isatty(STDIN_FILENO) != 1) {
    AssertAction('i', msg);
    return;
  }

  // Keep asking for action till we get a legal one
  do {
    PError << '\n' << ActionMessage << "? " << flush;
  } while (AssertAction(getchar(), msg));
}


// End Of File ///////////////////////////////////////////////////////////////
