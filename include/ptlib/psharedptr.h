/*
 * psharedptr.h
 *
 * SharedPtr template
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
 *
 */

#ifndef PTLIB_SHAREDPTR_H
#define PTLIB_SHAREDPTR_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <ptlib.h>
#include <memory>

// For backward compatibility
template <typename T> using PSharedPtr = std::shared_ptr<T>;

#if defined(__GNUC__) && __GNUC__ < 12
  /* Despite GNU C++ version 10 claims of supporting C++20, it does not
     support std::atomic<std::shared_ptr<T>> as indicated by the standard,
     so we need to implement our own wrapper for it. */
  template <typename T> struct PAtomicSharedPtr
  {
    using ptr_type = std::shared_ptr<T>;
    PAtomicSharedPtr() = default;
    PAtomicSharedPtr(const PAtomicSharedPtr &) = delete;
    PAtomicSharedPtr & operator=(PAtomicSharedPtr &) = delete;

    explicit PAtomicSharedPtr(ptr_type ptr)
    {
      store(std::move(ptr));
    }

    PAtomicSharedPtr & operator=(ptr_type ptr)
    {
      store(std::move(ptr));
      return *this;
    }

    operator ptr_type() const
    {
      return load();
    }

    ptr_type load(std::memory_order order = std::memory_order_seq_cst) const
    {
      return std::atomic_load_explicit(&m_ptr, order);
    }

    void store(const ptr_type & ptr, std::memory_order order = std::memory_order_seq_cst)
    {
      std::atomic_store_explicit(&m_ptr, std::move(ptr), order);
    }

    ptr_type exchange(ptr_type ptr, std::memory_order order = std::memory_order_seq_cst)
    {
      return std::atomic_exchange_explicit(&m_ptr, std::move(ptr), order);
    }

    bool compare_exchange_strong(ptr_type & expected,
                                 ptr_type desired,
                                 std::memory_order success = std::memory_order_seq_cst,
                                 std::memory_order failure = std::memory_order_seq_cst)
    {
      return std::atomic_compare_exchange_strong_explicit(&m_ptr, &expected, std::move(desired), success, failure);
    }

    bool compare_exchange_weak(ptr_type & expected,
                               ptr_type desired,
                               std::memory_order success = std::memory_order_seq_cst,
                               std::memory_order failure = std::memory_order_seq_cst)
    {
      return std::atomic_compare_exchange_weak_explicit(&m_ptr, &expected, std::move(desired), success, failure);
    }

  private:
    ptr_type m_ptr;
  };
#else
  template <typename T> using PAtomicSharedPtr = std::atomic<std::shared_ptr<T>>;
#endif

#endif // PTLIB_SHAREDPTR_H


// End Of File ///////////////////////////////////////////////////////////////
