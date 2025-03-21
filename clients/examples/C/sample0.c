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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <mapi.h>

#define die(dbh,hdl)	do {						\
				if (hdl)				\
					mapi_explain_result(hdl,stderr); \
				else if (dbh)				\
					mapi_explain(dbh,stderr);	\
				else					\
					fprintf(stderr,"command failed\n"); \
				exit(-1);				\
			} while (0)

int
main(int argc, char **argv)
{
	Mapi dbh;
	MapiHdl hdl = NULL;

	if (argc != 4) {
		fprintf(stderr, "usage:%s <host> <port> <language>\n", argv[0]);
		exit(-1);
	}

	dbh = mapi_connect(argv[1], atoi(argv[2]), "monetdb", "monetdb", argv[3], NULL);
	if (dbh == NULL || mapi_error(dbh))
		die(dbh, hdl);

	/* mapi_trace(dbh, true); */
	if (strcmp(argv[3], "sql") == 0) {
		/* switch of autocommit */
		if (mapi_setAutocommit(dbh, false) != MOK || mapi_error(dbh))
			die(dbh,NULL);
		if ((hdl = mapi_query(dbh, "create table emp(name varchar(20), age int)")) == NULL || mapi_error(dbh))
			die(dbh, hdl);
		if (mapi_close_handle(hdl) != MOK)
			die(dbh, hdl);
		if ((hdl = mapi_query(dbh, "insert into emp values('John', 23)")) == NULL || mapi_error(dbh))
			die(dbh, hdl);
		if (mapi_close_handle(hdl) != MOK)
			die(dbh, hdl);
		if ((hdl = mapi_query(dbh, "insert into emp values('Mary', 22)")) == NULL || mapi_error(dbh))
			die(dbh, hdl);
		if (mapi_close_handle(hdl) != MOK)
			die(dbh, hdl);
		if ((hdl = mapi_query(dbh, "select * from emp")) == NULL || mapi_error(dbh))
			die(dbh, hdl);
		int n = 0;
		while (mapi_fetch_row(hdl)) {
			char *nme = mapi_fetch_field(hdl, 0);
			char *age = mapi_fetch_field(hdl, 1);

			switch (n) {
			case 0:
				if (strcmp(nme, "John") != 0 || strcmp(age, "23") != 0)
					fprintf(stderr, "unexpected result: %s|%s\n", nme, age);
				break;
			case 1:
				if (strcmp(nme, "Mary") != 0 || strcmp(age, "22") != 0)
					fprintf(stderr, "unexpected result: %s|%s\n", nme, age);
				break;

			default:
				fprintf(stderr, "too many results: %s|%s\n", nme, age);
				break;
			}
			n++;
		}
	} else if (strcmp(argv[3], "mal") == 0) {
		if ((hdl = mapi_query(dbh, "emp := bat.new(:oid,:str);")) == NULL || mapi_error(dbh))
			die(dbh, hdl);
		if ((hdl = mapi_query(dbh, "age := bat.new(:oid,:int);")) == NULL || mapi_error(dbh))
			die(dbh, hdl);
		if (mapi_close_handle(hdl) != MOK)
			die(dbh, hdl);
		if ((hdl = mapi_query(dbh, "bat.append(emp, \"John\");")) == NULL || mapi_error(dbh))
			die(dbh, hdl);
		if ((hdl = mapi_query(dbh, "bat.append(age, 23);")) == NULL || mapi_error(dbh))
			die(dbh, hdl);
		if (mapi_close_handle(hdl) != MOK)
			die(dbh, hdl);
		if ((hdl = mapi_query(dbh, "bat.append(emp, \"Mary\");")) == NULL || mapi_error(dbh))
			die(dbh, hdl);
		if ((hdl = mapi_query(dbh, "bat.append(age, 22);")) == NULL || mapi_error(dbh))
			die(dbh, hdl);
		if (mapi_close_handle(hdl) != MOK)
			die(dbh, hdl);
		if ((hdl = mapi_query(dbh, "io.print(emp,age);")) == NULL || mapi_error(dbh))
			die(dbh, hdl);
		int n = 0;
		while (mapi_fetch_row(hdl)) {
			char *nme = mapi_fetch_field(hdl, 1);
			char *age = mapi_fetch_field(hdl, 2);

			switch (n) {
			case 0:
				if (strcmp(nme, "John") != 0 || strcmp(age, "23") != 0)
					fprintf(stderr, "unexpected result: %s|%s\n", nme, age);
				break;
			case 1:
				if (strcmp(nme, "Mary") != 0 || strcmp(age, "22") != 0)
					fprintf(stderr, "unexpected result: %s|%s\n", nme, age);
				break;

			default:
				fprintf(stderr, "too many results: %s|%s\n", nme, age);
				break;
			}
			n++;
		}
	} else {
		fprintf(stderr, "%s: unknown language, only mal and sql supported\n", argv[0]);
		exit(1);
	}

	if (mapi_error(dbh))
		die(dbh, hdl);
	/* mapi_stat(dbh);
	   printf("mapi_ping %d\n",mapi_ping(dbh)); */
	if (mapi_close_handle(hdl) != MOK)
		die(dbh, hdl);
	mapi_destroy(dbh);

	return 0;
}
