/*
 * atomic.h
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

#ifndef PTLIB_ATOMIC_H
#define PTLIB_ATOMIC_H

#include <atomic>

using std::atomic;
#define PAtomicEnum std::atomic


// For backward compatibility
class PAtomicInteger : public atomic<long>
{
  public:
    typedef long IntegerType;
    explicit PAtomicInteger(IntegerType value = 0) : atomic<IntegerType>(value) { }
    __inline PAtomicInteger & operator=(IntegerType value) { atomic<IntegerType>::operator=(value); return *this; }
    void SetValue(IntegerType value) { atomic<IntegerType>::operator=(value); }
    __inline bool IsZero() const { return static_cast<IntegerType>(*this) == 0; }
};

// For backward compatibility
class PAtomicBoolean : public atomic<bool>
{
  public:
    typedef long IntegerType;
    explicit PAtomicBoolean(bool value = false) : atomic<bool>(value) { }
    __inline PAtomicBoolean & operator=(bool value) { atomic<bool>::operator=(value); return *this; }
    bool TestAndSet(bool value) { return exchange(value); }
};


#endif // PTLIB_ATOMIC_H


// End Of File ///////////////////////////////////////////////////////////////
