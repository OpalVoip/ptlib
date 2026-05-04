/*
 * profiling.h
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

#ifndef PTLIB_PROFILING_H
#define PTLIB_PROFILING_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif


 ///////////////////////////////////////////////////////////////////////////////

/**Template class for integral types (including PTimeInterval) for calculating
   minimum, maximum and average. */
template <typename ValueType, typename AccumType = ValueType>
class PMinMaxAvg
{
protected:
    ValueType   m_minimum;
    ValueType   m_maximum;
    AccumType   m_accumulator;
    unsigned    m_count;
    std::string m_units;
public:
    explicit PMinMaxAvg(const char * units = "")
        : m_units(units)
    {
        Reset();
    }

    void Accumulate(const ValueType & value)
    {
        if (value < m_minimum)
            m_minimum = value;
        if (value > m_maximum)
            m_maximum = value;
        m_accumulator += value;
        ++m_count;
    }

    void Reset()
    {
        m_minimum = numeric_limits<ValueType>::max();
        m_maximum = 0;
        m_accumulator = 0;
        m_count = 0;
    }

    ValueType GetMinimum() const { return m_minimum; }
    ValueType GetMaximum() const { return m_maximum; }
    ValueType GetAverage() const { return m_count > 0 ? (ValueType)(m_accumulator/m_count) : 0; }
    unsigned  GetCount()   const { return m_count; }

    friend ostream & operator<<(ostream & strm, const PMinMaxAvg & mma)
    {
        return strm << " min=" << mma.GetMinimum() << mma.m_units << ","
                       " avg=" << mma.GetAverage() << mma.m_units << ","
                       " max=" << mma.GetMaximum() << mma.m_units;
    }
};


///////////////////////////////////////////////////////////////////////////////
// Profiling

#if defined( __GNUC__) && !defined(__clang__)
  #define PPROFILE_EXCLUDE(func)  func  __attribute__((no_instrument_function))
#else
  #define PPROFILE_EXCLUDE(func) func
  #ifdef _MSC_VER
    #define __PRETTY_FUNCTION__ __FUNCSIG__
  #else
    #define __PRETTY_FUNCTION__ __FUNCTION__
  #endif
#endif

class PThread;
class PTimeInterval;

namespace PProfiling
{
  __inline uint64_t GetCycles()
  {
#if defined(P_HAS_RDTSC)
    return __rdtsc();
#elif defined(__i386__) || defined(__x86_64__)
    uint32_t l,h;
    __asm__ __volatile__ ("rdtsc" : "=a"(l), "=d"(h));
    return ((uint64_t)h<<32)|l;
#elif defined(_WIN32)
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    return li.QuadPart;
#elif defined(CLOCK_MONOTONIC)
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec*1000000000ULL+ts.tv_nsec;
#else
    timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec*1000000ULL+tv.tv_usec;
#endif
  }

  PPROFILE_EXCLUDE(int64_t CyclesToNanoseconds(uint64_t cycles));
  PPROFILE_EXCLUDE(float CyclesToSeconds(uint64_t cycles));


#if P_PROFILING

  struct Function
  {
    unsigned    m_count;
    uint64_t    m_sum;
    uint64_t    m_minimum;
    uint64_t    m_maximum;

    Function()
      : m_count(0)
      , m_sum(0)
      , m_minimum(std::numeric_limits<uint64_t>::max())
      , m_maximum(0)
    {
    }
  };
  typedef std::map<std::string, Function> FunctionMap;

  struct Thread
  {
    std::string             m_name;
    std::thread::id         m_threadId;
    PUniqueThreadIdentifier m_uniqueId;
    float                   m_realTime;
    float                   m_systemCPU;
    float                   m_userCPU;
    bool                    m_running;
    FunctionMap             m_functions;

    Thread(
      std::thread::id threadId = std::thread::id(),
      PUniqueThreadIdentifier uniqueId = 0,
      const char * name = "",
      float realTime = 0,
      float systemCPU = 0,
      float userCPU = 0
    ) : m_name(name)
      , m_threadId(threadId)
      , m_uniqueId(uniqueId)
      , m_realTime(realTime)
      , m_systemCPU(systemCPU)
      , m_userCPU(userCPU)
      , m_running(false)
    {
    }
  };
  typedef std::map<PUniqueThreadIdentifier, Thread> ThreadByID;
  typedef std::multimap<float, Thread, std::greater<double> > ThreadByUsage; // percentage

  struct Analysis
  {
    uint64_t      m_durationCycles;
    unsigned      m_functionCount;
    ThreadByID    m_threadByID;
    ThreadByUsage m_threadByUsage;

    Analysis()
      : m_durationCycles(0)
      , m_functionCount(0)
    {
    }

    void ToText(ostream & strm) const;
    void ToHTML(ostream & strm) const;
  };

  void Analyse(Analysis & analysis);
  void Analyse(ostream & strm, bool html);

  PPROFILE_EXCLUDE(
    void Enable(bool enab)
  );
  PPROFILE_EXCLUDE(
    bool IsEnabled()
  );

  PPROFILE_EXCLUDE(
    void Reset()
  );
  PPROFILE_EXCLUDE(
    void Dump(ostream & strm)
  );

  PPROFILE_EXCLUDE(
    void OnThreadEnded(const PThread & thread, const PTimeInterval & realTime, const PTimeInterval & systemCPU, const PTimeInterval & userCPU)
  );

  PPROFILE_EXCLUDE(
    void PreSystem()
  );

  PPROFILE_EXCLUDE(
    void PostSystem()
  );

  class Block
  {
    public:
      PPROFILE_EXCLUDE(
        Block(
          const PDebugLocation & location
        )
      );
      PPROFILE_EXCLUDE(
        ~Block()
      );

    protected:
      PDebugLocation m_location;
  };

  #define PPROFILE_BLOCK(name) ::PProfiling::Block p_profile_block_instance(PDebugLocation(__FILE,__LINE__, name))
  #define PPROFILE_FUNCTION() PPROFILE_BLOCK(__PRETTY_FUNCTION__)

  #define PPROFILE_PRE_SYSTEM()  ::PProfiling::PreSystem()
  #define PPROFILE_POST_SYSTEM() ::PProfiling::PostSystem()
  #define PPROFILE_SYSTEM(...)  PPROFILE_PRE_SYSTEM(); __VA_ARGS__; PPROFILE_POST_SYSTEM()
#else
  #define PPROFILE_BLOCK(...)
  #define PPROFILE_FUNCTION()
  #define PPROFILE_PRE_SYSTEM()
  #define PPROFILE_POST_SYSTEM()
  #define PPROFILE_SYSTEM(...) __VA_ARGS__
#endif

#if PTRACING
  /**This class, along with the PPROFILE_TIMESCOPE() macro, allows the measurement of
     the time used by a section of code delimited by the scope (block till the close
     brace) with minimum, maximum and averages displayed as a PTRACE().
    */
  class TimeScope : PNonCopyable
  {
    protected:
      struct Implementation;
      Implementation * const m_implementation;

    public:
      /**Create a TimeScope instance. This is usually created as a static variable
         by the PPROFILE_TIMESCOPE() macro.
        */
      TimeScope(
        const PDebugLocation & location,  ///< Source location, typically from macro P_DEBUG_LOCATION
        unsigned thresholdTime,           ///< Threshold time in milliseconds
        unsigned throttleTime = 10000,    ///< Time between PTRACE output in milliseconds
        unsigned throttledLogLevel = 2,   ///< PTRACE level to use if enough samples are above thresholdTime
        unsigned unthrottledLogLevel = 6, ///< PTRACE level to use otherwise
        unsigned thresholdPercent = 5,    ///< Percentage of samples above thresholdTime to trigger throttledLogLevel
        unsigned maxHistory = 0           ///< Optional number of samples above thresholdTime to display sincle last PTRACE()
      );
      TimeScope(const TimeScope & other);

      /// Destroy scope timer
      ~TimeScope();

      /// Set time between PTRACE output in milliseconds
      void SetThrottleTime(unsigned throttleTime);

      /// Set PTRACE level to use if enough samples are above thresholdTime
      void SetThrottledLogLevel(unsigned throttledLogLevel);

      /// Set PTRACE level to use otherwise
      void SetUnthrottledLogLevel(unsigned unthrottledLogLevel);

      /// Set Percentage of samples above thresholdTime to trigger throttledLogLevel
      void SetThresholdPercent(unsigned thresholdPercent);

      /// Set optional number of samples above thresholdTime to display sincle last PTRACE()
      void SetMaxHistory(unsigned maxHistory);

      /// End and process a measurement period
      void EndMeasurement(
        const void * context,
        const PObject * object,
        const PDebugLocation * location,
        uint64_t startTime
      );

      // Get the last measurement duration. Must be after EndMeasurement() is called.
      const PTimeInterval & GetLastDuration() const;

      /** Class used by PPROFILE_TIMESCOPE() macro to do the measurement of
          time between construction and destruction.
        */
      class Measure
      {
        protected:
          TimeScope     &       m_scope;
          const void    * const m_context;
          const PObject * const m_object;
          uint64_t        const m_startCycle;

        public:
          Measure(TimeScope & scope, const PObject * object)
            : m_scope(scope)
            , m_context(object)
            , m_object(object)
            , m_startCycle(GetCycles())
          {
          }

          Measure(TimeScope & scope, const void * context)
            : m_scope(scope)
            , m_context(context)
            , m_object(NULL)
            , m_startCycle(GetCycles())
          {
          }

          ~Measure()
          {
            m_scope.EndMeasurement(m_context, m_object, NULL, m_startCycle);
          }
      };
  };

  #define PPROFILE_TIMESCOPE(name, context, ...) \
      static ::PProfiling::TimeScope p_profile_timescope_static_instance##name(PDebugLocation(__FILE__, __LINE__, #name), __VA_ARGS__); \
      ::PProfiling::TimeScope::Measure p_profile_timescope_instance##name(p_profile_timescope_static_instance##name, context)
#else
  #define PPROFILE_TIMESCOPE(...)
#endif // PTRACING

  struct HighWaterMarkData
  {
    std::string      m_name;
    atomic<unsigned> m_totalCount;
    atomic<unsigned> m_highWaterMark;

    HighWaterMarkData(const std::string & name);
    void NewInstance();
    static HighWaterMarkData & Get(const std::type_info & ti);
    static std::map<std::string, unsigned> Get();
  };

  template <class CLS> struct HighWaterMark
  {
    static HighWaterMarkData & GetHighWaterMarkData() { static HighWaterMarkData & data = HighWaterMarkData::Get(typeid(CLS)); return data; }
    HighWaterMark() { GetHighWaterMarkData().NewInstance(); }
    ~HighWaterMark() { --GetHighWaterMarkData().m_totalCount; }
  };
};


#endif // PTLIB_PROFILING_H


// End Of File ///////////////////////////////////////////////////////////////
