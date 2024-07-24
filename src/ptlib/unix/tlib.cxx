/*
 * tlib.cxx
 *
 * Miscelaneous class implementation
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

#define _OSUTIL_CXX

#define SIGNALS_DEBUG(fmt,...) //fprintf(stderr, fmt, ##__VA_ARGS__)


#pragma implementation "args.h"
#pragma implementation "pprocess.h"
#pragma implementation "thread.h"
#pragma implementation "semaphor.h"
#pragma implementation "mutex.h"
#pragma implementation "critsec.h"
#pragma implementation "psync.h"
#pragma implementation "syncpoint.h"
#pragma implementation "syncthrd.h"

#include "ptlib.h"
#include <ptlib/pprocess.h>
#include <ptclib/pxml.h>

#include <sys/time.h>
#include <pwd.h>
#include <grp.h>
#include <sys/wait.h>
#include <errno.h>

#if defined(P_LINUX)
#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <malloc.h>
#include <fstream>
#include <sys/resource.h>
#endif

#if defined(P_LINUX) || defined(P_FREEBSD) || defined(P_OPENBSD) || defined(P_NETBSD) || defined(P_MACOSX) || defined(P_IOS) || defined(P_ANDROID)
#include <sys/utsname.h>
#define  HAS_UNAME
#endif

#include "uerror.h"


PString PProcess::GetOSClass()
{
  return PString("Unix");
}

PString PProcess::GetOSName()
{
#if defined(HAS_UNAME)
  struct utsname info;
  uname(&info);
  return PString(info.sysname);
#else
#warning No GetOSName specified
  return PString("Unknown");
#endif
}

PString PProcess::GetOSHardware()
{
#if defined(HAS_UNAME)
  struct utsname info;
  uname(&info);
  return PString(info.machine);
#else
#warning No GetOSHardware specified
  return PString("unknown");
#endif
}

PString PProcess::GetOSVersion()
{
#if defined(HAS_UNAME)
  struct utsname info;
  uname(&info);
  return PString(info.release);
#else
#warning No GetOSVersion specified
  return PString("?.?");
#endif
}

bool PProcess::IsOSVersion(unsigned major, unsigned minor, unsigned build)
{
#if defined(HAS_UNAME)
  struct utsname info;
  uname(&info);
  unsigned maj, min, bld;
  sscanf(info.release, "%u.%u.%u", &maj, &min, &bld);
  if (maj < major)
    return false;
  if (maj > major)
    return true;

  if (min < minor)
    return false;
  if (min > minor)
    return true;

  return bld >= build;
#else
  return true;
#endif
}


PDirectory PProcess::GetOSConfigDir()
{
  return "/etc";
}


#if defined(P_GETPWUID_R5)
  #define GETPWUID(pw) \
    struct passwd pwd; \
    char buffer[1024]; \
    if (::getpwuid_r(geteuid(), &pwd, buffer, 1024, &pw) < 0) \
      pw = NULL
#elif defined(P_GETPWUID_R4)
  #define GETPWUID(pw) \
    struct passwd pwd; \
    char buffer[1024]; \
    pw = ::getpwuid_r(geteuid(), &pwd, buffer, 1024);
#else
  #define GETPWUID(pw) \
    pw = ::getpwuid(geteuid());
#endif


PString PProcess::GetUserName() const
{
#ifdef GETPWUID

  struct passwd * pw;
  GETPWUID(pw);

  if (pw != NULL && pw->pw_name != NULL)
    return pw->pw_name;

  const char * user;
  if ((user = getenv("USER")) != NULL)
    return user;

#endif // GETPWUID

  return GetName();
}


bool PProcess::SetUserName(const PString & username, bool permanent)
{
  if (username.IsEmpty())
    return seteuid(getuid()) != -1;

  int uid = -1;

  if (username[0] == '#') {
    PString s = username.Mid(1);
    if (s.FindSpan("1234567890") == P_MAX_INDEX)
      uid = s.AsInteger();
  }
  else {
    struct passwd * pw;
#if defined(P_GETPWNAM_R5)
    struct passwd pwd;
    char buffer[1024];
    if (::getpwnam_r(username, &pwd, buffer, 1024, &pw) < 0)
      pw = NULL;
#elif defined(P_GETPWNAM_R4)
    struct passwd pwd;
    char buffer[1024];
    pw = ::getpwnam_r(username, &pwd, buffer, 1024);
#else
    pw = ::getpwnam(username);
#endif

    if (pw != NULL)
      uid = pw->pw_uid;
    else {
      if (username.FindSpan("1234567890") == P_MAX_INDEX)
       uid = username.AsInteger();
    }
  }

  if (uid < 0)
    return false;

  if (permanent)
    return setuid(uid) != -1;
    
  return seteuid(uid) != -1;
}


PDirectory PProcess::GetHomeDirectory() const
{
#ifdef GETPWUID

  const char * home = getenv("HOME");
  if (home != NULL)
    return home;

  struct passwd * pw;
  GETPWUID(pw);

  if (pw != NULL && pw->pw_dir != NULL)
    return pw->pw_dir;

#endif // GETPWUID

  return ".";
}


///////////////////////////////////////////////////////////////////////////////
//
// PProcess
//
// Return the effective group name of the process, eg "wheel" etc.

PString PProcess::GetGroupName() const

{
#if !defined(P_THREAD_SAFE_LIBC)
  struct group grp;
  char buffer[1024];
  struct group * gr = NULL;
#if defined(P_GETGRGID_R5)
  ::getgrgid_r(getegid(), &grp, buffer, 1024, &gr);
#elif defined(P_GETGRGID_R4)
  gr = ::getgrgid_r(getegid(), &grp, buffer, 1024);
#else
  #error "Cannot identify getgrgid_r"
#endif
#else
  struct group * gr = ::getgrgid(getegid());
#endif

  char * ptr;
  if (gr != NULL && gr->gr_name != NULL)
    return PString(gr->gr_name);
  else if ((ptr = getenv("GROUP")) != NULL)
    return PString(ptr);
  else
    return PString("group");
}


bool PProcess::SetGroupName(const PString & groupname, bool permanent)
{
  if (groupname.IsEmpty())
    return setegid(getgid()) != -1;

  int gid = -1;

  if (groupname[0] == '#') {
    PString s = groupname.Mid(1);
    if (s.FindSpan("1234567890") == P_MAX_INDEX)
      gid = s.AsInteger();
  }
  else {
#if !defined(P_THREAD_SAFE_LIBC)
    struct group grp;
    char buffer[1024];
    struct group * gr = NULL;
#if defined (P_GETGRNAM_R5)
    ::getgrnam_r(groupname, &grp, buffer, 1024, &gr);
#elif defined(P_GETGRNAM_R4)
    gr = ::getgrnam_r(groupname, &grp, buffer, 1024);
#else
    #error "Cannot identify getgrnam_r"
#endif
#else
    struct group * gr = ::getgrnam(groupname);
#endif

    if (gr != NULL && gr->gr_name != NULL)
      gid = gr->gr_gid;
    else {
      if (groupname.FindSpan("1234567890") == P_MAX_INDEX)
       gid = groupname.AsInteger();
    }
  }

  if (gid < 0)
    return false;

  if (permanent)
    return setgid(gid) != -1;
    
  return setegid(gid) != -1;
}


void PProcess::HostSystemURLHandlerInfo::SetIcon(const PString & _icon)
{
}

PString PProcess::HostSystemURLHandlerInfo::GetIcon() const 
{
  return PString();
}

void PProcess::HostSystemURLHandlerInfo::SetCommand(const PString & key, const PString & _cmd)
{
}

PString PProcess::HostSystemURLHandlerInfo::GetCommand(const PString & key) const
{
  return PString();
}

bool PProcess::HostSystemURLHandlerInfo::GetFromSystem()
{
  return false;
}

bool PProcess::HostSystemURLHandlerInfo::CheckIfRegistered()
{
  return false;
}

bool PProcess::HostSystemURLHandlerInfo::Register()
{
  return false;
}



static void StaticSignalHandler(int signal, siginfo_t * info, void *)
{
  PProcess::Current().AsynchronousRunTimeSignal(signal, info ? info->si_pid : 0);
}


PRunTimeSignalHandler PProcess::PlatformSetRunTimeSignalHandler(int signal)
{
  struct sigaction action, previous;
  memset(&action, 0, sizeof(action));
  action.sa_sigaction = StaticSignalHandler;
  action.sa_flags = SA_SIGINFO|SA_RESTART;
  PAssertOS(sigaction(signal, &action, &previous) == 0);
  return previous.sa_sigaction;
}


void PProcess::PlatformResetRunTimeSignalHandler(int signal, PRunTimeSignalHandler previous)
{
  struct sigaction action;
  memset(&action, 0, sizeof(action));
  action.sa_sigaction = previous;
  PAssertOS(sigaction(signal, &action, NULL) == 0);
}


#define SIG_NAME(s) { s, #s }
POrdinalToString::Initialiser const PProcess::InternalSigNames[] = {
  SIG_NAME(SIGHUP   ),      /* Hangup (POSIX).  */
  SIG_NAME(SIGINT   ),      /* Interrupt (ANSI).  */
  SIG_NAME(SIGQUIT  ),      /* Quit (POSIX).  */
  SIG_NAME(SIGILL   ),      /* Illegal instruction (ANSI).  */
  SIG_NAME(SIGTRAP  ),      /* Trace trap (POSIX).  */
  SIG_NAME(SIGABRT  ),      /* Abort (ANSI).  */
  SIG_NAME(SIGIOT   ),      /* IOT trap (4.2 BSD).  */
  SIG_NAME(SIGBUS   ),      /* BUS error (4.2 BSD).  */
  SIG_NAME(SIGFPE   ),      /* Floating-point exception (ANSI).  */
  SIG_NAME(SIGKILL  ),      /* Kill, unblockable (POSIX).  */
  SIG_NAME(SIGUSR1  ),      /* User-defined signal 1 (POSIX).  */
  SIG_NAME(SIGSEGV  ),      /* Segmentation violation (ANSI).  */
  SIG_NAME(SIGUSR2  ),      /* User-defined signal 2 (POSIX).  */
  SIG_NAME(SIGPIPE  ),      /* Broken pipe (POSIX).  */
  SIG_NAME(SIGALRM  ),      /* Alarm clock (POSIX).  */
  SIG_NAME(SIGTERM  ),      /* Termination (ANSI).  */
#ifdef SIGSTKFLT
  SIG_NAME(SIGSTKFLT),      /* Stack fault.  */
#endif
  SIG_NAME(SIGCHLD  ),      /* Child status has changed (POSIX).  */
  SIG_NAME(SIGCONT  ),      /* Continue (POSIX).  */
  SIG_NAME(SIGSTOP  ),      /* Stop, unblockable (POSIX).  */
  SIG_NAME(SIGTSTP  ),      /* Keyboard stop (POSIX).  */
  SIG_NAME(SIGTTIN  ),      /* Background read from tty (POSIX).  */
  SIG_NAME(SIGTTOU  ),      /* Background write to tty (POSIX).  */
  SIG_NAME(SIGURG   ),      /* Urgent condition on socket (4.2 BSD).  */
  SIG_NAME(SIGXCPU  ),      /* CPU limit exceeded (4.2 BSD).  */
  SIG_NAME(SIGXFSZ  ),      /* File size limit exceeded (4.2 BSD).  */
  SIG_NAME(SIGVTALRM),      /* Virtual alarm clock (4.2 BSD).  */
  SIG_NAME(SIGPROF  ),      /* Profiling alarm clock (4.2 BSD).  */
  SIG_NAME(SIGWINCH ),      /* Window size change (4.3 BSD, Sun).  */
  SIG_NAME(SIGIO    ),      /* I/O now possible (4.2 BSD).  */
#ifdef SIGPWR
  SIG_NAME(SIGPWR   ),      /* Power failure restart (System V).  */
#endif
  SIG_NAME(SIGSYS   ),      /* Bad system call.  */
  { }
};


void PProcess::PlatformConstruct()
{
  // initialise the timezone information
  tzset();

  // get the file descriptor limit
  struct rlimit rl;
  PAssertOS(getrlimit(RLIMIT_NOFILE, &rl) == 0);
  m_maxHandles = rl.rlim_cur;
  PTRACE(4, "PTLib\tMaximum per-process file handles is " << m_maxHandles);
}


void PProcess::PlatformDestruct()
{
}


bool PProcess::SetMaxHandles(int newMax)
{
  // get the current process limit
  struct rlimit rl;
  PAssertOS(getrlimit(RLIMIT_NOFILE, &rl) == 0);

  // set the new current limit
  rl.rlim_cur = newMax;
  if (setrlimit(RLIMIT_NOFILE, &rl) == 0) {
    PAssertOS(getrlimit(RLIMIT_NOFILE, &rl) == 0);
    m_maxHandles = rl.rlim_cur;
    if (m_maxHandles == newMax) {
      PTRACE(2, "PTLib\tNew maximum per-process file handles set to " << m_maxHandles);
      return true;
    }
  }

  PTRACE(1, "PTLib\tCannot set per-process file handle limit to "
         << newMax << " (is " << m_maxHandles << ") - check permissions");
  return false;
}


#if defined(P_LINUX)

static inline PMicroSeconds jiffies_to_usecs(const uint64_t jiffies)
{
  static long sysconf_HZ = sysconf(_SC_CLK_TCK);
  return (jiffies * 1000000) / sysconf_HZ;
}


static bool InternalGetTimes(const char * filename, PThread::Times & times)
{
  // Do not use any PTLib functions in here as they could do a PTRACE, and thus deadlock

  /* From the man page on the "stat" file
  Status information about the process. This is used by ps(1). It is defined in /usr/src/linux/fs/proc/array.c.
  The fields, in order, with their proper scanf(3) format specifiers, are:
  pid         %d   The process ID.
  comm        %s   The filename of the executable, in parentheses. This is visible
  whether or not the executable is swapped out.
  state       %c   One character from the string "RSDZTW" where R is running, S is
  sleeping in an interruptible wait, D is waiting in uninterruptible
  disk sleep, Z is zombie, T is traced or stopped (on a signal), and
  W is paging.
  ppid        %d   The PID of the parent.
  pgrp        %d   The process group ID of the process.
  session     %d   The session ID of the process.
  tty_nr      %d   The tty the process uses.
  tpgid       %d   The process group ID of the process which currently owns the tty
  that the process is connected to.
  flags       %lu  The kernel flags word of the process. For bit meanings, see the
  PF_* defines in <linux/sched.h>. Details depend on the kernel
  version.
  minflt      %lu  The number of minor faults the process has made which have not
  required loading a memory page from disk.
  cminflt     %lu  The number of minor faults that the process's waited-for children
  have made.
  majflt      %lu  The number of major faults the process has made which have required
  loading a memory page from disk.
  cmajflt     %lu  The number of major faults that the process's waited-for children
  have made.
  utime       %lu  The number of jiffies that this process has been scheduled in user
  mode.
  stime       %lu  The number of jiffies that this process has been scheduled in kernel
  mode.
  cutime      %ld  The number of jiffies that this process's waited-for children have
  been scheduled in user mode. (See also times(2).)
  cstime      %ld  The number of jiffies that this process's waited-for children have
  been scheduled in kernel mode.
  priority    %ld  The standard nice value, plus fifteen. The value is never negative
  in the kernel.
  nice        %ld  The nice value ranges from 19 (nicest) to -19 (not nice to others).
  num_threads %ld  Number of threads.
  itrealvalue %ld  The time in jiffies before the next SIGALRM is sent to the process
  due to an interval timer.
  starttime   %lu  The time in jiffies the process started after system boot.
  vsize       %lu  Virtual memory size in bytes.
  rss         %ld  Resident Set Size: number of pages the process has in real memory,
  minus 3 for administrative purposes. This is just the pages which
  count towards text, data, or stack space. This does not include
  pages which have not been demand-loaded in, or which are swapped out.
  rlim        %lu  Current limit in bytes on the rss of the process
  (usually 4294967295 on i386).
  startcode   %lu  The address above which program text can run.
  endcode     %lu  The address below which program text can run.
  startstack  %lu  The address of the start of the stack.
  kstkesp     %lu  The current value of esp (stack pointer), as found in the kernel
  stack page for the process.
  kstkeip     %lu  The current EIP (instruction pointer).
  signal      %lu  The bitmap of pending signals.
  blocked     %lu  The bitmap of blocked signals.
  sigignore   %lu  The bitmap of ignored signals.
  sigcatch    %lu  The bitmap of caught signals.
  wchan       %lu  This is the "channel" in which the process is waiting. It is the
  address of a system call, and can be looked up in a namelist if you
  need a textual name. (If you have an up-to-date /etc/psdatabase, then
  try ps -l to see the WCHAN field in action.)
  nswap       %lu  Number of pages swapped (not maintained).
  cnswap      %lu  Cumulative nswap for child processes (not maintained).
  exit_signal %d   Signal to be sent to parent when we die.
  processor   %d   CPU number last executed on.
  rt_priority %lu  (since kernel 2.5.19) Real-time scheduling priority (see sched_setscheduler(2)).
  policy      %lu  (since kernel 2.5.19) Scheduling policy (see sched_setscheduler(2)).
  delayacct_blkio_ticks %llu (since Linux 2.6.18) Aggregated block I/O delays, measured in
  clock ticks (centiseconds).
  */

  for (int retry = 0; retry < 3; ++retry) {
    std::ifstream statfile(filename);

    char line[1000];
    statfile.getline(line, sizeof(line));
    if (!statfile.good())
      continue;

    // 17698 (MyApp:1234) R 1 17033 8586 34833 17467 4202560 7
    // 0 0 0 0 0 0 0 -100 0 16
    // 0 55172504 258756608 6741 4294967295 134512640 137352760 3217892976 8185700 15991824
    // 0 0 4 201349635 0 0 0 -1 7 99
    // 2 0
    //
    char * str = line;
    while (*str != '\0' && *str++ != ')'); //  get past pid and thread name
    while (*str != '\0' && !isdigit(*str++)); // get past state
    while (*str != '\0' && *str++ != ' '); // ppid
    while (*str != '\0' && *str++ != ' '); // pgrp
    while (*str != '\0' && *str++ != ' '); // session
    while (*str != '\0' && *str++ != ' '); // tty_nr
    while (*str != '\0' && *str++ != ' '); // tpgid
    while (*str != '\0' && *str++ != ' '); // flags
    while (*str != '\0' && *str++ != ' '); // minflt
    while (*str != '\0' && *str++ != ' '); // cminflt
    while (*str != '\0' && *str++ != ' '); // majflt
    while (*str != '\0' && *str++ != ' '); // cmajflt
    uint64_t utime = strtoull(str, &str, 10);
    uint64_t stime = strtoull(str, &str, 10);
    if (*str == '\0')
      continue;

    times.m_kernel = jiffies_to_usecs(stime);
    times.m_user = jiffies_to_usecs(utime);
    return true;
  }

  return false;
}


bool PThread::GetTimes(Times & times)
{
  // Do not use any PTLib functions in here as they could do a PTRACE, and thus deadlock
  times.m_name = GetThreadName();
  times.m_threadId = m_threadId;
  times.m_uniqueId = GetUniqueIdentifier();
  times.m_real = (m_threadEndTime.IsValid() ? m_threadEndTime : PTime()) - m_threadStartTime;
  times.m_sample.SetCurrentTime();

  std::stringstream path;
  path << "/proc/self/task/" << times.m_uniqueId << "/stat";
  return InternalGetTimes(path.str().c_str(), times);
}


bool PProcess::GetProcessTimes(Times & times) const
{
  times.m_name = "Process Total";
  times.m_real = PTime() - GetStartTime();
  times.m_sample.SetCurrentTime();
  return InternalGetTimes("/proc/self/stat", times);
}


bool PProcess::GetSystemTimes(Times & times)
{
  times.m_name = "System Total";

  for (int retry = 0; retry < 3; ++retry) {
    std::ifstream statfile("/proc/stat");

    char dummy[10];
    uint64_t user, nice, system, idle, iowait, irq, softirq, steal;
    statfile >> dummy >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;
    if (statfile.good()) {
      times.m_kernel = jiffies_to_usecs(system);
      times.m_user = jiffies_to_usecs(user);
      times.m_real = times.m_kernel + times.m_user + jiffies_to_usecs(idle);
      times.m_sample.SetCurrentTime();
      return true;
    }
  }
  return false;
}


void PProcess::GetMemoryUsage(MemoryUsage & usage) const
{
  usage = MemoryUsage();

  {
    ifstream proc("/proc/self/statm");
    size_t virtPages, resPages;
    proc >> virtPages >> resPages;
    if (proc.good()) {
      usage.m_virtual = virtPages * 4096;
      usage.m_resident = resPages * 4096;
    }
  }

  {
    usage.m_total = usage.m_available = 0;
    ifstream proc("/proc/meminfo");
    std::string line;
    size_t memfree = 0, buffers = 0, cached = 0;
    while (getline(proc, line).good()) {
      if (line.compare(0, 8, "MemTotal") == 0)
        usage.m_total = strtoull(line.c_str()+9, NULL, 10)*1000;
      else if (line.compare(0, 12, "MemAvailable") == 0)
        usage.m_available = strtoull(line.c_str()+13, NULL, 10)*1000;
      else if (line.compare(0, 7, "MemFree") == 0)
        memfree = strtoull(line.c_str()+8, NULL, 10)*1000;
      else if (line.compare(0, 7, "Buffers") == 0)
        buffers = strtoull(line.c_str()+8, NULL, 10)*1000;
      else if (line.compare(0, 6, "Cached") == 0)
        cached = strtoull(line.c_str()+7, NULL, 10)*1000;
    }
    if (usage.m_available == 0)
      usage.m_available = memfree + buffers + cached;
  }

#if P_HAS_MALLOC_INFO
  char * buffer = NULL;
  size_t size;
  FILE * mem = open_memstream(&buffer, &size);
  bool ok = malloc_info(0, mem) == 0;
  fclose(mem);

  if (ok) {
    // Find last </heap> in XML
    size_t offset = 0;
    static PRegularExpression HeapRE("< */ *heap *>", PRegularExpression::Extended);
    PINDEX pos, len;
    while (HeapRE.Execute(buffer + offset, pos, len))
      offset += pos + len;

    // The <system type="xxx" size=yyy> after the <heap>s is total

    PStringArray substrings(2);

    static PRegularExpression MaxRE("< *system *type *= *\"max\" *size *= *\"([0-9]+)", PRegularExpression::Extended);
    if (MaxRE.Execute(buffer + offset, substrings)) {
      usage.m_max = (size_t)substrings[1].AsUnsigned64();
      ok = usage.m_max > 0;
    }

    static PRegularExpression CurrentRE("< *system *type *= *\"current\" *size *= *\"([0-9]+)", PRegularExpression::Extended);
    if (CurrentRE.Execute(buffer + offset, substrings))
      usage.m_current = (size_t)substrings[1].AsUnsigned64();
  }

  runtime_free(buffer);

  if (!ok)
#endif // P_HAS_MALLOC_INFO
  {
    struct mallinfo info = mallinfo();
    usage.m_current = (unsigned)info.uordblks;
    usage.m_max = info.usmblks != 0 ? (unsigned)info.usmblks : (usage.m_current + (unsigned)info.fordblks + (unsigned)info.fsmblks);
    usage.m_blocks = info.hblks;
  }
}

#else //P_LINUX

void PProcess::GetMemoryUsage(MemoryUsage & usage) const
{
}


bool PThread::GetTimes(Times & times)
{
  return false;
}


bool PProcess::GetProcessTimes(Times & times) const
{
  return false;
}


bool PProcess::GetSystemTimes(Times & times)
{
  return false;
}

#endif // P_LINUX


unsigned PThread::GetNumProcessors()
{
  int numCPU;

#if defined(_SC_NPROCESSORS_ONLN)
  numCPU = sysconf(_SC_NPROCESSORS_ONLN);
#elif defined(HW_AVAILCPU)
  int mib[4];
  size_t len = sizeof(numCPU); 

  numCPU = -1;

  /* set the mib for hw.ncpu */
  mib[0] = CTL_HW;
  mib[1] = HW_AVAILCPU;
  sysctl(mib, 2, &numCPU, &len, NULL, 0);
  if (numCPU <= 0) {
    mib[1] = HW_NCPU;
    sysctl(mib, 2, &numCPU, &len, NULL, 0);
  }
#else
  return 1;
#endif

  return numCPU > 0 ? numCPU : 1;
}


//////////////////////////////////////////////////////////////////////////////
// P_fd_set

void P_fd_set::Construct()
{
  max_fd = PProcess::Current().GetMaxHandles();
  // Use an array for FORTIFY_SOURCE
  set = (fd_set *)malloc(((max_fd + FD_SETSIZE - 1) / FD_SETSIZE)*sizeof(fd_set));
}


void P_fd_set::Zero()
{
  if (PAssertNULL(set) != NULL)
    memset(set, 0, ((max_fd + FD_SETSIZE - 1) / FD_SETSIZE)*sizeof(fd_set));
}


P_fd_set::P_fd_set()
{
  Construct();
  Zero();
}

bool P_fd_set::IsPresent(intptr_t fd) const
{
  const int fd_num = fd / FD_SETSIZE;
  const int fd_off = fd % FD_SETSIZE;
  return FD_ISSET(fd_off, set + fd_num);
}

P_fd_set::P_fd_set(intptr_t fd)
{
  Construct();
  Zero();
  const int fd_num = fd / FD_SETSIZE;
  const int fd_off = fd % FD_SETSIZE;
  FD_SET(fd_off, set + fd_num);
}


P_fd_set & P_fd_set::operator=(intptr_t fd)
{
  PAssert(fd < max_fd, PInvalidParameter);
  Zero();
  const int fd_num = fd / FD_SETSIZE;
  const int fd_off = fd % FD_SETSIZE;
  FD_SET(fd_off, set + fd_num);
  return *this;
}


P_fd_set & P_fd_set::operator+=(intptr_t fd)
{
  PAssert(fd < max_fd, PInvalidParameter);
  const int fd_num = fd / FD_SETSIZE;
  const int fd_off = fd % FD_SETSIZE;
  FD_SET(fd_off, set + fd_num);
  return *this;
}


P_fd_set & P_fd_set::operator-=(intptr_t fd)
{
  PAssert(fd < max_fd, PInvalidParameter);
  const int fd_num = fd / FD_SETSIZE;
  const int fd_off = fd % FD_SETSIZE;
  FD_CLR(fd_off, set + fd_num);
  return *this;
}


///////////////////////////////////////////////////////////////////////////////

/*
 * tlibthrd.cxx
 *
 * Routines for pre-emptive threading system
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

#include <sched.h>
#include <pthread.h>
#include <sys/resource.h>

#define SUSPEND_SIG SIGVTALRM

#if defined(P_MACOSX)
#include <mach/mach.h>
#include <mach/thread_policy.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#elif defined(P_LINUX)
#include <sys/syscall.h>
#elif defined(P_ANDROID)
#include <asm/page.h>
#include <jni.h>
#endif

#ifdef P_ANDROID
static JavaVM* AndroidJavaVM;
JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
  AndroidJavaVM = vm;
  return JNI_VERSION_1_6;
}
#endif


static PINDEX const PThreadMinimumStack = 16 * PTHREAD_STACK_MIN; // Set a decent stack size that won't eat all virtual memory, or crash


int PX_NewHandle(const char*, int);


#define PAssertWithRetry(func, arg, ...) \
  { \
    unsigned threadOpRetry = 0; \
    while (PAssertThreadOp(func(arg, ##__VA_ARGS__), threadOpRetry, #func, \
              reinterpret_cast<const void *>(arg), PDebugLocation(__FILE__, __LINE__, __FUNCTION__))); \
  }


static bool PAssertThreadOp(int retval,
                            unsigned& retry,
                            const char* funcname,
                            const void* arg1,
                            const PDebugLocation& location)
{
  if (retval == 0) {
    #if PTRACING
    if (PTrace::CanTrace(2) && retry > 0)
      PTrace::Begin(2, location.m_file, location.m_line, NULL, "PTLib")
      << funcname << '(' << arg1 << ") required " << retry << " retries!" << PTrace::End;
    #endif
    return false;
  }

  int err = retval < 0 ? errno : retval;

  /* Retry on a temporary error. Technically an EINTR only happens on a signal,
     and EAGAIN not at all for most of the functions, but expereince is that when
     the system gets really busy, they do occur, lots. So we have to keep trying,
     but still give up and assert after a suitably huge effort. */
  if ((err == EINTR || err == EAGAIN) && ++retry < 1000) {
    PThread::Sleep(10); // Basically just swap out thread to try and clear blockage
    return true;        // Return value to try again
  }

  #if PTRACING || P_USE_ASSERTS
  std::ostringstream msg;
  msg << "Function " << funcname << '(' << arg1 << ") failed, errno=" << err << ' ' << strerror(err);
  #if P_USE_ASSERTS
  PAssertFunc(location, msg.str().c_str());
  #else
  PTrace::Begin(0, location.m_file, location.m_line, NULL, "PTLib") << msg.str() << PTrace::End;
  #endif
  #endif
  return false;
}


#if defined(P_LINUX)
static int GetSchedParam(PThread::Priority priority, sched_param& param)
{
  /*
    Set realtime scheduling if our effective user id is root (only then is this
    allowed) AND our priority is Highest.
      I don't know if other UNIX OSs have SCHED_FIFO and SCHED_RR as well.

    WARNING: a misbehaving thread (one that never blocks) started with Highest
    priority can hang the entire machine. That is why root permission is
    neccessary.
  */

  memset(&param, 0, sizeof(sched_param));

  switch (priority) {
    case PThread::HighestPriority:
      param.sched_priority = sched_get_priority_max(SCHED_RR);
      break;

    case PThread::HighPriority:
      param.sched_priority = (sched_get_priority_max(SCHED_RR) + sched_get_priority_min(SCHED_RR)) / 2;
      break;

      #ifdef SCHED_BATCH
    case PThread::LowestPriority:
    case PThread::LowPriority:
      return SCHED_BATCH;
      #endif

    default: // PThread::NormalPriority :
      return SCHED_OTHER;
  }

  #ifdef RLIMIT_RTPRIO
  struct rlimit rl;
  if (getrlimit(RLIMIT_RTPRIO, &rl) != 0) {
    PTRACE(2, "PTLib", "Could not get Real Time thread priority limit - " << strerror(errno));
    return SCHED_OTHER;
  }

  if ((int)rl.rlim_cur < (int)param.sched_priority) {
    rl.rlim_max = rl.rlim_cur = param.sched_priority;
    if (setrlimit(RLIMIT_RTPRIO, &rl) != 0) {
      PTRACE(geteuid() == 0 || errno != EPERM ? 2 : 3, "PTLib",
             "Could not increase Real Time thread priority limit to " << rl.rlim_cur << " - " << strerror(errno));
      param.sched_priority = 0;
      return SCHED_OTHER;
    }

    PTRACE(4, "PTLib", "Increased Real Time thread priority limit to " << rl.rlim_cur);
  }

  return SCHED_RR;
  #else
  if (geteuid() == 0)
    return SCHED_RR;

  param.sched_priority = 0;
  PTRACE(3, "PTLib", "No permission to set priority level " << priority);
  return SCHED_OTHER;
  #endif // RLIMIT_RTPRIO
}
#endif


#define new PNEW


//////////////////////////////////////////////////////////////////////////////

void PThread::PlatformConstruct()
{
  #ifndef P_HAS_SEMAPHORES
    PX_waitingSemaphore = NULL;
    PX_WaitSemMutex = MutexInitialiser;
  #endif
  PAssertOS(::pipe(m_threadUnblockPipe) == 0);
}


//
//  Called to destruct a PThread
//
//  If not called in the context of the thread being destroyed, we need to wait
//  for that thread to stop before continuing
//

void PThread::PlatformDestroy()
{
  // close I/O unblock pipes
  ::close(m_threadUnblockPipe[0]);
  ::close(m_threadUnblockPipe[1]);

  #ifndef P_HAS_SEMAPHORES
    pthread_mutex_destroy(&PX_WaitSemMutex);
  #endif
}


void PThread::PlatformSetThreadName(const char* name)
{
  #if defined(P_MACOSX)
    // For some bizarre reason Mac OS-X version of this function only has one argument.
    // So can only be called from the current thread, other calls to SetThreadName() will be ignored
    if (GetCurrentThreadId() == GetThreadId())
      pthread_setname_np(name);
  #else
    if (m_nativeHandle != 0)
      pthread_setname_np(m_nativeHandle, name);
  #endif
}


bool PThread::PlatformKill(PThreadIdentifier tid, PUniqueThreadIdentifier uid, int sig)
{
  if (!PProcess::IsInitialised())
    return false;

  pthread_t thread;
  PProcess& process = PProcess::Current();
  {
    PWaitAndSignal mutex(process.m_threadMutex);
    PProcess::ThreadMap::iterator it = process.m_activeThreads.find(tid);
    if (it == process.m_activeThreads.end() || (uid != 0 && it->second->GetUniqueIdentifier() != uid))
      return false;
    thread = it->second->m_nativeHandle;
  }

  int error = pthread_kill(thread, sig);
  switch (error) {
    case 0:
      return true;

      // If just test for existance, is true even if we don't have permission
    case EPERM:
    case ENOTSUP: // Mac OS-X does this for GCD threads
      return sig == 0;

      #if PTRACING
    case ESRCH:   // Thread not running any more
    case EINVAL:  // Id has never been used for a thread
      break;

    default:
      // Output direct to stream, do not use PTRACE as it might cause an infinite recursion.
      ostream* trace = PTrace::GetStream();
      if (trace != NULL)
        *trace << "pthread_kill failed for thread " << Identifiers(tid, uid)
        << " errno " << error << ' ' << strerror(error) << endl;
      #endif // PTRACING
  }

  return false;
}


#ifdef P_MACOSX
// obtain thread priority of the main thread
static unsigned long
GetThreadBasePriority()
{
  thread_basic_info_data_t threadInfo;
  policy_info_data_t       thePolicyInfo;
  unsigned int             count;
  pthread_t baseThread = PProcess::Current().GetThreadId();

  // get basic info
  count = THREAD_BASIC_INFO_COUNT;
  thread_info(pthread_mach_thread_np(baseThread), THREAD_BASIC_INFO,
              (integer_t*)&threadInfo, &count);

  switch (threadInfo.policy) {
    case POLICY_TIMESHARE:
      count = POLICY_TIMESHARE_INFO_COUNT;
      thread_info(pthread_mach_thread_np(baseThread),
                  THREAD_SCHED_TIMESHARE_INFO,
                  (integer_t*)&(thePolicyInfo.ts), &count);
      return thePolicyInfo.ts.base_priority;

    case POLICY_FIFO:
      count = POLICY_FIFO_INFO_COUNT;
      thread_info(pthread_mach_thread_np(baseThread),
                  THREAD_SCHED_FIFO_INFO,
                  (integer_t*)&(thePolicyInfo.fifo), &count);
      if (thePolicyInfo.fifo.depressed)
        return thePolicyInfo.fifo.depress_priority;
      return thePolicyInfo.fifo.base_priority;

    case POLICY_RR:
      count = POLICY_RR_INFO_COUNT;
      thread_info(pthread_mach_thread_np(baseThread),
                  THREAD_SCHED_RR_INFO,
                  (integer_t*)&(thePolicyInfo.rr), &count);
      if (thePolicyInfo.rr.depressed)
        return thePolicyInfo.rr.depress_priority;
      return thePolicyInfo.rr.base_priority;
  }

  return 0;
}
#endif

void PThread::SetPriority(Priority priorityLevel)
{
  PTRACE(4, "PTLib", "Setting thread priority to " << priorityLevel);

  if (m_nativeHandle == 0)
    return;

  #if defined(P_LINUX)
  struct sched_param params;
  PAssertWithRetry(pthread_setschedparam, m_nativeHandle, GetSchedParam(priorityLevel, params), &params);

  #elif defined(P_ANDROID)
  if (Current() != this) {
    PTRACE(2, "JNI", "Can only set priority for current thread.");
    return;
  }

  if (AndroidJavaVM == NULL) {
    PTRACE(2, "JNI", "JavaVM not set.");
    return;
  }

  JNIEnv* jni = NULL;
  bool detach = false;

  jint err = AndroidJavaVM->GetEnv((void**)&jni, JNI_VERSION_1_6);
  switch (err) {
    case JNI_EDETACHED:
      if ((err = AndroidJavaVM->AttachCurrentThread(&jni, NULL)) != JNI_OK) {
        PTRACE(2, "JNI", "Could not attach JNI environment, error=" << err);
        return;
      }
      detach = true;

    case JNI_OK:
      break;

    default:
      PTRACE(2, "JNI", "Could not get JNI environment, error=" << err);
      return;
  }

  //Get pointer to the java class
  jclass androidOsProcess = (jclass)jni->NewGlobalRef(jni->FindClass("android/os/Process"));
  if (androidOsProcess != NULL) {
    jmethodID setThreadPriority = jni->GetStaticMethodID(androidOsProcess, "setThreadPriority", "(I)V");
    if (setThreadPriority != NULL) {
      static const int Priorities[NumPriorities] = { 19, 10, 0,  -10, -19 };
      jni->CallStaticIntMethod(androidOsProcess, setThreadPriority, Priorities[priorityLevel]);
      PTRACE(5, "JNI", "setThreadPriority " << Priorities[priorityLevel]);
    }
    else {
      PTRACE(2, "JNI", "Could not find setThreadPriority");
    }
  }
  else {
    PTRACE(2, "JNI", "Could not find android.os.Process");
  }

  if (detach)
    AndroidJavaVM->DetachCurrentThread();

  #elif defined(P_MACOSX)
  if (priorityLevel == HighestPriority) {
    /* get fixed priority */
    {
      int result;

      thread_extended_policy_data_t   theFixedPolicy;
      thread_precedence_policy_data_t thePrecedencePolicy;
      long                            relativePriority;

      theFixedPolicy.timeshare = false; // set to true for a non-fixed thread
      result = thread_policy_set(pthread_mach_thread_np(m_threadId),
                                 THREAD_EXTENDED_POLICY,
                                 (thread_policy_t)&theFixedPolicy,
                                 THREAD_EXTENDED_POLICY_COUNT);
      if (result != KERN_SUCCESS) {
        PTRACE(1, "thread_policy - Couldn't set thread as fixed priority.");
      }

      // set priority

      // precedency policy's "importance" value is relative to
      // spawning thread's priority

      relativePriority = 62 - GetThreadBasePriority();
      PTRACE(3, "relativePriority is " << relativePriority << " base priority is " << GetThreadBasePriority());

      thePrecedencePolicy.importance = relativePriority;
      result = thread_policy_set(pthread_mach_thread_np(m_threadId),
                                 THREAD_PRECEDENCE_POLICY,
                                 (thread_policy_t)&thePrecedencePolicy,
                                 THREAD_PRECEDENCE_POLICY_COUNT);
      if (result != KERN_SUCCESS) {
        PTRACE(1, "thread_policy - Couldn't set thread priority.");
      }
    }
  }
  #endif
}


PThread::Priority PThread::GetPriority() const
{
  #if defined(LINUX)
  if (m_nativeHandle == 0)
    return NormalPriority;

  int policy;
  struct sched_param params;

  PAssertWithRetry(pthread_getschedparam, m_threadId, &policy, &params);

  switch (policy) {
    case SCHED_OTHER:
      break;

    case SCHED_FIFO:
    case SCHED_RR:
      return params.sched_priority > sched_get_priority_min(policy) ? HighestPriority : HighPriority;

      #ifdef SCHED_BATCH
    case SCHED_BATCH:
      return LowPriority;
      #endif

    default:
      /* Unknown scheduler. We don't know what priority this thread has. */
      PTRACE(1, "PTLib\tPThread::GetPriority: unknown scheduling policy #" << policy);
  }
  #endif

  return NormalPriority;
}


#ifndef P_HAS_SEMAPHORES
void PThread::PXSetWaitingSemaphore(PSemaphore* sem)
{
  PAssertWithRetry(pthread_mutex_lock, &PX_WaitSemMutex);
  PX_waitingSemaphore = sem;
  PAssertWithRetry(pthread_mutex_unlock, &PX_WaitSemMutex);
}
#endif


//
//  Terminate the specified thread
//
void PThread::Terminate()
{
  // if thread was not created by PTLib, then don't terminate it
  if (m_type == Type::IsExternal)
    return;

  // if the thread is already terminated, then nothing to do
  if (IsTerminated())
    return;

  // if thread calls Terminate on itself, then do it
  // don't use PThread::Current, as the thread may already not be in the
  // active threads list
  if (GetThreadId() == GetCurrentThreadId()) {
    pthread_exit(0);
    return;   // keeps compiler happy
  }

  // otherwise force thread to die
  PTRACE(2, "PTLib\tForcing termination of thread id=0x" << hex << m_threadId << dec);

  PXAbortBlock();
  if (WaitForTermination(100))
    return;

  #ifndef P_HAS_SEMAPHORES
  PAssertWithRetry(pthread_mutex_lock, &PX_WaitSemMutex);
  if (PX_waitingSemaphore != NULL) {
    PAssertWithRetry(pthread_mutex_lock, &PX_waitingSemaphore->mutex);
    PX_waitingSemaphore->queuedLocks--;
    PAssertWithRetry(pthread_mutex_unlock, &PX_waitingSemaphore->mutex);
    PX_waitingSemaphore = NULL;
  }
  PAssertWithRetry(pthread_mutex_unlock, &PX_WaitSemMutex);
  #endif

  if (m_threadId != PNullThreadIdentifier) {
    #if P_USE_THREAD_CANCEL
    pthread_cancel(m_nativeHandle);
    #else
    pthread_kill(m_nativeHandle, SIGKILL);
    #endif
    WaitForTermination(100);
  }
}


PUniqueThreadIdentifier PThread::GetCurrentUniqueIdentifier()
{
  #if defined(P_LINUX)
    return syscall(SYS_gettid);
  #elif defined(P_MACOSX)
    PUniqueThreadIdentifier id;
    return pthread_threadid_np(::pthread_self(), &id) == 0 ? id : 0;
  #else
    return (PUniqueThreadIdentifier)GetCurrentThreadId();
  #endif
}


int PThread::PXBlockOnIO(int handle, int type, const PTimeInterval& timeout)
{
  PTRACE(7, "PTLib\tPThread::PXBlockOnIO(" << handle << ',' << type << ')');

  if ((handle < 0) || (handle >= PProcess::Current().GetMaxHandles())) {
    PTRACE(2, "PTLib\tAttempt to use illegal handle in PThread::PXBlockOnIO, handle=" << handle);
    errno = EBADF;
    return -1;
  }

  int retval;

  #if P_HAS_POLL
  struct pollfd pfd[2];
  pfd[0].fd = handle;
  switch (type) {
    case PChannel::PXReadBlock:
    case PChannel::PXAcceptBlock:
      pfd[0].events = POLLIN;
      break;

    case PChannel::PXWriteBlock:
    case PChannel::PXConnectBlock:
      pfd[0].events = POLLOUT;
      break;
  }
  pfd[0].events |= POLLERR;

  pfd[1].fd = m_threadUnblockPipe[0];
  pfd[1].events = POLLIN;

  do {
    retval = ::poll(pfd, PARRAYSIZE(pfd), timeout.GetInterval());
  } while (retval < 0 && errno == EINTR);

  uint8_t dummy;
  if (retval > 0 && pfd[1].revents != 0 && ::read(m_threadUnblockPipe[0], &dummy, 1) == 1) {
    errno = ECANCELED;
    retval = -1;
    PTRACE(6, "PTLib\tUnblocked I/O fd=" << m_threadUnblockPipe[0]);
  }

  #else // P_HAS_POLL

  P_fd_set read_fds;
  P_fd_set write_fds;
  P_fd_set exception_fds;

  do {
    switch (type) {
      case PChannel::PXReadBlock:
      case PChannel::PXAcceptBlock:
        read_fds = handle;
        write_fds.Zero();
        exception_fds.Zero();
        break;
      case PChannel::PXWriteBlock:
        read_fds.Zero();
        write_fds = handle;
        exception_fds.Zero();
        break;
      case PChannel::PXConnectBlock:
        read_fds.Zero();
        write_fds = handle;
        exception_fds = handle;
        break;
      default:
        PAssertAlways(PLogicError);
        return 0;
    }

    // include the termination pipe into all blocking I/O functions
    read_fds += m_threadUnblockPipe[0];

    P_timeval tval = timeout;
    PPROFILE_SYSTEM(
      retval = ::select(PMAX(handle, m_threadUnblockPipe[0]) + 1,
                        read_fds, write_fds, exception_fds, tval);
    );
  } while (retval < 0 && errno == EINTR);

  uint8_t dummy;
  if (retval > 0 && read_fds.IsPresent(m_threadUnblockPipe[0]) && ::read(m_threadUnblockPipe[0], &ch, 1) == 1) {
    errno = ECANCELED;
    retval = -1;
    PTRACE(6, "PTLib\tUnblocked I/O fd=" << m_threadUnblockPipe[0]);
  }

  #endif // P_HAS_POLL

  return retval;
}

void PThread::PXAbortBlock() const
{
  static uint8_t ch = 0;
  PAssertOS(::write(m_threadUnblockPipe[1], &ch, 1) == 1);
  PTRACE(6, "PTLib\tUnblocking I/O fd=" << m_threadUnblockPipe[0] << " thread=" << *this);
}


///////////////////////////////////////////////////////////////////////////////

PSemaphore::~PSemaphore()
{
  #if defined(P_HAS_SEMAPHORES)
  #if defined(P_HAS_NAMED_SEMAPHORES)
  if (m_namedSemaphore.ptr != NULL) {
    PAssertWithRetry(sem_close, m_namedSemaphore.ptr);
  }
  else
    #endif
    PAssertWithRetry(sem_destroy, &m_semaphore);
  #else
  PAssert(queuedLocks == 0, "Semaphore destroyed with queued locks");
  PAssertWithRetry(pthread_mutex_destroy, &mutex);
  PAssertWithRetry(pthread_cond_destroy, &condVar);
  #endif
}


void PSemaphore::Reset(unsigned initial, unsigned maximum)
{
  m_maximum = std::min(maximum, (unsigned)INT_MAX);
  m_initial = std::min(initial, m_maximum);

  #if defined(P_HAS_SEMAPHORES)
    /* Due to bug in some Linux/Kernel versions, need to clear structure manually, sem_init does not do the job.
       See http://stackoverflow.com/questions/1832395/sem-timedwait-not-supported-properly-on-redhat-enterprise-linux-5-3-onwards
       While the above link was for RHEL, seems to happen on some Fedoras as well. */
  memset(&m_semaphore, 0, sizeof(sem_t));

  #if defined(P_HAS_NAMED_SEMAPHORES)
  if (m_name.IsEmpty() && sem_init(&m_semaphore, 0, m_initial) == 0)
    m_namedSemaphore.ptr = NULL;
  else {
    // Since sem_open and sem_unlink are two operations, there is a small
    // window of opportunity that two simultaneous accesses may return
    // the same semaphore. Therefore, the static mutex is used to
    // prevent this.
    static pthread_mutex_t semCreationMutex = PTHREAD_MUTEX_INITIALIZER;
    PAssertWithRetry(pthread_mutex_lock, &semCreationMutex);

    if (!m_name.IsEmpty())
      m_namedSemaphore.ptr = sem_open(m_name, (O_CREAT | O_EXCL), 700, m_initial);
    else {
      PStringStream generatedName;
      generatedName << "/ptlib/" << getpid() << '/' << this;
      sem_unlink(generatedName);
      m_namedSemaphore.ptr = sem_open(generatedName, (O_CREAT | O_EXCL), 700, m_initial);
    }

    PAssertWithRetry(pthread_mutex_unlock, &semCreationMutex);

    if (!PAssert(m_namedSemaphore.ptr != SEM_FAILED, "Couldn't create named semaphore"))
      m_namedSemaphore.ptr = NULL;
  }
  #else
  if (m_name.IsEmpty())
    PAssertWithRetry(sem_init, &m_semaphore, 0, m_initial);
  #endif
  #else
  if (m_name.IsEmpty()) {
    PAssertWithRetry(pthread_mutex_init, &mutex, NULL);
    PAssertWithRetry(pthread_cond_init, &condVar, NULL);
    currentCount = initial;
  }
  else
    currentCount = INT_MAX;
  queuedLocks = 0;
  #endif
}


void PSemaphore::Wait()
{
  #if defined(P_HAS_SEMAPHORES)
  PAssertWithRetry(sem_wait, GetSemPtr());
  #else
  if (currentCount == INT_MAX)
    return;

  PAssertWithRetry(pthread_mutex_lock, &mutex);

  queuedLocks++;
  PThread::Current()->PXSetWaitingSemaphore(this);

  while (currentCount == 0) {
    PPROFILE_SYSTEM(
      int err = pthread_cond_wait(&condVar, &mutex);
    );
    PAssert(err == 0 || err == EINTR, psprintf("wait error = %i", err));
  }

  PThread::Current()->PXSetWaitingSemaphore(NULL);
  queuedLocks--;

  currentCount--;

  PAssertWithRetry(pthread_mutex_unlock, &mutex);
  #endif
}


bool PSemaphore::Wait(const PTimeInterval& waitTime)
{
  if (waitTime == PMaxTimeInterval) {
    Wait();
    return true;
  }

  // create absolute finish time 
  PTime finishTime;
  finishTime += waitTime;

  #if defined(P_HAS_SEMAPHORES)
  #ifdef P_HAS_SEMAPHORES_XPG6
    // use proper timed spinlocks if supported.
    // http://www.opengroup.org/onlinepubs/007904975/functions/sem_timedwait.html

  struct timespec absTime;
  absTime.tv_sec = finishTime.GetTimeInSeconds();
  absTime.tv_nsec = finishTime.GetMicrosecond() * 1000;

  PPROFILE_PRE_SYSTEM();
  do {
    if (sem_timedwait(GetSemPtr(), &absTime) == 0) {
      PPROFILE_POST_SYSTEM();
      return true;
    }
  } while (errno == EINTR);
  PPROFILE_POST_SYSTEM();

  PAssert(errno == ETIMEDOUT, strerror(errno));

  #else
    // loop until timeout, or semaphore becomes available
    // don't use a PTimer, as this causes the housekeeping
    // thread to get very busy
  PPROFILE_PRE_SYSTEM();
  do {
    if (sem_trywait(GetSemPtr()) == 0) {
      PPROFILE_POST_SYSTEM();
      return true;
    }

    // tight loop is bad karma
    // for the linux scheduler: http://www.ussg.iu.edu/hypermail/linux/kernel/0312.2/1127.html
    PThread::Sleep(10);
  } while (PTime() < finishTime);
  PPROFILE_POST_SYSTEM();
  #endif

  return false;

  #else
  if (currentCount == INT_MAX)
    return false;

  struct timespec absTime;
  absTime.tv_sec = finishTime.GetTimeInSeconds();
  absTime.tv_nsec = finishTime.GetMicrosecond() * 1000;

  PPROFILE_PRE_SYSTEM();

  PAssertWithRetry(pthread_mutex_lock, &mutex);

  PThread* thread = PThread::Current();
  thread->PXSetWaitingSemaphore(this);
  queuedLocks++;

  bool ok = true;
  while (currentCount == 0) {
    int err = pthread_cond_timedwait(&condVar, &mutex, &absTime);
    if (err == ETIMEDOUT) {
      ok = false;
      break;
    }
    else
      PAssert(err == 0 || err == EINTR, psprintf("timed wait error = %i", err));
  }

  thread->PXSetWaitingSemaphore(NULL);
  queuedLocks--;

  if (ok)
    currentCount--;

  PAssertWithRetry(pthread_mutex_unlock, &mutex);

  PPROFILE_POST_SYSTEM();

  return ok;
  #endif
}


void PSemaphore::Signal()
{
  #if defined(P_HAS_SEMAPHORES)
  PAssertWithRetry(sem_post, GetSemPtr());
  #else
  PAssertWithRetry(pthread_mutex_lock, &mutex);

  if (currentCount < m_maximum)
    currentCount++;

  if (queuedLocks > 0)
    PAssertWithRetry(pthread_cond_signal, &condVar);

  PAssertWithRetry(pthread_mutex_unlock, &mutex);
  #endif
}

