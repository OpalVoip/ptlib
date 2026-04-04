/*
 * ptlib.inl
 *
 * Operating System classes inline function implementation
 *
 * Portable Windows Library
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
 *
 * $Id: ptlib.inl 19008 2007-11-29 09:17:41Z rjongbloed $
 */

#if defined(P_LINUX)
#if (__GNUC_MINOR__ < 7 && __GNUC__ <= 2)
#include <localeinfo.h>
#else
#define P_USE_LANGINFO
#endif
#endif

#ifdef P_USE_LANGINFO
#include <langinfo.h>
#endif

PINLINE PProcessIdentifier PProcess::GetCurrentProcessID()
{
  return getpid();
}

///////////////////////////////////////////////////////////////////////////////

PINLINE unsigned PTimer::Resolution()
{
  return 1;
}

///////////////////////////////////////////////////////////////////////////////

PINLINE bool PDirectory::IsRoot() const
  { return IsSeparator((*this)[0]) && ((*this)[1] == '\0'); }

PINLINE PDirectory PDirectory::GetRoot() const
  { return PString(PDIR_SEPARATOR); }

PINLINE bool PDirectory::IsSeparator(char ch)
  { return ch == PDIR_SEPARATOR; }

PINLINE bool PDirectory::Change(const PString & p)
  { return chdir((char *)(const char *)p) == 0; }

///////////////////////////////////////////////////////////////////////////////

PINLINE PString PFilePath::GetVolume() const
  { return PString::Empty(); }

PINLINE PString PFilePath::GetPath() const
  { return GetDirectory(); }

///////////////////////////////////////////////////////////////////////////////

PINLINE bool PFile::Remove(const PFilePath & name, bool)
  { return unlink((char *)(const char *)name) == 0; }

PINLINE bool PFile::Remove(const PString & name, bool)
  { return unlink((char *)(const char *)name) == 0; }

///////////////////////////////////////////////////////////////////////////////

#if defined(P_PTHREADS)
PINLINE PThreadIdentifier PThread::GetCurrentThreadId() { return ::pthread_self(); }
#else
PINLINE PThreadIdentifier PThread::GetCurrentThreadId() { return 0; }
#endif


// End Of File ///////////////////////////////////////////////////////////////
