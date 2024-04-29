/*
 * simplescript.cxx
 *
 * Ultra simple script language
 * No functions, only string variables, can only concatenate string expressions.
 *
 * Portable Tools Library
 *
 * Copyright (C) 2021 by Vox Lucida Pty. Ltd.
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
 * The Initial Developer of the Original Code is Vox Lucida Pty. Ltd.
 *
 * Contributor(s): Craig Southeren
 *                 Robert Jongbloed
 */

#ifdef P_USE_PRAGMA
#pragma implementation "simplescript.h"
#endif

#include <ptlib.h>

#include <ptclib/simplescript.h>

#define new PNEW

#define PTraceModule() "SimpleScript"


static PConstString const SimpleName("SimpleScript");
PFACTORY_CREATE(PFactory<PScriptLanguage>, PSimpleScript, SimpleName, false);



PSimpleScript::PSimpleScript()
{
}


PString PSimpleScript::LanguageName()
{
  return SimpleName;
}


PString PSimpleScript::GetLanguageName() const
{
  return SimpleName;
}


bool PSimpleScript::IsInitialised() const
{
  return true;
}


bool PSimpleScript::LoadFile(const PFilePath &)
{
  return false;
}


bool PSimpleScript::LoadText(const PString &)
{
  return false;
}


static PConstString const LegalVariableChars("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_.$[]");

bool PSimpleScript::Run(const char * script)
{
  PWaitAndSignal mutex(m_mutex);

  PString varName, expr;
  if (!PString(script).Split('=', varName, expr, PString::SplitAfterNonEmpty|PString::SplitBeforeNonEmpty)) {
    if (InternalEvaluateExpr(script) != "undefined")
      return true;
    OnError(1, "Only assignment expressions, or simple function calls, are allowed");
    return false;
  }

  if (varName.FindSpan(LegalVariableChars) != P_MAX_INDEX) {
    OnError(1, "Illegal assignment variable name");
    return false;
  }

  if (expr.FindSpan(LegalVariableChars) == P_MAX_INDEX && GetVarType(expr) == CompositeType) {
    m_variables.SetAt(varName + ".$", expr + ".$");
    return true;
  }

  PINDEX comparison;
  if ((comparison = expr.Find("==")) != P_MAX_INDEX)
    return SetString(varName, PString(InternalEvaluateExpr(expr.Left(comparison)) == InternalEvaluateExpr(expr.Mid(comparison+2))));
  if ((comparison = expr.Find("!=")) != P_MAX_INDEX)
    return SetString(varName, PString(InternalEvaluateExpr(expr.Left(comparison)) != InternalEvaluateExpr(expr.Mid(comparison+2))));

  return SetString(varName, InternalEvaluateExpr(expr));
}


static bool IsFunction(const PString & expr, PINDEX offset, PINDEX & endname, PINDEX & startarg, PINDEX & endarg)
{
  if (!isalpha(expr[offset]) && expr[offset] != '_')
    return false;

  PINDEX pos = offset + 1;
  while (isalnum(expr[pos]) || expr[pos] == '_')
    ++pos;
  endname = pos - 1;

  while (iswspace(expr[pos]))
    ++pos;

  if (expr[pos] != '(')
    return false;

  startarg = pos + 1;

  endarg = expr.Find(')', startarg);
  if (endarg != P_MAX_INDEX)
    --endarg;
  else
    PTRACE(2, "Mismatched parenthesis");

  return true;
}


PString PSimpleScript::InternalEvaluateExpr(const PString & expr)
{
  PString result;

  PINDEX pos = 0;
  while (pos < expr.GetLength()) {
    PINDEX endname, startarg, endarg;
    if (IsFunction(expr, pos, endname, startarg, endarg)) {
      PString funcname = expr(pos, endname);
      PString expression = expr(startarg, endarg);
      PTRACE(4, "Found " << funcname << " function: expression=\"" << expression << "\"");
      if (funcname == "eval")
        result += InternalEvaluateExpr(InternalEvaluateExpr(expression));
      else if (funcname == "Date")
        result += PTime().AsString(PTime::ShortDate);
      else if (funcname == "Time")
        result += PTime().AsString(PTime::LongTime);
      else {
        FunctionMap::iterator func = m_functions.find(funcname);
        if (func == m_functions.end()) {
          OnError(2, "Unknown function");
          return "undefined";
        }
        Signature signature;
        signature.m_arguments.push_back(InternalEvaluateExpr(expression));
        func->second(*this, signature);
        if (!signature.m_results.empty())
          result += signature.m_results.front().AsString();
      }
      pos = endarg + 2;
    }
    else if (expr[pos] == '\'') {
      PINDEX quote = expr.Find('\'', ++pos);
      if (quote == P_MAX_INDEX)
        return "undefined";
      result += expr(pos, quote-1);
      pos = quote+1;
    }
    else if (expr[pos] == '"') {
      PINDEX quote= pos;
      do {
        quote = expr.Find('"', quote+1);
        if (quote == P_MAX_INDEX)
          return "undefined";
      }  while (expr[quote-1] == '\\');
      result += expr(pos+1, quote-1);
      pos = quote+1;
    }
    else if (isalpha(expr[pos])) {
      PINDEX span = expr.FindSpan(LegalVariableChars, pos);
      result += GetString(expr(pos, span-1));
      pos = span;
    }
    else if (isdigit(expr[pos])) {
      PINDEX span = expr.FindSpan("0123456789", pos);
      result += GetString(expr(pos, span-1));
      pos = span;
    }
    else if (expr[pos] == '+' || isspace(expr[pos]))
      pos++;
    else {
      PTRACE(2, "Only '+' operator and literal strings supported.");
      break;
    }
  }

  return result;
}


bool PSimpleScript::CreateComposite(const PString & name, unsigned sequenceSize)
{
  if (sequenceSize == 0)
    m_variables.SetAt(name+".$", "");
  else {
    for (unsigned i = 0; i < sequenceSize; ++i)
      m_variables.SetAt(PSTRSTRM(name<< '[' << i << ']'), "");
  }
  return true;
}


PScriptLanguage::VarTypes PSimpleScript::GetVarType(const PString & name)
{
  PWaitAndSignal mutex(m_mutex);

  if (m_variables.Contains(name))
    return StringType;

  if (m_variables.Contains(name+".$"))
    return CompositeType;

  return UndefinedType;
}


PString * PSimpleScript::InternalFindVar(const PString & varName) const
{
  PString name = varName;
  PINDEX dot = name.Find('.');
  if (dot != P_MAX_INDEX) {
    PString * reference = m_variables.GetAt(name.Left(dot) + ".$");
    if (reference != NULL && !reference->empty())
      name = reference->Left(reference->Find('.')) + name.Mid(dot);
  }

  PString * value;
  for (PStringList::const_iterator it = m_scopeChain.rbegin(); it != m_scopeChain.rend(); --it) {
    if ((value = m_variables.GetAt(PSTRSTRM(*it << '.' << name))) != NULL)
      return value;
  }

  return m_variables.GetAt(name);
}


bool PSimpleScript::GetVar(const PString & varName, PVarType & var)
{
  PWaitAndSignal mutex(m_mutex);

  PString * value = InternalFindVar(varName);
  if (value == NULL)
    return false;

  var = *value;
  return true;
}


bool PSimpleScript::SetVar(const PString & varName, const PVarType & var)
{
  PWaitAndSignal mutex(m_mutex);

  PString * value = InternalFindVar(varName);
  if (value != NULL)
    *value = var.AsString();
  else
    m_variables.SetAt(varName, var.AsString());
  return true;
}


bool PSimpleScript::ReleaseVariable(const PString & name)
{
  bool hasVar = true;
  if (m_variables.Contains(name))
    m_variables.RemoveAt(name);
  else {
    PString withDot = name + '.';
    hasVar = false;
    for (PStringToString::iterator it = m_variables.begin(); it != m_variables.end(); ) {
      if (it->first.NumCompare(withDot) != EqualTo)
        ++it;
      else {
        m_variables.erase(it++);
        hasVar = true;
      }
    }
  }
  return hasVar;
}


bool PSimpleScript::Call(const PString &, const char *, ...)
{
  return false;
}


bool PSimpleScript::Call(const PString &, Signature &)
{
  return false;
}


bool PSimpleScript::SetFunction(const PString & name, const FunctionNotifier & func)
{
  PWaitAndSignal mutex(m_mutex);

  if (!InternalSetFunction(name, func))
    m_functions[name] = func;
  return true;
}
