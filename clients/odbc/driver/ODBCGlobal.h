/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0.  If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright 2024, 2025 MonetDB Foundation;
 * Copyright August 2008 - 2023 MonetDB B.V.;
 * Copyright 1997 - July 2008 CWI.
 */

/*
 * This code was created by Peter Harvey (mostly during Christmas 98/99).
 * This code is LGPL. Please ensure that this message remains in future
 * distributions and uses of this code (that's about all I get out of it).
 * - Peter Harvey pharvey@codebydesign.com
 *
 * This file has been modified for the MonetDB project.  See the file
 * Copyright in this directory for more information.
 */

/**********************************************
 * ODBCGlobal.h
 *
 * Description:
 * The global MonetDB ODBC include file which
 * includes all needed external include files.
 *
 * Author: Martin van Dinther, Sjoerd Mullender
 * Date  : 30 aug 2002
 *
 **********************************************/

#ifndef _H_ODBCGLOBAL
#define _H_ODBCGLOBAL

#include "monetdb_config.h"
#include "mstring.h"

/**** Define the ODBC Version this ODBC driver complies with ****/
#define ODBCVER 0x0352		/* Important: this must be defined before include of sqlext.h */

/* some general defines */
#define MONETDB_ODBC_VER     "03.52"	/* must be synchronous with ODBCVER */
#define MONETDB_DRIVER_NAME  "MonetDBODBClib"
#define MONETDB_PRODUCT_NAME "MonetDB ODBC driver"
#define MONETDB_SERVER_NAME  "MonetDB"

#define ODBCDEBUG	1

/* standard ODBC driver include files */
#include <sqltypes.h>		/* ODBC C typedefs */
/* Note: sqlext.h includes sql.h so it is not needed here to be included */
/* Note2: if you include sql.h it will give an error because it will find
	src/sql/common/sql.h instead, which is not the one we need */
#include <sqlext.h>		/* ODBC API definitions and prototypes */
#include <sqlucode.h>		/* ODBC Unicode defs and prototypes */

/* standard ODBC driver installer & configurator include files */
#include <odbcinst.h>		/* ODBC installer definitions and prototypes */

/* standard C include files */
#include <string.h>		/* for strcpy() etc. */
#include <ctype.h>

#ifdef SQLLEN			/* it's a define for 32, a typedef for 64 */
#define LENFMT		"%" PRId32
#define ULENFMT		"%" PRIu32
#define LENCAST		(int32_t)
#define ULENCAST	(uint32_t)
#else
#define LENFMT		"%" PRId64
#define ULENFMT		"%" PRIu64
#define LENCAST		(int64_t)
#define ULENCAST	(uint64_t)
#endif

#define SQL_HUGEINT	0x4000

/* these functions are called from within the library */
SQLRETURN MNDBAllocHandle(SQLSMALLINT nHandleType, SQLHANDLE nInputHandle, SQLHANDLE *pnOutputHandle);
SQLRETURN MNDBEndTran(SQLSMALLINT nHandleType, SQLHANDLE nHandle, SQLSMALLINT nCompletionType);
SQLRETURN MNDBFreeHandle(SQLSMALLINT handleType, SQLHANDLE handle);
SQLRETURN MNDBGetDiagRec(SQLSMALLINT handleType, SQLHANDLE handle, SQLSMALLINT recNumber, SQLCHAR *sqlState, SQLINTEGER *nativeErrorPtr, SQLCHAR *messageText, SQLSMALLINT bufferLength, SQLSMALLINT *textLengthPtr);

#ifdef ODBCDEBUG
#ifdef NATIVE_WIN32
extern const wchar_t *ODBCdebug;
#else
extern const char *ODBCdebug;
#endif

extern void setODBCdebug(const char *filename, bool overrideEnvVar);

static inline void ODBCLOG(_In_z_ _Printf_format_string_ const char *fmt, ...)
	__attribute__((__format__(__printf__, 1, 2)));

static inline void
ODBCLOG(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	if (ODBCdebug == NULL) {
#ifdef NATIVE_WIN32
		if ((ODBCdebug = _wgetenv(L"ODBCDEBUG")) == NULL)
			ODBCdebug = _wcsdup(L"");
		else
			ODBCdebug = _wcsdup(ODBCdebug);
#else
		if ((ODBCdebug = getenv("ODBCDEBUG")) == NULL)
			ODBCdebug = strdup("");
		else
			ODBCdebug = strdup(ODBCdebug);
#endif
	}
	if (ODBCdebug != NULL && *ODBCdebug != 0) {
		FILE *f;

#ifdef NATIVE_WIN32
		f = _wfopen(ODBCdebug, L"a");
#else
		f = fopen(ODBCdebug, "a");
#endif
		if (f) {
			vfprintf(f, fmt, ap);
			fclose(f);
		} else
			vfprintf(stderr, fmt, ap);
	}
	va_end(ap);
}


char *translateCType(SQLSMALLINT ValueType);
char *translateSQLType(SQLSMALLINT ParameterType);
char *translateFieldIdentifier(SQLSMALLINT FieldIdentifier);
char *translateFetchOrientation(SQLUSMALLINT FetchOrientation);
char *translateConnectAttribute(SQLINTEGER Attribute);
char *translateConnectOption(SQLUSMALLINT Option);
char *translateEnvAttribute(SQLINTEGER Attribute);
char *translateStmtAttribute(SQLINTEGER Attribute);
char *translateStmtOption(SQLUSMALLINT Option);
char *translateCompletionType(SQLSMALLINT CompletionType);
#endif

#endif
