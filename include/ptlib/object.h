/*
 * object.h
 *
 * Mother of all ancestor classes.
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
 * The Original Code is Portable Windows Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Portions are Copyright (C) 1993 Free Software Foundation, Inc.
 * All Rights Reserved.
 *
 * Contributor(s): ______________________________________.
 */

#ifndef PTLIB_OBJECT_H
#define PTLIB_OBJECT_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <ptlib_config.h>

#if defined(_WIN32)
#include "msos/ptlib/platform.h"
#else
#include "unix/ptlib/platform.h"
#endif

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include <string>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <algorithm>
#include <functional>
#include <limits>
#include <typeinfo>
#include <memory>

#include "atomic.h"

// Not a good practice (name space pollution), but will take too long to fix.
using std::cin, std::cout, std::cerr,
      std::ostream, std::istream, std::wostream, std::ostringstream, std::wstringstream, std::ifstream, std::ofstream,
      std::streambuf, std::streamoff, std::streamsize, std::ios, std::ios_base,
      std::ends, std::endl, std::flush, std::ws, std::hex, std::oct, std::dec, std::right, std::left,
      std::setiosflags, std::resetiosflags, std::setfill, std::setw, std::setprecision,
      std::showpos, std::scientific, std::fixed, std::showbase, std::noshowbase, std::boolalpha,
      std::numeric_limits, std::string, std::list, std::vector, std::map, std::set, std::pair, std::make_pair;

// Somewhere in C headers you get this, which blows up STL version
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif


#if !defined __GNUC__ || __GNUC__ >= 5
  #define P_MAX_INDEX ::std::numeric_limits<PINDEX>::max()
#elif P_PINDEX_IS_SIZE_T
  const PINDEX P_MAX_INDEX = SIZE_MAX;
#else
  const PINDEX P_MAX_INDEX = INT_MAX;
#endif

#if PINDEX_SIGNED
  #define PASSERTINDEX(idx) PAssert((idx) >= 0, PInvalidArrayIndex)
#else
  #define PASSERTINDEX(idx)
#endif



///////////////////////////////////////////////////////////////////////////////

#define P_REMOVE_VIRTUAL_INTERNAL_BASE(fn) __inline virtual struct ptlib_virtual_function_changed_or_removed ****** fn { return 0; }

#if defined(_MSC_VER)
  #if _MSC_VER < 1310
    #define P_DEPRECATED
    #define P_REMOVE_VIRTUAL_INTERNAL(type, fn, body) P_REMOVE_VIRTUAL_INTERNAL_BASE(fn)
  #elif _MSC_VER < 1400
    #define P_DEPRECATED __declspec(deprecated)
    #define P_REMOVE_VIRTUAL_INTERNAL(type, fn, body) __inline virtual __declspec(deprecated) type fn body
  #else
    #define P_DEPRECATED __declspec(deprecated)
    #define P_REMOVE_VIRTUAL_INTERNAL(type, fn, body) __inline virtual __declspec(deprecated("Virtual function signature changed or function deprecated")) type fn body
  #endif
#elif defined(__GNUC__)
  #if __GNUC__ < 4
    #define P_DEPRECATED
    #define P_REMOVE_VIRTUAL_INTERNAL(type, fn, body) P_REMOVE_VIRTUAL_INTERNAL_BASE(fn)
  #else
    #define P_DEPRECATED __attribute__((deprecated))
    #define P_REMOVE_VIRTUAL_INTERNAL(type, fn, body) __attribute__((warn_unused_result)) __attribute__((deprecated)) P_REMOVE_VIRTUAL_INTERNAL_BASE(fn)
  #endif
#else
    #define P_DEPRECATED
    #define P_REMOVE_VIRTUAL_INTERNAL(type, fn, body) P_REMOVE_VIRTUAL_INTERNAL_BASE(fn)
#endif

#define P_REMOVE_VIRTUAL_VOID(fn)       P_REMOVE_VIRTUAL_INTERNAL(void, fn, {})
#define P_REMOVE_VIRTUAL(type, fn, ret) P_REMOVE_VIRTUAL_INTERNAL(type, fn, { return ret; })


#ifdef _MSC_VER
  #define P_PUSH_MSVC_WARNINGS(warnings) __pragma(warning(push)) __pragma(warning(disable:warnings))
  #define P_POP_MSVC_WARNINGS()          __pragma(warning(pop))
  #define P_PUSH_GCC_WARNING(warning)
  #define P_POP_GCC_WARNING()
#else
  #define P_PUSH_MSVC_WARNINGS(warnings)
  #define P_POP_MSVC_WARNINGS()
  #define P_GCC_PRAGMA(token) _Pragma(#token)
  #define P_PUSH_GCC_WARNING(warning) _Pragma("GCC diagnostic push") P_GCC_PRAGMA(GCC diagnostic ignored warning)
  #define P_POP_GCC_WARNING()         _Pragma("GCC diagnostic pop")
  //#pragma clang diagnostic ignored "-Winconsistent-missing-override"
#endif // _MSC_VER
#define P_DISABLE_MSVC_WARNINGS(warnings, statement) P_PUSH_MSVC_WARNINGS(warnings) statement  P_POP_MSVC_WARNINGS()
#define P_DISABLE_GCC_WARNING(warning, statement)    P_PUSH_GCC_WARNING(warning)    statement; P_POP_GCC_WARNING()

#ifdef _MSC_VER
  #define PIGNORE_RETURN(t,e)	(void)(e)
#else
  #define PIGNORE_RETURN(t,e)	do { t unused __attribute__((unused)) = (e); } while(0)
#endif

// We are gradually converting over to standard C++ names, these
// are for backward compatibility only

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

typedef bool   PBoolean;
#define PTrue  true
#define PFalse false


///////////////////////////////////////////////////////////////////////////////
// Disable inlines when debugging for faster compiles (the compiler doesn't
// actually inline the function with debug on any way).

#ifndef P_USE_INLINES
#ifdef _DEBUG
#define P_USE_INLINES 0
#else
#define P_USE_INLINES 0
#endif
#endif

#if P_USE_INLINES
#define PINLINE __inline
#else
#define PINLINE
#endif


///////////////////////////////////////////////////////////////////////////////
// Handy macros

/// Count the number of arguments passed in macro
#if _MSC_VER
  #define PARG_COUNT(...) PARG_COUNT_PART2(PARG_COUNT_PART1(__VA_ARGS__))
  #define PARG_COUNT_PART1(...) unused, __VA_ARGS__
  #define PARG_COUNT_PART2(...) PARG_COUNT_PART3(PARG_COUNT_PART4(__VA_ARGS__,39,38,37,36,35,34,33,32,31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0))
  #define PARG_COUNT_PART3(x) x
  #define PARG_COUNT_PART4(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,N,...) N
#else
  #define PARG_COUNT(...) PARG_COUNT_INTERNAL(0,##__VA_ARGS__,40,39,38,37,36,35,34,33,32,31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0)
  #define PARG_COUNT_INTERNAL(_0,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,N,...) N
#endif

/// Turn the argument into a string
#define P_STRINGISE(v) P_STRINGISE_PART2(v)
#define P_STRINGIZE(v) P_STRINGISE_PART2(v)
#define P_STRINGISE_PART2(v) #v

/** This declares a standard enumeration (enum) of symbols with ++ and -- operators.
    The symbols Begin##name and End##name are automatically added to the enumeration
    and is equal to the first value, and one beyond the last value respectively.
    This operates in a similar manner to STL iterators so that a for loop like:
    <PRE><CODE>
       for (MyEnum e = BeginMyEnum; e < EndMyEnum; ++e)
    </CODE></PRE>
    works as epected.

    A symbol for the count of enumerations (End##name - Begin##name) is also defined.
  */
#define P_DECLARE_ENUM_EX(name, countName, firstName, firstValue, ...) \
  enum name { firstName = firstValue, Begin##name = firstName, __VA_ARGS__, End##name, countName = End##name-Begin##name }; \
  friend __inline name operator++(name & e     ) { PAssert(e <    End##name, PInvalidParameter); return    e = (name)(e+1);           } \
  friend __inline name operator++(name & e, int) { PAssert(e <    End##name, PInvalidParameter); name o=e; e = (name)(e+1); return o; } \
  friend __inline name operator--(name & e     ) { PAssert(e >= Begin##name, PInvalidParameter); return    e = (name)(e-1);           } \
  friend __inline name operator--(name & e, int) { PAssert(e >= Begin##name, PInvalidParameter); name o=e; e = (name)(e-1); return o; } \
  static __inline name name##FromInt(int v) { return (name)(v < Begin##name ? Begin##name : v >= End##name ? (End##name-1) : v); }

/** This declares a standard enumeration (enum) of symbols with ++ and -- operators.
    The symbols Begin##name and End##name are automatically added to the enumeration
    and is equal to the first value, and one beyond the last value respectively.
    This operates in a similar manner to STL iterators so that a for loop like:
    <PRE><CODE>
       for (MyEnum e = BeginMyEnum; e < EndMyEnum; ++e)
    </CODE></PRE>
    works as epected.

    A symbol Num##name for the count of enumerations (End##name - Begin##name) is also defined.
  */
#define P_DECLARE_ENUM(name, first, ...) P_DECLARE_ENUM_EX(name, Num##name, first, 0, __VA_ARGS__)

extern void PPrintEnum(std::ostream & strm, int e, int begin, int end, char const * const * names);
extern int PReadEnum(std::istream & strm, int begin, int end, char const * const * names, bool matchCase = true);
extern int PParseEnum(const char * str, int begin, int end, char const * const * names, bool matchCase = true);

#define P_ENUM_NAMES_PART1(narg, args)P_ENUM_NAMES_PART2(narg, args)
#define P_ENUM_NAMES_PART2(narg, args) P_ENUM_NAMES_ARG_##narg args
#define P_ENUM_NAMES_ARG_1(_1                                                                                                                                                     )#_1
#define P_ENUM_NAMES_ARG_2(_1,_2                                                                                                                                                  )#_1,#_2
#define P_ENUM_NAMES_ARG_3(_1,_2,_3                                                                                                                                               )#_1,#_2,#_3
#define P_ENUM_NAMES_ARG_4(_1,_2,_3,_4                                                                                                                                            )#_1,#_2,#_3,#_4
#define P_ENUM_NAMES_ARG_5(_1,_2,_3,_4,_5                                                                                                                                         )#_1,#_2,#_3,#_4,#_5
#define P_ENUM_NAMES_ARG_6(_1,_2,_3,_4,_5,_6                                                                                                                                      )#_1,#_2,#_3,#_4,#_5,#_6
#define P_ENUM_NAMES_ARG_7(_1,_2,_3,_4,_5,_6,_7                                                                                                                                   )#_1,#_2,#_3,#_4,#_5,#_6,#_7
#define P_ENUM_NAMES_ARG_8(_1,_2,_3,_4,_5,_6,_7,_8                                                                                                                                )#_1,#_2,#_3,#_4,#_5,#_6,#_7,#_8
#define P_ENUM_NAMES_ARG_9(_1,_2,_3,_4,_5,_6,_7,_8,_9                                                                                                                             )#_1,#_2,#_3,#_4,#_5,#_6,#_7,#_8,#_9
#define P_ENUM_NAMES_ARG_10(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10                                                                                                                        )#_1,#_2,#_3,#_4,#_5,#_6,#_7,#_8,#_9,#_10
#define P_ENUM_NAMES_ARG_11(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11                                                                                                                    )#_1,#_2,#_3,#_4,#_5,#_6,#_7,#_8,#_9,#_10,#_11
#define P_ENUM_NAMES_ARG_12(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12                                                                                                                )#_1,#_2,#_3,#_4,#_5,#_6,#_7,#_8,#_9,#_10,#_11,#_12
#define P_ENUM_NAMES_ARG_13(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13                                                                                                            )#_1,#_2,#_3,#_4,#_5,#_6,#_7,#_8,#_9,#_10,#_11,#_12,#_13
#define P_ENUM_NAMES_ARG_14(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14                                                                                                        )#_1,#_2,#_3,#_4,#_5,#_6,#_7,#_8,#_9,#_10,#_11,#_12,#_13,#_14
#define P_ENUM_NAMES_ARG_15(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15                                                                                                    )#_1,#_2,#_3,#_4,#_5,#_6,#_7,#_8,#_9,#_10,#_11,#_12,#_13,#_14,#_15
#define P_ENUM_NAMES_ARG_16(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16                                                                                                )#_1,#_2,#_3,#_4,#_5,#_6,#_7,#_8,#_9,#_10,#_11,#_12,#_13,#_14,#_15,#_16
#define P_ENUM_NAMES_ARG_17(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17                                                                                            )#_1,#_2,#_3,#_4,#_5,#_6,#_7,#_8,#_9,#_10,#_11,#_12,#_13,#_14,#_15,#_16,#_17
#define P_ENUM_NAMES_ARG_18(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18                                                                                        )#_1,#_2,#_3,#_4,#_5,#_6,#_7,#_8,#_9,#_10,#_11,#_12,#_13,#_14,#_15,#_16,#_17,#_18
#define P_ENUM_NAMES_ARG_19(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19                                                                                    )#_1,#_2,#_3,#_4,#_5,#_6,#_7,#_8,#_9,#_10,#_11,#_12,#_13,#_14,#_15,#_16,#_17,#_18,#_19
#define P_ENUM_NAMES_ARG_20(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20                                                                                )#_1,#_2,#_3,#_4,#_5,#_6,#_7,#_8,#_9,#_10,#_11,#_12,#_13,#_14,#_15,#_16,#_17,#_18,#_19,#_20
#define P_ENUM_NAMES_ARG_21(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21                                                                            )#_1,#_2,#_3,#_4,#_5,#_6,#_7,#_8,#_9,#_10,#_11,#_12,#_13,#_14,#_15,#_16,#_17,#_18,#_19,#_20,#_21
#define P_ENUM_NAMES_ARG_22(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22                                                                        )#_1,#_2,#_3,#_4,#_5,#_6,#_7,#_8,#_9,#_10,#_11,#_12,#_13,#_14,#_15,#_16,#_17,#_18,#_19,#_20,#_21,#_22
#define P_ENUM_NAMES_ARG_23(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23                                                                    )#_1,#_2,#_3,#_4,#_5,#_6,#_7,#_8,#_9,#_10,#_11,#_12,#_13,#_14,#_15,#_16,#_17,#_18,#_19,#_20,#_21,#_22,#_23
#define P_ENUM_NAMES_ARG_24(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24                                                                )#_1,#_2,#_3,#_4,#_5,#_6,#_7,#_8,#_9,#_10,#_11,#_12,#_13,#_14,#_15,#_16,#_17,#_18,#_19,#_20,#_21,#_22,#_23,#_24
#define P_ENUM_NAMES_ARG_25(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25                                                            )#_1,#_2,#_3,#_4,#_5,#_6,#_7,#_8,#_9,#_10,#_11,#_12,#_13,#_14,#_15,#_16,#_17,#_18,#_19,#_20,#_21,#_22,#_23,#_24,#_25
#define P_ENUM_NAMES_ARG_26(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26                                                        )#_1,#_2,#_3,#_4,#_5,#_6,#_7,#_8,#_9,#_10,#_11,#_12,#_13,#_14,#_15,#_16,#_17,#_18,#_19,#_20,#_21,#_22,#_23,#_24,#_25,#_26
#define P_ENUM_NAMES_ARG_27(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27                                                    )#_1,#_2,#_3,#_4,#_5,#_6,#_7,#_8,#_9,#_10,#_11,#_12,#_13,#_14,#_15,#_16,#_17,#_18,#_19,#_20,#_21,#_22,#_23,#_24,#_25,#_26,#_27
#define P_ENUM_NAMES_ARG_28(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28                                                )#_1,#_2,#_3,#_4,#_5,#_6,#_7,#_8,#_9,#_10,#_11,#_12,#_13,#_14,#_15,#_16,#_17,#_18,#_19,#_20,#_21,#_22,#_23,#_24,#_25,#_26,#_27,#_28
#define P_ENUM_NAMES_ARG_29(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29                                            )#_1,#_2,#_3,#_4,#_5,#_6,#_7,#_8,#_9,#_10,#_11,#_12,#_13,#_14,#_15,#_16,#_17,#_18,#_19,#_20,#_21,#_22,#_23,#_24,#_25,#_26,#_27,#_28,#_29
#define P_ENUM_NAMES_ARG_30(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30                                        )#_1,#_2,#_3,#_4,#_5,#_6,#_7,#_8,#_9,#_10,#_11,#_12,#_13,#_14,#_15,#_16,#_17,#_18,#_19,#_20,#_21,#_22,#_23,#_24,#_25,#_26,#_27,#_28,#_29,#_30
#define P_ENUM_NAMES_ARG_31(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31                                    )#_1,#_2,#_3,#_4,#_5,#_6,#_7,#_8,#_9,#_10,#_11,#_12,#_13,#_14,#_15,#_16,#_17,#_18,#_19,#_20,#_21,#_22,#_23,#_24,#_25,#_26,#_27,#_28,#_29,#_30,#_31
#define P_ENUM_NAMES_ARG_32(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32                                )#_1,#_2,#_3,#_4,#_5,#_6,#_7,#_8,#_9,#_10,#_11,#_12,#_13,#_14,#_15,#_16,#_17,#_18,#_19,#_20,#_21,#_22,#_23,#_24,#_25,#_26,#_27,#_28,#_29,#_30,#_31,#_32
#define P_ENUM_NAMES_ARG_33(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33                            )#_1,#_2,#_3,#_4,#_5,#_6,#_7,#_8,#_9,#_10,#_11,#_12,#_13,#_14,#_15,#_16,#_17,#_18,#_19,#_20,#_21,#_22,#_23,#_24,#_25,#_26,#_27,#_28,#_29,#_30,#_31,#_32,#_33
#define P_ENUM_NAMES_ARG_34(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34                        )#_1,#_2,#_3,#_4,#_5,#_6,#_7,#_8,#_9,#_10,#_11,#_12,#_13,#_14,#_15,#_16,#_17,#_18,#_19,#_20,#_21,#_22,#_23,#_24,#_25,#_26,#_27,#_28,#_29,#_30,#_31,#_32,#_33,#_34
#define P_ENUM_NAMES_ARG_35(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35                    )#_1,#_2,#_3,#_4,#_5,#_6,#_7,#_8,#_9,#_10,#_11,#_12,#_13,#_14,#_15,#_16,#_17,#_18,#_19,#_20,#_21,#_22,#_23,#_24,#_25,#_26,#_27,#_28,#_29,#_30,#_31,#_32,#_33,#_34,#_35
#define P_ENUM_NAMES_ARG_36(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36                )#_1,#_2,#_3,#_4,#_5,#_6,#_7,#_8,#_9,#_10,#_11,#_12,#_13,#_14,#_15,#_16,#_17,#_18,#_19,#_20,#_21,#_22,#_23,#_24,#_25,#_26,#_27,#_28,#_29,#_30,#_31,#_32,#_33,#_34,#_35,#_36
#define P_ENUM_NAMES_ARG_37(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37            )#_1,#_2,#_3,#_4,#_5,#_6,#_7,#_8,#_9,#_10,#_11,#_12,#_13,#_14,#_15,#_16,#_17,#_18,#_19,#_20,#_21,#_22,#_23,#_24,#_25,#_26,#_27,#_28,#_29,#_30,#_31,#_32,#_33,#_34,#_35,#_36,#_37
#define P_ENUM_NAMES_ARG_38(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38        )#_1,#_2,#_3,#_4,#_5,#_6,#_7,#_8,#_9,#_10,#_11,#_12,#_13,#_14,#_15,#_16,#_17,#_18,#_19,#_20,#_21,#_22,#_23,#_24,#_25,#_26,#_27,#_28,#_29,#_30,#_31,#_32,#_33,#_34,#_35,#_36,#_37,#_38
#define P_ENUM_NAMES_ARG_39(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39    )#_1,#_2,#_3,#_4,#_5,#_6,#_7,#_8,#_9,#_10,#_11,#_12,#_13,#_14,#_15,#_16,#_17,#_18,#_19,#_20,#_21,#_22,#_23,#_24,#_25,#_26,#_27,#_28,#_29,#_30,#_31,#_32,#_33,#_34,#_35,#_36,#_37,#_38,#_39
#define P_ENUM_NAMES_ARG_40(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40)#_1,#_2,#_3,#_4,#_5,#_6,#_7,#_8,#_9,#_10,#_11,#_12,#_13,#_14,#_15,#_16,#_17,#_18,#_19,#_20,#_21,#_22,#_23,#_24,#_25,#_26,#_27,#_28,#_29,#_30,#_31,#_32,#_33,#_34,#_35,#_36,#_37,#_38,#_39,#_40

/** This declares the functions and string table for the text names of an enum.
    The enum is expected to be defined using P_DECLARE_ENUM() or P_DECLARE_ENUM_EX()
    and adds functions MyEnumToString() and MyEnumFromString() to convert between
    the string representation and the enum. The iostream operator<< and operator>>
    is also defined.
  */
#define P_DECLARE_ENUM_NAMES(name, ...) \
  struct PEnumNames_##name { \
    static char const * const * Names() { static char const * const Strings[] = { __VA_ARGS__ }; return Strings; } \
  }; \
  friend __inline std::ostream & operator<<(std::ostream & strm, name e) \
    { PPrintEnum(strm, e, Begin##name, End##name, PEnumNames_##name::Names()); return strm; } \
  friend __inline std::istream & operator>>(std::istream & strm, name & e) \
    { e = (name)PReadEnum(strm, Begin##name, End##name, PEnumNames_##name::Names()); return strm; } \
  static __inline const char * name##ToString(name e) { return e >= Begin##name && e < End##name ? PAssertNULL(PEnumNames_##name::Names()[e-Begin##name]) : ""; } \
  static __inline name name##FromString(const char * str, bool matchCase = true) { return (name)PParseEnum(str, Begin##name, End##name, PEnumNames_##name::Names(), matchCase); }

/** This declares a standard enumeration using P_DECLARE_ENUM_EX() and adds
    the text names so can be streamed using operator<< or operator>>. See
    P_DECLARE_ENUM_NAMES() for more
  */
#define P_DECLARE_STREAMABLE_ENUM_EX(name, countName, firstName, firstValue, ...) \
  P_DECLARE_ENUM_EX(name, countName, firstName, firstValue, __VA_ARGS__) \
  P_DECLARE_ENUM_NAMES(name, #firstName, P_ENUM_NAMES_PART1(PARG_COUNT(__VA_ARGS__), (__VA_ARGS__)))

/** This declares a standard enumeration using P_DECLARE_ENUM() and adds
    the text names so can be streamed using operator<< or operator>>. See
    P_DECLARE_ENUM_NAMES() for more
  */
#define P_DECLARE_STREAMABLE_ENUM(name, first, ...) P_DECLARE_STREAMABLE_ENUM_EX(name, Num##name, first, 0, __VA_ARGS__)


///////////////////////////////////////////////////////////////////////////////
// Declare the debugging support

/// Information about a source file location
struct PDebugLocation
{
  const char * m_file;
  unsigned     m_line;
  const char * m_extra; // Function or class name, if available

  PDebugLocation(const char * extra = NULL) // Not explicit for backward compatibility
      : m_file(NULL), m_line(0), m_extra(extra) { }
  PDebugLocation(const char * file, unsigned line, const char * extra = NULL)
      : m_file(file), m_line(line), m_extra(extra) { }
  PDebugLocation(const PDebugLocation * location);

  void PrintOn(ostream & strm, const char * prefix = NULL) const;
};

__inline ostream & operator<<(ostream & strm, const PDebugLocation & location) { location.PrintOn(strm); return strm; }

#define P_DEBUG_LOCATION PDebugLocation(__FILE__, __LINE__)


#ifndef P_USE_ASSERTS
#define P_USE_ASSERTS 1
#endif

#if !P_USE_ASSERTS

#define PAssert(b, m) (b)
#define PAssert2(b, c, m) (b)
#define PAssertOS(b) (b)
#define PAssertNULL(p) (p)
#define PAssertAlways(m) {}
#define PAssertAlways2(c, m) {}

#else // P_USE_ASSERTS

/// Standard assert messages for the PAssert macro.
enum PStandardAssertMessage {
  PLogicError,              ///< A logic error occurred.
  POutOfMemory,             ///< A new or malloc failed.
  PNullPointerReference,    ///< A reference was made through a NULL pointer.
  PInvalidCast,             ///< An invalid cast to descendant is required.
  PInvalidArrayIndex,       ///< An index into an array was negative.
  PInvalidArrayElement,     ///< A NULL array element object was accessed.
  PStackEmpty,              ///< A Pop() was made of a stack with no elements.
  PUnimplementedFunction,   ///< Funtion is not implemented.
  PInvalidParameter,        ///< Invalid parameter was passed to a function.
  POperatingSystemError,    ///< Error was returned by Operating System.
  PChannelNotOpen,          ///< Operation attempted when channel not open.
  PUnsupportedFeature,      ///< Feature is not supported.
  PInvalidWindow,           ///< Access through invalid window.
  PMaxStandardAssertMessage ///< Number of standard assert message.
};

#define __CLASS__ NULL

enum PAssertWalkStackModes { PAssertWalkStackDisabled, PAssertWalkStackSymbols, PAssertWalkStackNoSymbols };
extern PAssertWalkStackModes PAssertWalkStackMode;
extern unsigned PAssertCount;
bool PAssertFunc(const PDebugLocation & location, PStandardAssertMessage msg);
bool PAssertFunc(const PDebugLocation & location, const char * msg);
bool PAssertException(const char * source, const std::exception * ex);


/** This macro is used to assert that a condition must be true.
If the condition is false then an assert function is called with the source
file and line number the macro was instantiated on, plus the message described
by the <code>msg</code> parameter. This parameter may be either a standard value
from the <code>PStandardAssertMessage</code> enum or a literal string.
*/
#define PAssert(b, msg) ((b)?true:PAssertFunc(PDebugLocation(__FILE__,__LINE__,__CLASS__),(msg)))

/** This macro is used to assert that a condition must be true.
If the condition is false then an assert function is called with the source
file and line number the macro was instantiated on, plus the message described
by the <code>msg</code> parameter. This parameter may be either a standard value
from the <code>PStandardAssertMessage</code> enum or a literal string.
The <code>cls</code> parameter specifies the class name that the error occurred in
*/
#define PAssert2(b, cls, msg) ((b)?true:PAssertFunc(PDebugLocation(__FILE__,__LINE__,(cls)),(msg)))

/** This macro is used to assert that an operating system call succeeds.
If the condition is false then an assert function is called with the source
file and line number the macro was instantiated on, plus the message
described by the <code>POperatingSystemError</code> value in the <code>PStandardAssertMessage</code>
enum.
 */
#define PAssertOS(b) ((b)?true:PAssertFunc(PDebugLocation(__FILE__,__LINE__,__CLASS__),POperatingSystemError))

/** This macro is used to assert that a pointer must be non-null.
If the pointer is NULL then an assert function is called with the source file
and line number the macro was instantiated on, plus the message described by
the PNullPointerReference value in the <code>PStandardAssertMessage</code> enum.

Note that this evaluates the expression defined by <code>ptr</code> twice. To
prevent incorrect behaviour with this, the macro will assume that the
<code>ptr</code> parameter is an L-Value.
 */
#define PAssertNULL(ptr) (((ptr)!=NULL)?(ptr): \
                     (PAssertFunc(PDebugLocation(__FILE__,__LINE__, __CLASS__), PNullPointerReference),(ptr)))

/** This macro is used to assert immediately.
The assert function is called with the source file and line number the macro
was instantiated on, plus the message described by the <code>msg</code> parameter. This
parameter may be either a standard value from the <code>PStandardAssertMessage</code>
enum or a literal string.
*/
#define PAssertAlways(msg) PAssertFunc(PDebugLocation(__FILE__,__LINE__,__CLASS__),(msg))

/** This macro is used to assert immediately.
The assert function is called with the source file and line number the macro
was instantiated on, plus the message described by the <code>msg</code> parameter. This
parameter may be either a standard value from the <code>PStandardAssertMessage</code>
enum or a literal string.
*/
#define PAssertAlways2(cls, msg) PAssertFunc(PDebugLocation(__FILE__,__LINE__,(cls)),(msg))

#endif // P_USE_ASSERTS


#define PCLASSINFO_ALIGNED(cls, par, align) \
  public: \
    typedef cls P_thisClass; \
    __inline static const char * Class() { return typeid(cls).name(); } \
    __inline bool IsClass(const char * name) const { return strcmp(name, Class()) == 0; } \
    virtual PObject::Comparison CompareObjectMemoryDirect(const PObject & obj) const override \
      { return PObject::InternalCompareObjectMemoryDirect(this, dynamic_cast<const cls *>(&obj), sizeof(cls)); } \
    PNEW_AND_DELETE_FUNCTIONS(align)


/** Declare all the standard PTLib class information.
This macro is used to provide the basic run-time typing capability needed
by the library. All descendent classes from the <code>PObject</code> class require
these functions for correct operation. Either use this macro or the
<code>#PDECLARE_CLASS</code> macro.

The use of the <code>#PDECLARE_CLASS</code> macro is no longer recommended for reasons
of compatibility with documentation systems.
*/

#define PCLASSINFO(cls, par) PCLASSINFO_ALIGNED(cls, (par), 0)

/// Declare all the standard PTLib class information, plus Clone().
#define PCLASSINFO_WITH_CLONE(cls, par) \
    PCLASSINFO(cls, par) \
    virtual PObject * Clone() const override { return new cls(*this); }


#define PIsDescendant(ptr, cls)    (dynamic_cast<const cls *>(ptr) != NULL) 
#define PRemoveConst(cls, ptr)  (const_cast<cls*>(ptr))

template <class CLS, class BASE> CLS & PCastPtrToRef(BASE * obj)
{
  if (obj)
    return dynamic_cast<CLS &>(*obj);
  PAssertAlways(PNullPointerReference);
  std::abort();
}

template <class CLS, class BASE> const CLS & PConstCastPtrToRef(const BASE * obj)
{
  return PCastPtrToRef<const CLS, const BASE>(obj);
}

#if P_USE_ASSERTS
template<class BaseClass> inline BaseClass * PAssertCast(BaseClass * obj, const char * file, int line) 
  { if (obj == NULL) PAssertFunc(PDebugLocation(file, line, obj->Class()), PInvalidCast); return obj; }
#define PDownCast(cls, ptr) PAssertCast<cls>(dynamic_cast<cls*>(ptr),__FILE__,__LINE__)
#else
#define PDownCast(cls, ptr) (dynamic_cast<cls*>(ptr))
#endif


/** Declare a class with PWLib class information.
This macro is used to declare a new class with a single public ancestor. It
starts the class declaration and then uses the <code>#PCLASSINFO</code> macro to
get all the run-time type functions.

The use of this macro is no longer recommended for reasons of compatibility
with documentation systems.
*/
#define PDECLARE_CLASS(cls, par) class cls : public par { PCLASSINFO(cls, par)
#ifdef DOC_PLUS_PLUS
} Match previous opening brace in doc++
#endif

///////////////////////////////////////////////////////////////////////////////
// The root of all evil ... umm classes

/** Ultimate parent class for all objects in the class library.
This provides functionality provided to all classes, eg run-time types,
default comparison operations, simple stream I/O and serialisation support.
*/
class PObject {
  protected:
    unsigned m_traceContextIdentifier;
#if PTRACING==2
  public:
    /**Get PTRACE context identifier
      */
    __inline unsigned GetTraceContextIdentifier() const { return m_traceContextIdentifier; }
    __inline void SetTraceContextIdentifier(unsigned id) { m_traceContextIdentifier = id; }
    __inline void SetTraceContextIdentifier(const PObject & obj) { m_traceContextIdentifier = obj.m_traceContextIdentifier; }
    __inline void SetTraceContextIdentifier(const PObject * obj) { if (obj != NULL) m_traceContextIdentifier = obj->m_traceContextIdentifier; }
    __inline void CopyTraceContextIdentifier(PObject & obj) const { obj.m_traceContextIdentifier = m_traceContextIdentifier; }
    __inline void CopyTraceContextIdentifier(PObject * obj) const { if (obj != NULL) obj->m_traceContextIdentifier = m_traceContextIdentifier; }
    __inline static void CopyTraceContextIdentifier(PObject & to, const PObject & from) { to.SetTraceContextIdentifier(from); }
    __inline static void CopyTraceContextIdentifier(PObject & to, const PObject * from) { to.SetTraceContextIdentifier(from); }
    __inline static void CopyTraceContextIdentifier(PObject * to, const PObject & from) { from.CopyTraceContextIdentifier(to); }
    __inline static void CopyTraceContextIdentifier(PObject * to, const PObject * from) { if (to != NULL) to->SetTraceContextIdentifier(from); }
#endif // PTRACING==2

  protected:
    /** Constructor for PObject, made protected so cannot ever create one on
       its own.
     */
    PObject()
      : m_traceContextIdentifier(0)
    { }

  public:
    /* Destructor required to get the "virtual". A PObject really has nothing
       to destroy.
     */
    virtual ~PObject() { }

    // Backward compatibility, use RTTI from now on!
    __inline static const char * Class() { return typeid(PObject).name(); }
    __inline const char * GetClass() const { return typeid(const_cast<PObject &>(*this)).name(); }
    __inline bool IsClass(const char * name) const { return strcmp(name, Class()) == 0; }
    static std::string GetClassName(const std::type_info & t);
    __inline std::string GetClassName() const { return GetClassName(typeid(const_cast<PObject &>(*this))); }

    __inline const PObject * PTraceObjectInstance() const { return this; }
    __inline static const PObject * PTraceObjectInstance(const char *) { return NULL; }
    __inline static const PObject * PTraceObjectInstance(const PObject * obj) { return obj; }

  /**@name Comparison functions */
  //@{
    /** Result of the comparison operation performed by the <code>Compare()</code>
       function.
      */
    enum Comparison {
      LessThan = -1,
      EqualTo = 0,
      GreaterThan = 1
    };

    /// Compare two types, returning Comparison type.
    template<typename T> static Comparison Compare2(T v1, T v2)
    {
      if (v1 < v2)
        return LessThan;
      if (v1 > v2)
        return GreaterThan;
      return EqualTo;
    }

    /** Compare the two objects and return their relative rank. This function is
       usually overridden by descendent classes to yield the ranking according
       to the semantics of the object.
       
       The default function is to use the <code>CompareObjectMemoryDirect()</code>
       function to do a byte wise memory comparison of the two objects.

       @return
       <code>LessThan</code>, <code>EqualTo</code> or <code>GreaterThan</code>
       according to the relative rank of the objects.
     */
    virtual Comparison Compare(
      const PObject & obj   // Object to compare against.
    ) const;
    
    /** Determine the byte wise comparison of two objects. This is the default
       comparison operation for objects that do not explicitly override the
       <code>Compare()</code> function.
    
       The <code>#PCLASSINFO</code> macro declares an override of this function for
       the particular class. The user need not implement it.

       @return
       <code>LessThan</code>, <code>EqualTo</code> or <code>GreaterThan</code>
       according to the result <code>memcpy()</code> function.
     */
    virtual Comparison CompareObjectMemoryDirect(
      const PObject & obj   // Object to compare against.
    ) const;

    /// Internal function caled from CompareObjectMemoryDirect()
    static Comparison InternalCompareObjectMemoryDirect(
      const PObject * obj1,
      const PObject * obj2,
      PINDEX size
    );

    /** Compare the two objects.
    
       @return
       true if objects are equal.
     */
    bool operator==(
      const PObject & obj   // Object to compare against.
    ) const { return Compare(obj) == EqualTo; }

    /** Compare the two objects.
    
       @return
       true if objects are not equal.
     */
    bool operator!=(
      const PObject & obj   // Object to compare against.
    ) const { return Compare(obj) != EqualTo; }

    /** Compare the two objects.
    
       @return
       true if objects are less than.
     */
    bool operator<(
      const PObject & obj   // Object to compare against.
    ) const { return Compare(obj) == LessThan; }

    /** Compare the two objects.
    
       @return
       true if objects are greater than.
     */
    bool operator>(
      const PObject & obj   // Object to compare against.
    ) const { return Compare(obj) == GreaterThan; }

    /** Compare the two objects.
    
       @return
       true if objects are less than or equal.
     */
    bool operator<=(
      const PObject & obj   // Object to compare against.
    ) const { return Compare(obj) != GreaterThan; }

    /** Compare the two objects.
    
       @return
       true if objects are greater than or equal.
     */
    bool operator>=(
      const PObject & obj   // Object to compare against.
    ) const { return Compare(obj) != LessThan; }
  //@}

  /**@name I/O functions */
  //@{
    /** Output the contents of the object to the stream. The exact output is
       dependent on the exact semantics of the descendent class. This is
       primarily used by the standard <code>#operator<<</code> function.

       The default behaviour is to print the class name.
     */
    virtual void PrintOn(
      ostream &strm   // Stream to print the object into.
    ) const;

    /** Input the contents of the object from the stream. The exact input is
       dependent on the exact semantics of the descendent class. This is
       primarily used by the standard <code>#operator>></code> function.

       The default behaviour is to do nothing.
     */
    virtual void ReadFrom(
      istream &strm   // Stream to read the objects contents from.
    );


    /** Global function for using the standard << operator on objects descended
       from PObject. This simply calls the objects <code>PrintOn()</code> function.
       
       @return the \p strm parameter.
     */
    inline friend ostream & operator<<(
      ostream &strm,       ///< Stream to print the object into.
      const PObject & obj  ///< Object to print to the stream.
    ) { obj.PrintOn(strm); return strm; }

    /** Global function for using the standard >> operator on objects descended
       from PObject. This simply calls the objects <code>ReadFrom()</code> function.

       @return the \p strm parameter.
     */
    inline friend istream & operator>>(
      istream &strm,   ///< Stream to read the objects contents from.
      PObject & obj    ///< Object to read inormation into.
    ) { obj.ReadFrom(strm); return strm; }


  /**@name Miscellaneous functions */
  //@{
    /** Create a copy of the class on the heap. The exact semantics of the
       descendent class determine what is required to make a duplicate of the
       instance. Not all classes can even \b do a clone operation.
       
       The main user of the clone function is the <code>PDictionary</code> class as
       it requires copies of the dictionary keys.

       The default behaviour is for this function to assert.

       @return
       pointer to new copy of the class instance.
     */
    virtual PObject * Clone() const;

    /** As for Clone() but converts to specified type.
      */
    template <class CLS>
    CLS * CloneAs() const
    {
      PObject * clone = Clone();
      CLS * obj = dynamic_cast<CLS *>(clone);
      if (obj != NULL)
        return obj;
      delete clone;
      return NULL;
    }

    /** This function yields a hash value required by the <code>PDictionary</code>
       class. A descendent class that is required to be the key of a dictionary
       should override this function. The precise values returned is dependent
       on the semantics of the class. For example, the <code>PString</code> class
       overrides it to provide a hash function for distinguishing text strings.

       The default behaviour is to return the value zero.

       @return
       hash function value for class instance.
     */
    virtual PINDEX HashFunction() const;
  //@}
};

///////////////////////////////////////////////////////////////////////////////
// Platform independent types

// All these classes encapsulate primitive types such that they may be
// transfered in a platform independent manner. In particular it is used to
// do byte swapping for little endien and big endien processor architectures
// as well as accommodating structure packing rules for memory structures.

#define PANSI_CHAR 1
#define PLITTLE_ENDIAN 2
#define PBIG_ENDIAN 3


template <typename type>
struct PIntSameOrder {
  __inline PIntSameOrder()                            : data(0)              { }
  __inline PIntSameOrder(type value)                  : data(value)          { }
  __inline PIntSameOrder(const PIntSameOrder & value) : data(value.data)     { }
  __inline PIntSameOrder & operator=(type value)                             { data = value; return *this; }
  __inline PIntSameOrder & operator=(const PIntSameOrder & value)            { data = value.data; return *this; }
  __inline operator type() const                                             { return data; }
  __inline friend ostream & operator<<(ostream & s, const PIntSameOrder & v) { return s << v.data; }
  __inline friend istream & operator>>(istream & s, PIntSameOrder & v)       { return s >> v.data; }

  private:
    type data;
};


template <typename type>
struct PIntReversedOrder {
  __inline PIntReversedOrder()                                : data(0)                   { }
  __inline PIntReversedOrder(type value)                      : data(ReverseBytes(value)) { }
  __inline PIntReversedOrder(const PIntReversedOrder & value) : data(value.data)          { }
  __inline PIntReversedOrder & operator=(type value)                                      { data = ReverseBytes(value);  return *this; }
  __inline PIntReversedOrder & operator=(const PIntReversedOrder & value)                 { data = value.data;           return *this; }
  __inline operator type() const                                                          { return ReverseBytes(data); }
  __inline friend ostream & operator<<(ostream & s, const PIntReversedOrder & value)      { return s << ReverseBytes(value.data); }
  __inline friend istream & operator>>(istream & s, PIntReversedOrder & value)            { type i = 0; s >> i; value = i; return s; }

  private:
    type data;

#if _MSC_VER
__inline static  int8_t  ReverseBytes(const  int8_t  src) { return src; }
__inline static uint8_t  ReverseBytes(const uint8_t  src) { return src; }
__inline static  int16_t ReverseBytes(const  int16_t src) { return _byteswap_ushort(src); }
__inline static uint16_t ReverseBytes(const uint16_t src) { return _byteswap_ushort(src); }
__inline static  int32_t ReverseBytes(const  int32_t src) { return _byteswap_ulong(src); }
__inline static uint32_t ReverseBytes(const uint32_t src) { return _byteswap_ulong(src); }
__inline static  int64_t ReverseBytes(const  int64_t src) { return _byteswap_uint64(src); }
__inline static uint64_t ReverseBytes(const uint64_t src) { return _byteswap_uint64(src); }
__inline static float    ReverseBytes(const    float src) { uint32_t dst = ReverseBytes(*(uint32_t *)&src); return *(float *)&dst; }
__inline static double   ReverseBytes(const   double src) { uint64_t dst = ReverseBytes(*(uint64_t *)&src); return *(double *)&dst; }
#elif __GLIBC__
__inline static  int8_t  ReverseBytes(const  int8_t  src) { return src; }
__inline static uint8_t  ReverseBytes(const uint8_t  src) { return src; }
__inline static  int16_t ReverseBytes(const  int16_t src) { return __bswap_16(src); }
__inline static uint16_t ReverseBytes(const uint16_t src) { return __bswap_16(src); }
__inline static  int32_t ReverseBytes(const  int32_t src) { return __bswap_32(src); }
__inline static uint32_t ReverseBytes(const uint32_t src) { return __bswap_32(src); }
__inline static  int64_t ReverseBytes(const  int64_t src) { return __bswap_64(src); }
__inline static uint64_t ReverseBytes(const uint64_t src) { return __bswap_64(src); }
__inline static float    ReverseBytes(const    float src) { uint32_t dst = ReverseBytes(*(uint32_t *)&src); return *(float *)&dst; }
__inline static double   ReverseBytes(const   double src) { uint64_t dst = ReverseBytes(*(uint64_t *)&src); return *(double *)&dst; }
#else
  static __inline type ReverseBytes(const type & src)
  {
    type dst;
    size_t s = sizeof(type)-1;
    for (size_t d = 0; d < sizeof(type); ++d,--s)
      ((BYTE *)&dst)[d] = ((const BYTE *)&src)[s];
    return dst;
  }
#endif
};

#ifndef PCHAR8
#define PCHAR8 PANSI_CHAR
#endif

#if PCHAR8==PANSI_CHAR
typedef PIntSameOrder<int8_t> PChar8;
#endif

typedef PIntSameOrder<int8_t> PInt8;

typedef PIntSameOrder<uint8_t> PUInt8;

#if PBYTE_ORDER==PLITTLE_ENDIAN
typedef PIntSameOrder<int16_t> PInt16l;
#elif PBYTE_ORDER==PBIG_ENDIAN
typedef PIntReversedOrder<int16_t> PInt16l;
#endif

#if PBYTE_ORDER==PLITTLE_ENDIAN
typedef PIntReversedOrder<int16_t> PInt16b;
#elif PBYTE_ORDER==PBIG_ENDIAN
typedef PIntSameOrder<int16_t> PInt16b;
#endif

#if PBYTE_ORDER==PLITTLE_ENDIAN
typedef PIntSameOrder<uint16_t> PUInt16l;
#elif PBYTE_ORDER==PBIG_ENDIAN
typedef PIntReversedOrder<uint16_t> PUInt16l;
#endif

#if PBYTE_ORDER==PLITTLE_ENDIAN
typedef PIntReversedOrder<uint16_t> PUInt16b;
#elif PBYTE_ORDER==PBIG_ENDIAN
typedef PIntSameOrder<uint16_t> PUInt16b;
#endif

#if PBYTE_ORDER==PLITTLE_ENDIAN
typedef PIntSameOrder<int32_t> PInt32l;
#elif PBYTE_ORDER==PBIG_ENDIAN
typedef PIntReversedOrder<int32_t> PInt32l;
#endif

#if PBYTE_ORDER==PLITTLE_ENDIAN
typedef PIntReversedOrder<int32_t> PInt32b;
#elif PBYTE_ORDER==PBIG_ENDIAN
typedef PIntSameOrder<PInt32> int32_t;
#endif

#if PBYTE_ORDER==PLITTLE_ENDIAN
typedef PIntSameOrder<uint32_t> PUInt32l;
#elif PBYTE_ORDER==PBIG_ENDIAN
typedef PIntReversedOrder<uint32_t> PUInt32l;
#endif

#if PBYTE_ORDER==PLITTLE_ENDIAN
typedef PIntReversedOrder<uint32_t> PUInt32b;
#elif PBYTE_ORDER==PBIG_ENDIAN
typedef PIntSameOrder<uint32_t> PUInt32b;
#endif

#if PBYTE_ORDER==PLITTLE_ENDIAN
typedef PIntSameOrder<int64_t> PInt64l;
#elif PBYTE_ORDER==PBIG_ENDIAN
typedef PIntReversedOrder<int64_t> PInt64l;
#endif

#if PBYTE_ORDER==PLITTLE_ENDIAN
typedef PIntReversedOrder<int64_t> PInt64b;
#elif PBYTE_ORDER==PBIG_ENDIAN
typedef PIntSameOrder<int64_t> PInt64b;
#endif

#if PBYTE_ORDER==PLITTLE_ENDIAN
typedef PIntSameOrder<uint64_t> PUInt64l;
#elif PBYTE_ORDER==PBIG_ENDIAN
typedef PIntReversedOrder<uint64_t> PUInt64l;
#endif

#if PBYTE_ORDER==PLITTLE_ENDIAN
typedef PIntReversedOrder<uint64_t> PUInt64b;
#elif PBYTE_ORDER==PBIG_ENDIAN
typedef PIntSameOrder<uint64_t> PUInt64b;
#endif

#if PBYTE_ORDER==PLITTLE_ENDIAN
typedef PIntSameOrder<float> PFloat32l;
#elif PBYTE_ORDER==PBIG_ENDIAN
typedef PIntReversedOrder<float> PFloat32l;
#endif

#if PBYTE_ORDER==PLITTLE_ENDIAN
typedef PIntReversedOrder<float> PFloat32b;
#elif PBYTE_ORDER==PBIG_ENDIAN
typedef PIntSameOrder<float> PFloat32b;
#endif

#if PBYTE_ORDER==PLITTLE_ENDIAN
typedef PIntSameOrder<double> PFloat64l;
#elif PBYTE_ORDER==PBIG_ENDIAN
typedef PIntReversedOrder<double> PFloat64l;
#endif

#if PBYTE_ORDER==PLITTLE_ENDIAN
typedef PIntReversedOrder<double> PFloat64b;
#elif PBYTE_ORDER==PBIG_ENDIAN
typedef PIntSameOrder<double> PFloat64b;
#endif

#ifndef NO_LONG_DOUBLE // stupid OSX compiler
#if PBYTE_ORDER==PLITTLE_ENDIAN
typedef PIntSameOrder<long double> PFloat80l;
#elif PBYTE_ORDER==PBIG_ENDIAN
typedef PIntReversedOrder<long double> PFloat80l;
#endif

#if PBYTE_ORDER==PLITTLE_ENDIAN
typedef PIntReversedOrder<long double> PFloat80b;
#elif PBYTE_ORDER==PBIG_ENDIAN
typedef PIntSameOrder<long double> PFloat80b;
#endif
#endif

typedef intptr_t P_INT_PTR;

#if defined(_MSC_VER)
#define P_ALIGN_FIELD(fieldType,fieldName,alignment) fieldType __declspec(align(alignment)) fieldName
#elif defined(__GNUC__)
#define P_ALIGN_FIELD(fieldType,fieldName,alignment) fieldType fieldName __attribute__ ((aligned(alignment)))
#endif

typedef intptr_t P_INT_PTR;

///////////////////////////////////////////////////////////////////////////////
// Miscellaneous

/*$MACRO PARRAYSIZE(array)
   This macro is used to calculate the number of array elements in a static
   array.
 */
#define PARRAYSIZE(array) ((PINDEX)(sizeof(array)/sizeof(array[0])))

/*$MACRO PMIN(v1, v2)
   This macro is used to calculate the minimum of two values.
   Maps to std::min and is for backward compatibility only.
 */
#define PMIN(v1, v2) std::min(v1, v2)

/*$MACRO PMAX(v1, v2)
   This macro is used to calculate the maximum of two values.
   Maps to std::max and is for backward compatibility only.
 */
#define PMAX(v1, v2) std::max(v1, v2)

/*$MACRO PABS(val)
   This macro is used to calculate an absolute value.
   Maps to std::abs and is for backward compatibility only.
 */
#define PABS(v) std::abs(v)


///////////////////////////////////////////////////////////////////////////////
// Deal with different C++ versions and std::auto_ptr deprecation
#if __cplusplus < 201103L
#define override

struct PNonCopyable
{
  PNonCopyable() { }
private:
  PNonCopyable(const PNonCopyable &) { }
  PNonCopyable & operator=(const PNonCopyable &) { return *this; }
};
template <typename T> class PAutoPtr : public std::auto_ptr<T>
{
public:
  PAutoPtr() { }
  explicit PAutoPtr(T * p) : std::auto_ptr<T>(p) { }
  PAutoPtr(PAutoPtr & other) : std::auto_ptr<T>(other.release()) { }
  void transfer(PAutoPtr & other) { this->reset(other.release()); }
  bool operator!() const { return this->get() == NULL; }
  operator bool() const { return this->get() != NULL; }
};
#else // __cplusplus
struct PNonCopyable
{
  PNonCopyable() = default;
  PNonCopyable(const PNonCopyable &) = delete;
  PNonCopyable & operator=(const PNonCopyable &) = delete;
};
template <typename T> class PAutoPtr : public std::unique_ptr<T>
{
public:
  PAutoPtr() = default;
  explicit PAutoPtr(T * p) : std::unique_ptr<T>(p) { }
  PAutoPtr(PAutoPtr & other) : std::unique_ptr<T>(other.release()) { }
  void transfer(PAutoPtr & other) { std::unique_ptr<T>::operator=(std::move(other)); }
};
#endif // __cplusplus


///////////////////////////////////////////////////////////////////////////////

void PThreadYield();

template<class Type> Type * PSingletonCreatorDefault()
{
  return new Type();
}

/** Template class for a simple singleton object.
     Usage is typically like:
<pre><code>
        typedef PSingleton<MyClass> MySingleton;
        MySingleton()->DoSomething();
</code></pre>
     Default is not thread safe, however the following is:
<pre><code>
        typedef PSingleton<MyClass, atomic<uint32_t> > MySafeSingleton;
        MySafeSingleton()->DoSomething();
</code></pre>
     If the singleton class requires parameters on construction, then a creator
     function may be included in the template parameters:
 <pre><code>
        MyClass * CreateMyClass() { return new MyClass("fred"); }
        typedef PSingleton<MyClass, atomic<uint32_t>, CreateMyClass> MySafeSingleton;
        MySafeSingleton()->DoSomething();
</code></pre>
*/
template <class Type, typename GuardType = unsigned, Type * (*Creator)() = PSingletonCreatorDefault<Type> >
class PSingleton
{
protected:
  Type * m_instance;
public:
  PSingleton()
  {
    static PAutoPtr<Type> s_pointer;
    static GuardType s_guard(0);
    if (s_guard++ != 0) {
      s_guard = 1;
      while ((m_instance = s_pointer.get()) == NULL)
        PThreadYield();
    }
    else {
#if PMEMORY_HEAP
        // Do this to make sure debugging is initialised as early as possible
      PMemoryHeap::Validate(NULL, NULL, NULL);
#endif
      s_pointer.reset(Creator());
      m_instance = s_pointer.get();
    }
  }

  Type * operator->() const { return  m_instance; }
  Type & operator* () const { return *m_instance; }
};


// Template class for thread safe singleton
template <class Type, Type * (*Creator)() = PSingletonCreatorDefault<Type> >
class PSafeSingleton : public PSingleton<Type, atomic<unsigned>, Creator>
{ };



#include "tracing.h"
#include "memchk.h"
#include "profiling.h"

#endif // PTLIB_OBJECT_H


// End Of File ///////////////////////////////////////////////////////////////
