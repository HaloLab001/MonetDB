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

/**********************************************************************
 * SQLProcedureColumns()
 * CLI Compliance: ODBC (Microsoft)
 *
 * Author: Sjoerd Mullender
 * Date  : 28 Feb 2018
 *
 **********************************************************************/

#include "ODBCGlobal.h"
#include "ODBCStmt.h"
#include "ODBCUtil.h"
#include "ODBCQueries.h"


static SQLRETURN
MNDBProcedureColumns(ODBCStmt *stmt,
		     const SQLCHAR *CatalogName,
		     SQLSMALLINT NameLength1,
		     const SQLCHAR *SchemaName,
		     SQLSMALLINT NameLength2,
		     const SQLCHAR *ProcName,
		     SQLSMALLINT NameLength3,
		     const SQLCHAR *ColumnName,
		     SQLSMALLINT NameLength4)
{
	RETCODE rc;

	/* buffer for the constructed query to do meta data retrieval */
	char *query = NULL;
	size_t querylen;
	size_t pos = 0;
	char *sch = NULL, *prc = NULL, *col = NULL;

	fixODBCstring(CatalogName, NameLength1, SQLSMALLINT,
		      addStmtError, stmt, return SQL_ERROR);
	fixODBCstring(SchemaName, NameLength2, SQLSMALLINT,
		      addStmtError, stmt, return SQL_ERROR);
	fixODBCstring(ProcName, NameLength3, SQLSMALLINT,
		      addStmtError, stmt, return SQL_ERROR);
	fixODBCstring(ColumnName, NameLength4, SQLSMALLINT,
		      addStmtError, stmt, return SQL_ERROR);

#ifdef ODBCDEBUG
	ODBCLOG("\"%.*s\" \"%.*s\" \"%.*s\" \"%.*s\"\n",
		(int) NameLength1, CatalogName ? (char *) CatalogName : "",
		(int) NameLength2, SchemaName ? (char *) SchemaName : "",
		(int) NameLength3, ProcName ? (char *) ProcName : "",
		(int) NameLength4, ColumnName ? (char *) ColumnName : "");
#endif

	if (stmt->Dbc->sql_attr_metadata_id == SQL_FALSE) {
		if (NameLength2 > 0) {
			sch = ODBCParsePV("s", "name",
					  (const char *) SchemaName,
					  (size_t) NameLength2,
					  stmt->Dbc);
			if (sch == NULL)
				goto nomem;
		}
		if (NameLength3 > 0) {
			prc = ODBCParsePV("p", "name",
					  (const char *) ProcName,
					  (size_t) NameLength3,
					  stmt->Dbc);
			if (prc == NULL)
				goto nomem;
		}
		if (NameLength4 > 0) {
			col = ODBCParsePV("a", "name",
					  (const char *) ColumnName,
					  (size_t) NameLength4,
					  stmt->Dbc);
			if (col == NULL)
				goto nomem;
		}
	} else {
		if (NameLength2 > 0) {
			sch = ODBCParseID("s", "name",
					  (const char *) SchemaName,
					  (size_t) NameLength2);
			if (sch == NULL)
				goto nomem;
		}
		if (NameLength3 > 0) {
			prc = ODBCParseID("p", "name",
					  (const char *) ProcName,
					  (size_t) NameLength3);
			if (prc == NULL)
				goto nomem;
		}
		if (NameLength4 > 0) {
			col = ODBCParseID("a", "name",
					  (const char *) ColumnName,
					  (size_t) NameLength4);
			if (col == NULL)
				goto nomem;
		}
	}

	/* construct the query now */
	querylen = 6700 + (sch ? strlen(sch) : 0) + (prc ? strlen(prc) : 0) +
		(col ? strlen(col) : 0);
	query = malloc(querylen);
	if (query == NULL)
		goto nomem;

	/* SQLProcedureColumns returns a table with the following columns:
	   VARCHAR      PROCEDURE_CAT
	   VARCHAR      PROCEDURE_SCHEM
	   VARCHAR      PROCEDURE_NAME NOT NULL
	   VARCHAR      COLUMN_NAME NOT NULL
	   SMALLINT     COLUMN_TYPE NOT NULL
	   SMALLINT     DATA_TYPE NOT NULL
	   VARCHAR      TYPE_NAME NOT NULL
	   INTEGER      COLUMN_SIZE
	   INTEGER      BUFFER_LENGTH
	   SMALLINT     DECIMAL_DIGITS
	   SMALLINT     NUM_PREC_RADIX
	   SMALLINT     NULLABLE NOT NULL
	   VARCHAR      REMARKS
	   VARCHAR      COLUMN_DEF
	   SMALLINT     SQL_DATA_TYPE NOT NULL
	   SMALLINT     SQL_DATETIME_SUB
	   INTEGER      CHAR_OCTET_LENGTH
	   INTEGER      ORDINAL_POSITION NOT NULL
	   VARCHAR      IS_NULLABLE
	   VARCHAR      SPECIFIC_NAME	(Note this is a MonetDB extension, needed to differentiate between overloaded procedure/function names)
					(similar to JDBC DatabaseMetaData methods getProcedureColumns() and getFunctionColumns())
	 */

/* see sql/include/sql_catalog.h */
#define F_FUNC 1
#define F_PROC 2
#define F_UNION 5
	pos += snprintf(query + pos, querylen - pos,
		"select cast(null as varchar(1)) as \"PROCEDURE_CAT\", "
		       "s.name as \"PROCEDURE_SCHEM\", "
		       "p.name as \"PROCEDURE_NAME\", "
		       "a.name as \"COLUMN_NAME\", "
		       "cast(case when a.inout = 1 then %d "
			    "when p.type = %d then %d "
			    "else %d "
		       "end as smallint) as \"COLUMN_TYPE\", "
		DATA_TYPE(a) ", "
		TYPE_NAME(a) ", "
		COLUMN_SIZE(a) ", "
		BUFFER_LENGTH(a) ", "
		DECIMAL_DIGITS(a) ", "
		NUM_PREC_RADIX(a) ", "
		       "cast(%d as smallint) as \"NULLABLE\", "
		       "%s as \"REMARKS\", "
		       "cast(null as varchar(1)) as \"COLUMN_DEF\", "
		SQL_DATA_TYPE(a) ", "
		SQL_DATETIME_SUB(a) ", "
		CHAR_OCTET_LENGTH(a) ", "
		       "cast(case when p.type = 5 and a.inout = 0 then a.number + 1 "
			    "when p.type = 5 and a.inout = 1 then a.number - x.maxout "
			    "when p.type = 2 and a.inout = 1 then a.number + 1 "
			    "when a.inout = 0 then 0 "
			    "else a.number "
		       "end as integer) as \"ORDINAL_POSITION\", "
		       "'' as \"IS_NULLABLE\", "
			/* Only the id value uniquely identifies a specific procedure.
			   Include it to be able to differentiate between multiple
			   overloaded procedures with the same name and schema */
			"cast(p.id as varchar(10)) AS \"SPECIFIC_NAME\" "
		"from sys.schemas s, "
		     "sys.functions p left outer join (select func_id, max(number) as maxout from sys.args where inout = 0 group by func_id) as x on p.id = x.func_id, "
		     "sys.args a%s "
		"where s.id = p.schema_id and "
		      "p.id = a.func_id and "
		      "p.type in (%d, %d, %d)",
		/* column_type: */
		SQL_PARAM_INPUT, F_UNION, SQL_RESULT_COL, SQL_RETURN_VALUE,
#ifdef DATA_TYPE_ARGS
		DATA_TYPE_ARGS,
#endif
#ifdef TYPE_NAME_ARGS
		TYPE_NAME_ARGS,
#endif
#ifdef COLUMN_SIZE_ARGS
		COLUMN_SIZE_ARGS,
#endif
#ifdef BUFFER_LENGTH_ARGS
		BUFFER_LENGTH_ARGS,
#endif
#ifdef DECIMAL_DIGITS_ARGS
		DECIMAL_DIGITS_ARGS,
#endif
#ifdef NUM_PREC_RADIX_ARGS
		NUM_PREC_RADIX_ARGS,
#endif
		/* nullable: */
		SQL_NULLABLE_UNKNOWN,
		/* remarks: */
		stmt->Dbc->has_comment ? "c.remark" : "cast(null as varchar(1))",
#ifdef SQL_DATA_TYPE_ARGS
		SQL_DATA_TYPE_ARGS,
#endif
#ifdef SQL_DATETIME_SUB_ARGS
		SQL_DATETIME_SUB_ARGS,
#endif
#ifdef CHAR_OCTET_LENGTH_ARGS
		CHAR_OCTET_LENGTH_ARGS,
#endif
		/* from clause: */
		stmt->Dbc->has_comment ? " left outer join sys.comments c on c.id = a.id" : "",
		/* where clause: */
		F_FUNC, F_PROC, F_UNION);

	/* depending on the input parameter values we must add a
	   variable selection condition dynamically */

	/* Construct the selection condition query part */
	if (NameLength1 > 0 && CatalogName != NULL) {
		/* filtering requested on catalog name */
		if (strcmp((char *) CatalogName, msetting_string(stmt->Dbc->settings, MP_DATABASE)) != 0) {
			/* catalog name does not match the database name, so return no rows */
			pos += snprintf(query + pos, querylen - pos, " and 1=2");
		}
	}
	if (sch) {
		/* filtering requested on schema name */
		pos += snprintf(query + pos, querylen - pos, " and %s", sch);
		free(sch);
	}
	if (prc) {
		/* filtering requested on procedure name */
		pos += snprintf(query + pos, querylen - pos, " and %s", prc);
		free(prc);
	}
	if (col) {
		/* filtering requested on column name */
		pos += snprintf(query + pos, querylen - pos, " and %s", col);
		free(col);
	}

	/* add the ordering (exclude procedure_cat as it is the same for all rows) */
	pos += strcpy_len(query + pos, " order by \"PROCEDURE_SCHEM\", \"PROCEDURE_NAME\", \"SPECIFIC_NAME\", \"COLUMN_TYPE\", \"ORDINAL_POSITION\"", querylen - pos);
	if (pos >= querylen)
		fprintf(stderr, "pos >= querylen, %zu > %zu\n", pos, querylen);
	assert(pos < querylen);

	/* debug: fprintf(stdout, "SQLProcedureColumns query (pos: %zu, len: %zu):\n%s\n\n", pos, strlen(query), query); */

	/* query the MonetDB data dictionary tables */
	rc = MNDBExecDirect(stmt, (SQLCHAR *) query, (SQLINTEGER) pos);

	free(query);

	return rc;

  nomem:
	/* note that query must be NULL when we get here */
	if (sch)
		free(sch);
	if (prc)
		free(prc);
	if (col)
		free(col);
	/* Memory allocation error */
	addStmtError(stmt, "HY001", NULL, 0);
	return SQL_ERROR;
}

SQLRETURN SQL_API
SQLProcedureColumns(SQLHSTMT StatementHandle,
		    SQLCHAR *CatalogName,
		    SQLSMALLINT NameLength1,
		    SQLCHAR *SchemaName,
		    SQLSMALLINT NameLength2,
		    SQLCHAR *ProcName,
		    SQLSMALLINT NameLength3,
		    SQLCHAR *ColumnName,
		    SQLSMALLINT NameLength4)
{
	ODBCStmt *stmt = (ODBCStmt *) StatementHandle;

#ifdef ODBCDEBUG
	ODBCLOG("SQLProcedureColumns %p ", StatementHandle);
#endif

	if (!isValidStmt(stmt))
		 return SQL_INVALID_HANDLE;

	clearStmtErrors(stmt);

	return MNDBProcedureColumns(stmt,
				    CatalogName, NameLength1,
				    SchemaName, NameLength2,
				    ProcName, NameLength3,
				    ColumnName, NameLength4);
}

SQLRETURN SQL_API
SQLProcedureColumnsA(SQLHSTMT StatementHandle,
		     SQLCHAR *CatalogName,
		     SQLSMALLINT NameLength1,
		     SQLCHAR *SchemaName,
		     SQLSMALLINT NameLength2,
		     SQLCHAR *ProcName,
		     SQLSMALLINT NameLength3,
		     SQLCHAR *ColumnName,
		     SQLSMALLINT NameLength4)
{
	return SQLProcedureColumns(StatementHandle,
				   CatalogName, NameLength1,
				   SchemaName, NameLength2,
				   ProcName, NameLength3,
				   ColumnName, NameLength4);
}

SQLRETURN SQL_API
SQLProcedureColumnsW(SQLHSTMT StatementHandle,
		     SQLWCHAR *CatalogName,
		     SQLSMALLINT NameLength1,
		     SQLWCHAR *SchemaName,
		     SQLSMALLINT NameLength2,
		     SQLWCHAR *ProcName,
		     SQLSMALLINT NameLength3,
		     SQLWCHAR *ColumnName,
		     SQLSMALLINT NameLength4)
{
	ODBCStmt *stmt = (ODBCStmt *) StatementHandle;
	SQLRETURN rc = SQL_ERROR;
	SQLCHAR *catalog = NULL, *schema = NULL, *proc = NULL, *column = NULL;

#ifdef ODBCDEBUG
	ODBCLOG("SQLProcedureColumnsW %p ", StatementHandle);
#endif

	if (!isValidStmt(stmt))
		 return SQL_INVALID_HANDLE;

	clearStmtErrors(stmt);

	fixWcharIn(CatalogName, NameLength1, SQLCHAR, catalog,
		   addStmtError, stmt, goto bailout);
	fixWcharIn(SchemaName, NameLength2, SQLCHAR, schema,
		   addStmtError, stmt, goto bailout);
	fixWcharIn(ProcName, NameLength3, SQLCHAR, proc,
		   addStmtError, stmt, goto bailout);
	fixWcharIn(ColumnName, NameLength4, SQLCHAR, column,
		   addStmtError, stmt, goto bailout);

	rc = MNDBProcedureColumns(stmt,
				  catalog, SQL_NTS,
				  schema, SQL_NTS,
				  proc, SQL_NTS,
				  column, SQL_NTS);

      bailout:
	if (catalog)
		free(catalog);
	if (schema)
		free(schema);
	if (proc)
		free(proc);
	if (column)
		free(column);

	return rc;
}
