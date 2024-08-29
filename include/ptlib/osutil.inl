/*
 * osutil.inl
 *
 * Operating System Classes Inline Function Definitions
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
 *
 * $Id: osutil.inl 19426 2008-02-09 03:19:51Z rjongbloed $
 */

#include "ptlib.h"

///////////////////////////////////////////////////////////////////////////////
// PTimeInterval

__inline PTimeInterval::PTimeInterval(int64_t millisecs)
  : m_nanoseconds(millisecs*MilliToNano) { }

__inline PTimeInterval::PTimeInterval(const std::chrono::nanoseconds& nsecs)
  : m_nanoseconds(nsecs.count()) { }

__inline PTimeInterval::PTimeInterval(const std::chrono::microseconds& usecs)
  : PTimeInterval(std::chrono::duration_cast<std::chrono::nanoseconds>(usecs)) { }

__inline PTimeInterval::PTimeInterval(const std::chrono::milliseconds& msecs)
  : PTimeInterval(std::chrono::duration_cast<std::chrono::nanoseconds>(msecs)) { }

__inline PTimeInterval::PTimeInterval(const std::chrono::seconds& secs)
  : PTimeInterval(std::chrono::duration_cast<std::chrono::nanoseconds>(secs)) { }

__inline PTimeInterval::PTimeInterval(const std::chrono::minutes & mins)
  : PTimeInterval(std::chrono::duration_cast<std::chrono::nanoseconds>(mins)) { }

__inline PTimeInterval::PTimeInterval(const std::chrono::hours & hours)
  : PTimeInterval(std::chrono::duration_cast<std::chrono::nanoseconds>(hours)) { }

__inline PTimeInterval::PTimeInterval(const std::chrono::days & days)
  : PTimeInterval(std::chrono::duration_cast<std::chrono::nanoseconds>(days)) { }

__inline PTimeInterval::PTimeInterval(const PTimeInterval & other)
  : m_nanoseconds(other.InternalGet()) { }

__inline PTimeInterval & PTimeInterval::operator=(const PTimeInterval & other)
  { InternalSet(other.InternalGet()); return *this; }

__inline PObject * PTimeInterval::Clone() const
  { return PNEW PTimeInterval(GetMilliSeconds()); }

__inline PTimeInterval PTimeInterval::Frequency(double frequency)
  { PTimeInterval t; t.SetFrequency(frequency); return t; }

__inline PTimeInterval PTimeInterval::NanoSeconds(int64_t nsecs, int secs)
  { PTimeInterval t; t.SetNanoSeconds(nsecs, secs); return t; }

__inline PTimeInterval PTimeInterval::MicroSeconds(int64_t usecs, int secs)
  { PTimeInterval t; t.SetMicroSeconds(usecs, secs); return t; }

__inline PTimeInterval PTimeInterval::Seconds(double secs)
  { PTimeInterval t; t.SetSeconds(secs); return t; }

__inline int64_t PTimeInterval::GetNanoSeconds() const
  { return InternalGet(); }

__inline void PTimeInterval::SetNanoSeconds(int64_t nsecs, int secs)
  { InternalSet(nsecs+secs*SecsToNano); }

__inline int64_t PTimeInterval::GetMicroSeconds() const
  { return InternalGet()/MicroToNano; }

__inline void PTimeInterval::SetMicroSeconds(int64_t usecs, int secs)
  { InternalSet(usecs*MicroToNano+secs*SecsToNano); }

__inline int64_t PTimeInterval::GetMilliSeconds() const
  { return InternalGet()/MilliToNano; }

__inline void PTimeInterval::SetMilliSeconds(int64_t msecs)
  { InternalSet(msecs*MilliToNano); }

__inline double PTimeInterval::GetFrequency() const
  { return 1000000000.0/InternalGet(); }

__inline void PTimeInterval::SetFrequency(double frequency)
  { InternalSet((int64_t)(SecsToNano/frequency)); }

__inline long PTimeInterval::GetSeconds() const
  { return (long)(InternalGet()/SecsToNano); }

__inline double PTimeInterval::GetSecondsAsDouble() const
  { return InternalGet()/(double)SecsToNano; }

__inline void PTimeInterval::SetSeconds(double secs)
  { InternalSet((int64_t)(secs*SecsToNano)); }

__inline long PTimeInterval::GetMinutes() const
  { return (long)(InternalGet()/MinsToNano); }

__inline int PTimeInterval::GetHours() const
  { return (int)(InternalGet()/HoursToNano); }

__inline int PTimeInterval::GetDays() const
  { return (int)(InternalGet()/DaysToNano); }


__inline PTimeInterval PTimeInterval::operator-() const
  { return PTimeInterval::NanoSeconds(-GetNanoSeconds()); }

__inline PTimeInterval PTimeInterval::operator+(const PTimeInterval & t) const
  { return PTimeInterval::NanoSeconds(GetNanoSeconds() + t.GetNanoSeconds()); }

__inline PTimeInterval operator+(int64_t left, const PTimeInterval & right)
  { return PTimeInterval(left + right.GetMilliSeconds()); }

__inline PTimeInterval & PTimeInterval::operator+=(const PTimeInterval & t)
  { SetNanoSeconds(GetNanoSeconds() + t.GetNanoSeconds()); return *this; }

__inline PTimeInterval PTimeInterval::operator-(const PTimeInterval & t) const
  { return PTimeInterval::NanoSeconds(GetNanoSeconds() - t.GetNanoSeconds()); }

__inline PTimeInterval operator-(int64_t left, const PTimeInterval & right)
  { return PTimeInterval(left - right.GetMilliSeconds()); }

__inline PTimeInterval & PTimeInterval::operator-=(const PTimeInterval & t)
  { SetNanoSeconds(GetNanoSeconds() - t.GetNanoSeconds()); return *this; }

__inline PTimeInterval PTimeInterval::operator*(int f) const
  { return PTimeInterval::NanoSeconds(GetNanoSeconds() * f); }

__inline PTimeInterval operator*(int left, const PTimeInterval & right)
  { return PTimeInterval::NanoSeconds(left * right.GetNanoSeconds()); }

__inline PTimeInterval & PTimeInterval::operator*=(int f)
  { SetNanoSeconds(GetNanoSeconds() * f); return *this; }

__inline int PTimeInterval::operator/(const PTimeInterval & t) const
  { return (int)(GetNanoSeconds() / t.GetNanoSeconds()); }

__inline PTimeInterval PTimeInterval::operator/(int f) const
  { return PTimeInterval::NanoSeconds(GetNanoSeconds() / f); }

__inline PTimeInterval operator/(int64_t left, const PTimeInterval & right)
  { return PTimeInterval::NanoSeconds(left / right.GetNanoSeconds()); }

__inline PTimeInterval & PTimeInterval::operator/=(int f)
  { SetNanoSeconds(GetNanoSeconds() / f); return *this; }


__inline bool PTimeInterval::operator==(const PTimeInterval & t) const
  { return GetNanoSeconds() == t.GetNanoSeconds(); }

__inline bool PTimeInterval::operator!=(const PTimeInterval & t) const
  { return GetNanoSeconds() != t.GetNanoSeconds(); }

__inline bool PTimeInterval::operator> (const PTimeInterval & t) const
  { return GetNanoSeconds() > t.GetNanoSeconds(); }

__inline bool PTimeInterval::operator>=(const PTimeInterval & t) const
  { return GetNanoSeconds() >= t.GetNanoSeconds(); }

__inline bool PTimeInterval::operator< (const PTimeInterval & t) const
  { return GetNanoSeconds() < t.GetNanoSeconds(); }

__inline bool PTimeInterval::operator<=(const PTimeInterval & t) const
  { return GetNanoSeconds() <= t.GetNanoSeconds(); }

__inline bool PTimeInterval::operator==(long msecs) const
  { return GetMilliSeconds() == (int64_t)msecs; }

__inline bool PTimeInterval::operator!=(long msecs) const
  { return GetMilliSeconds() != (int64_t)msecs; }

__inline bool PTimeInterval::operator> (long msecs) const
  { return GetMilliSeconds() > (int64_t)msecs; }

__inline bool PTimeInterval::operator>=(long msecs) const
  { return GetMilliSeconds() >= (int64_t)msecs; }

__inline bool PTimeInterval::operator< (long msecs) const
  { return GetMilliSeconds() < (int64_t)msecs; }

__inline bool PTimeInterval::operator<=(long msecs) const
  { return GetMilliSeconds() <= (int64_t)msecs; }


///////////////////////////////////////////////////////////////////////////////
// PTime

__inline PTime::PTime(const PTime & other)
  : m_microSecondsSinceEpoch(other.m_microSecondsSinceEpoch.load()) { }

__inline PTime & PTime::operator=(const PTime & other)
  { m_microSecondsSinceEpoch.store(other.m_microSecondsSinceEpoch.load()); return *this; }

__inline PObject * PTime::Clone() const
  { return PNEW PTime(*this); }

__inline void PTime::SetCurrentTime()
  { m_microSecondsSinceEpoch.store(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count()); }

__inline bool PTime::IsValid() const
  { return m_microSecondsSinceEpoch.load() > 46800000000LL; }

__inline int64_t PTime::GetTimestamp() const
  { return m_microSecondsSinceEpoch.load(); }

__inline PTime & PTime::SetTimestamp(time_t seconds, int64_t usecs)
  { m_microSecondsSinceEpoch.store(seconds*Micro + usecs); return *this; }

__inline PTime & PTime::AddTimestamp(int64_t usecs)
 { m_microSecondsSinceEpoch += usecs; return *this; }

__inline time_t PTime::GetTimeInSeconds() const
  { return m_microSecondsSinceEpoch.load()/Micro; }

__inline unsigned PTime::GetMicrosecond() const
  { return m_microSecondsSinceEpoch.load()%Micro; }

__inline PTimeInterval PTime::GetElapsed() const
  { return IsValid() ? (PTime() - *this) : 0; }

__inline bool PTime::IsPast() const
  { return GetElapsed() > 0; }

__inline bool PTime::IsFuture() const
  { return GetElapsed() < 0; }


__inline PString PTime::AsString(const PString & format, int zone) const
  { return AsString((const char *)format, zone); }

__inline int PTime::GetTimeZone() 
  { return GetTimeZone(IsDaylightSavings() ? DaylightSavings : StandardTime); }

__inline PTime PTime::operator+(const PTimeInterval & t) const
  { return PTime(0, GetTimestamp() +  t.GetMicroSeconds()); }

__inline PTime & PTime::operator+=(const PTimeInterval & t)
  { m_microSecondsSinceEpoch += t.GetMicroSeconds(); return *this; }

__inline PTimeInterval PTime::operator-(const PTime & t) const
  { return PTimeInterval::MicroSeconds(GetTimestamp() - t.GetTimestamp()); }

__inline PTime PTime::operator-(const PTimeInterval & t) const
  { return PTime(0, GetTimestamp() - t.GetMicroSeconds()); }

__inline PTime & PTime::operator-=(const PTimeInterval & t)
  { m_microSecondsSinceEpoch -= t.GetMicroSeconds(); return *this; }


#if P_TIMERS
///////////////////////////////////////////////////////////////////////////////
// PSimpleTimer

__inline void PSimpleTimer::Stop()
  { SetInterval(0); }

__inline PTimeInterval PSimpleTimer::GetElapsed() const
  { return PTimer::Tick() - m_startTick; }

__inline bool PSimpleTimer::IsRunning() const
  { return (PTimer::Tick() - m_startTick) < *this; }

__inline bool PSimpleTimer::HasExpired() const
  { return (PTimer::Tick() - m_startTick) >= *this; }

__inline PSimpleTimer::operator bool() const
  { return HasExpired(); }


///////////////////////////////////////////////////////////////////////////////
// PTimer

__inline PTimeInterval PTimer::GetResetTime() const
  { return PTimeInterval::NanoSeconds(PTimeInterval::InternalGet()); }

__inline const PNotifier & PTimer::GetNotifier() const
  { return m_callback; }

__inline void PTimer::SetNotifier(const PNotifier & func, const PString & threadName)
  { m_callback = func; m_threadName = threadName; }

__inline PTimeInterval PTimer::Tick()
{
  return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch());
}

__inline float PTimer::Resolution()
{
  return float(MilliToNano)/std::chrono::high_resolution_clock::period::den;
}

#endif // P_TIMERS

///////////////////////////////////////////////////////////////////////////////

__inline PChannelStreamBuffer::PChannelStreamBuffer(const PChannelStreamBuffer & sbuf)
  : channel(sbuf.channel) { }

__inline PChannelStreamBuffer &
          PChannelStreamBuffer::operator=(const PChannelStreamBuffer & sbuf)
  { channel = sbuf.channel; return *this; }

__inline PChannel::PChannel(const PChannel &)
  : std::iostream(cout.rdbuf())
  { PAssertAlways("Cannot copy channels"); }

__inline PChannel & PChannel::operator=(const PChannel &)
  { PAssertAlways("Cannot assign channels"); return *this; }

__inline void PChannel::SetReadTimeout(const PTimeInterval & time)
  { readTimeout = time; }

__inline PTimeInterval PChannel::GetReadTimeout() const
  { return readTimeout; }

__inline void PChannel::SetWriteTimeout(const PTimeInterval & time)
  { writeTimeout = time; }

__inline PTimeInterval PChannel::GetWriteTimeout() const
  { return writeTimeout; }

__inline intptr_t PChannel::GetHandle() const
  { return os_handle; }

__inline int PChannel::GetOSHandleAsInt() const
  { return static_cast<int>(GetHandle()); }

__inline void PChannel::AbortCommandString()
  { m_abortCommandString = true; }


///////////////////////////////////////////////////////////////////////////////
// PIndirectChannel

__inline PIndirectChannel::~PIndirectChannel()
  { Close(); }

__inline PChannel * PIndirectChannel::GetReadChannel() const
  { return readChannel; }

__inline PChannel * PIndirectChannel::GetWriteChannel() const
  { return writeChannel; }


///////////////////////////////////////////////////////////////////////////////
// PDirectory

__inline PDirectory & PDirectory::operator=(const PDirectory & str)
  { Close(); PFilePathString::operator=(str); return *this; }

__inline PDirectory & PDirectory::operator=(const PString & str)
  { Close(); PFilePathString::operator=(PFilePath::Canonicalise(str, true)); return *this; }

__inline PDirectory & PDirectory::operator=(const char * cstr)
  { Close(); PFilePathString::operator=(PFilePath::Canonicalise(cstr, true)); return *this; }


__inline bool PDirectory::Exists() const
  { return Exists(*this); }

__inline bool PDirectory::Change() const
  { return Change(*this); }

__inline bool PDirectory::Create(int perm, bool recurse) const
  { return Create(*this, perm, recurse); }

__inline bool PDirectory::Remove()
  { Close(); return Remove(*this); }


///////////////////////////////////////////////////////////////////////////////

__inline PFilePath::PFilePath()
  { }

__inline PFilePath::PFilePath(const PFilePath & path)
  : PFilePathString(path) { }

__inline PFilePath & PFilePath::operator=(const PFilePath & path)
  { PFilePathString::operator=(path); return *this; }

__inline PFilePath & PFilePath::operator=(const PString & str)
  { PFilePathString::operator=(Canonicalise(str, false)); return *this; }

__inline PFilePath & PFilePath::operator=(const char * cstr)
  { PFilePathString::operator=(Canonicalise(cstr, false)); return *this; }

__inline PFilePath & PFilePath::operator+=(const PString & str)
  { PFilePathString::operator=(Canonicalise(*this + str, false)); return *this; }

__inline PFilePath & PFilePath::operator+=(const char * cstr)
  { PFilePathString::operator=(Canonicalise(*this + cstr, false)); return *this; }


///////////////////////////////////////////////////////////////////////////////

__inline PFile::PFile()
  { os_handle = -1; m_removeOnClose = false; }

__inline PFile::PFile(OpenMode mode, OpenOptions opts)
  { os_handle = -1; m_removeOnClose = false; Open(mode, opts); }

__inline PFile::PFile(const PFilePath & name, OpenMode mode, OpenOptions opts)
  { os_handle = -1; m_removeOnClose = false; Open(name, mode, opts); }


__inline bool PFile::Exists() const
  { return Exists(m_path); }

__inline bool PFile::Access(OpenMode mode)
  { return ConvertOSError(Access(m_path, mode) ? 0 : -1); }

__inline bool PFile::Touch(const PFilePath & name, const PTime & accessTime)
  { return Touch(name, accessTime, accessTime); }

__inline bool PFile::Touch(const PTime & accessTime)
  { return ConvertOSError(Touch(m_path, accessTime) ? 0 : -1); }

__inline bool PFile::Touch(const PTime & accessTime, const PTime & modTime)
  { return ConvertOSError(Touch(m_path, accessTime, modTime) ? 0 : -1); }

__inline bool PFile::Remove(bool force)
  { Close(); return ConvertOSError(Remove(m_path, force) ? 0 : -1); }

__inline bool PFile::Copy(const PFilePath & newname, bool force, bool recurse)
  { return ConvertOSError(Copy(m_path, newname, force, recurse) ? 0 : -1); }

__inline bool PFile::GetInfo(PFileInfo & info)
  { return ConvertOSError(GetInfo(m_path, info) ? 0 : -1); }

__inline bool PFile::SetPermissions(PFileInfo::Permissions permissions)
  { return ConvertOSError(SetPermissions(m_path, permissions) ? 0 : -1); }


__inline const PFilePath & PFile::GetFilePath() const
  { return m_path; }
      

__inline PString PFile::GetName() const
  { return m_path; }

__inline off_t PFile::GetPosition() const
  { return _lseek(GetOSHandleAsInt(), 0, SEEK_CUR); }


///////////////////////////////////////////////////////////////////////////////

__inline PTextFile::PTextFile()
  { }

__inline PTextFile::PTextFile(OpenMode mode, OpenOptions opts)
  { Open(mode, opts); }

__inline PTextFile::PTextFile(const PFilePath & name, OpenMode mode, OpenOptions opts)
  { Open(name, mode, opts); }


///////////////////////////////////////////////////////////////////////////////
// PConfig

#ifdef P_CONFIG_FILE

__inline PConfig::PConfig(Source src)
  : defaultSection(DefaultSectionName()) { Construct(src, PString::Empty(), PString::Empty()); }

__inline PConfig::PConfig(Source src, const PString & appname)
  : defaultSection(DefaultSectionName()) { Construct(src, appname, PString::Empty()); }

__inline PConfig::PConfig(Source src, const PString & appname, const PString & manuf)
  : defaultSection(DefaultSectionName()) { Construct(src, appname, manuf); }

__inline PConfig::PConfig(const PString & section, Source src)
  : defaultSection(section) { Construct(src, PString::Empty(), PString::Empty()); }

__inline PConfig::PConfig(const PString & section, Source src, const PString & appname)
  : defaultSection(section) { Construct(src, appname, PString::Empty()); }

__inline PConfig::PConfig(const PString & section,
                         Source src,
                         const PString & appname,
                         const PString & manuf)
  : defaultSection(section) { Construct(src, appname, manuf); }

__inline PConfig::PConfig(const PFilePath & filename, const PString & section)
  : defaultSection(section) { Construct(filename); }

__inline void PConfig::SetDefaultSection(const PString & section)
  { defaultSection = section; }

__inline PString PConfig::GetDefaultSection() const
  { return defaultSection; }

__inline PStringArray PConfig::GetKeys() const
  { return GetKeys(defaultSection); }

__inline PStringToString PConfig::GetAllKeyValues() const
  { return GetAllKeyValues(defaultSection); }

__inline void PConfig::DeleteSection()
  { DeleteSection(defaultSection); }

__inline void PConfig::DeleteKey(const PString & key)
  { DeleteKey(defaultSection, key); }

__inline bool PConfig::HasKey(const PString & key) const
  { return HasKey(defaultSection, key); }

__inline PString PConfig::GetString(const PString & key) const
  { return GetString(defaultSection, key, PString()); }

__inline PString PConfig::GetString(const PString & key, const PString & dflt) const
  { return GetString(defaultSection, key, dflt); }

__inline void PConfig::SetString(const PString & key, const PString & value)
  { SetString(defaultSection, key, value); }

__inline bool PConfig::GetBoolean(const PString & key, bool dflt) const
  { return GetBoolean(defaultSection, key, dflt); }

__inline void PConfig::SetBoolean(const PString & key, bool value)
  { SetBoolean(defaultSection, key, value); }

__inline long PConfig::GetInteger(const PString & key, long dflt) const
  { return GetInteger(defaultSection, key, dflt); }

__inline void PConfig::SetInteger(const PString & key, long value)
  { SetInteger(defaultSection, key, value); }

__inline int64_t PConfig::GetInt64(const PString & key, int64_t dflt) const
  { return GetInt64(defaultSection, key, dflt); }

__inline void PConfig::SetInt64(const PString & key, int64_t value)
  { SetInt64(defaultSection, key, value); }

__inline double PConfig::GetReal(const PString & key, double dflt) const
  { return GetReal(defaultSection, key, dflt); }

__inline void PConfig::SetReal(const PString & key, double value)
  { SetReal(defaultSection, key, value); }

__inline PTime PConfig::GetTime(const PString & key) const
  { return GetTime(defaultSection, key); }

__inline PTime PConfig::GetTime(const PString & key, const PTime & dflt) const
  { return GetTime(defaultSection, key, dflt); }

__inline void PConfig::SetTime(const PString & key, const PTime & value)
  { SetTime(defaultSection, key, value); }


#endif // P_CONFIG_FILE


///////////////////////////////////////////////////////////////////////////////
// PArgList

__inline void PArgList::SetArgs(int argc, char ** argv)
  { SetArgs(PStringArray(argc, argv)); }

__inline bool PArgList::Parse(const PString & theArgumentSpec, bool optionsBeforeParams)
  { return Parse((const char *)theArgumentSpec, optionsBeforeParams); }

__inline bool PArgList::HasOption(char option) const
  { return GetOptionCount(option) != 0; }

__inline bool PArgList::HasOption(const char * option) const
  { return GetOptionCount(option) != 0; }

__inline bool PArgList::HasOption(const PString & option) const
  { return GetOptionCount(option) != 0; }

__inline PINDEX PArgList::GetCount() const
  { return m_parameterIndex.GetSize()-m_shift; }

__inline PString PArgList::operator[](PINDEX num) const
  { return GetParameter(num); }

__inline PArgList & PArgList::operator<<(int sh)
  { Shift(sh); return *this; }

__inline PArgList & PArgList::operator>>(int sh)
  { Shift(-sh); return *this; }


///////////////////////////////////////////////////////////////////////////////
// PThread

__inline PThreadIdentifier PThread::GetThreadId() const
  { return m_threadId; }

__inline PUniqueThreadIdentifier PThread::GetUniqueIdentifier() const
  { return m_uniqueId; }

__inline PThreadIdentifier PThread::GetCurrentThreadId()
  { return std::this_thread::get_id(); }

__inline void PThread::Sleep(const PTimeInterval& delay)
  { std::this_thread::sleep_for(delay.AsChronoNS()); }

__inline void PThread::Yield()
  { std::this_thread::yield(); }


///////////////////////////////////////////////////////////////////////////////
// PProcess

__inline PArgList & PProcess::GetArguments()
  { return m_arguments; }

__inline const PString & PProcess::GetManufacturer() const
  { return m_manufacturer; }

__inline const PString & PProcess::GetName() const
  { return m_productName; }

__inline const PFilePath & PProcess::GetFile() const
  { return m_executableFile; }

__inline int PProcess::GetMaxHandles() const
  { return m_maxHandles; }

__inline void PProcess::SetTerminationValue(int value)
  { m_terminationValue = value; }

__inline int PProcess::GetTerminationValue() const
  { return m_terminationValue; }

__inline void PThreadYield()
  { PThread::Yield(); }


// End Of File ///////////////////////////////////////////////////////////////
