/*
 * tracing.h
 *
 * Mother of all ancestor classes.
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

#ifndef PTLIB_TRACING_H
#define PTLIB_TRACING_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif


 /** Get the stream being used for error output.
This stream is used for all trace output using the various trace functions
and macros.
*/
ostream & PGetErrorStream();

/** Set the stream to be used for error output.
This stream is used for all error output using the <code>PError</code> macro.
*/
void PSetErrorStream(ostream * strm /** New stream for error output */ );

/** This macro is used to access the platform specific error output stream.
This is to be used in preference to assuming <code>cerr</code> is always available. On
Unix platforms this {\b is} <code>cerr</code> but for MS-Windows this is another stream
that uses the OutputDebugString() Windows API function. Note that a MS-DOS or
Windows NT console application would still use <code>cerr</code>.

The <code>PError</code> stream would normally only be used for debugging information as
a suitable display is not always available in windowed environments.
   
The macro is a wrapper for a global variable error stream. The internal variable
is initialised to <i>std::cerr</i> for all but MS-Windows and NT GUI applications.
An application could change this pointer to a <i>std::ofstream</i> variable of
#PError output is wished to be redirected to a file.
*/
#define PError (PGetErrorStream())


///////////////////////////////////////////////////////////////////////////////
// Debug and tracing

#if PTRACING

class PObject;
class PArgList;

/**Class to encapsulate tracing functions.
   This class does not require any instances and is only being used as a
   method of grouping functions together in a name space.

   There are a number of macros for supporting tracing. These will all
   evaluate as empty in a "No Trace" build of the system:
     - PTRACE()
     - PTRACE_IF()
     - PTRACE_PARAM()
     - PTRACE_BLOCK()
     - PTRACE_LINE()
  */
class PTrace
{
public:
  /// Options for trace output.
  enum Options {
    Blocks            = 0x0001,   /**< Include PTrace::Block constructs in output
                                       If this is bit is clear, all PTrace::Block output is inhibited
                                       regardless of the trace level. If set, the PTrace::Block may occur
                                       provided the trace level is greater than zero.
                                    */
    DateAndTime       = 0x0002,   ///< Include date and time in all output
    Timestamp         = 0x0004,   ///< Include (millisecond) timestamp in all output
    Thread            = 0x0008,   ///< Include identifier for thread trace is made from in all output
    TraceLevel        = 0x0010,   ///< Include trace level, as a numeric value, in all output
    FileAndLine       = 0x0020,   ///< Include the file and line for the trace call in all output
    ThreadAddress     = 0x0040,   ///< Include thread object pointer address in all trace output
    AppendToFile      = 0x0080,   ///< Append to log file rather than resetting every time
    GMTTime           = 0x0100,   ///< Output timestamps in GMT time rather than local time
    RotateDaily       = 0x0200,   ///< If set, log file will be rotated daily
    RotateHourly      = 0x0400,   ///< If set, log file will be rotated hourly
    RotateMinutely    = 0x0800,   ///< If set, log file will be rotated every minute
    RotateLogMask     = RotateDaily + RotateHourly + RotateMinutely,
                                  ///< Mask for all the rotate bits
    ObjectInstance    = 0x1000,   ///< Include object instance in all trace output
    ContextIdentifier = 0x2000,   ///< Include context identifier in all trace output
    SingleLine        = 0x4000,   ///< Assure trace output is always on a single line
    SystemLogStream   = 0x8000,   /**< SystemLog flag for tracing within a PServiceProcess
                                       application. Setting this flag will automatically
                                       execute <code>#SetStream(new PSystemLog)</code>. */
    OutputJSON         = 0x10000,   ///< Output log in JSON format
    HasFilePermissions = 0x8000000, ///< Flag indicating file permissions are to be set
    FilePermissionMask = 0x7ff0000, /**< Mask for setting standard file permission mask as used in
                                         open() or creat() system function calls. */
    FilePermissionShift = 16
  };


  #define PTRACE_ARGLIST_OPT_HELP \
    "use +X or -X to add/remove option where X is one of:\r" \
    "  block    PTrace::Block constructs in output\r" \
    "  time     time since program start\r" \
    "  date     date and time\r" \
    "  gmt      Date/time is in UTC\r" \
    "  thread   thread name and identifier\r" \
    "  level    log level\r" \
    "  source   source file name and line number\r" \
    "  object   PObject pointer\r" \
    "  context  context identifier\r" \
    "  json     JSON format output\r" \
    "  single   single line output\r" \
    "  daily    rotate output file daily\r" \
    "  hour     rotate output file hourly\r" \
    "  minute   rotate output file every minute\r" \
    "  append   append to output file, otherwise overwrites\r" \
    "  <perm>   file permission similar to unix chmod, but starts\r" \
    "           with +/- and only has one combination at a time,\r" \
    "           e.g. +uw is user write, +or is other read, etc"

  #define PTRACE_ARG_TRACE    "trace"
  #define PTRACE_ARG_LEVEL    "trace-level"
  #define PTRACE_ARG_OUTPUT   "output"
  #define PTRACE_ARG_ROLLOVER "trace-rollover"
  #define PTRACE_ARG_OPTION   "trace-option"

  #define PTRACE_ARGLIST_EXT(t,l,o,r,O) \
    t "-" PTRACE_ARG_TRACE ".     Trace enable (use multiple times for more detail).\n" \
    l "-" PTRACE_ARG_LEVEL ":     Specify trace detail level.\n" \
    o "-" PTRACE_ARG_OUTPUT ":    Specify filename for trace output\rMay be special value such as \"stderr\" dependent on platform.\n" \
    r "-" PTRACE_ARG_ROLLOVER ":  Specify trace file rollover file name pattern.\n" \
    O "-" PTRACE_ARG_OPTION ":    Specify trace option(s),\r" PTRACE_ARGLIST_OPT_HELP "\n"

  #define PTRACE_ARGLIST PTRACE_ARGLIST_EXT("t","","o","","")
  
  #define PTRACE_INITIALISE(...) PTrace::Initialise(__VA_ARGS__)

  /**Set the most common trace options.
     This sets trace options based on command line arguments.
    */
  static void Initialise(
    const PArgList & args,                   ///< Command line arguments
    unsigned options =
#ifdef _DEBUG
          FileAndLine |
#endif
                Timestamp | Thread | Blocks, ///< Default #Options for tracing
    const char * traceCount = PTRACE_ARG_TRACE,       ///< Argument option name for trace count
    const char * outputFile = PTRACE_ARG_OUTPUT,      ///< Argument option name for log output file
    const char * traceOpts  = PTRACE_ARG_OPTION,      ///< Argument option name for trace options
    const char * traceRollover = PTRACE_ARG_ROLLOVER, ///< Argument option name for trace file roll over pattern
    const char * traceLevel = PTRACE_ARG_LEVEL        ///< Argument option name for trace level
  );

  /**Set the most common trace options.
     If \p filename is not NULL then a PTextFile is created and attached the
     trace output stream. This object is never closed or deleted until the
     termination of the program.

     There are several special values for \p filename:
       <dl>
       <dt>"stderr"      <dd>Output to standard error
       <dt>"stdout"      <dd>Output to standard output
       <dt>"DEBUGSTREAM" <dd>Output to debugger (Windows only)
       <dt>"syslog,ident,priority,options,facility"
            <dd>Output to syslog (Unix variants only)
                The ident, priority, options & facility components are optional,
                so "syslog", "syslog,myappname", "syslog,myappname,3" are all
                acceptable. See PSystemLogToSyslog for their exact meaning.
       <dt>"network,host:port,facility"
            <dd>Output to RFC3164 network log server.
                The server and facility components are optional. The default
                server would be "localhost" Also, the port is an optional
                field for the server component, defaulting to the RFC3164
                standard value. So, "network,10.0.1.1" or "network,fred:1234"
                are acceptable. See PSystemLogToNetwork for more.
       </dl>

     If \p rolloverPattern is not NULL it is used as the time format pattern
     appended to filename if the #RotateLogMask bits are set. Default is
     "yyyy_MM_dd".

     A trace output of the program name, version, OS abnd other information is
     written to the log immediately.
    */
  static void Initialise(
    unsigned level,                               ///< Level for tracing
    const char * filename = NULL,                 ///< Filename for log output
    unsigned options = Timestamp | Thread | Blocks, ///< #Options for tracing
    const char * rolloverPattern = NULL             ///< Pattern for rolling over trace files
  );

  // Deprecated - for backward compatibility
  static void Initialise(
    unsigned level,
    const char * filename,
    const char * rolloverPattern,
    unsigned options = Timestamp | Thread | Blocks
  ) { Initialise(level, filename, options, rolloverPattern); }

  /** Set the trace output file.
  */
  static void SetFilename(
    const char * filename ///< New output file for tracing
  );

  /**Get the current filename for trace output.
     Note that the returned pointer is only valid until the next call to
     SetFilename() or Initialise().
   */
  static const char * GetFilename();

  /**Set the mmaximum length for logs.
     Default is 10k
    */
  static void SetMaxLength(
    PINDEX length   ///< Maximum line length for log
  );

  /**Get the mmaximum length for logs.
     Default is 10k
    */
  static PINDEX GetMaxLength();

  /** Set the trace options by name.
      The parameter string consists of a series of keywords separated
      by a + or -. Use +X or -X to add/remove option where X is one of:
        block    PTrace::Block constructs in output
        time     time since prgram start
        date     date and time
        gmt      Date/time is in UTC
        thread   thread name and identifier
        level    log level
        file     source file name and line number
        object   PObject pointer
        context  context identifier
        single   single line output
        daily    rotate output file daily
        hour     rotate output file hourly
        minute   rotate output file every minute
        append   append to output file, otherwise overwrites
        <perm>   file permission similar to unix chmod, but starts
                 with +/- and only has one combination at a time,
                 e.g. +uw is user write, +or is other read, etc"
  */
  static void SetOptionsByName(
    const char * options ///< New option bits for tracing
  );

  /** Get current trace options by name.
  */
  static std::string GetOptionsByName();

  /** Set the trace options.
      The PTRACE(), PTRACE_BLOCK() and PTRACE_LINE() macros output trace text that
      may contain assorted values. These are defined by the #Options enum.

      Note this function OR's the bits included in the options parameter.
  */
  static void SetOptions(
    unsigned options ///< New option bits for tracing
  );

  /** Clear the trace options.
      The <code>PTRACE()</code>, <code>PTRACE_BLOCK()</code> and
      <code>PTRACE_LINE()</code> macros output trace text that
      may contain assorted values. These are defined by the #Options enum.

      Note this function AND's the complement of the bits included in the options
      parameter.
  */
  static void ClearOptions(
    unsigned options ///< Option bits to turn off
  );

  /** Get the current trace options.
      The <code>PTRACE()</code>, <code>PTRACE_BLOCK()</code> and
      <code>PTRACE_LINE()</code> macros output trace text that
      may contain assorted values. These are defined by the #Options enum.
  */
  static unsigned GetOptions();

  /** Set the trace level.
      The <code>PTRACE()</code> macro checks to see if its level is equal to or lower then the
      level set by this function. If so then the trace text is output to the trace
      stream.
  */
  static void SetLevel(
    unsigned level ///< New level for tracing
  );

  /** Get the trace level.
      The <code>PTRACE()</code> macro checks to see if its level is equal to or lower then the
      level set by this function. If so then the trace text is output to the trace
      stream.
  */
  static unsigned GetLevel();

  /** Determine if the level may cause trace output.
      This checks against the current global trace level set by SetLevel()
      for if the trace output may be emitted. This is used by the PTRACE() macro.
  */
  static PBoolean CanTrace(
    unsigned level ///< Trace level to check
  );

  /** Set the stream to be used for trace output.
      This stream is used for all trace output using the various trace functions
      and macros.
  */
  static void SetStream(
    ostream * out ///< New output stream from trace.
  );

  /** Get the stream being used for trace output.
  */
  static ostream * GetStream();

  /** Get the trace output stream and lock it until out of scope.
   */
  class LockedStream
  {
    private:
      ostream * m_stream;
    public:
      LockedStream();
      ~LockedStream();
      ostream & operator*() const { return *m_stream; }
      ostream * operator->() const { return m_stream; }
      operator bool() const { return m_stream != NULL; }
      bool operator!() const { return m_stream == NULL; }
    friend class PTrace;
  };

  /// Get the time zone being used for logging.
  static int GetTimeZone();

  /** Output trace parameters (level, output, options etc) to stream.
    */
  static ostream & PrintInfo(
    ostream & strm,
    bool crlf = true
  );

  /** Begin a trace output.
      If the trace stream output is used outside of the provided macros, it
      should be noted that a mutex is obtained on the call to Begin() which
      will prevent any other threads from using the trace stream until the
      End() function is called.

      So a typical usage would be:
      <pre><code>
        ostream & s = PTrace::Begin(3, __FILE__, __LINE__);
        s << "hello";
        if (want_there)
          s << " there";
        s << '!' << PTrace::End;
      </code></pre>
  */
  static ostream & Begin(
    unsigned level,         ///< Log level for output
    const char * fileName,  ///< Filename of source file being traced
    int lineNum,            ///< Line number of source file being traced.
    const PObject * instance = NULL,  ///< Instance for object logging occurred in
    const char * module = NULL        ///< Module or subsection string
  );

  static ostream & Begin(
    unsigned level,           ///< Log level for output
    const char * fileName,    ///< Filename of source file being traced
    int lineNum,              ///< Line number of source file being traced.
    const char * module,      ///< Module or subsection string
    const PObject * instance, ///< Instance for object logging occurred in
    const char * defModule
  ) { return Begin(level, fileName, lineNum, instance, module != NULL ? module : defModule); }

  static ostream & Begin(
    unsigned level,           ///< Log level for output
    const char * fileName,    ///< Filename of source file being traced
    int lineNum,              ///< Line number of source file being traced.
    const PObject * instance, ///< Instance for object logging occurred in
    const PObject * defInstance,
    const char * module       ///< Module or subsection string
  ) { return Begin(level, fileName, lineNum, instance != NULL ? instance : defInstance, module); }

  /** End a trace output.
      If the trace stream output is used outside of the provided macros, the
      End() function must be used at the end of the section of trace
      output. A mutex is obtained on the call to Begin() which will prevent
      any other threads from using the trace stream until the End(). The
      End() is used in a similar manner to <code>std::endl</code> or
      <code>std::flush</code>.

      So a typical usage would be:
      <pre><code>
        ostream & s = PTrace::Begin();
        s << "hello";
        if (want_there)
          s << " there";
        s << '!' << PTrace::End;
      </code></pre>
  */
  static ostream & End(
    ostream & strm ///< Trace output stream being completed
  );

  /** Class to trace Execution blocks.
      This class is used for tracing the entry and exit of program blocks. Upon
      construction it outputs an entry trace message and on destruction outputs an
      exit trace message. This is normally only used from in the <code>PTRACE_BLOCK()</code> macro.
  */
  class Block {
    public:
      /** Output entry trace message. */
      Block(
        const char * fileName, ///< Filename of source file being traced
        int lineNum,           ///< Line number of source file being traced.
        const char * traceName ///< String to be output with trace, typically it is the function name.
      );
      Block(const Block & obj);

      /// Output exit trace message.
      ~Block();

    protected:
      Block & operator=(const Block &)
      { return *this; }
      const char * file;
      int          line;
      const char * name;
  };

  /** Class to reduce noise level for some logging.
      A log is emitted at the lowLevel every interval milliseconds. All logs
      within the time interval are emitted at highLevel.

      An optional log output of the count of times the log occurred during may
      achived by simply including the throttle instance at the end of the log,
      e.g.
      <pre><code>
        PTRACE_THROTTLE_STATIC(m_throttleIt, 2, 2000);
        PTRACE(m_throttleIt, "A very frequent log" << m_throttleIt);
      </code></pre>

      The maxShown parameter indicates the maximum number of trace outputs that
      can occur in the time interval.

      Additional note: the "repeated" count optionally output is not 100% accurate
      in multi-threaded usage.
    */
  class ThrottleBase
  {
    public:
      ThrottleBase(
        unsigned lowLevel,          ///< Level at which low frequency logs made
        unsigned interval = 60000,  ///< Time between low frequency logs
        unsigned highLevel = 6,     ///< Level for high frequency (every) logs
        unsigned maxShown = 1       ///< Max shown messages in time interval
      );
      ThrottleBase(const ThrottleBase & other);
      ThrottleBase & operator=(const ThrottleBase & other);

      bool CanTrace(int64_t now = 0);
      operator unsigned() const { return m_currentLevel; }

      friend ostream & operator<<(ostream & strm, const ThrottleBase & throttle);

      __inline unsigned GetLowLevel() const { return m_lowLevel; }
      __inline unsigned GetHighLevel() const { return m_highLevel; }
      __inline unsigned GetCurrentLevel() const { return m_currentLevel; }
      __inline unsigned GetHiddenCount() const { return m_hiddenCount; }

    protected:
      unsigned         m_interval;
      unsigned         m_lowLevel;
      unsigned         m_highLevel;
      unsigned         m_maxShown;
      atomic<unsigned> m_currentLevel;
      atomic<int64_t>  m_nextLog;
      atomic<unsigned> m_repeatCount;
      atomic<unsigned> m_hiddenCount;
  };

  /** Template class to reduce noise level for some logging.
      This is primarily to set configuration values to compile time defaults
      without needing to be in ctor. */
  template <unsigned lowLevel,          ///< Level at which low frequency logs made
            unsigned interval = 60000,  ///< Time between low frequency logs
            unsigned highLevel = 6,     ///< Level for high frequency (every) logs
            unsigned maxShown = 1       ///< Max repeated message in time interval
           > struct Throttle : ThrottleBase
  {
    Throttle() : ThrottleBase(lowLevel, interval, highLevel, maxShown) { }
  };

  /** Output logging for an object.
    */
  template <class CLS> class InternalLogObject
  {
    const CLS & m_obj;
  public:
    inline InternalLogObject(const CLS & obj) : m_obj(obj) { }
    friend inline ostream & operator<<(ostream & strm, const InternalLogObject & log) { log.m_obj.ToLogging(strm); return strm; }
  };
  template <class CLS> static inline InternalLogObject<CLS> LogObject(const CLS & obj) { return InternalLogObject<CLS>(obj); }


  static bool CanTrace(const ThrottleBase & throttle) { return const_cast<ThrottleBase &>(throttle).CanTrace(); }

  static void WalkStack(
    ostream & strm,
    PThreadIdentifier id = PNullThreadIdentifier,
    PUniqueThreadIdentifier uid = 0,
    bool noSymbols = false
  );

  static unsigned MaxStackWalk; // Default 20

#if PTRACING==2
  static unsigned GetNextContextIdentifier();
#endif
};

/* Macro to conditionally declare a parameter to a function to avoid compiler
   warning due that parameter only being used in a <code>PTRACE()</code> */
#define PTRACE_PARAM(...) __VA_ARGS__

/** Trace an execution block.
This macro creates a trace variable for tracking the entry and exit of program
blocks. It creates an instance of the PTraceBlock class that will output a
trace message at the line <code>PTRACE_BLOCK()</code> is called and then on exit from the
scope it is defined in.
*/
#define PTRACE_BLOCK(name) PTrace::Block __trace_block_instance(__FILE__, __LINE__, name)

/** Trace the execution of a line.
This macro outputs a trace of a source file line execution.
*/
#define PTRACE_LINE() \
    (PTrace::CanTrace(1)) ? (void)(PTrace::Begin(1, __FILE__, __LINE__) << __FILE__ << '(' << __LINE__ << ')' << PTrace::End) : (void)0



#define PTRACE_INTERNAL(level, condition, args, ...) \
    (PTrace::CanTrace(level) condition) ? ((PTrace::Begin(level, __FILE__, __LINE__, __VA_ARGS__) << args << PTrace::End), true) : false

#define PTRACE_NO_CONDITION


#define PTRACE_PART1(narg, args) PTRACE_PART2(narg, args)
#define PTRACE_PART2(narg, args) PTRACE_ARG_##narg args

#define PTRACE_ARG_4(level, object, module, args) \
        PTRACE_INTERNAL(level, PTRACE_NO_CONDITION, args, object, module)

#define PTRACE_ARG_3(level, objectOrModule, args) \
        PTRACE_INTERNAL(level, PTRACE_NO_CONDITION, args, objectOrModule, PTraceObjectInstance(objectOrModule), PTraceModule())

#define PTRACE_ARG_2(level, args) \
        PTRACE_INTERNAL(level, PTRACE_NO_CONDITION, args, PTraceObjectInstance(), PTraceModule())


#define PTRACE_IF_PART1(narg, args) PTRACE_IF_PART2(narg, args)
#define PTRACE_IF_PART2(narg, args) PTRACE_IF_ARG_##narg args

#define PTRACE_IF_ARG_5(level, condition, object, module, args) \
        PTRACE_INTERNAL(level, && (condition), args, object, module)

#define PTRACE_IF_ARG_4(level, condition, objectOrModule, args) \
        PTRACE_INTERNAL(level, && (condition), args, objectOrModule, PTraceObjectInstance(objectOrModule), PTraceModule())

#define PTRACE_IF_ARG_3(level, condition, args) \
        PTRACE_INTERNAL(level, && (condition), args, PTraceObjectInstance(), PTraceModule())


#define PTRACE_BEGIN_PART1(narg, args) PTRACE_BEGIN_PART2(narg, args)
#define PTRACE_BEGIN_PART2(narg, args) PTRACE_BEGIN_ARG_##narg args

#define PTRACE_BEGIN_ARG_3(level, object, module) \
      PTrace::Begin(level, __FILE__, __LINE__, object, module)

#define PTRACE_BEGIN_ARG_2(level, objectOrModule) \
      PTrace::Begin(level, __FILE__, __LINE__, objectOrModule, PTraceObjectInstance(objectOrModule), PTraceModule())

#define PTRACE_BEGIN_ARG_1(level) \
      PTrace::Begin(level, __FILE__, __LINE__, PTraceObjectInstance(), PTraceModule())


// Backward compatibility
#define PTRACE2(level, object, args) \
        PTRACE_INTERNAL(level, PTRACE_NO_CONDITION, args, object, PTraceModule())
#define PTRACE_IF2(level, condition, object, args) \
  PTRACE_INTERNAL(level, && (condition), args, object, PTraceModule())

/** Output trace.
This macro outputs a trace of any information needed, using standard stream
output operators. The output is only made if the trace level set by the
SetLevel() function is greater than or equal to the first argument.

There can be variable numbers of arguments to this macro. Its full form is:

  PTRACE(level, instance, module, stream)

The level is the level for this trace log, instance is a PObject instance to
generate a context for the trace, module is a string representing a subsytem
for filtering and stream is a standard C++ stream output expression.

Both instance and module can by NULL, or absent. If only one of instance or
module is present it is determined by its type (PObject * or char *) as to
which it is.

The stream is only evaluated if level is below the PTrace::GetLevel()
threshold, so do not expect any functions to always be executed.

Note: If this is used with a static function of a PObject descendant there
will be an issue with the default usage of PTraceObjectInstance(). To avoid
the issue you will need to make sure the PTRACE macro has four parameters, as
in the full form descibed above.

The general policy for levels in trace logs is:
  Level 0 - Fatal error - program will likely crash
  Level 1 - Error       - program should continue, but call will likely fail.
  Level 2 - Warning     - something not right, call might continue with something missing
  Level 3 - Info        - all should be well, but enough information to track something odd
  Level 4 - Debug1      - More info on what's ahppening, including protocol packet dumps
  Level 5 - Debug2      - A lot more information, including more details on protocol
  Level 6 - Debug3      - A ridiculous amount of debugging, includes media packets.
  Level 7 - Debug4      - A hard disk filler. Used only to track down the nastiest of problems
*/
#define PTRACE(...) PTRACE_PART1(PARG_COUNT(__VA_ARGS__), (__VA_ARGS__))

/** Output trace on condition.
This macro conditionally outputs a trace of any information needed.

This macro has variable arguments, and is of the form:

  PTRACE_IF(level, condition, instance, module, stream)

Note condition is only evaluated if level is below the PTrace::GetLevel()
threshold, and stream is only evaluated if condition is evaluated to true.

See PTRACE() for more information on level, instance, module and stream.
*/
#define PTRACE_IF(...) PTRACE_IF_PART1(PARG_COUNT(__VA_ARGS__), (__VA_ARGS__))

/**Execute trace precisely once, and no more.

Note, this will execute once only if the current log level is sufficient.
Thus, if log level is increased it will still execute even if the statement
was passed previously.

See PTRACE() for more information on level, instance, module and stream.
*/
#define PTRACE_ONCE(level, ...) \
    { \
      static PAtomicBoolean firstTrace(true); \
      PTRACE_IF((level), firstTrace.TestAndSet(false), __VA_ARGS__); \
    }

/** Begin output trace.
This macro returns a ostream & for trace output.

This macro has variable arguments, and is of the form:

  PTRACE_BEGIN(level, instance, module)

See PTRACE() for more information on level, instance, module.
*/
#define PTRACE_BEGIN(...) PTRACE_BEGIN_PART1(PARG_COUNT(__VA_ARGS__), (__VA_ARGS__))


/* Macro to create a throttle context for use in <code>PTRACE()</code> */
#define PTRACE_THROTTLE(var, ...) PTrace::Throttle<__VA_ARGS__> var
#define PTRACE_THROTTLE_STATIC(var, ...) static PTrace::Throttle<__VA_ARGS__> var


__inline const PObject * PTraceObjectInstance() { return NULL; }
__inline static const PObject * PTraceObjectInstance(const void *) { return NULL; }
__inline const char * PTraceModule() { return NULL; }


#if PTRACING==2

/**Propagate PTRACE context identifier in an object from another.
   The context identifier can group objects together with a single
   identifier which can aid in debugging highly threaded systems where
   the logs from various threads become very interleaved.
  */
#define PTRACE_CONTEXT_ID_NEW() SetTraceContextIdentifier(PTrace::GetNextContextIdentifier())
#define PTRACE_CONTEXT_ID_SET(to, from) PObject::CopyTraceContextIdentifier(to, from)
#define PTRACE_CONTEXT_ID_FROM(obj) SetTraceContextIdentifier(obj)
#define PTRACE_CONTEXT_ID_TO(obj) CopyTraceContextIdentifier(obj)

class PTraceSaveContextIdentifier
{
  private:
    class PThread * m_currentThread;
    unsigned        m_savedContextIdentifier;
  public:
    PTraceSaveContextIdentifier(const PObject & obj);
    PTraceSaveContextIdentifier(const PObject * obj);
    ~PTraceSaveContextIdentifier();
};

#define PTRACE_CONTEXT_ID_PUSH_THREAD(obj) PTraceSaveContextIdentifier ptraceSavedContextIdentifier(obj)

#endif // PTRACING==2


#define P_DECLARE_TRACED_ENUM    P_DECLARE_STREAMABLE_ENUM
#define P_DECLARE_TRACED_ENUM_EX P_DECLARE_STREAMABLE_ENUM_EX

#endif // PTRACING

#ifndef PTRACE_ARGLIST_EXT
#define PTRACE_ARGLIST_EXT(...) ""
#endif

#ifndef PTRACE_ARGLIST
#define PTRACE_ARGLIST ""
#endif

#ifndef PTRACE_INITIALISE
#define PTRACE_INITIALISE(...)
#endif

#ifndef PTRACE_PARAM
#define PTRACE_PARAM(...)
#endif

#ifndef PTRACE_BLOCK
#define PTRACE_BLOCK(n)
#endif

#ifndef PTRACE_LINE
#define PTRACE_LINE()
#endif

#ifndef PTRACE
#define PTRACE(...)
#endif

#ifndef PTRACE_IF
#define PTRACE_IF(...)
#endif

#ifndef PTRACE_BEGIN
#define PTRACE_BEGIN(...)
#endif

#ifndef PTRACE_THROTTLE
#define PTRACE_THROTTLE(...)
#endif

#ifndef PTRACE2
#define PTRACE2(level, obj, arg)
#endif

#ifndef PTRACE_IF2
#define PTRACE_IF2(level, cond, obj, args)
#endif

#ifndef PTRACE_CONTEXT_ID_NEW
#define PTRACE_CONTEXT_ID_NEW()
#endif

#ifndef PTRACE_CONTEXT_ID_SET
#define PTRACE_CONTEXT_ID_SET(to, from)
#endif

#ifndef PTRACE_CONTEXT_ID_FROM
#define PTRACE_CONTEXT_ID_FROM(obj)
#endif

#ifndef PTRACE_CONTEXT_ID_TO
#define PTRACE_CONTEXT_ID_TO(obj)
#endif

#ifndef PTRACE_CONTEXT_ID_PUSH_THREAD
#define PTRACE_CONTEXT_ID_PUSH_THREAD(obj)
#endif

#ifndef P_DECLARE_TRACED_ENUM
#define P_DECLARE_TRACED_ENUM P_DECLARE_ENUM
#endif

#ifndef P_DECLARE_TRACED_ENUM_EX
#define P_DECLARE_TRACED_ENUM_EX P_DECLARE_ENUM_EX
#endif


#endif // PTLIB_TRACING_H


// End Of File ///////////////////////////////////////////////////////////////
