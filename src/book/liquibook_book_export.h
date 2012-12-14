
// -*- C++ -*-
// $Id$
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl LIQUIBOOK_BOOK
// ------------------------------
#ifndef LIQUIBOOK_BOOK_EXPORT_H
#define LIQUIBOOK_BOOK_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (LIQUIBOOK_BOOK_HAS_DLL)
#  define LIQUIBOOK_BOOK_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && LIQUIBOOK_BOOK_HAS_DLL */

#if !defined (LIQUIBOOK_BOOK_HAS_DLL)
#  define LIQUIBOOK_BOOK_HAS_DLL 1
#endif /* ! LIQUIBOOK_BOOK_HAS_DLL */

#if defined (LIQUIBOOK_BOOK_HAS_DLL) && (LIQUIBOOK_BOOK_HAS_DLL == 1)
#  if defined (LIQUIBOOK_BOOK_BUILD_DLL)
#    define LIQUIBOOK_BOOK_Export ACE_Proper_Export_Flag
#    define LIQUIBOOK_BOOK_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define LIQUIBOOK_BOOK_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* LIQUIBOOK_BOOK_BUILD_DLL */
#    define LIQUIBOOK_BOOK_Export ACE_Proper_Import_Flag
#    define LIQUIBOOK_BOOK_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define LIQUIBOOK_BOOK_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* LIQUIBOOK_BOOK_BUILD_DLL */
#else /* LIQUIBOOK_BOOK_HAS_DLL == 1 */
#  define LIQUIBOOK_BOOK_Export
#  define LIQUIBOOK_BOOK_SINGLETON_DECLARATION(T)
#  define LIQUIBOOK_BOOK_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* LIQUIBOOK_BOOK_HAS_DLL == 1 */

// Set LIQUIBOOK_BOOK_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (LIQUIBOOK_BOOK_NTRACE)
#  if (ACE_NTRACE == 1)
#    define LIQUIBOOK_BOOK_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define LIQUIBOOK_BOOK_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !LIQUIBOOK_BOOK_NTRACE */

#if (LIQUIBOOK_BOOK_NTRACE == 1)
#  define LIQUIBOOK_BOOK_TRACE(X)
#else /* (LIQUIBOOK_BOOK_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define LIQUIBOOK_BOOK_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (LIQUIBOOK_BOOK_NTRACE == 1) */

#endif /* LIQUIBOOK_BOOK_EXPORT_H */

// End of auto generated file.
