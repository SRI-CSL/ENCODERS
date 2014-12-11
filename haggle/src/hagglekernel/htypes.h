/* Copyright (c) 2014 SRI International
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Hasnain Lakhani (HL)
 */

// Haggle's type definitions for compatability between stl and haggle objects is incomplete
// This file aims to resolve that for classes who change dependant on the ENABLE_STL define
#include <libcpphaggle/Exception.h>
#include <libcpphaggle/HashMap.h>
#include <libcpphaggle/List.h>
#include <libcpphaggle/Map.h>
#include <libcpphaggle/Pair.h>
#include <libcpphaggle/String.h>

#define HMAP Map // Haggle already has switching enabled for Map
#define HEXCEPTION haggle::Exception

#if defined(ENABLE_STL)
  #define HHASHMAP  std::multimap
  #define HLIST     std::list
  #define HMULTIMAP std::multimap
  #define HPAIR     std::pair
  #define HSTRING   std::string
#else
  #define HHASHMAP  haggle::HashMap
  #define HLIST     haggle::List  
  #define HMULTIMAP haggle::MultiMap
  #define HPAIR     haggle::Pair
  #define HSTRING   haggle::String
#endif
