/*
 * script.cxx
 *
 * Abstract class for script language interfaces
 *
 * Portable Tools Library
 *
 * Copyright (C) 2010-2012 by Post Increment
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
 * The Initial Developer of the Original Code is Post Increment
 *
 * Contributor(s): Craig Southeren
 *                 Robert Jongbloed
 */

#ifdef P_USE_PRAGMA
#pragma implementation "script.h"
#endif

#include <ptlib.h>

#include <ptclib/script.h>

#define new PNEW

PScriptLanguage::PScriptLanguage()
  : m_loaded(false)
  , m_lastErrorCode(0)
{
}


PScriptLanguage::~PScriptLanguage()
{
}


bool PScriptLanguage::Load(const PString & script)
{
  PFilePath filename = script;
  if (PFile::Exists(filename)) {
    if (!LoadFile(filename))
      return false;
  }
  else {
    if (!LoadText(script))
      return false;
  }

  return true;
}


bool PScriptLanguage::LoadFile(const PFilePath & filename)
{
  PTextFile file;
  if (!file.Open(filename, PFile::ReadOnly))
    return false;

  return LoadText(file.ReadString(P_MAX_INDEX));
}


bool PScriptLanguage::PushScopeChain(const PString & name, bool create)
{
  PWaitAndSignal lock(m_mutex);
  VarTypes type = GetVarType(name);
  if (type != CompositeType) {
    if (type != UndefinedType || !create)
      return false;
    if (!CreateComposite(name))
      return false;
  }
  m_scopeChain.push_back(name);
  return true;
}


PString PScriptLanguage::PopScopeChain(bool release)
{
  PWaitAndSignal lock(m_mutex);
  if (m_scopeChain.empty())
    return PString::Empty();
  PString popped = m_scopeChain.back();
  m_scopeChain.pop_back();
  if (release)
    ReleaseVariable(popped);
  return popped;
}


bool PScriptLanguage::GetBoolean(const PString & name)
{
  PVarType var;
  return GetVar(name, var) && var.AsBoolean();
}


bool PScriptLanguage::SetBoolean(const PString & name, bool value)
{
  PVarType var(value);
  return SetVar(name, value);
}


int PScriptLanguage::GetInteger(const PString & name)
{
  PVarType var;
  return GetVar(name, var) ? var.AsInteger() : 0;
}


bool PScriptLanguage::SetInteger(const PString & name, int value)
{
  return SetVar(name, value);
}


double PScriptLanguage::GetNumber(const PString & name)
{
  PVarType var;
  return GetVar(name, var) ? var.AsFloat() : 0.0;
}


bool PScriptLanguage::SetNumber(const PString & name, double value)
{
  return SetVar(name, value);
}


PString PScriptLanguage::GetString(const PString & name)
{
  PVarType var;
  return GetVar(name, var) ? var.AsString() : PString::Empty();
}


bool PScriptLanguage::SetString(const PString & name, const char * value)
{
  return SetVar(name, value);
}


void PScriptLanguage::OnError(int code, const PString & str)
{
  m_mutex.Wait();
  m_lastErrorCode = code;
  m_lastErrorText = str.IsEmpty() && code != 0 ? psprintf("Unknown error %i", code) : str;
  PTRACE(2, this, GetLanguageName(), "Error " << code << ": " << m_lastErrorText);
  m_mutex.Signal();
}


bool PScriptLanguage::InternalSetFunction(const PString & name, const FunctionNotifier & func)
{
  FunctionMap::iterator it = m_functions.find(PString(name));
  if (it == m_functions.end())
    return !func;

  if (!func)
    m_functions.erase(it);
  else
    it->second = func;
  return true;
}


void PScriptLanguage::InternalRemoveFunction(const PString & prefix)
{
  FunctionMap::iterator it = m_functions.lower_bound(PString(prefix));
  while (it != m_functions.end() && it->first.NumCompare(prefix) == EqualTo) {
    if (isalnum(it->first[prefix.GetLength()]))
      ++it;
    else
      m_functions.erase(it++);
  }
}


PScriptLanguage * PScriptLanguage::Create(const PString & language)
{
  PScriptLanguage * script = PFactory<PScriptLanguage>::CreateInstance(language);
  if (script != NULL && script->IsInitialised())
    return script;

  delete script;
  return NULL;
}


PScriptLanguage * PScriptLanguage::CreateOne(const PStringArray & languages)
{
  for (PINDEX i = 0; i < languages.GetSize(); ++i) {
    PScriptLanguage * script = Create(languages[i]);
    if (script != NULL && script->IsInitialised())
      return script;
    delete script;
  }
  return NULL;
}


PStringArray PScriptLanguage::GetLanguages()
{
  PStringArray names = PFactory<PScriptLanguage>::GetKeyList();
  for (PINDEX i = 0; i < names.GetSize(); ) {
    PScriptLanguage * script = Create(names[i]);
    if (script == NULL)
      names.RemoveAt(i);
    else {
      delete script;
      ++i;
    }
  }
  return names;
}
