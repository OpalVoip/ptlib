/*
 * ptlib.inl
 *
 * Non-GUI classes inline function implementation.
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
 * $Id: ptlib.inl 21219 2008-10-02 08:52:00Z rjongbloed $
 */


///////////////////////////////////////////////////////////////////////////////
// PDirectory

__inline bool PDirectory::IsSeparator(char c)
  { return c == ':' || c == '/' || c == '\\'; }

__inline bool PDirectory::Remove(const PString & p)
  { return _wrmdir(p.AsWide()) == 0; }


__inline bool PDirectory::Restart(PFileInfo::FileTypes scanMask)
  { return Open(scanMask); }



///////////////////////////////////////////////////////////////////////////////
// PFile

__inline bool PFile::Exists(const PFilePath & name)
  { return _waccess_s(name.AsWide(), 0) == 0; }

__inline bool PFile::Remove(const PFilePath & name, bool force)
  { return PFile::Remove((const PString &)name, force); }


///////////////////////////////////////////////////////////////////////////////
// PThread

__inline PUniqueThreadIdentifier PThread::GetCurrentUniqueIdentifier()
  { return ::GetCurrentThreadId(); }

// End Of File ///////////////////////////////////////////////////////////////
