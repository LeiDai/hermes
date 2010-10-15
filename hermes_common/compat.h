/*
   This header file contains platform compatibility layer.

   It is included from common.h, so it is automatically included in all hermes
   sources. The implementation of the functions in this file is in the
   src/compat directory.
*/

#ifndef __H3D_COMPAT_H
#define __H3D_COMPAT_H
#include <stdio.h>

#ifndef HAVE_FMEMOPEN
/// Implementation of GNU fmemopen. Intended to be used if the current platform does not support it.
FILE *fmemopen (void *buf, size_t size, const char *opentype);
#endif

// Windows DLL export/import definitions
#if defined(WIN32) || defined(_WINDOWS)

  #if defined(EXPORT_HERMES_DLL)
  // when building DLL (target project defines this macro)
    #define HERMES_API __declspec(dllexport)
    #define HERMES_API_USED_TEMPLATE(__implementation) template class HERMES_API __implementation
  #else  
  // when using the DLL by a client project
    #define HERMES_API __declspec(dllimport)
    #define HERMES_API_USED_TEMPLATE(__implementation)
    //#define HERMES_API_USED_TEMPLATE(__implementation) extern template class HERMES_API __implementation
  #endif
  #define HERMES_API_USED_STL_VECTOR(__type) HERMES_API_USED_TEMPLATE(std::allocator<__type>); HERMES_API_USED_TEMPLATE(std::vector<__type>)
  
#else 

  #define HERMES_API
  #define HERMES_API_USED_TEMPLATE(__implementation)
  #define HERMES_API_USED_STL_VECTOR(__type)

#endif


//C99 functions
#include "compat/c99_functions.h"

#endif
