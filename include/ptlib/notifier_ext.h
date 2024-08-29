/*
 * notifier_ext.h
 *
 * Smart Notifiers and Notifier Lists
 *
 * Portable Tools Library
 *
 * Copyright (c) 2004 Reitek S.p.A.
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
 * Contributor(s): ______________________________________.
 */

#ifndef PTLIB_NOTIFIER_EXT_H
#define PTLIB_NOTIFIER_EXT_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <list>
#include <ptlib/notifier.h>


class PTimeInterval;


///////////////////////////////////////////////////////////////////////

/** Declare a safe notifier object class.
    See PDECLARE_NOTIFIER_EXT for more information.
  */
#define PDECLARE_SAFE_NOTIFIER_EXT(NotifierType, notifierArg, TargetType, func, ParamType, paramArg) \
        using func##Notifier = PNotifierTemplate<ParamType, PSafeObject, PSafePtr<PSafeObject>>; \
        PDECLARE_NOTIFIER_INTERNAL(NotifierType, notifierArg, TargetType, func, ParamType, paramArg, func##Notifier)

/** Declare a safe notifier object class.
    See PDECLARE_SAFE_NOTIFIER2 for more information.
  */
#define PDECLARE_SAFE_NOTIFIER2(   NotifierType,   TargetType, func, ParamType  ) \
        PDECLARE_SAFE_NOTIFIER_EXT(NotifierType, , TargetType, func, ParamType, )

/** Declare safe PNotifier derived class with intptr_t parameter.
    Uses PDECLARE_SAFE_NOTIFIER2 macro.
  */
#define PDECLARE_SAFE_NOTIFIER( NotifierType, TargetType, func) \
        PDECLARE_SAFE_NOTIFIER2(NotifierType, TargetType, func, intptr_t)


///////////////////////////////////////////////////////////////////////

typedef unsigned long PNotifierIdentifer;

/** Validated PNotifier class.
    This uses a mechanism to assure the caller that the target class still exists
    rather than simply calling through a pointer as the base PNotifierFuntion class
    will. Thus the call simply will not happen rather than crash.

    A notification target class must derive from PValidatedNotifierTarget, usually
    as a multiple inheritance, which will associate a unique ID with the
    specific class instance. This is saved in a global table. When the object
    is destoyed the ID is removed from that table so the caller knows if the
    target still exists or not.

    As well as deriving from PValidatedNotifierTarget class, the notifier functions
    must be declared using the PDECLARE_VALIDATED_NOTIFIER rather than the
    usual PDECLARE_NOTIFIER macro.
  */
class PValidatedNotifierTarget
{
  public:
    PValidatedNotifierTarget();
    virtual ~PValidatedNotifierTarget();

    // Do not copy the id across
    PValidatedNotifierTarget(const PValidatedNotifierTarget &);
    void operator=(const PValidatedNotifierTarget &) { }

    static bool Exists(PNotifierIdentifer id);

  private:
    PNotifierIdentifer m_validatedNotifierId;

    template <typename ParamType, typename NotifierType, typename TargetType>
    friend class PValidatedNotifier;
};


/**
   This is an abstract class for which a descendent is declared for every
   function that may be called. The <code>PDECLARE_VALIDATED_NOTIFIER</code>
   macro makes this declaration.

   See PNotifierFunctionTemplate for more information.
  */
template <typename ParamType, typename NotifierType = PObject, typename TargetType = void *>
class PValidatedNotifier : public PNotifierTemplate<ParamType, NotifierType, TargetType>
{
    typedef PNotifierTemplate<ParamType, NotifierType, TargetType> Parent;
    PCLASSINFO(PValidatedNotifier, Parent);
  public:
    PValidatedNotifier() { }
    PValidatedNotifier(typename Parent::Function fn, TargetType target, PValidatedNotifierTarget & notifierTarget)
      : Parent(std::move(fn), target)
      , m_targetID(notifierTarget.m_validatedNotifierId)
    { }
    PValidatedNotifier(typename Parent::Function fn, TargetType target, PValidatedNotifierTarget * notifierTarget)
      : Parent(std::move(fn), target)
      , m_targetID(notifierTarget->m_validatedNotifierId)
    { }

    void operator()(NotifierType & notifier, ParamType param)
    {
      if (PValidatedNotifierTarget::Exists(this->m_targetID))
        Parent:: operator()(notifier, param);
    }

  protected:
    PNotifierIdentifer m_targetID;
};


/** Declare a validated notifier object class.
    See PDECLARE_NOTIFIER_EXT for more information.
  */
#define PDECLARE_VALIDATED_NOTIFIER_EXT(NotifierType, notifierArg, TargetType, func, ParamType, paramArg) \
        PDECLARE_NOTIFIER_INTERNAL(     NotifierType, notifierArg, TargetType, func, ParamType, paramArg, PValidatedNotifier<ParamType>, target)

/** Declare a validated notifier object class.
    Uses PDECLARE_VALIDATED_NOTIFIER_EXT macro.
  */
#define PDECLARE_VALIDATED_NOTIFIER2(   NotifierType,   TargetType, func, ParamType  ) \
        PDECLARE_VALIDATED_NOTIFIER_EXT(NotifierType, , TargetType, func, ParamType, )

/** Declare validated PNotifier derived class with intptr_t parameter.
    Uses PDECLARE_VALIDATED_NOTIFIER2 macro.
 */
#define PDECLARE_VALIDATED_NOTIFIER( NotifierType, TargetType, func) \
        PDECLARE_VALIDATED_NOTIFIER2(NotifierType, TargetType, func, intptr_t)


///////////////////////////////////////////////////////////////////////

struct PAsyncNotifierCallback
{
  virtual ~PAsyncNotifierCallback() { }
  virtual void Call() = 0;

  static void Queue(PNotifierIdentifer id, PAsyncNotifierCallback * callback);
};


/** Asynchronous PNotifier class.
    This is a notification mechanism disconnects the caller from the target
    via a queue. The primary use would be to assure that the target class
    instance is only accesses via a specific thread.

    A notification target class must derive from PAsyncNotifierTarget, usually
    as a multiple inheritance, which will associate a a queue with the
    specific class instance. 

    As well as deriving from PAsyncNotifierTarget class, the notifier functions
    must be declared using the PDECLARE_ASYNC_NOTIFIER rather than the
    usual PDECLARE_NOTIFIER macro.
  */
class PAsyncNotifierTarget
{
  public:
    PAsyncNotifierTarget();
    virtual ~PAsyncNotifierTarget();

    /**Execute any notifications queued.
       The target must call this function for the asynchronous notifications
       to occur.

       @return true if a notification was executed.
      */
    bool AsyncNotifierExecute(
      const PTimeInterval & wait = 0  ///< Time to wait for a notification
    );

    /**Signal the target that there are notifications pending.
       The infrastructure will call this virtual from a random thread to
       indicate that a notification has been queued. What happens is
       applicaation dependent, but typically this would indicate to a main
       thread that it is time to call AsyncNotifierExecute(). For example for
       a GUI, this would post a message to the windowing system. That message
       is captured in teh GUI thread and AsyncNotifierExecute() called.
      */
    virtual void AsyncNotifierSignal();

  private:
    PNotifierIdentifer m_asyncNotifierId;

  template <typename ParmType, typename NotifierType, typename TargetType>
  friend class PAsyncNotifier;
};


/**
   This is an abstract class for which a descendent is declared for every
   function that may be called. The <code>PDECLARE_ASYNC_NOTIFIER</code>
   macro makes this declaration.

   See PNotifierFunctionTemplate for more information.

   Note: the user must be very careful with the type of ParamType. The lifetime
   must be guaranteed until the PAsyncNotifierTarget::AsyncNotifierExecute()
   function is called. It is usually easier to simply make sure the data is
   passed by value and never pointer or reference.
  */
template <typename ParamType, typename NotifierType = PObject, typename TargetType = void*>
class PAsyncNotifier : public PNotifierTemplate<ParamType, NotifierType, TargetType>, PAsyncNotifierCallback
{
    typedef PNotifierTemplate<ParamType, NotifierType, TargetType> Parent;
    PCLASSINFO(PAsyncNotifier, Parent);
  protected:
    PNotifierIdentifer m_asyncNotifierId;
    NotifierType     * m_notifier;
    ParamType          m_param;
  public:
    PAsyncNotifier(typename Parent::Function fn, TargetType target, PAsyncNotifierTarget & notifierTarget)
      : Parent(std::move(fn), target)
      , m_asyncNotifierId(notifierTarget.m_asyncNotifierId)
      , m_notifier(nullptr)
    { }
    PAsyncNotifier(typename Parent::Function fn, TargetType target, PAsyncNotifierTarget * notifierTarget)
      : Parent(std::move(fn), target)
      , m_asyncNotifierId(notifierTarget->m_asyncNotifierId)
      , m_notifier(nullptr)
    { }

    void operator()(NotifierType & notifier, ParamType param)
    {
      if (PAssert(m_notifier == nullptr || m_notifier == &notifier, "Cannot execute PAsyncNotifier from different sources")) {
        m_notifier = &notifier;
        m_param = param;
        Queue(this);
      }
    }

  protected:
    virtual void Call() override
    {
      Parent:: operator()(*m_notifier, m_param);
    }
};


/** Declare an asynchronous notifier object class.
    See PDECLARE_NOTIFIER_EXT for more information.
  */
#define PDECLARE_ASYNC_NOTIFIER_EXT(NotifierType, notifierArg, TargetType, func, ParamType, paramArg) \
        PDECLARE_NOTIFIER_INTERNAL( NotifierType, notifierArg, TargetType, func, ParamType, paramArg, PAsyncNotifier<ParamType>, target)

/** Declare an asynchronous notifier object class.
    Uses PDECLARE_ASYNC_NOTIFIER_EXT macro.
  */
#define PDECLARE_ASYNC_NOTIFIER2(   NotifierType,   TargetType, func, ParamType) \
        PDECLARE_ASYNC_NOTIFIER_EXT(NotifierType, , TargetType, func, ParamType, )

/** Declare an asynchronous PNotifier derived class with intptr_t parameter.
    Uses PDECLARE_ASYNC_NOTIFIER2 macro.
  */
#define PDECLARE_ASYNC_NOTIFIER( NotifierType, TargetType, func) \
        PDECLARE_ASYNC_NOTIFIER2(NotifierType, TargetType, func, intptr_t)


///////////////////////////////////////////////////////////////////////

/**Maintain a list of notifiers to be called all at once.
  */
template <typename ParamType, typename NotifierType = PObject, typename TargetType = void*>
class PNotifierListTemplate : public PObject
{
    PCLASSINFO(PNotifierListTemplate, PObject);
  private:
    using Notifier = PNotifierTemplate<ParamType, NotifierType, TargetType>;
    using List = std::list<Notifier>;
    List m_list;

  public:
    /// Indicate the number of notifiers in list.
    PINDEX GetSize() const { return this->m_list.size(); }

    // Remove all the notifiers
    void RemoveAll() { this->m_list.clear(); }

    /// Add a notifier to the list
    bool Add(const Notifier & handler)
    {
        if (std::find(this->m_list.begin(), this->m_list.end(), handler) != this->m_list.end())
            return false;
        this->m_list.push_back(handler);
        return true;
    }

    /// Remove notifier from teh list
    bool Remove(const Notifier & handler)
    {
      typename List::iterator it = std::find(this->m_list.begin(), this->m_list.end(), handler);
      if (it == this->m_list.end())
          return false;
      this->m_list.erase(it);
      return true;
    }

    /// Determine if notifier is in list
    bool Contains(const Notifier & handler) const
    {
      return std::find(this->m_list.begin(), this->m_list.end(), handler) != this->m_list.end();
    }

    /// Indicate notifier list is empty
    bool IsEmpty() const
    {
      return this->m_list.empty();
    }

    /// Remove all notifiers that use the specified target object.
    void RemoveTarget(void * const target)
    {
      this->m_list.remove_if([target](const Notifier & test) { return test.GetTarget() == target; });
    }

    /// Execute all notifiers in the list.
    bool operator()(NotifierType & obj, ParamType param)
    {
      if (this->m_list.empty())
        return false;
      for (auto it : this->m_list)
        it(obj, param);
      return true;
    }
};

typedef PNotifierListTemplate<intptr_t> PNotifierList;


#endif  // PTLIB_NOTIFIER_EXT_H


// End of File ///////////////////////////////////////////////////////////////
