/*
 * notifier.h
 *
 * Notified type safe function pointer class.
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
 * Contributor(s): ______________________________________.
 */

#ifndef PTLIB_NOTIFIER_H
#define PTLIB_NOTIFIER_H

#include <functional>


class PObject;


///////////////////////////////////////////////////////////////////////////////
// General notification mechanism from one object to another

/** Notifer function instance.
    This is a wrapper around std::function() to allow comparison operations.
  */
template <typename ParamType,
          typename NotifierType = PObject,
          typename TargetType = void*,
          typename FunctionType = void(NotifierType &, ParamType)>
class PNotifierTemplate : public PObject, public std::function<FunctionType>
{
    PCLASSINFO(PNotifierTemplate, PObject);
  protected:
    TargetType m_target;
  public:
    using Function = std::function<FunctionType>;

    PNotifierTemplate() : m_target() { }
    PNotifierTemplate(nullptr_t *) : m_target() { }
    PNotifierTemplate(Function fn, TargetType target) : Function(std::move(fn)), m_target(target) { }

    bool operator==(const PNotifierTemplate & other) const { return m_target == other.m_target && this->target<FunctionType>() == other.target<FunctionType>(); }
    bool operator!=(const PNotifierTemplate & other) const { return !operator==(other); }

    P_DEPRECATED bool IsNULL() const { return Function::operator bool(); }
    TargetType GetTarget() const { return m_target; }
};

/** \class PNotifier
    Class specialisation for PNotifierTemplate<intptr_t>
  */
using PNotifier = PNotifierTemplate<intptr_t>;


#define PDECLARE_NOTIFIER_INTERNAL_EXT(NotifierType, notifierArg, TargetType, func, caller, ParamType, paramArg, FunctionType, ...) \
    static FunctionType func##CreateNotifier(TargetType * target) \
      { return FunctionType(std::bind(&TargetType::caller,  target, std::placeholders::_1, std::placeholders::_2),  target, ##__VA_ARGS__); } \
    static FunctionType func##CreateNotifier(TargetType & target) \
      { return FunctionType(std::bind(&TargetType::caller, &target, std::placeholders::_1, std::placeholders::_2), &target, ##__VA_ARGS__); } \
    P_PUSH_MSVC_WARNINGS(4100) \
    virtual void func(NotifierType & notifierArg, ParamType paramArg) \
    P_POP_MSVC_WARNINGS()

#define PDECLARE_NOTIFIER_INTERNAL(NotifierType, notifierArg, TargetType, func, ParamType, paramArg, FunctionType, ...) \
        void func##CallWithCast(PObject & n, ParamType p) { func(dynamic_cast<NotifierType &>(n), p); } \
        PDECLARE_NOTIFIER_INTERNAL_EXT(NotifierType, notifierArg, TargetType, func, func##CallWithCast, ParamType, paramArg, FunctionType, ##__VA_ARGS__)

/** Declare PNotifier derived class with arbitrary type as second parameter and function definition.
    This can be used when you wish a notifier with an inline implmentation.
  */
#define PDECLARE_NOTIFIER_EXT(     NotifierType, notifierArg, TargetType, func, ParamType, paramArg) \
        PDECLARE_NOTIFIER_INTERNAL(NotifierType, notifierArg, TargetType, func, ParamType, paramArg, PNotifierTemplate<ParamType>)

/** Declare PNotifier derived class with intptr_t parameter.
    Uses PDECLARE_NOTIFIER_EXT macro.
  */
#define PDECLARE_NOTIFIER2(   NotifierType,   TargetType, func, ParamType) \
        PDECLARE_NOTIFIER_EXT(NotifierType, , TargetType, func, ParamType, )

/** Declare PNotifier derived class with intptr_t parameter.
    Uses PDECLARE_NOTIFIER2 macro.
  */
#define PDECLARE_NOTIFIER( NotifierType, TargetType, func) \
        PDECLARE_NOTIFIER2(NotifierType, TargetType, func, intptr_t)

/// Declare notifier with better compile time checks. Not derived from PNotifier.
#define PDECLARE_NOTIFIER_FUNCTION(TargetType, func, NotifierType, ParamType) \
  using func##NotifierClass = PNotifierTemplate<ParamType, NotifierType>; \
  PDECLARE_NOTIFIER_INTERNAL_EXT(NotifierType, notifierArg, TargetType, func, func, ParamType, paramArg, func##NotifierClass)


/** Create a PNotifier object instance.
  This macro creates an instance of the particular <code>PNotifier</code> class using
  the \p func parameter as the member function to call.

  The \p obj parameter is the instance to call the function against.
  If the instance to be called is the current instance, ie if \p obj is
  \p this then the <code>PCREATE_NOTIFIER</code> macro should be used.
 */
#define PCREATE_NOTIFIER_EXT(obj, TargetType, func) TargetType::func##CreateNotifier(obj)


/** Create a PNotifier object instance.
  This macro creates an instance of the particular <code>PNotifier</code> class using
  the \p func parameter as the member function to call.

  The \p this object is used as the instance to call the function
  against. The <code>PCREATE_NOTIFIER_EXT</code> macro may be used if the instance to be
  called is not the current object instance.
 */
#define PCREATE_NOTIFIER(func) P_DISABLE_MSVC_WARNINGS(4355, func##CreateNotifier(this))


#endif // PTLIB_NOTIFIER_H


// End Of File ///////////////////////////////////////////////////////////////
