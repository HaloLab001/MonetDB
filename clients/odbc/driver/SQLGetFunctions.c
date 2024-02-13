/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0.  If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright 2024 MonetDB Foundation;
 * Copyright August 2008 - 2023 MonetDB B.V.;
 * Copyright 1997 - July 2008 CWI.
 */

/*
 * This code was created by Peter Harvey (mostly during Christmas 98/99).
 * This code is LGPL. Please ensure that this message remains in future
 * distributions and uses of this code (thats about all I get out of it).
 * - Peter Harvey pharvey@codebydesign.com
 *
 * This file has been modified for the MonetDB project.  See the file
 * Copyright in this directory for more information.
 */

/**********************************************************************
 * SQLGetFunctions()
 * CLI Compliance: ISO 92
 *
 * Author: Sjoerd Mullender
 * Date  : 4 sep 2003
 *
 **********************************************************************/

#include "ODBCGlobal.h"
#include "ODBCStmt.h"


/* this table contains all functions for which SQLGetFunctions should
   return SQL_TRUE */
static UWORD FuncImplemented[] = {
	SQL_API_SQLALLOCCONNECT,
	SQL_API_SQLALLOCENV,
#ifdef SQL_API_SQLALLOCHANDLE
	SQL_API_SQLALLOCHANDLE,
#endif
	SQL_API_SQLALLOCSTMT,
	SQL_API_SQLBINDCOL,
	SQL_API_SQLBINDPARAMETER,
	SQL_API_SQLBROWSECONNECT,
	SQL_API_SQLCANCEL,
#ifdef SQL_API_SQLCLOSECURSOR
	SQL_API_SQLCLOSECURSOR,
#endif
#ifdef SQL_API_SQLCOLATTRIBUTE
	SQL_API_SQLCOLATTRIBUTE,	/* == SQL_API_SQLCOLATTRIBUTES */
#endif
	SQL_API_SQLCOLATTRIBUTES,
	SQL_API_SQLCOLUMNPRIVILEGES,
	SQL_API_SQLCOLUMNS,
	SQL_API_SQLCONNECT,
#ifdef SQL_API_SQLCOPYDESC
	SQL_API_SQLCOPYDESC,
#endif
	SQL_API_SQLDESCRIBECOL,
	SQL_API_SQLDESCRIBEPARAM,
	SQL_API_SQLDISCONNECT,
	SQL_API_SQLDRIVERCONNECT,
#ifdef SQL_API_SQLENDTRAN
	SQL_API_SQLENDTRAN,
#endif
	SQL_API_SQLERROR,
	SQL_API_SQLEXECDIRECT,
	SQL_API_SQLEXECUTE,
	SQL_API_SQLEXTENDEDFETCH,
	SQL_API_SQLFETCH,
#ifdef SQL_API_SQLFETCHSCROLL
	SQL_API_SQLFETCHSCROLL,
#endif
	SQL_API_SQLFOREIGNKEYS,
	SQL_API_SQLFREECONNECT,
	SQL_API_SQLFREEENV,
#ifdef SQL_API_SQLFREEHANDLE
	SQL_API_SQLFREEHANDLE,
#endif
	SQL_API_SQLFREESTMT,
#ifdef SQL_API_SQLGETCONNECTATTR
	SQL_API_SQLGETCONNECTATTR,
#endif
	SQL_API_SQLGETCONNECTOPTION,
	SQL_API_SQLGETDATA,
#ifdef SQL_API_SQLGETDESCFIELD
	SQL_API_SQLGETDESCFIELD,
#endif
#ifdef SQL_API_SQLGETDESCREC
	SQL_API_SQLGETDESCREC,
#endif
#ifdef SQL_API_SQLGETDIAGREC
	SQL_API_SQLGETDIAGREC,
#endif
#ifdef SQL_API_SQLGETENVATTR
	SQL_API_SQLGETENVATTR,
#endif
	SQL_API_SQLGETFUNCTIONS,
	SQL_API_SQLGETINFO,
#ifdef SQL_API_SQLGETSTMTATTR
	SQL_API_SQLGETSTMTATTR,
#endif
	SQL_API_SQLGETSTMTOPTION,
	SQL_API_SQLGETTYPEINFO,
	SQL_API_SQLMORERESULTS,
	SQL_API_SQLNATIVESQL,
	SQL_API_SQLNUMPARAMS,
	SQL_API_SQLNUMRESULTCOLS,
	SQL_API_SQLPARAMOPTIONS,
	SQL_API_SQLPREPARE,
	SQL_API_SQLPRIMARYKEYS,
	SQL_API_SQLPROCEDURECOLUMNS,
	SQL_API_SQLPROCEDURES,
	SQL_API_SQLROWCOUNT,
#ifdef SQL_API_SQLSETCONNECTATTR
	SQL_API_SQLSETCONNECTATTR,
#endif
	SQL_API_SQLSETCONNECTOPTION,
#ifdef SQL_API_SQLSETDESCFIELD
	SQL_API_SQLSETDESCFIELD,
#endif
#ifdef SQL_API_SQLSETDESCREC
	SQL_API_SQLSETDESCREC,
#endif
#ifdef SQL_API_SQLSETENVATTR
	SQL_API_SQLSETENVATTR,
#endif
	SQL_API_SQLSETPARAM,
	SQL_API_SQLSETPOS,
#ifdef SQL_API_SQLSETSTMTATTR
	SQL_API_SQLSETSTMTATTR,
#endif
	SQL_API_SQLSETSTMTOPTION,
	SQL_API_SQLSPECIALCOLUMNS,
	SQL_API_SQLSTATISTICS,
	SQL_API_SQLTABLEPRIVILEGES,
	SQL_API_SQLTABLES,
	SQL_API_SQLTRANSACT,
#if 0
/* not yet implemented */
#ifdef SQL_API_SQLALLOCHANDLESTD
	SQL_API_SQLALLOCHANDLESTD,
#endif
#ifdef SQL_API_SQLBINDPARAM
	SQL_API_SQLBINDPARAM,
#endif
#ifdef SQL_API_SQLBULKOPERATIONS
	SQL_API_SQLBULKOPERATIONS,
#endif
	SQL_API_SQLDATASOURCES,
	SQL_API_SQLDRIVERS,
	SQL_API_SQLGETCURSORNAME,
#ifdef SQL_API_SQLGETDIAGFIELD
	SQL_API_SQLGETDIAGFIELD,
#endif
	SQL_API_SQLPARAMDATA,
	SQL_API_SQLPUTDATA,
	SQL_API_SQLSETCURSORNAME,
	SQL_API_SQLSETSCROLLOPTIONS,
#endif /* not yet implemented */
};

#define NFUNCIMPLEMENTED (sizeof(FuncImplemented)/sizeof(FuncImplemented[0]))

/* this table is a bit map compatible with
   SQL_API_ODBC3_ALL_FUNCTIONS, to be initialized from the table
   above */
static UWORD FuncExistMap[SQL_API_ODBC3_ALL_FUNCTIONS_SIZE];

#ifdef ODBCDEBUG
static char *
translateFunctionId(SQLUSMALLINT FunctionId)
{
	switch (FunctionId) {
	case SQL_API_ODBC3_ALL_FUNCTIONS:
		return "SQL_API_ODBC3_ALL_FUNCTIONS";
	case SQL_API_SQLALLOCCONNECT:
		return "SQL_API_SQLALLOCCONNECT";
	case SQL_API_SQLALLOCENV:
		return "SQL_API_SQLALLOCENV";
#ifdef SQL_API_SQLALLOCHANDLE
	case SQL_API_SQLALLOCHANDLE:
		return "SQL_API_SQLALLOCHANDLE";
#endif
	case SQL_API_SQLALLOCSTMT:
		return "SQL_API_SQLALLOCSTMT";
	case SQL_API_SQLBINDCOL:
		return "SQL_API_SQLBINDCOL";
	case SQL_API_SQLBINDPARAMETER:
		return "SQL_API_SQLBINDPARAMETER";
	case SQL_API_SQLBROWSECONNECT:
		return "SQL_API_SQLBROWSECONNECT";
	case SQL_API_SQLCANCEL:
		return "SQL_API_SQLCANCEL";
#ifdef SQL_API_SQLCLOSECURSOR
	case SQL_API_SQLCLOSECURSOR:
		return "SQL_API_SQLCLOSECURSOR";
#endif
#ifdef SQL_API_SQLCOLATTRIBUTE
	case SQL_API_SQLCOLATTRIBUTE:
		return "SQL_API_SQLCOLATTRIBUTE";
#endif
	case SQL_API_SQLCOLUMNPRIVILEGES:
		return "SQL_API_SQLCOLUMNPRIVILEGES";
	case SQL_API_SQLCOLUMNS:
		return "SQL_API_SQLCOLUMNS";
	case SQL_API_SQLCONNECT:
		return "SQL_API_SQLCONNECT";
#ifdef SQL_API_SQLCOPYDESC
	case SQL_API_SQLCOPYDESC:
		return "SQL_API_SQLCOPYDESC";
#endif
	case SQL_API_SQLDESCRIBECOL:
		return "SQL_API_SQLDESCRIBECOL";
	case SQL_API_SQLDESCRIBEPARAM:
		return "SQL_API_SQLDESCRIBEPARAM";
	case SQL_API_SQLDISCONNECT:
		return "SQL_API_SQLDISCONNECT";
	case SQL_API_SQLDRIVERCONNECT:
		return "SQL_API_SQLDRIVERCONNECT";
#ifdef SQL_API_SQLENDTRAN
	case SQL_API_SQLENDTRAN:
		return "SQL_API_SQLENDTRAN";
#endif
	case SQL_API_SQLERROR:
		return "SQL_API_SQLERROR";
	case SQL_API_SQLEXECDIRECT:
		return "SQL_API_SQLEXECDIRECT";
	case SQL_API_SQLEXECUTE:
		return "SQL_API_SQLEXECUTE";
	case SQL_API_SQLEXTENDEDFETCH:
		return "SQL_API_SQLEXTENDEDFETCH";
	case SQL_API_SQLFETCH:
		return "SQL_API_SQLFETCH";
#ifdef SQL_API_SQLFETCHSCROLL
	case SQL_API_SQLFETCHSCROLL:
		return "SQL_API_SQLFETCHSCROLL";
#endif
	case SQL_API_SQLFOREIGNKEYS:
		return "SQL_API_SQLFOREIGNKEYS";
	case SQL_API_SQLFREECONNECT:
		return "SQL_API_SQLFREECONNECT";
	case SQL_API_SQLFREEENV:
		return "SQL_API_SQLFREEENV";
#ifdef SQL_API_SQLFREEHANDLE
	case SQL_API_SQLFREEHANDLE:
		return "SQL_API_SQLFREEHANDLE";
#endif
	case SQL_API_SQLFREESTMT:
		return "SQL_API_SQLFREESTMT";
#ifdef SQL_API_SQLGETCONNECTATTR
	case SQL_API_SQLGETCONNECTATTR:
		return "SQL_API_SQLGETCONNECTATTR";
#endif
	case SQL_API_SQLGETCONNECTOPTION:
		return "SQL_API_SQLGETCONNECTOPTION";
	case SQL_API_SQLGETDATA:
		return "SQL_API_SQLGETDATA";
#ifdef SQL_API_SQLGETDESCFIELD
	case SQL_API_SQLGETDESCFIELD:
		return "SQL_API_SQLGETDESCFIELD";
#endif
#ifdef SQL_API_SQLGETDESCREC
	case SQL_API_SQLGETDESCREC:
		return "SQL_API_SQLGETDESCREC";
#endif
#ifdef SQL_API_SQLGETDIAGREC
	case SQL_API_SQLGETDIAGREC:
		return "SQL_API_SQLGETDIAGREC";
#endif
#ifdef SQL_API_SQLGETENVATTR
	case SQL_API_SQLGETENVATTR:
		return "SQL_API_SQLGETENVATTR";
#endif
	case SQL_API_SQLGETFUNCTIONS:
		return "SQL_API_SQLGETFUNCTIONS";
	case SQL_API_SQLGETINFO:
		return "SQL_API_SQLGETINFO";
#ifdef SQL_API_SQLGETSTMTATTR
	case SQL_API_SQLGETSTMTATTR:
		return "SQL_API_SQLGETSTMTATTR";
#endif
	case SQL_API_SQLGETSTMTOPTION:
		return "SQL_API_SQLGETSTMTOPTION";
	case SQL_API_SQLGETTYPEINFO:
		return "SQL_API_SQLGETTYPEINFO";
	case SQL_API_SQLMORERESULTS:
		return "SQL_API_SQLMORERESULTS";
	case SQL_API_SQLNATIVESQL:
		return "SQL_API_SQLNATIVESQL";
	case SQL_API_SQLNUMPARAMS:
		return "SQL_API_SQLNUMPARAMS";
	case SQL_API_SQLNUMRESULTCOLS:
		return "SQL_API_SQLNUMRESULTCOLS";
	case SQL_API_SQLPARAMOPTIONS:
		return "SQL_API_SQLPARAMOPTIONS";
	case SQL_API_SQLPREPARE:
		return "SQL_API_SQLPREPARE";
	case SQL_API_SQLPRIMARYKEYS:
		return "SQL_API_SQLPRIMARYKEYS";
	case SQL_API_SQLPROCEDURES:
		return "SQL_API_SQLPROCEDURES";
	case SQL_API_SQLROWCOUNT:
		return "SQL_API_SQLROWCOUNT";
#ifdef SQL_API_SQLSETCONNECTATTR
	case SQL_API_SQLSETCONNECTATTR:
		return "SQL_API_SQLSETCONNECTATTR";
#endif
	case SQL_API_SQLSETCONNECTOPTION:
		return "SQL_API_SQLSETCONNECTOPTION";
#ifdef SQL_API_SQLSETDESCFIELD
	case SQL_API_SQLSETDESCFIELD:
		return "SQL_API_SQLSETDESCFIELD";
#endif
#ifdef SQL_API_SQLSETDESCREC
	case SQL_API_SQLSETDESCREC:
		return "SQL_API_SQLSETDESCREC";
#endif
#ifdef SQL_API_SQLSETENVATTR
	case SQL_API_SQLSETENVATTR:
		return "SQL_API_SQLSETENVATTR";
#endif
	case SQL_API_SQLSETPARAM:
		return "SQL_API_SQLSETPARAM";
	case SQL_API_SQLSETPOS:
		return "SQL_API_SQLSETPOS";
#ifdef SQL_API_SQLSETSTMTATTR
	case SQL_API_SQLSETSTMTATTR:
		return "SQL_API_SQLSETSTMTATTR";
#endif
	case SQL_API_SQLSETSTMTOPTION:
		return "SQL_API_SQLSETSTMTOPTION";
	case SQL_API_SQLSPECIALCOLUMNS:
		return "SQL_API_SQLSPECIALCOLUMNS";
	case SQL_API_SQLSTATISTICS:
		return "SQL_API_SQLSTATISTICS";
	case SQL_API_SQLTABLEPRIVILEGES:
		return "SQL_API_SQLTABLEPRIVILEGES";
	case SQL_API_SQLTABLES:
		return "SQL_API_SQLTABLES";
	case SQL_API_SQLTRANSACT:
		return "SQL_API_SQLTRANSACT";
#ifdef SQL_API_SQLALLOCHANDLESTD
	case SQL_API_SQLALLOCHANDLESTD:
		return "SQL_API_SQLALLOCHANDLESTD";
#endif
#ifdef SQL_API_SQLBINDPARAM
	case SQL_API_SQLBINDPARAM:
		return "SQL_API_SQLBINDPARAM";
#endif
#ifdef SQL_API_SQLBULKOPERATIONS
	case SQL_API_SQLBULKOPERATIONS:
		return "SQL_API_SQLBULKOPERATIONS";
#endif
	case SQL_API_SQLDATASOURCES:
		return "SQL_API_SQLDATASOURCES";
	case SQL_API_SQLDRIVERS:
		return "SQL_API_SQLDRIVERS";
	case SQL_API_SQLGETCURSORNAME:
		return "SQL_API_SQLGETCURSORNAME";
#ifdef SQL_API_SQLGETDIAGFIELD
	case SQL_API_SQLGETDIAGFIELD:
		return "SQL_API_SQLGETDIAGFIELD";
#endif
	case SQL_API_SQLPARAMDATA:
		return "SQL_API_SQLPARAMDATA";
	case SQL_API_SQLPROCEDURECOLUMNS:
		return "SQL_API_SQLPROCEDURECOLUMNS";
	case SQL_API_SQLPUTDATA:
		return "SQL_API_SQLPUTDATA";
	case SQL_API_SQLSETCURSORNAME:
		return "SQL_API_SQLSETCURSORNAME";
	case SQL_API_SQLSETSCROLLOPTIONS:
		return "SQL_API_SQLSETSCROLLOPTIONS";
	default:
		return "unknown";
	}
}
#endif

SQLRETURN SQL_API
SQLGetFunctions(SQLHDBC ConnectionHandle,
		SQLUSMALLINT FunctionId,
		SQLUSMALLINT *SupportedPtr)
{
	ODBCDbc *dbc = (ODBCDbc *) ConnectionHandle;

#ifdef ODBCDEBUG
	ODBCLOG("SQLGetFunctions %p %s\n",
		ConnectionHandle, translateFunctionId(FunctionId));
#endif

	if (!isValidDbc(dbc))
		return SQL_INVALID_HANDLE;

	clearDbcErrors(dbc);

	if (!SQL_FUNC_EXISTS(FuncExistMap, FuncImplemented[0])) {
		/* not yet initialized, so do it now */
		UWORD *p;

		for (p = FuncImplemented; p < &FuncImplemented[NFUNCIMPLEMENTED]; p++)
			FuncExistMap[*p >> 4] |= (UWORD) 1 << (*p & 0xF);
	}

	if (FunctionId == SQL_API_ODBC3_ALL_FUNCTIONS) {
		memcpy(SupportedPtr, FuncExistMap,
		       SQL_API_ODBC3_ALL_FUNCTIONS_SIZE * sizeof(FuncExistMap[0]));
		return SQL_SUCCESS;
	}

	if (FunctionId < SQL_API_ODBC3_ALL_FUNCTIONS_SIZE * 16) {
		*SupportedPtr = SQL_FUNC_EXISTS(FuncExistMap, FunctionId);
		return SQL_SUCCESS;
	}

	/* Function type out of range */
	addDbcError(dbc, "HY095", NULL, 0);
	return SQL_ERROR;
}
