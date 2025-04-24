/*
 * critsec.h
 *
 * Critical section mutex class.
 *
 * Portable Windows Library
 *
 * Copyright (C) 2004 Post Increment
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
 * The Initial Developer of the Original Code is Post Increment
 *
 * Contributor(s): ______________________________________.
 */

#ifndef PTLIB_CRITICALSECTION_H
#define PTLIB_CRITICALSECTION_H

#ifdef _MSC_VER
// Accessing a local variable x via an Interlocked function : This is an unusual usage which could be reconsidered.
// Yes, this warning is in Microsofts own code!
#pragma warning(disable:28113)
#include <atomic>
#pragma warning(default:28113)
#else
#include <atomic>
#endif // _MSC_VER

#define PAtomicEnum std::atomic

// For backward compatibility
class PAtomicInteger : public std::atomic<long>
{
  public:
    typedef long IntegerType;
    explicit PAtomicInteger(IntegerType value = 0) : std::atomic<IntegerType>(value) { }
    __inline PAtomicInteger & operator=(IntegerType value) { std::atomic<IntegerType>::operator=(value); return *this; }
    void SetValue(IntegerType value) { std::atomic<IntegerType>::operator=(value); }
    __inline bool IsZero() const { return static_cast<IntegerType>(*this) == 0; }
};

// For backward compatibility
class PAtomicBoolean : public std::atomic<bool>
{
  public:
    typedef long IntegerType;
    explicit PAtomicBoolean(bool value = false) : std::atomic<bool>(value) { }
    __inline PAtomicBoolean & operator=(bool value) { std::atomic<bool>::operator=(value); return *this; }
    bool TestAndSet(bool value) { return exchange(value); }
};

#endif // PTLIB_CRITICALSECTION_H


// End Of File ///////////////////////////////////////////////////////////////
