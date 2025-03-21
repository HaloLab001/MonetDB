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

/* In this file we implement three new types with supporting code.
 * The types are:
 *
 * - daytime - representing a time-of-day between 00:00:00 (included)
 *   and 24:00:00 (not included);
 * - date - representing a date between the year -4712 and 170050;
 * - timestamp - a combination of date and daytime, representing an
 *   exact point in time.
 *
 * Dates, both in the date and the timestamp types, are represented in
 * the so-called proleptic Gregorian calendar, that is to say, the
 * Gregorian calendar (which is in common use today) is extended
 * backwards.  See e.g.
 * <https://en.wikipedia.org/wiki/Proleptic_Gregorian_calendar>.
 *
 * Times, both in the daytime and the timestamp types, are recorded
 * with microsecond precision.
 *
 * Times and timestamps are all in UTC.  Conversion from the system
 * time zone where appropriate is done automatically.
 */

#include "monetdb_config.h"
#include "mtime.h"
#include "mal_client.h"


#ifndef HAVE_STRPTIME
extern char *strptime(const char *, const char *, struct tm *);
extern char *strptime2(const char *, const char *, struct tm *, int *);
#endif

/* interfaces callable from MAL, not used from any C code */

static str
MTIMEcurrent_date(date *ret)
{
	*ret = timestamp_date(timestamp_current());
	return MAL_SUCCEED;
}

static str
MTIMEcurrent_time(daytime *ret)
{
	*ret = timestamp_daytime(timestamp_current());
	return MAL_SUCCEED;
}

static str
MTIMEcurrent_timestamp(timestamp *ret)
{
	*ret = timestamp_current();
	return MAL_SUCCEED;
}

#define is_str_nil strNil

#define MTIME_STR_BUFFER_LENGTH MAX(strlen(str_nil) + 1, 512)


#define DEC_VAR_R(TYPE, ARG) TYPE *restrict ptr##ARG

#define DEC_VAR(TYPE, ARG) TYPE *ptr##ARG

#define DEC_ITER(TYPE, ARG)

#define DEC_BUFFER(OUTTYPE, RES, MALFUNC) \
	OUTTYPE RES = GDKmalloc(MTIME_STR_BUFFER_LENGTH); \
	if (!res) {	\
		msg = createException(MAL, "batmtime." MALFUNC, SQLSTATE(HY013) MAL_MALLOC_FAIL); \
		goto bailout; \
	}

#define DEC_INT(OUTTYPE, RES, MALFUNC) OUTTYPE RES = (OUTTYPE){0}

#define INIT_VAROUT(ARG) ptr##ARG = Tloc(b##ARG, 0)
#define INIT_VARIN(ARG) ptr##ARG = b##ARG##i.base
#define INIT_ITERIN(ARG)

#define APPEND_VAR(MALFUNC) ptrn[i] = res

#define GET_NEXT_VAR(ARG, OFF) ptr##ARG[OFF]

#define APPEND_STR(MALFUNC) \
	if (tfastins_nocheckVAR(bn, i, res) != GDK_SUCCEED) { \
		msg = createException(SQL, "batmtime." MALFUNC, SQLSTATE(HY013) MAL_MALLOC_FAIL); \
		break; \
	}

#define GET_NEXT_ITER(ARG, OFF) BUNtvar(b##ARG##i, OFF)

#define DEC_NOTHING(TYPE, ARG)
#define INIT_NOTHING(ARG)

#define FINISH_BUFFER_SINGLE(MALFUNC) \
bailout: \
	*ret = NULL; \
	if (!msg && res && !(*ret = GDKstrdup(res))) \
		msg = createException(MAL, "batmtime." MALFUNC, SQLSTATE(HY013) MAL_MALLOC_FAIL); \
	GDKfree(res)

#define FINISH_INT_SINGLE(MALFUNC) *ret = res

#define FINISH_BUFFER_MULTI(RES) GDKfree(RES)

#define CLEAR_NOTHING(RES)


#define COPYFLAGS(n)	do { bn->tsorted = b1i.sorted; bn->trevsorted = b1i.revsorted; } while (0)
#define SETFLAGS(n)	do { bn->tsorted = bn->trevsorted = n < 2; } while (0)
#define func1(NAME, MALFUNC, INTYPE, OUTTYPE,							\
			  FUNC, SETFLAGS, FUNC_CALL,								\
			  DEC_SRC, DEC_OUTPUT,										\
			  INIT_SRC, INIT_OUTPUT, GET_NEXT_SRC)						\
static str																\
NAME(OUTTYPE *ret, const INTYPE *src)									\
{																		\
	str msg = MAL_SUCCEED;												\
	do {																\
		FUNC_CALL(FUNC, (*ret), *src);									\
	} while (0);														\
	return msg;															\
}																		\
static str																\
NAME##_bulk(Client cntxt, MalBlkPtr mb, MalStkPtr stk, InstrPtr pci)	\
{																		\
	str msg = MAL_SUCCEED;												\
	BAT *b1 = NULL, *s = NULL, *bn = NULL;								\
	struct canditer ci = {0};											\
	oid off;															\
	bool nils = false;													\
	bat *ret = getArgReference_bat(stk, pci, 0),						\
		*bid = getArgReference_bat(stk, pci, 1),						\
		*sid = pci->argc == 3 ? getArgReference_bat(stk, pci, 2) : NULL; \
	BATiter b1i;														\
	DEC_SRC(INTYPE, 1);													\
	DEC_OUTPUT(OUTTYPE, n);												\
																		\
	(void) cntxt;														\
	(void) mb;															\
	if ((b1 = BATdescriptor(*bid)) == NULL)	{							\
		msg = createException(MAL, "batmtime." MALFUNC,					\
			  SQLSTATE(HY002) RUNTIME_OBJECT_MISSING);					\
		goto bailout;													\
	}																	\
	b1i = bat_iterator(b1);												\
	if (sid && !is_bat_nil(*sid) && (s = BATdescriptor(*sid)) == NULL) { \
		msg = createException(MAL, "batmtime." MALFUNC,					\
			  SQLSTATE(HY002) RUNTIME_OBJECT_MISSING);					\
		goto bailout;													\
	}																	\
	off = b1->hseqbase;													\
	canditer_init(&ci, b1, s);											\
	if ((bn = COLnew(ci.hseq, TYPE_##OUTTYPE, ci.ncand, TRANSIENT)) == NULL) { \
		msg = createException(MAL, "batmtime." MALFUNC,					\
			  SQLSTATE(HY013) MAL_MALLOC_FAIL);							\
		goto bailout;													\
	}																	\
	INIT_SRC(1);														\
	INIT_OUTPUT(n);														\
	if (ci.tpe == cand_dense) {											\
		for (BUN i = 0; i < ci.ncand; i++) {							\
			oid p = (canditer_next_dense(&ci) - off);					\
			FUNC_CALL(FUNC, ptrn[i], GET_NEXT_SRC(1, p));				\
			nils |= is_##OUTTYPE##_nil(ptrn[i]);						\
		}																\
	} else {															\
		for (BUN i = 0; i < ci.ncand; i++) {							\
			oid p = (canditer_next(&ci) - off);							\
			FUNC_CALL(FUNC, ptrn[i], GET_NEXT_SRC(1, p));				\
			nils |= is_##OUTTYPE##_nil(ptrn[i]);						\
		}																\
	}																	\
	BATsetcount(bn, ci.ncand);											\
	bn->tnonil = !nils;													\
	bn->tnil = nils;													\
	bn->tkey = ci.ncand < 2;											\
	SETFLAGS(ci.ncand);													\
bailout:																\
	if (b1) {															\
		bat_iterator_end(&b1i);											\
		BBPunfix(b1->batCacheid);										\
	}																	\
	BBPreclaim(s);														\
	if (bn) {															\
		if (msg)														\
			BBPreclaim(bn);												\
		else {															\
			*ret = bn->batCacheid;										\
			BBPkeepref(bn);												\
		}																\
	}																	\
	return msg;															\
}

#define func1_noexcept(FUNC, RET, PARAM) RET = FUNC(PARAM)
#define func1_except(FUNC, RET, PARAM) msg = FUNC(&RET, PARAM); if (msg) break

#define func2(NAME, MALFUNC,											\
			  INTYPE1, INTYPE2, OUTTYPE, FUNC, FUNC_CALL,				\
			  DEC_SRC1, DEC_SRC2, DEC_OUTPUT, DEC_EXTRA,				\
			  INIT_SRC1, INIT_SRC2, INIT_OUTPUT,						\
			  GET_NEXT_SRC1, GET_NEXT_SRC2,								\
			  APPEND_NEXT, CLEAR_EXTRA_SINGLE, CLEAR_EXTRA_MULTI)		\
static str																\
NAME(OUTTYPE *ret, const INTYPE1 *v1, const INTYPE2 *v2)				\
{																		\
	str msg = MAL_SUCCEED;												\
	DEC_EXTRA(OUTTYPE, res, MALFUNC);									\
																		\
	do {																\
		FUNC_CALL(FUNC, res, *v1, *v2);									\
	} while (0);														\
	CLEAR_EXTRA_SINGLE(MALFUNC);										\
	return msg;															\
}																		\
static str																\
NAME##_bulk(Client cntxt, MalBlkPtr mb, MalStkPtr stk, InstrPtr pci)	\
{																		\
	str msg = MAL_SUCCEED;												\
	BAT *b1 = NULL, *b2 = NULL, *s1 = NULL, *s2 = NULL, *bn = NULL;		\
	oid off1, off2;														\
	struct canditer ci1 = {0}, ci2 = {0};								\
	bool nils = false;													\
	bat *ret = getArgReference_bat(stk, pci, 0),						\
		*bid1 = getArgReference_bat(stk, pci, 1),						\
		*bid2 = getArgReference_bat(stk, pci, 2),						\
		*sid1 = pci->argc == 5 ? getArgReference_bat(stk, pci, 3) : NULL, \
		*sid2 = pci->argc == 5 ? getArgReference_bat(stk, pci, 4) : NULL; \
	BATiter b1i, b2i = (BATiter){ .vh = NULL };							\
	DEC_SRC1(INTYPE1, 1);												\
	DEC_SRC2(INTYPE2, 2);												\
	DEC_OUTPUT(OUTTYPE, n);												\
																		\
	(void) cntxt;														\
	(void) mb;															\
	b1 = BATdescriptor(*bid1);											\
	b2 = BATdescriptor(*bid2);											\
	b1i = bat_iterator(b1);												\
	b2i = bat_iterator(b2);												\
	DEC_EXTRA(OUTTYPE, res, MALFUNC);									\
	if (b1 == NULL || b2 == NULL) {										\
		msg = createException(MAL, "batmtime." MALFUNC,					\
			  SQLSTATE(HY002) RUNTIME_OBJECT_MISSING);					\
		goto bailout;													\
	}																	\
	if (sid1 && !is_bat_nil(*sid1) && (s1 = BATdescriptor(*sid1)) == NULL) { \
		msg = createException(MAL, "batmtime." MALFUNC,					\
			  SQLSTATE(HY002) RUNTIME_OBJECT_MISSING);					\
		goto bailout;													\
	}																	\
	if (sid2 && !is_bat_nil(*sid2) && (s2 = BATdescriptor(*sid2)) == NULL) { \
		msg = createException(MAL, "batmtime." MALFUNC,					\
			  SQLSTATE(HY002) RUNTIME_OBJECT_MISSING);					\
		goto bailout;													\
	}																	\
	canditer_init(&ci1, b1, s1);										\
	canditer_init(&ci2, b2, s2);										\
	if (ci2.ncand != ci1.ncand || ci1.hseq != ci2.hseq) {				\
		msg = createException(MAL, "batmtime." MALFUNC,					\
			  "inputs not the same size");								\
		goto bailout;													\
	}																	\
	if ((bn = COLnew(ci1.hseq, TYPE_##OUTTYPE, ci1.ncand, TRANSIENT)) == NULL) { \
		msg = createException(MAL, "batmtime." MALFUNC,					\
			  SQLSTATE(HY013) MAL_MALLOC_FAIL);							\
		goto bailout;													\
	}																	\
	off1 = b1->hseqbase;												\
	off2 = b2->hseqbase;												\
	INIT_SRC1(1);														\
	INIT_SRC2(2);														\
	INIT_OUTPUT(n);														\
	if (ci1.tpe == cand_dense && ci2.tpe == cand_dense) {				\
		for (BUN i = 0; i < ci1.ncand; i++) {							\
			oid p1 = (canditer_next_dense(&ci1) - off1);				\
			oid p2 = (canditer_next_dense(&ci2) - off2);				\
			FUNC_CALL(FUNC, res, GET_NEXT_SRC1(1, p1), GET_NEXT_SRC2(2, p2)); \
			APPEND_NEXT(MALFUNC);										\
			nils |= is_##OUTTYPE##_nil(res);							\
		}																\
	} else {															\
		for (BUN i = 0; i < ci1.ncand; i++) {							\
			oid p1 = (canditer_next(&ci1) - off1);						\
			oid p2 = (canditer_next(&ci2) - off2);						\
			FUNC_CALL(FUNC, res, GET_NEXT_SRC1(1, p1), GET_NEXT_SRC2(2, p2)); \
			APPEND_NEXT(MALFUNC);										\
			nils |= is_##OUTTYPE##_nil(res);							\
		}																\
	}																	\
	BATsetcount(bn, ci1.ncand);											\
	bn->tnonil = !nils;													\
	bn->tnil = nils;													\
	bn->tsorted = ci1.ncand < 2;										\
	bn->trevsorted = ci1.ncand < 2;										\
	bn->tkey = ci1.ncand < 2;											\
bailout:																\
	CLEAR_EXTRA_MULTI(res);												\
	bat_iterator_end(&b1i);												\
	bat_iterator_end(&b2i);												\
	BBPreclaim(b1);														\
	BBPreclaim(b2);														\
	BBPreclaim(s1);														\
	BBPreclaim(s2);														\
	if (bn) {															\
		if (msg)														\
			BBPreclaim(bn);												\
		else {															\
			*ret = bn->batCacheid;										\
			BBPkeepref(bn);												\
		}																\
	}																	\
	return msg;															\
}																		\
static str																\
NAME##_bulk_p1(Client cntxt, MalBlkPtr mb, MalStkPtr stk, InstrPtr pci)	\
{																		\
	str msg = MAL_SUCCEED;												\
	BAT *b2 = NULL, *s2 = NULL, *bn = NULL;								\
	oid off2;															\
	struct canditer ci2 = {0};											\
	bool nils = false;													\
	bat *ret = getArgReference_bat(stk, pci, 0),						\
		*bid2 = getArgReference_bat(stk, pci, 2),						\
		*sid2 = pci->argc == 4 ? getArgReference_bat(stk, pci, 3) : NULL; \
	const INTYPE1 src1 = *(INTYPE1*)getArgReference(stk, pci, 1);		\
	BATiter b2i;														\
	DEC_SRC2(INTYPE2, 2);												\
	DEC_OUTPUT(OUTTYPE, n);												\
	DEC_EXTRA(OUTTYPE, res, MALFUNC);									\
																		\
	(void) cntxt;														\
	(void) mb;															\
	if ((b2 = BATdescriptor(*bid2)) == NULL) {							\
		throw(MAL, "batmtime." MALFUNC,									\
			  SQLSTATE(HY002) RUNTIME_OBJECT_MISSING);					\
	}																	\
	b2i = bat_iterator(b2);												\
	if (sid2 && !is_bat_nil(*sid2) && (s2 = BATdescriptor(*sid2)) == NULL) { \
		msg = createException(MAL, "batmtime." MALFUNC,					\
			  SQLSTATE(HY002) RUNTIME_OBJECT_MISSING);					\
		goto bailout;													\
	}																	\
	canditer_init(&ci2, b2, s2);										\
	if ((bn = COLnew(ci2.hseq, TYPE_##OUTTYPE, ci2.ncand, TRANSIENT)) == NULL) { \
		msg = createException(MAL, "batmtime." MALFUNC,					\
			  SQLSTATE(HY013) MAL_MALLOC_FAIL);							\
		goto bailout;													\
	}																	\
	off2 = b2->hseqbase;												\
	INIT_SRC2(2);														\
	INIT_OUTPUT(n);														\
	if (ci2.tpe == cand_dense) {										\
		for (BUN i = 0; i < ci2.ncand; i++) {							\
			oid p2 = (canditer_next_dense(&ci2) - off2);				\
			FUNC_CALL(FUNC, res, src1, GET_NEXT_SRC2(2, p2));			\
			APPEND_NEXT(MALFUNC);										\
			nils |= is_##OUTTYPE##_nil(res);							\
		}																\
	} else {															\
		for (BUN i = 0; i < ci2.ncand; i++) {							\
			oid p2 = (canditer_next(&ci2) - off2);						\
			FUNC_CALL(FUNC, res, src1, GET_NEXT_SRC2(2, p2));			\
			APPEND_NEXT(MALFUNC);										\
			nils |= is_##OUTTYPE##_nil(res);							\
		}																\
	}																	\
	BATsetcount(bn, ci2.ncand);											\
	bn->tnonil = !nils;													\
	bn->tnil = nils;													\
	bn->tsorted = ci2.ncand < 2;										\
	bn->trevsorted = ci2.ncand < 2;										\
	bn->tkey = ci2.ncand < 2;											\
bailout:																\
	CLEAR_EXTRA_MULTI(res);												\
	if (b2) {															\
		bat_iterator_end(&b2i);											\
		BBPunfix(b2->batCacheid);										\
	}																	\
	BBPreclaim(s2);														\
	if (bn) {															\
		if (msg)														\
			BBPreclaim(bn);												\
		else {															\
			*ret = bn->batCacheid;										\
			BBPkeepref(bn);												\
		}																\
	}																	\
	return msg;															\
}																		\
static str																\
NAME##_bulk_p2(Client cntxt, MalBlkPtr mb, MalStkPtr stk, InstrPtr pci)	\
{																		\
	str msg = MAL_SUCCEED;												\
	BAT *b1 = NULL, *s1 = NULL, *bn = NULL;								\
	oid off1;															\
	struct canditer ci1 = {0};											\
	bool nils = false;													\
	bat *ret = getArgReference_bat(stk, pci, 0),						\
		*bid1 = getArgReference_bat(stk, pci, 1),						\
		*sid1 = pci->argc == 4 ? getArgReference_bat(stk, pci, 3) : NULL; \
	BATiter b1i;														\
	DEC_SRC1(INTYPE1, 1);												\
	const INTYPE2 src2 = *(INTYPE2*)getArgReference(stk, pci, 2);		\
	DEC_OUTPUT(OUTTYPE, n);												\
	DEC_EXTRA(OUTTYPE, res, MALFUNC);									\
																		\
	(void) cntxt;														\
	(void) mb;															\
	if ((b1 = BATdescriptor(*bid1)) == NULL) {							\
		msg = createException(MAL, "batmtime." MALFUNC,					\
			  SQLSTATE(HY002) RUNTIME_OBJECT_MISSING);					\
		goto bailout;													\
	}																	\
	b1i = bat_iterator(b1);												\
	if (sid1 && !is_bat_nil(*sid1) && (s1 = BATdescriptor(*sid1)) == NULL) { \
		msg = createException(MAL, "batmtime." MALFUNC,					\
			  SQLSTATE(HY002) RUNTIME_OBJECT_MISSING);					\
		goto bailout;													\
	}																	\
	canditer_init(&ci1, b1, s1);										\
	if ((bn = COLnew(ci1.hseq, TYPE_##OUTTYPE, ci1.ncand, TRANSIENT)) == NULL) { \
		msg = createException(MAL, "batmtime." MALFUNC,					\
			  SQLSTATE(HY013) MAL_MALLOC_FAIL);							\
		goto bailout;													\
	}																	\
	off1 = b1->hseqbase;												\
	INIT_SRC1(1);														\
	INIT_OUTPUT(n);														\
	if (ci1.tpe == cand_dense) {										\
		for (BUN i = 0; i < ci1.ncand; i++) {							\
			oid p1 = (canditer_next_dense(&ci1) - off1);				\
			FUNC_CALL(FUNC, res, GET_NEXT_SRC1(1, p1), src2);			\
			APPEND_NEXT(MALFUNC);										\
			nils |= is_##OUTTYPE##_nil(res);							\
		}																\
	} else {															\
		for (BUN i = 0; i < ci1.ncand; i++) {							\
			oid p1 = (canditer_next(&ci1) - off1);						\
			FUNC_CALL(FUNC, res, GET_NEXT_SRC1(1, p1), src2);			\
			APPEND_NEXT(MALFUNC);										\
			nils |= is_##OUTTYPE##_nil(res);							\
		}																\
	}																	\
	BATsetcount(bn, ci1.ncand);											\
	bn->tnonil = !nils;													\
	bn->tnil = nils;													\
	bn->tsorted = ci1.ncand < 2;										\
	bn->trevsorted = ci1.ncand < 2;										\
	bn->tkey = ci1.ncand < 2;											\
bailout:																\
	CLEAR_EXTRA_MULTI(res);												\
	if (b1) {															\
		bat_iterator_end(&b1i);											\
		BBPunfix(b1->batCacheid);										\
	}																	\
	BBPreclaim(s1);														\
	if (bn) {															\
		if (msg)														\
			BBPreclaim(bn);												\
		else {															\
			*ret = bn->batCacheid;										\
			BBPkeepref(bn);												\
		}																\
	}																	\
	return msg;															\
}

#define func2_noexcept(FUNC, RET, PARAM1, PARAM2) RET = FUNC(PARAM1, PARAM2)
#define func2_except(FUNC, RET, PARAM1, PARAM2) msg = FUNC(&RET, PARAM1, PARAM2); if (msg) break

#define func3(NAME, MALFUNC,											\
			  INTYPE1, INTYPE2, OUTTYPE, FUNC, FUNC_CALL,				\
			  DEC_SRC1, DEC_SRC2, DEC_OUTPUT, DEC_EXTRA,				\
			  INIT_SRC1, INIT_SRC2, INIT_OUTPUT,						\
			  GET_NEXT_SRC1, GET_NEXT_SRC2,								\
			  APPEND_NEXT, CLEAR_EXTRA_SINGLE, CLEAR_EXTRA_MULTI)		\
static str																\
NAME(OUTTYPE *ret, const INTYPE1 *v1, const INTYPE2 *v2, const lng *extra)				\
{																		\
	str msg = MAL_SUCCEED;												\
	DEC_EXTRA(OUTTYPE, res, MALFUNC);									\
																		\
	do {																\
		FUNC_CALL(FUNC, res, *v1, *v2, *extra);									\
	} while (0);														\
	CLEAR_EXTRA_SINGLE(MALFUNC);										\
	return msg;															\
}																		\
static str																\
NAME##_bulk(Client cntxt, MalBlkPtr mb, MalStkPtr stk, InstrPtr pci)	\
{																		\
	str msg = MAL_SUCCEED;												\
	BAT *b1 = NULL, *b2 = NULL, *s1 = NULL, *s2 = NULL, *bn = NULL;		\
	oid off1, off2;														\
	struct canditer ci1 = {0}, ci2 = {0};								\
	bool nils = false;													\
	bat *ret = getArgReference_bat(stk, pci, 0),						\
		*bid1 = getArgReference_bat(stk, pci, 1),						\
		*bid2 = getArgReference_bat(stk, pci, 2),						\
		*sid1 = pci->argc == 6 ? getArgReference_bat(stk, pci, 3) : NULL, \
		*sid2 = pci->argc == 6 ? getArgReference_bat(stk, pci, 4) : NULL; \
	lng *extra = getArgReference_lng(stk, pci, pci->argc-1);						\
	BATiter b1i, b2i = (BATiter){ .vh = NULL };							\
	DEC_SRC1(INTYPE1, 1);												\
	DEC_SRC2(INTYPE2, 2);												\
	DEC_OUTPUT(OUTTYPE, n);												\
																		\
	(void) cntxt;														\
	(void) mb;															\
	b1 = BATdescriptor(*bid1);											\
	b2 = BATdescriptor(*bid2);											\
	b1i = bat_iterator(b1);												\
	b2i = bat_iterator(b2);												\
	DEC_EXTRA(OUTTYPE, res, MALFUNC);									\
	if (b1 == NULL || b2 == NULL) {										\
		msg = createException(MAL, "batmtime." MALFUNC,					\
			  SQLSTATE(HY002) RUNTIME_OBJECT_MISSING);					\
		goto bailout;													\
	}																	\
	if (sid1 && !is_bat_nil(*sid1) && (s1 = BATdescriptor(*sid1)) == NULL) { \
		msg = createException(MAL, "batmtime." MALFUNC,					\
			  SQLSTATE(HY002) RUNTIME_OBJECT_MISSING);					\
		goto bailout;													\
	}																	\
	if (sid2 && !is_bat_nil(*sid2) && (s2 = BATdescriptor(*sid2)) == NULL) { \
		msg = createException(MAL, "batmtime." MALFUNC,					\
			  SQLSTATE(HY002) RUNTIME_OBJECT_MISSING);					\
		goto bailout;													\
	}																	\
	canditer_init(&ci1, b1, s1);										\
	canditer_init(&ci2, b2, s2);										\
	if (ci2.ncand != ci1.ncand || ci1.hseq != ci2.hseq) {				\
		msg = createException(MAL, "batmtime." MALFUNC,					\
			  "inputs not the same size");								\
		goto bailout;													\
	}																	\
	if ((bn = COLnew(ci1.hseq, TYPE_##OUTTYPE, ci1.ncand, TRANSIENT)) == NULL) { \
		msg = createException(MAL, "batmtime." MALFUNC,					\
			  SQLSTATE(HY013) MAL_MALLOC_FAIL);							\
		goto bailout;													\
	}																	\
	off1 = b1->hseqbase;												\
	off2 = b2->hseqbase;												\
	INIT_SRC1(1);														\
	INIT_SRC2(2);														\
	INIT_OUTPUT(n);														\
	if (ci1.tpe == cand_dense && ci2.tpe == cand_dense) {				\
		for (BUN i = 0; i < ci1.ncand; i++) {							\
			oid p1 = (canditer_next_dense(&ci1) - off1);				\
			oid p2 = (canditer_next_dense(&ci2) - off2);				\
			FUNC_CALL(FUNC, res, GET_NEXT_SRC1(1, p1), GET_NEXT_SRC2(2, p2), *extra); \
			APPEND_NEXT(MALFUNC);										\
			nils |= is_##OUTTYPE##_nil(res);							\
		}																\
	} else {															\
		for (BUN i = 0; i < ci1.ncand; i++) {							\
			oid p1 = (canditer_next(&ci1) - off1);						\
			oid p2 = (canditer_next(&ci2) - off2);						\
			FUNC_CALL(FUNC, res, GET_NEXT_SRC1(1, p1), GET_NEXT_SRC2(2, p2), *extra); \
			APPEND_NEXT(MALFUNC);										\
			nils |= is_##OUTTYPE##_nil(res);							\
		}																\
	}																	\
	BATsetcount(bn, ci1.ncand);											\
	bn->tnonil = !nils;													\
	bn->tnil = nils;													\
	bn->tsorted = ci1.ncand < 2;										\
	bn->trevsorted = ci1.ncand < 2;										\
	bn->tkey = ci1.ncand < 2;											\
bailout:																\
	CLEAR_EXTRA_MULTI(res);												\
	bat_iterator_end(&b1i);												\
	bat_iterator_end(&b2i);												\
	BBPreclaim(b1);														\
	BBPreclaim(b2);														\
	BBPreclaim(s1);														\
	BBPreclaim(s2);														\
	if (bn) {															\
		if (msg)														\
			BBPreclaim(bn);												\
		else {															\
			*ret = bn->batCacheid;										\
			BBPkeepref(bn);												\
		}																\
	}																	\
	return msg;															\
}																		\
static str																\
NAME##_bulk_p1(Client cntxt, MalBlkPtr mb, MalStkPtr stk, InstrPtr pci)	\
{																		\
	str msg = MAL_SUCCEED;												\
	BAT *b2 = NULL, *s2 = NULL, *bn = NULL;								\
	oid off2;															\
	struct canditer ci2 = {0};											\
	bool nils = false;													\
	bat *ret = getArgReference_bat(stk, pci, 0),						\
		*bid2 = getArgReference_bat(stk, pci, 2),						\
		*sid2 = pci->argc == 5 ? getArgReference_bat(stk, pci, 4) : NULL; \
	lng *extra = getArgReference_lng(stk, pci, 3);						\
	const INTYPE1 src1 = *(INTYPE1*)getArgReference(stk, pci, 1);		\
	BATiter b2i;														\
	DEC_SRC2(INTYPE2, 2);												\
	DEC_OUTPUT(OUTTYPE, n);												\
	DEC_EXTRA(OUTTYPE, res, MALFUNC);									\
																		\
	(void) cntxt;														\
	(void) mb;															\
	if ((b2 = BATdescriptor(*bid2)) == NULL) {							\
		throw(MAL, "batmtime." MALFUNC,									\
			  SQLSTATE(HY002) RUNTIME_OBJECT_MISSING);					\
	}																	\
	b2i = bat_iterator(b2);												\
	if (sid2 && !is_bat_nil(*sid2) && (s2 = BATdescriptor(*sid2)) == NULL) { \
		msg = createException(MAL, "batmtime." MALFUNC,					\
			  SQLSTATE(HY002) RUNTIME_OBJECT_MISSING);					\
		goto bailout;													\
	}																	\
	canditer_init(&ci2, b2, s2);										\
	if ((bn = COLnew(ci2.hseq, TYPE_##OUTTYPE, ci2.ncand, TRANSIENT)) == NULL) { \
		msg = createException(MAL, "batmtime." MALFUNC,					\
			  SQLSTATE(HY013) MAL_MALLOC_FAIL);							\
		goto bailout;													\
	}																	\
	off2 = b2->hseqbase;												\
	INIT_SRC2(2);														\
	INIT_OUTPUT(n);														\
	if (ci2.tpe == cand_dense) {										\
		for (BUN i = 0; i < ci2.ncand; i++) {							\
			oid p2 = (canditer_next_dense(&ci2) - off2);				\
			FUNC_CALL(FUNC, res, src1, GET_NEXT_SRC2(2, p2), *extra);	\
			APPEND_NEXT(MALFUNC);										\
			nils |= is_##OUTTYPE##_nil(res);							\
		}																\
	} else {															\
		for (BUN i = 0; i < ci2.ncand; i++) {							\
			oid p2 = (canditer_next(&ci2) - off2);						\
			FUNC_CALL(FUNC, res, src1, GET_NEXT_SRC2(2, p2), *extra);	\
			APPEND_NEXT(MALFUNC);										\
			nils |= is_##OUTTYPE##_nil(res);							\
		}																\
	}																	\
	BATsetcount(bn, ci2.ncand);											\
	bn->tnonil = !nils;													\
	bn->tnil = nils;													\
	bn->tsorted = ci2.ncand < 2;										\
	bn->trevsorted = ci2.ncand < 2;										\
	bn->tkey = ci2.ncand < 2;											\
bailout:																\
	CLEAR_EXTRA_MULTI(res);												\
	if (b2) {															\
		bat_iterator_end(&b2i);											\
		BBPunfix(b2->batCacheid);										\
	}																	\
	BBPreclaim(s2);														\
	if (bn) {															\
		if (msg)														\
			BBPreclaim(bn);												\
		else {															\
			*ret = bn->batCacheid;										\
			BBPkeepref(bn);												\
		}																\
	}																	\
	return msg;															\
}																		\
static str																\
NAME##_bulk_p2(Client cntxt, MalBlkPtr mb, MalStkPtr stk, InstrPtr pci)	\
{																		\
	str msg = MAL_SUCCEED;												\
	BAT *b1 = NULL, *s1 = NULL, *bn = NULL;								\
	oid off1;															\
	struct canditer ci1 = {0};											\
	bool nils = false;													\
	bat *ret = getArgReference_bat(stk, pci, 0),						\
		*bid1 = getArgReference_bat(stk, pci, 1),						\
		*sid1 = pci->argc == 5 ? getArgReference_bat(stk, pci, 4) : NULL; \
	lng *extra = getArgReference_lng(stk, pci, 3);						\
	BATiter b1i;														\
	DEC_SRC1(INTYPE1, 1);												\
	const INTYPE2 src2 = *(INTYPE2*)getArgReference(stk, pci, 2);		\
	DEC_OUTPUT(OUTTYPE, n);												\
	DEC_EXTRA(OUTTYPE, res, MALFUNC);									\
																		\
	(void) cntxt;														\
	(void) mb;															\
	if ((b1 = BATdescriptor(*bid1)) == NULL) {							\
		msg = createException(MAL, "batmtime." MALFUNC,					\
			  SQLSTATE(HY002) RUNTIME_OBJECT_MISSING);					\
		goto bailout;													\
	}																	\
	b1i = bat_iterator(b1);												\
	if (sid1 && !is_bat_nil(*sid1) && (s1 = BATdescriptor(*sid1)) == NULL) { \
		msg = createException(MAL, "batmtime." MALFUNC,					\
			  SQLSTATE(HY002) RUNTIME_OBJECT_MISSING);					\
		goto bailout;													\
	}																	\
	canditer_init(&ci1, b1, s1);										\
	if ((bn = COLnew(ci1.hseq, TYPE_##OUTTYPE, ci1.ncand, TRANSIENT)) == NULL) { \
		msg = createException(MAL, "batmtime." MALFUNC,					\
			  SQLSTATE(HY013) MAL_MALLOC_FAIL);							\
		goto bailout;													\
	}																	\
	off1 = b1->hseqbase;												\
	INIT_SRC1(1);														\
	INIT_OUTPUT(n);														\
	if (ci1.tpe == cand_dense) {										\
		for (BUN i = 0; i < ci1.ncand; i++) {							\
			oid p1 = (canditer_next_dense(&ci1) - off1);				\
			FUNC_CALL(FUNC, res, GET_NEXT_SRC1(1, p1), src2, *extra);	\
			APPEND_NEXT(MALFUNC);										\
			nils |= is_##OUTTYPE##_nil(res);							\
		}																\
	} else {															\
		for (BUN i = 0; i < ci1.ncand; i++) {							\
			oid p1 = (canditer_next(&ci1) - off1);						\
			FUNC_CALL(FUNC, res, GET_NEXT_SRC1(1, p1), src2, *extra);	\
			APPEND_NEXT(MALFUNC);										\
			nils |= is_##OUTTYPE##_nil(res);							\
		}																\
	}																	\
	BATsetcount(bn, ci1.ncand);											\
	bn->tnonil = !nils;													\
	bn->tnil = nils;													\
	bn->tsorted = ci1.ncand < 2;										\
	bn->trevsorted = ci1.ncand < 2;										\
	bn->tkey = ci1.ncand < 2;											\
bailout:																\
	CLEAR_EXTRA_MULTI(res);												\
	if (b1) {															\
		bat_iterator_end(&b1i);											\
		BBPunfix(b1->batCacheid);										\
	}																	\
	BBPreclaim(s1);														\
	if (bn) {															\
		if (msg)														\
			BBPreclaim(bn);												\
		else {															\
			*ret = bn->batCacheid;										\
			BBPkeepref(bn);												\
		}																\
	}																	\
	return msg;															\
}

#define func3_noexcept(FUNC, RET, PARAM1, PARAM2, E) RET = FUNC(PARAM1, PARAM2, E)
#define func3_except(FUNC, RET, PARAM1, PARAM2, E) msg = FUNC(&RET, PARAM1, PARAM2, E); if (msg) break

func2(MTIMEdate_diff, "diff",
	  date, date, lng, date_diff_imp, func2_noexcept,
	  DEC_VAR, DEC_VAR, DEC_VAR_R, DEC_INT,
	  INIT_VARIN, INIT_VARIN, INIT_VAROUT,
	  GET_NEXT_VAR, GET_NEXT_VAR, APPEND_VAR, FINISH_INT_SINGLE, CLEAR_NOTHING)
func2(MTIMEdaytime_diff_msec, "diff",
	  daytime, daytime, lng, daytime_diff, func2_noexcept,
	  DEC_VAR, DEC_VAR, DEC_VAR_R, DEC_INT,
	  INIT_VARIN, INIT_VARIN, INIT_VAROUT,
	  GET_NEXT_VAR, GET_NEXT_VAR, APPEND_VAR, FINISH_INT_SINGLE, CLEAR_NOTHING)
func2(MTIMEdate_sub_msec_interval, "date_sub_msec_interval",
	  date, lng, date, date_sub_msec_interval, func2_except,
	  DEC_VAR_R, DEC_VAR_R, DEC_VAR_R, DEC_INT,
	  INIT_VARIN, INIT_VARIN, INIT_VAROUT,
	  GET_NEXT_VAR, GET_NEXT_VAR, APPEND_VAR, FINISH_INT_SINGLE, CLEAR_NOTHING)
func2(MTIMEdate_add_msec_interval, "date_add_msec_interval",
	  date, lng, date, date_add_msec_interval, func2_except,
	  DEC_VAR_R, DEC_VAR_R, DEC_VAR_R, DEC_INT,
	  INIT_VARIN, INIT_VARIN, INIT_VAROUT,
	  GET_NEXT_VAR, GET_NEXT_VAR, APPEND_VAR, FINISH_INT_SINGLE, CLEAR_NOTHING)
func2(MTIMEtimestamp_sub_msec_interval, "timestamp_sub_msec_interval",
	  timestamp, lng, timestamp, timestamp_sub_msec_interval, func2_except,
	  DEC_VAR_R, DEC_VAR_R, DEC_VAR_R, DEC_INT,
	  INIT_VARIN, INIT_VARIN, INIT_VAROUT,
	  GET_NEXT_VAR, GET_NEXT_VAR, APPEND_VAR, FINISH_INT_SINGLE, CLEAR_NOTHING)
func2(MTIMEtimestamp_add_msec_interval, "timestamp_add_msec_interval",
	  timestamp, lng, timestamp, timestamp_add_msec_interval, func2_except,
	  DEC_VAR_R, DEC_VAR_R, DEC_VAR_R, DEC_INT,
	  INIT_VARIN, INIT_VARIN, INIT_VAROUT,
	  GET_NEXT_VAR, GET_NEXT_VAR, APPEND_VAR, FINISH_INT_SINGLE, CLEAR_NOTHING)
func2(MTIMEodbc_timestamp_add_msec_interval_time,
	  "odbc_timestamp_add_msec_time", daytime, lng, timestamp,
	  odbc_timestamp_add_msec_interval_time, func2_except, DEC_VAR_R, DEC_VAR_R,
	  DEC_VAR_R, DEC_INT, INIT_VARIN, INIT_VARIN, INIT_VAROUT, GET_NEXT_VAR,
	  GET_NEXT_VAR, APPEND_VAR, FINISH_INT_SINGLE, CLEAR_NOTHING)
func2(MTIMEodbc_timestamp_add_month_interval_time,
	  "odbc_timestamp_add_month_time", daytime, int, timestamp,
	  odbc_timestamp_add_month_interval_time, func2_except, DEC_VAR_R,
	  DEC_VAR_R, DEC_VAR_R, DEC_INT, INIT_VARIN, INIT_VARIN, INIT_VAROUT,
	  GET_NEXT_VAR, GET_NEXT_VAR, APPEND_VAR, FINISH_INT_SINGLE, CLEAR_NOTHING)
func2(MTIMEodbc_timestamp_add_msec_interval_date,
	  "odbc_timestamp_add_msec_date", date, lng, timestamp,
	  odbc_timestamp_add_msec_interval_date, func2_except, DEC_VAR_R, DEC_VAR_R,
	  DEC_VAR_R, DEC_INT, INIT_VARIN, INIT_VARIN, INIT_VAROUT, GET_NEXT_VAR,
	  GET_NEXT_VAR, APPEND_VAR, FINISH_INT_SINGLE, CLEAR_NOTHING)
func2(MTIMEtimestamp_sub_month_interval, "timestamp_sub_month_interval",
	  timestamp, int, timestamp, timestamp_sub_month_interval, func2_except,
	  DEC_VAR_R, DEC_VAR_R, DEC_VAR_R, DEC_INT, INIT_VARIN, INIT_VARIN,
	  INIT_VAROUT, GET_NEXT_VAR, GET_NEXT_VAR, APPEND_VAR, FINISH_INT_SINGLE,
	  CLEAR_NOTHING)
func2(MTIMEtimestamp_add_month_interval, "timestamp_add_month_interval",
	  timestamp, int, timestamp, timestamp_add_month_interval, func2_except,
	  DEC_VAR_R, DEC_VAR_R, DEC_VAR_R, DEC_INT, INIT_VARIN, INIT_VARIN,
	  INIT_VAROUT, GET_NEXT_VAR, GET_NEXT_VAR, APPEND_VAR, FINISH_INT_SINGLE,
	  CLEAR_NOTHING)
func2(MTIMEtime_sub_msec_interval, "time_sub_msec_interval", daytime, lng,
	  daytime, time_sub_msec_interval, func2_noexcept, DEC_VAR_R, DEC_VAR_R,
	  DEC_VAR_R, DEC_INT, INIT_VARIN, INIT_VARIN, INIT_VAROUT, GET_NEXT_VAR,
	  GET_NEXT_VAR, APPEND_VAR, FINISH_INT_SINGLE, CLEAR_NOTHING)
func2(MTIMEtime_add_msec_interval, "time_add_msec_interval", daytime, lng,
	  daytime, time_add_msec_interval, func2_noexcept, DEC_VAR_R, DEC_VAR_R,
	  DEC_VAR_R, DEC_INT, INIT_VARIN, INIT_VARIN, INIT_VAROUT, GET_NEXT_VAR,
	  GET_NEXT_VAR, APPEND_VAR, FINISH_INT_SINGLE, CLEAR_NOTHING)
func2(MTIMEdate_submonths, "date_sub_month_interval", date, int, date, date_submonths,
	  func2_except, DEC_VAR_R, DEC_VAR_R, DEC_VAR_R, DEC_INT, INIT_VARIN,
	  INIT_VARIN, INIT_VAROUT, GET_NEXT_VAR, GET_NEXT_VAR, APPEND_VAR,
	  FINISH_INT_SINGLE, CLEAR_NOTHING)
func2(MTIMEdate_addmonths, "addmonths", date, int, date, date_addmonths,
	  func2_except, DEC_VAR_R, DEC_VAR_R, DEC_VAR_R, DEC_INT, INIT_VARIN,
	  INIT_VARIN, INIT_VAROUT, GET_NEXT_VAR, GET_NEXT_VAR, APPEND_VAR,
	  FINISH_INT_SINGLE, CLEAR_NOTHING)
func1(MTIMEdate_extract_century, "century", date, int, date_century,
	  COPYFLAGS, func1_noexcept, DEC_VAR_R, DEC_VAR_R, INIT_VARIN, INIT_VAROUT,
	  GET_NEXT_VAR)
func1(MTIMEdate_extract_decade, "decade", date, int, date_decade,
	  COPYFLAGS, func1_noexcept, DEC_VAR_R, DEC_VAR_R, INIT_VARIN, INIT_VAROUT,
	  GET_NEXT_VAR)
func1(MTIMEdate_extract_year, "year", date, int, date_year, COPYFLAGS,
	  func1_noexcept, DEC_VAR_R, DEC_VAR_R, INIT_VARIN, INIT_VAROUT,
	  GET_NEXT_VAR)
func1(MTIMEdate_extract_quarter, "quarter", date, bte, date_quarter,
	  SETFLAGS, func1_noexcept, DEC_VAR_R, DEC_VAR_R, INIT_VARIN, INIT_VAROUT,
	  GET_NEXT_VAR)
func1(MTIMEdate_extract_month, "month", date, bte, date_month, SETFLAGS,
	  func1_noexcept, DEC_VAR_R, DEC_VAR_R, INIT_VARIN, INIT_VAROUT,
	  GET_NEXT_VAR)
func1(MTIMEdate_extract_day, "day", date, bte, date_day, SETFLAGS,
	  func1_noexcept, DEC_VAR_R, DEC_VAR_R, INIT_VARIN, INIT_VAROUT,
	  GET_NEXT_VAR)
func1(MTIMEdate_extract_dayofyear, "dayofyear", date, sht, date_dayofyear,
	  SETFLAGS, func1_noexcept, DEC_VAR_R, DEC_VAR_R, INIT_VARIN, INIT_VAROUT,
	  GET_NEXT_VAR)
func1(MTIMEdate_extract_weekofyear, "weekofyear", date, bte,
	  date_weekofyear, SETFLAGS, func1_noexcept, DEC_VAR_R, DEC_VAR_R,
	  INIT_VARIN, INIT_VAROUT, GET_NEXT_VAR)
func1(MTIMEdate_extract_usweekofyear, "usweekofyear", date, bte,
	  date_usweekofyear, SETFLAGS, func1_noexcept, DEC_VAR_R, DEC_VAR_R,
	  INIT_VARIN, INIT_VAROUT, GET_NEXT_VAR)
func1(MTIMEdate_extract_dayofweek, "dayofweek", date, bte, date_dayofweek,
	  SETFLAGS, func1_noexcept, DEC_VAR_R, DEC_VAR_R, INIT_VARIN, INIT_VAROUT,
	  GET_NEXT_VAR)
func1(MTIMEdate_extract_epoch_ms, "epoch_ms", date, lng,
	  date_to_msec_since_epoch, COPYFLAGS, func1_noexcept, DEC_VAR_R, DEC_VAR_R,
	  INIT_VARIN, INIT_VAROUT, GET_NEXT_VAR)
func1(MTIMEdaytime_extract_hours, "hours", daytime, bte, daytime_hour,
	  COPYFLAGS, func1_noexcept, DEC_VAR_R, DEC_VAR_R, INIT_VARIN, INIT_VAROUT,
	  GET_NEXT_VAR)
func1(MTIMEdaytime_extract_minutes, "minutes", daytime, bte,
	  daytime_min, SETFLAGS, func1_noexcept, DEC_VAR_R, DEC_VAR_R, INIT_VARIN,
	  INIT_VAROUT, GET_NEXT_VAR)
func1(MTIMEdaytime_extract_sql_seconds, "sql_seconds", daytime, int,
	  daytime_sec_usec, SETFLAGS, func1_noexcept, DEC_VAR_R, DEC_VAR_R,
	  INIT_VARIN, INIT_VAROUT, GET_NEXT_VAR)
func1(MTIMEdaytime_extract_epoch_ms, "epoch_ms", daytime, lng,
	  daytime_to_msec_since_epoch, COPYFLAGS, func1_noexcept, DEC_VAR_R,
	  DEC_VAR_R, INIT_VARIN, INIT_VAROUT, GET_NEXT_VAR)
func2(MTIMEtimestamp_diff_msec, "diff", timestamp, timestamp, lng, TSDIFF,
	  func2_noexcept, DEC_VAR, DEC_VAR, DEC_VAR_R, DEC_INT, INIT_VARIN,
	  INIT_VARIN, INIT_VAROUT, GET_NEXT_VAR, GET_NEXT_VAR, APPEND_VAR,
	  FINISH_INT_SINGLE, CLEAR_NOTHING)
func1(MTIMEtimestamp_century, "century", timestamp, int,
	  timestamp_century, COPYFLAGS, func1_noexcept, DEC_VAR_R, DEC_VAR_R,
	  INIT_VARIN, INIT_VAROUT, GET_NEXT_VAR)
func1(MTIMEtimestamp_decade, "decade", timestamp, int,
	  timestamp_decade, COPYFLAGS, func1_noexcept, DEC_VAR_R, DEC_VAR_R,
	  INIT_VARIN, INIT_VAROUT, GET_NEXT_VAR)
func1(MTIMEtimestamp_year, "year", timestamp, int, timestamp_year,
	  COPYFLAGS, func1_noexcept, DEC_VAR_R, DEC_VAR_R, INIT_VARIN, INIT_VAROUT,
	  GET_NEXT_VAR)
func1(MTIMEtimestamp_quarter, "quarter", timestamp, bte,
	  timestamp_quarter, SETFLAGS, func1_noexcept, DEC_VAR_R, DEC_VAR_R,
	  INIT_VARIN, INIT_VAROUT, GET_NEXT_VAR)
func1(MTIMEtimestamp_month, "month", timestamp, bte, timestamp_month,
	  SETFLAGS, func1_noexcept, DEC_VAR_R, DEC_VAR_R, INIT_VARIN, INIT_VAROUT,
	  GET_NEXT_VAR)
func1(MTIMEtimestamp_day, "day", timestamp, bte, timestamp_day,
	  SETFLAGS, func1_noexcept, DEC_VAR_R, DEC_VAR_R, INIT_VARIN, INIT_VAROUT,
	  GET_NEXT_VAR)
func1(MTIMEtimestamp_hours, "hours", timestamp, bte, timestamp_hours,
	  SETFLAGS, func1_noexcept, DEC_VAR_R, DEC_VAR_R, INIT_VARIN, INIT_VAROUT,
	  GET_NEXT_VAR)
func1(MTIMEtimestamp_minutes, "minutes", timestamp, bte,
	  timestamp_minutes, SETFLAGS, func1_noexcept, DEC_VAR_R, DEC_VAR_R,
	  INIT_VARIN, INIT_VAROUT, GET_NEXT_VAR)
func1(MTIMEtimestamp_sql_seconds, "sql_seconds", timestamp, int,
	  timestamp_extract_usecond, SETFLAGS, func1_noexcept, DEC_VAR_R, DEC_VAR_R,
	  INIT_VARIN, INIT_VAROUT, GET_NEXT_VAR)
func1(MTIMEtimestamp_extract_epoch_ms, "epoch_ms", timestamp, lng,
	  timestamp_to_msec_since_epoch, COPYFLAGS, func1_noexcept, DEC_VAR_R,
	  DEC_VAR_R, INIT_VARIN, INIT_VAROUT, GET_NEXT_VAR)
func1(MTIMEsql_year, "year", int, int, sql_year, COPYFLAGS, func1_noexcept,
	  DEC_VAR_R, DEC_VAR_R, INIT_VARIN, INIT_VAROUT, GET_NEXT_VAR)
func1(MTIMEsql_month, "month", int, int, sql_month, SETFLAGS,
	  func1_noexcept, DEC_VAR_R, DEC_VAR_R, INIT_VARIN, INIT_VAROUT,
	  GET_NEXT_VAR)
func1(MTIMEsql_day, "day", lng, lng, sql_day, COPYFLAGS, func1_noexcept,
	  DEC_VAR_R, DEC_VAR_R, INIT_VARIN, INIT_VAROUT, GET_NEXT_VAR)
func1(MTIMEsql_hours, "hours", lng, int, sql_hours, SETFLAGS,
	  func1_noexcept, DEC_VAR_R, DEC_VAR_R, INIT_VARIN, INIT_VAROUT,
	  GET_NEXT_VAR)
func1(MTIMEsql_minutes, "minutes", lng, int, sql_minutes, SETFLAGS,
	  func1_noexcept, DEC_VAR_R, DEC_VAR_R, INIT_VARIN, INIT_VAROUT,
	  GET_NEXT_VAR)
func1(MTIMEsql_seconds, "seconds", lng, int, sql_seconds, SETFLAGS,
	  func1_noexcept, DEC_VAR_R, DEC_VAR_R, INIT_VARIN, INIT_VAROUT,
	  GET_NEXT_VAR)
func1(MTIMEmsec_extract_epoch_ms, "epoch_ms", lng, lng, msec_since_epoch,
	  COPYFLAGS, func1_noexcept, DEC_VAR_R, DEC_VAR_R, INIT_VARIN, INIT_VAROUT,
	  GET_NEXT_VAR)
static inline str
date_fromstr_func(date *ret, const char *s)
{
	if (date_fromstr(s, &(size_t) { sizeof(date) }, &ret, false) < 0) {
		if (strNil(s))
			throw(MAL, "mtime.date_fromstr",
				  SQLSTATE(42000) "Conversion of NULL string to date failed");
		throw(MAL, "mtime.date_fromstr",
			  SQLSTATE(22007) "Conversion of string '%s' to date failed", s);
	}
	return MAL_SUCCEED;
}

func1(MTIMEdate_fromstr, "date", char * const, date,
	  date_fromstr_func, SETFLAGS, func1_except,
	  DEC_ITER, DEC_VAR_R, INIT_ITERIN, INIT_VAROUT, GET_NEXT_ITER)
#define date_date(m) m
func1(MTIMEdate_date, "date", date, date,
	  date_date, COPYFLAGS, func1_noexcept,
	  DEC_VAR_R, DEC_VAR_R, INIT_VARIN, INIT_VAROUT, GET_NEXT_VAR)
func1(MTIMEtimestamp_extract_date, "date", timestamp, date,
	  timestamp_date, COPYFLAGS, func1_noexcept,
	  DEC_VAR_R, DEC_VAR_R, INIT_VARIN, INIT_VAROUT, GET_NEXT_VAR)

static inline date
timestamp_tz_date(timestamp ts, lng tz_msec)
{
	ts = timestamp_add_usec(ts, tz_msec * LL_CONSTANT(1000));
	return timestamp_date(ts);
}

func2(MTIMEtimestamp_tz_extract_date, "date",
	  timestamp, lng, date, timestamp_tz_date, func2_noexcept,
	  DEC_VAR, DEC_VAR, DEC_VAR_R, DEC_INT,
	  INIT_VARIN, INIT_VARIN, INIT_VAROUT,
	  GET_NEXT_VAR, GET_NEXT_VAR, APPEND_VAR, FINISH_INT_SINGLE, CLEAR_NOTHING)

static inline str
timestamp_fromstr_func(timestamp *ret, const char *s)
{
	if (timestamp_fromstr(s, &(size_t) { sizeof(timestamp) }, &ret, false) < 0)
		throw(MAL, "mtime.timestamp_fromstr", GDK_EXCEPTION);
	return MAL_SUCCEED;
}

func1(MTIMEtimestamp_fromstr, "timestamp", char * const, timestamp,
	  timestamp_fromstr_func, SETFLAGS, func1_except,
	  DEC_ITER, DEC_VAR_R, INIT_ITERIN, INIT_VAROUT, GET_NEXT_ITER)
#define timestamp_timestamp(m) m
func1(MTIMEtimestamp_timestamp, "timestamp", timestamp, timestamp,
	  timestamp_timestamp, COPYFLAGS, func1_noexcept,
	  DEC_VAR_R, DEC_VAR_R, INIT_VARIN, INIT_VAROUT, GET_NEXT_VAR)
#define mkts(dt)	timestamp_create(dt, daytime_create(0, 0, 0, 0))
func1(MTIMEtimestamp_fromdate, "timestamp", date, timestamp,
	  mkts, COPYFLAGS, func1_noexcept,
	  DEC_VAR_R, DEC_VAR_R, INIT_VARIN, INIT_VAROUT, GET_NEXT_VAR)
#define seconds_since_epoch(t) is_timestamp_nil(t) ? int_nil : (int) (timestamp_diff(t, unixepoch) / 1000000);
func1(MTIMEseconds_since_epoch, "epoch", timestamp, int,
	  seconds_since_epoch, COPYFLAGS, func1_noexcept,
	  DEC_VAR_R, DEC_VAR_R, INIT_VARIN, INIT_VAROUT, GET_NEXT_VAR)
#define mktsfromsec(sec)	(is_int_nil(sec) ?							\
							 timestamp_nil :							\
							 timestamp_add_usec(unixepoch,				\
												(sec) * LL_CONSTANT(1000000)))
#define mktsfrommsec(msec)	(is_lng_nil(msec) ?							\
							 timestamp_nil :							\
							 timestamp_add_usec(unixepoch,				\
												(msec) * LL_CONSTANT(1000)))
/* TODO later I have to remove this call */
func1(MTIMEtimestamp_fromsecond_epoch, "epoch", int,
	  timestamp, mktsfromsec, COPYFLAGS, func1_noexcept, DEC_VAR_R, DEC_VAR_R,
	  INIT_VARIN, INIT_VAROUT, GET_NEXT_VAR)
func1(MTIMEtimestamp_fromsecond, "timestamp", int, timestamp,
	  mktsfromsec, COPYFLAGS, func1_noexcept, DEC_VAR_R, DEC_VAR_R, INIT_VARIN,
	  INIT_VAROUT, GET_NEXT_VAR)
/* TODO later I have to remove this call */
func1(MTIMEtimestamp_frommsec_epoch, "epoch", lng, timestamp,
	  mktsfrommsec, COPYFLAGS, func1_noexcept,
	  DEC_VAR_R, DEC_VAR_R, INIT_VARIN, INIT_VAROUT, GET_NEXT_VAR)
func1(MTIMEtimestamp_frommsec, "timestamp", lng, timestamp,
	  mktsfrommsec, COPYFLAGS, func1_noexcept,
	  DEC_VAR_R, DEC_VAR_R, INIT_VARIN, INIT_VAROUT, GET_NEXT_VAR)
static inline str
daytime_fromstr_func(daytime *ret, const char *s)
{
	if (daytime_fromstr(s, &(size_t) { sizeof(daytime) }, &ret, false) < 0)
		throw(MAL, "mtime.daytime_fromstr", GDK_EXCEPTION);
	return MAL_SUCCEED;
}

func1(MTIMEdaytime_fromstr, "daytime", char * const, daytime,
	  daytime_fromstr_func, SETFLAGS, func1_except,
	  DEC_ITER, DEC_VAR_R, INIT_ITERIN, INIT_VAROUT, GET_NEXT_ITER)
#define daytime_daytime(m) m
func1(MTIMEdaytime_daytime, "daytime", daytime, daytime,
	  daytime_daytime, COPYFLAGS, func1_noexcept,
	  DEC_VAR_R, DEC_VAR_R, INIT_VARIN, INIT_VAROUT, GET_NEXT_VAR)
static inline str
daytime_fromseconds(daytime *ret, lng secs)
{
	if (is_lng_nil(secs))
		*ret = daytime_nil;
	else if (secs < 0 || secs >= 24 * 60 * 60)
		throw(MAL, "mtime.daytime_fromseconds",
			  SQLSTATE(42000) ILLEGAL_ARGUMENT);
	else
		*ret = (daytime) (secs * 1000000);
	return MAL_SUCCEED;
}

func1(MTIMEdaytime_fromseconds, "daytime", lng, daytime,
	  daytime_fromseconds, COPYFLAGS, func1_except,
	  DEC_VAR_R, DEC_VAR_R, INIT_VARIN, INIT_VAROUT, GET_NEXT_VAR)
func1(MTIMEtimestamp_extract_daytime, "daytime", timestamp,
	  daytime, timestamp_daytime, SETFLAGS, func1_noexcept, DEC_VAR_R,
	  DEC_VAR_R, INIT_VARIN, INIT_VAROUT, GET_NEXT_VAR)
/* return current system time zone offset in seconds East of Greenwich */
static int
local_timezone(int *isdstp)
{
	int tzone = 0;
	int isdst = -1;

#if defined(_MSC_VER)
	DYNAMIC_TIME_ZONE_INFORMATION tzinf;

	/* documentation says: UTC = localtime + Bias (in minutes),
	 * but experimentation during DST period says, UTC = localtime
	 * + Bias + DaylightBias, and presumably during non DST
	 * period, UTC = localtime + Bias */
	switch (GetDynamicTimeZoneInformation(&tzinf)) {
	case TIME_ZONE_ID_STANDARD:	/* using standard time */
	case TIME_ZONE_ID_UNKNOWN:	/* no daylight saving time in this zone */
		isdst = 0;
		tzone = -(int) tzinf.Bias * 60;
		break;
	case TIME_ZONE_ID_DAYLIGHT:	/* using daylight saving time */
		isdst = 1;
		tzone = -(int) (tzinf.Bias + tzinf.DaylightBias) * 60;
		break;
	default:					/* aka TIME_ZONE_ID_INVALID */
		/* call failed, we don't know the time zone */
		tzone = 0;
		break;
	}
#elif defined(HAVE_TM_GMTOFF)
	time_t t;
	struct tm tm = (struct tm) { 0 };

	if ((t = time(NULL)) != (time_t) - 1 && localtime_r(&t, &tm)) {
		tzone = (int) tm.tm_gmtoff;
		isdst = tm.tm_isdst;
	}
#else
	time_t t;
	struct tm tm = (struct tm) { 0 };

	if ((t = time(NULL)) != (time_t) - 1 && gmtime_r(&t, &tm)) {
		timestamp lt, gt;
		gt = timestamp_create(date_create(tm.tm_year + 1900,
										  tm.tm_mon + 1,
										  tm.tm_mday),
							  daytime_create(tm.tm_hour,
											 tm.tm_min,
											 tm.tm_sec == 60 ? 59 : tm.tm_sec,
											 0));
		if (localtime_r(&t, &tm)) {
			isdst = tm.tm_isdst;
			lt = timestamp_create(date_create(tm.tm_year + 1900,
											  tm.tm_mon + 1,
											  tm.tm_mday),
								  daytime_create(tm.tm_hour,
												 tm.tm_min,
												 tm.tm_sec == 60 ? 59 : tm.tm_sec, 0));
			tzone = (int) (timestamp_diff(lt, gt) / 1000000);
		}
	}
#endif
	if (isdstp)
		*isdstp = isdst;
	return tzone;
}

static str
MTIMElocal_timezone_msec(lng *ret)
{
	int tzone = local_timezone(NULL);
	*ret = (lng) tzone *1000;
	return MAL_SUCCEED;
}

static str
timestamp_to_str_withtz(str *buf, const timestamp *d, const char *const *format, const char *type,
				 const char *malfunc, long gmtoff)
{
	date dt;
	daytime t;
	struct tm tm;

	if (is_timestamp_nil(*d) || strNil(*format)) {
		strcpy(*buf, str_nil);
		return MAL_SUCCEED;
	}
	dt = timestamp_date(*d);
	t = timestamp_daytime(*d);
	tm = (struct tm) {
		.tm_year = date_year(dt) - 1900,
		.tm_mon = date_month(dt) - 1,
		.tm_mday = date_day(dt),
		.tm_wday = date_dayofweek(dt) % 7,
		.tm_yday = date_dayofyear(dt) - 1,
		.tm_hour = daytime_hour(t),
		.tm_min = daytime_min(t),
		.tm_sec = daytime_sec(t),
#if defined(HAVE_TM_GMTOFF)
		.tm_gmtoff = gmtoff,
#endif
	};
	if (strftime(*buf, MTIME_STR_BUFFER_LENGTH, *format, &tm) == 0)
		throw(MAL, malfunc, "cannot convert %s", type);
	return MAL_SUCCEED;
}

static str
timestamp_to_str(str *buf, const timestamp *d, const char *const *format, const char *type,
				 const char *malfunc)
{
	return timestamp_to_str_withtz( buf, d, format, type, malfunc, 0);
}

static str
timestamptz_to_str(str *buf, const timestamp *d, const char *const *format, const char *type,
				 const char *malfunc, long gmtoff)
{
	timestamp t = *d;
	t = timestamp_add_usec(t, gmtoff * LL_CONSTANT(1000000));
	return timestamp_to_str_withtz( buf, &t, format, type, malfunc, gmtoff);
}


static str
str_to_timestamp(timestamp *ret, const char *const *s, const char *const *format, const long gmtoff, const char *type,
				 const char *malfunc)
{
	struct tm tm = (struct tm) {
		.tm_isdst = -1,
		.tm_mday = 1,
#ifdef HAVE_TM_GMTOFF
		.tm_gmtoff = (int) gmtoff,
#endif
	};
	int tz = (int) gmtoff;

	if (strNil(*s) || strNil(*format)) {
		*ret = timestamp_nil;
		return MAL_SUCCEED;
	}
#ifdef HAVE_STRPTIME
	if (strptime(*s, *format, &tm) == NULL)
#else
	if (strptime2(*s, *format, &tm, &tz) == NULL)
#endif
		throw(MAL, malfunc,
			  "format '%s', doesn't match %s '%s'", *format, type, *s);
	*ret = timestamp_create(date_create(tm.tm_year + 1900,
										tm.tm_mon + 1,
										tm.tm_mday),
							daytime_create(tm.tm_hour,
										   tm.tm_min,
										   tm.tm_sec == 60 ? 59 : tm.tm_sec,
										   0));
#ifdef HAVE_TM_GMTOFF
	tz = tm.tm_gmtoff;
#endif
	*ret = timestamp_add_usec(*ret, -tz * LL_CONSTANT(1000000));
	if (is_timestamp_nil(*ret))
		throw(MAL, malfunc, "bad %s '%s'", type, *s);
	return MAL_SUCCEED;
}

static inline str
str_to_date(date *ret, const char *s, const char *format, lng tz_msec)
{
	str msg = MAL_SUCCEED;
	timestamp ts;
	(void)tz_msec;
	if ((msg = str_to_timestamp(&ts, &s, &format, /*tz_msec/1000*/0, "date",
								"mtime.str_to_date")) != MAL_SUCCEED)
		return msg;
	*ret = timestamp_date(ts);
	return MAL_SUCCEED;
}

func3(MTIMEstr_to_date, "str_to_date",
	  char * const, char * const, date, str_to_date, func3_except,
	  DEC_ITER, DEC_ITER, DEC_VAR_R, DEC_INT,
	  INIT_ITERIN, INIT_ITERIN, INIT_VAROUT,
	  GET_NEXT_ITER, GET_NEXT_ITER,
	  APPEND_VAR, FINISH_INT_SINGLE, CLEAR_NOTHING)

static inline str
str_to_time(daytime *ret, const char *s, const char *format, lng tz_msec)
{
	str msg = MAL_SUCCEED;
	timestamp ts;
	if ((msg = str_to_timestamp(&ts, &s, &format, (long)(tz_msec/1000), "time",
								"mtime.str_to_time")) != MAL_SUCCEED)
		return msg;
	*ret = timestamp_daytime(ts);
	return MAL_SUCCEED;
}

func3(MTIMEstr_to_time, "str_to_time",
	  char * const, char * const, daytime, str_to_time, func3_except,
	  DEC_ITER, DEC_ITER, DEC_VAR_R, DEC_INT,
	  INIT_ITERIN, INIT_ITERIN, INIT_VAROUT,
	  GET_NEXT_ITER, GET_NEXT_ITER,
	  APPEND_VAR, FINISH_INT_SINGLE, CLEAR_NOTHING)

static inline str
str_to_timestamp_func(timestamp *ret, const char *s, const char *format, lng tz_msec)
{
	return str_to_timestamp(ret, &s, &format, (long)(tz_msec/1000), "timestamp",
							"mtime.str_to_timestamp");
}

func3(MTIMEstr_to_timestamp, "str_to_timestamp",
	  char * const, char * const, timestamp, str_to_timestamp_func, func3_except,
	  DEC_ITER, DEC_ITER, DEC_VAR_R, DEC_INT,
	  INIT_ITERIN, INIT_ITERIN, INIT_VAROUT,
	  GET_NEXT_ITER, GET_NEXT_ITER,
	  APPEND_VAR, FINISH_INT_SINGLE, CLEAR_NOTHING)

static inline str
date_to_str(str *ret, date d, const char *format)
{
	timestamp ts = timestamp_create(d, timestamp_daytime(timestamp_current()));
	return timestamp_to_str(ret, &ts, &format, "date", "mtime.date_to_str");
}

func2(MTIMEdate_to_str, "date_to_str",
	  date, char * const, str, date_to_str, func2_except,
	  DEC_VAR, DEC_ITER, DEC_NOTHING, DEC_BUFFER,
	  INIT_VARIN, INIT_ITERIN, INIT_NOTHING,
	  GET_NEXT_VAR, GET_NEXT_ITER,
	  APPEND_STR, FINISH_BUFFER_SINGLE, FINISH_BUFFER_MULTI)

static inline str
time_to_str(str *ret, daytime d, const char *format)
{
	timestamp ts = timestamp_create(timestamp_date(timestamp_current()), d);
	return timestamp_to_str(ret, &ts, &format, "time", "mtime.time_to_str");
}

func2(MTIMEtime_to_str, "time_to_str",
	  daytime, char * const, str, time_to_str, func2_except,
	  DEC_VAR, DEC_ITER, DEC_NOTHING, DEC_BUFFER,
	  INIT_VARIN, INIT_ITERIN, INIT_NOTHING,
	  GET_NEXT_VAR, GET_NEXT_ITER,
	  APPEND_STR, FINISH_BUFFER_SINGLE, FINISH_BUFFER_MULTI)

static inline str
timetz_to_str(str *ret, daytime d, const char *format, lng tz_msec)
{
	timestamp ts = timestamp_create(timestamp_date(timestamp_current()), d);
	return timestamptz_to_str(ret, &ts, &format, "time", "mtime.timetz_to_str", (long)(tz_msec/1000));
}
func3(MTIMEtimetz_to_str, "timetz_to_str",
	  daytime, char * const, str, timetz_to_str, func3_except,
	  DEC_VAR, DEC_ITER, DEC_NOTHING, DEC_BUFFER,
	  INIT_VARIN, INIT_ITERIN, INIT_NOTHING,
	  GET_NEXT_VAR, GET_NEXT_ITER,
	  APPEND_STR, FINISH_BUFFER_SINGLE, FINISH_BUFFER_MULTI)

static inline str
timestamp_to_str_func(str *ret, timestamp d, const char *format)
{
	return timestamp_to_str(ret, &d, &format, "timestamp", "mtime.timestamp_to_str");
}

func2(MTIMEtimestamp_to_str, "timestamp_to_str",
	  timestamp, char * const, str, timestamp_to_str_func, func2_except,
	  DEC_VAR, DEC_ITER, DEC_NOTHING, DEC_BUFFER,
	  INIT_VARIN, INIT_ITERIN, INIT_NOTHING,
	  GET_NEXT_VAR, GET_NEXT_ITER,
	  APPEND_STR, FINISH_BUFFER_SINGLE, FINISH_BUFFER_MULTI)

static inline str
timestamptz_to_str_func(str *ret, timestamp d, const char *format, lng tz_msec)
{
	return timestamptz_to_str(ret, &d, &format, "timestamp", "mtime.timestamptz_to_str", (long)(tz_msec/1000));
}

func3(MTIMEtimestamptz_to_str, "timestamptz_to_str",
	  timestamp, char * const, str, timestamptz_to_str_func, func3_except,
	  DEC_VAR, DEC_ITER, DEC_NOTHING, DEC_BUFFER,
	  INIT_VARIN, INIT_ITERIN, INIT_NOTHING,
	  GET_NEXT_VAR, GET_NEXT_ITER,
	  APPEND_STR, FINISH_BUFFER_SINGLE, FINISH_BUFFER_MULTI)

static inline lng
timestampdiff_sec(timestamp t1, timestamp t2)
{
	return TSDIFF(t1, t2) / 1000;
}

static inline lng
timestampdiff_sec_date_timestamp(date d, timestamp ts)
{
	return timestampdiff_sec(timestamp_fromdate(d), ts);
}

static inline lng
timestampdiff_sec_timestamp_date(timestamp ts, date d)
{
	return timestampdiff_sec(ts, timestamp_fromdate(d));
}

static inline lng
timestampdiff_min(timestamp t1, timestamp t2)
{
	return TSDIFF(t1, t2) / 1000 / 60;
}

static inline lng
timestampdiff_min_date_timestamp(date d, timestamp ts)
{
	return timestampdiff_min(timestamp_fromdate(d), ts);
}

static inline lng
timestampdiff_min_timestamp_date(timestamp ts, date d)
{
	return timestampdiff_min(ts, timestamp_fromdate(d));
}

static inline lng
timestampdiff_hour(timestamp t1, timestamp t2)
{
	return TSDIFF(t1, t2) / 1000 / 60 / 60;
}

static inline lng
timestampdiff_hour_date_timestamp(date d, timestamp ts)
{
	return timestampdiff_hour(timestamp_fromdate(d), ts);
}

static inline lng
timestampdiff_hour_timestamp_date(timestamp ts, date d)
{
	return timestampdiff_hour(ts, timestamp_fromdate(d));
}

static inline int
timestampdiff_day(timestamp t1, timestamp t2)
{
	return date_diff(timestamp_date(t1), timestamp_date(t2));
}

static inline int
timestampdiff_day_time_timestamp(daytime t, timestamp ts)
{
	date today = timestamp_date(timestamp_current());
	return timestampdiff_day(timestamp_create(today, t), ts);
}

static inline int
timestampdiff_day_timestamp_time(timestamp ts, daytime t)
{
	date today = timestamp_date(timestamp_current());
	return timestampdiff_day(ts, timestamp_create(today, t));
}

static inline int
timestampdiff_week(timestamp t1, timestamp t2)
{
	return date_diff(timestamp_date(t1), timestamp_date(t2)) / 7;
}

static inline int
timestampdiff_week_time_timestamp(daytime t, timestamp ts)
{
	date today = timestamp_date(timestamp_current());
	return timestampdiff_week(timestamp_create(today, t), ts);
}

static inline int
timestampdiff_week_timestamp_time(timestamp ts, daytime t)
{
	date today = timestamp_date(timestamp_current());
	return timestampdiff_week(ts, timestamp_create(today, t));
}

static inline int
timestampdiff_month(timestamp t1, timestamp t2)
{
	date d1 = timestamp_date(t1);
	date d2 = timestamp_date(t2);
	return ((date_year(d1) - date_year(d2)) * 12) + (date_month(d1) -
													 date_month(d2));
}

static inline int
timestampdiff_month_time_timestamp(daytime t, timestamp ts)
{
	date today = timestamp_date(timestamp_current());
	return timestampdiff_month(timestamp_create(today, t), ts);
}

static inline int
timestampdiff_month_timestamp_time(timestamp ts, daytime t)
{
	date today = timestamp_date(timestamp_current());
	return timestampdiff_month(ts, timestamp_create(today, t));
}

static inline int
timestampdiff_quarter(timestamp t1, timestamp t2)
{
	date d1 = timestamp_date(t1);
	date d2 = timestamp_date(t2);
	return ((date_year(d1) - date_year(d2)) * 4) + (date_quarter(d1) -
													date_quarter(d2));
}

static inline int
timestampdiff_quarter_time_timestamp(daytime t, timestamp ts)
{
	date today = timestamp_date(timestamp_current());
	return timestampdiff_quarter(timestamp_create(today, t), ts);
}

static inline int
timestampdiff_quarter_timestamp_time(timestamp ts, daytime t)
{
	date today = timestamp_date(timestamp_current());
	return timestampdiff_quarter(ts, timestamp_create(today, t));
}

static inline int
timestampdiff_year(timestamp t1, timestamp t2)
{
	date d1 = timestamp_date(t1);
	date d2 = timestamp_date(t2);
	return date_year(d1) - date_year(d2);
}

static inline int
timestampdiff_year_time_timestamp(daytime t, timestamp ts)
{
	date today = timestamp_date(timestamp_current());
	return timestampdiff_year(timestamp_create(today, t), ts);
}

static inline int
timestampdiff_year_timestamp_time(timestamp ts, daytime t)
{
	date today = timestamp_date(timestamp_current());
	return timestampdiff_year(ts, timestamp_create(today, t));
}

// odbc timestampdiff variants
func2(MTIMEtimestampdiff_sec, "timestampdiff_sec",
	  timestamp, timestamp, lng, timestampdiff_sec, func2_noexcept,
	  DEC_VAR, DEC_VAR, DEC_VAR_R, DEC_INT,
	  INIT_VARIN, INIT_VARIN, INIT_VAROUT,
	  GET_NEXT_VAR, GET_NEXT_VAR, APPEND_VAR, FINISH_INT_SINGLE, CLEAR_NOTHING)
func2(MTIMEtimestampdiff_sec_d_ts, "timestampdiff_sec",
	  date, timestamp, lng, timestampdiff_sec_date_timestamp, func2_noexcept,
	  DEC_VAR, DEC_VAR, DEC_VAR_R, DEC_INT,
	  INIT_VARIN, INIT_VARIN, INIT_VAROUT,
	  GET_NEXT_VAR, GET_NEXT_VAR, APPEND_VAR, FINISH_INT_SINGLE, CLEAR_NOTHING)
func2(MTIMEtimestampdiff_sec_ts_d, "timestampdiff_sec",
	  timestamp, date, lng, timestampdiff_sec_timestamp_date, func2_noexcept,
	  DEC_VAR, DEC_VAR, DEC_VAR_R, DEC_INT,
	  INIT_VARIN, INIT_VARIN, INIT_VAROUT,
	  GET_NEXT_VAR, GET_NEXT_VAR, APPEND_VAR, FINISH_INT_SINGLE, CLEAR_NOTHING)
func2(MTIMEtimestampdiff_min, "timestampdiff_min",
	  timestamp, timestamp, lng, timestampdiff_min, func2_noexcept,
	  DEC_VAR, DEC_VAR, DEC_VAR_R, DEC_INT,
	  INIT_VARIN, INIT_VARIN, INIT_VAROUT,
	  GET_NEXT_VAR, GET_NEXT_VAR, APPEND_VAR, FINISH_INT_SINGLE, CLEAR_NOTHING)
func2(MTIMEtimestampdiff_min_d_ts, "timestampdiff_min",
	  date, timestamp, lng, timestampdiff_min_date_timestamp, func2_noexcept,
	  DEC_VAR, DEC_VAR, DEC_VAR_R, DEC_INT,
	  INIT_VARIN, INIT_VARIN, INIT_VAROUT,
	  GET_NEXT_VAR, GET_NEXT_VAR, APPEND_VAR, FINISH_INT_SINGLE, CLEAR_NOTHING)
func2(MTIMEtimestampdiff_min_ts_d, "timestampdiff_min",
	  timestamp, date, lng, timestampdiff_min_timestamp_date, func2_noexcept,
	  DEC_VAR, DEC_VAR, DEC_VAR_R, DEC_INT,
	  INIT_VARIN, INIT_VARIN, INIT_VAROUT,
	  GET_NEXT_VAR, GET_NEXT_VAR, APPEND_VAR, FINISH_INT_SINGLE, CLEAR_NOTHING)
func2(MTIMEtimestampdiff_hour, "timestampdiff_hour",
	  timestamp, timestamp, lng, timestampdiff_hour, func2_noexcept,
	  DEC_VAR, DEC_VAR, DEC_VAR_R, DEC_INT,
	  INIT_VARIN, INIT_VARIN, INIT_VAROUT,
	  GET_NEXT_VAR, GET_NEXT_VAR, APPEND_VAR, FINISH_INT_SINGLE, CLEAR_NOTHING)
func2(MTIMEtimestampdiff_hour_d_ts, "timestampdiff_hour",
	  date, timestamp, lng, timestampdiff_hour_date_timestamp, func2_noexcept,
	  DEC_VAR, DEC_VAR, DEC_VAR_R, DEC_INT,
	  INIT_VARIN, INIT_VARIN, INIT_VAROUT,
	  GET_NEXT_VAR, GET_NEXT_VAR, APPEND_VAR, FINISH_INT_SINGLE, CLEAR_NOTHING)
func2(MTIMEtimestampdiff_hour_ts_d, "timestampdiff_hour",
	  timestamp, date, lng, timestampdiff_hour_timestamp_date, func2_noexcept,
	  DEC_VAR, DEC_VAR, DEC_VAR_R, DEC_INT,
	  INIT_VARIN, INIT_VARIN, INIT_VAROUT,
	  GET_NEXT_VAR, GET_NEXT_VAR, APPEND_VAR, FINISH_INT_SINGLE, CLEAR_NOTHING)
func2(MTIMEtimestampdiff_day, "timestampdiff_day",
	  timestamp, timestamp, int, timestampdiff_day, func2_noexcept,
	  DEC_VAR, DEC_VAR, DEC_VAR_R, DEC_INT,
	  INIT_VARIN, INIT_VARIN, INIT_VAROUT,
	  GET_NEXT_VAR, GET_NEXT_VAR, APPEND_VAR, FINISH_INT_SINGLE, CLEAR_NOTHING)
func2(MTIMEtimestampdiff_day_t_ts, "timestampdiff_day",
	  daytime, timestamp, int, timestampdiff_day_time_timestamp, func2_noexcept,
	  DEC_VAR, DEC_VAR, DEC_VAR_R, DEC_INT,
	  INIT_VARIN, INIT_VARIN, INIT_VAROUT,
	  GET_NEXT_VAR, GET_NEXT_VAR, APPEND_VAR, FINISH_INT_SINGLE, CLEAR_NOTHING)
func2(MTIMEtimestampdiff_day_ts_t, "timestampdiff_day",
	  timestamp, daytime, int, timestampdiff_day_timestamp_time, func2_noexcept,
	  DEC_VAR, DEC_VAR, DEC_VAR_R, DEC_INT,
	  INIT_VARIN, INIT_VARIN, INIT_VAROUT,
	  GET_NEXT_VAR, GET_NEXT_VAR, APPEND_VAR, FINISH_INT_SINGLE, CLEAR_NOTHING)
func2(MTIMEtimestampdiff_week, "timestampdiff_week",
	  timestamp, timestamp, int, timestampdiff_week, func2_noexcept,
	  DEC_VAR, DEC_VAR, DEC_VAR_R, DEC_INT,
	  INIT_VARIN, INIT_VARIN, INIT_VAROUT,
	  GET_NEXT_VAR, GET_NEXT_VAR, APPEND_VAR, FINISH_INT_SINGLE, CLEAR_NOTHING)
func2(MTIMEtimestampdiff_week_t_ts, "timestampdiff_week",
	  daytime, timestamp, int, timestampdiff_week_time_timestamp,
	  func2_noexcept, DEC_VAR, DEC_VAR, DEC_VAR_R, DEC_INT, INIT_VARIN,
	  INIT_VARIN, INIT_VAROUT, GET_NEXT_VAR, GET_NEXT_VAR, APPEND_VAR,
	  FINISH_INT_SINGLE, CLEAR_NOTHING)
func2(MTIMEtimestampdiff_week_ts_t, "timestampdiff_week", timestamp,
	  daytime, int, timestampdiff_week_timestamp_time, func2_noexcept, DEC_VAR,
	  DEC_VAR, DEC_VAR_R, DEC_INT, INIT_VARIN, INIT_VARIN, INIT_VAROUT,
	  GET_NEXT_VAR, GET_NEXT_VAR, APPEND_VAR, FINISH_INT_SINGLE, CLEAR_NOTHING)
func2(MTIMEtimestampdiff_month, "timestampdiff_month", timestamp, timestamp,
	  int, timestampdiff_month, func2_noexcept, DEC_VAR, DEC_VAR, DEC_VAR_R,
	  DEC_INT, INIT_VARIN, INIT_VARIN, INIT_VAROUT, GET_NEXT_VAR, GET_NEXT_VAR,
	  APPEND_VAR, FINISH_INT_SINGLE, CLEAR_NOTHING)
func2(MTIMEtimestampdiff_month_t_ts, "timestampdiff_month",
	  daytime, timestamp, int, timestampdiff_month_time_timestamp,
	  func2_noexcept, DEC_VAR, DEC_VAR, DEC_VAR_R, DEC_INT, INIT_VARIN,
	  INIT_VARIN, INIT_VAROUT, GET_NEXT_VAR, GET_NEXT_VAR, APPEND_VAR,
	  FINISH_INT_SINGLE, CLEAR_NOTHING)
func2(MTIMEtimestampdiff_month_ts_t, "timestampdiff_month",
	  timestamp, daytime, int, timestampdiff_month_timestamp_time,
	  func2_noexcept, DEC_VAR, DEC_VAR, DEC_VAR_R, DEC_INT, INIT_VARIN,
	  INIT_VARIN, INIT_VAROUT, GET_NEXT_VAR, GET_NEXT_VAR, APPEND_VAR,
	  FINISH_INT_SINGLE, CLEAR_NOTHING)
func2(MTIMEtimestampdiff_quarter, "timestampdiff_quarter", timestamp, timestamp,
	  int, timestampdiff_quarter, func2_noexcept, DEC_VAR, DEC_VAR, DEC_VAR_R,
	  DEC_INT, INIT_VARIN, INIT_VARIN, INIT_VAROUT, GET_NEXT_VAR, GET_NEXT_VAR,
	  APPEND_VAR, FINISH_INT_SINGLE, CLEAR_NOTHING)
func2(MTIMEtimestampdiff_quarter_t_ts, "timestampdiff_quarter",
	  daytime, timestamp, int, timestampdiff_quarter_time_timestamp,
	  func2_noexcept, DEC_VAR, DEC_VAR, DEC_VAR_R, DEC_INT, INIT_VARIN,
	  INIT_VARIN, INIT_VAROUT, GET_NEXT_VAR, GET_NEXT_VAR, APPEND_VAR,
	  FINISH_INT_SINGLE, CLEAR_NOTHING)
func2(MTIMEtimestampdiff_quarter_ts_t, "timestampdiff_quarter",
	  timestamp, daytime, int, timestampdiff_quarter_timestamp_time,
	  func2_noexcept, DEC_VAR, DEC_VAR, DEC_VAR_R, DEC_INT, INIT_VARIN,
	  INIT_VARIN, INIT_VAROUT, GET_NEXT_VAR, GET_NEXT_VAR, APPEND_VAR,
	  FINISH_INT_SINGLE, CLEAR_NOTHING)
func2(MTIMEtimestampdiff_year, "timestampdiff_year", timestamp, timestamp, int,
	  timestampdiff_year, func2_noexcept, DEC_VAR, DEC_VAR, DEC_VAR_R, DEC_INT,
	  INIT_VARIN, INIT_VARIN, INIT_VAROUT, GET_NEXT_VAR, GET_NEXT_VAR,
	  APPEND_VAR, FINISH_INT_SINGLE, CLEAR_NOTHING)
func2(MTIMEtimestampdiff_year_t_ts, "timestampdiff_year",
	  daytime, timestamp, int, timestampdiff_year_time_timestamp,
	  func2_noexcept, DEC_VAR, DEC_VAR, DEC_VAR_R, DEC_INT, INIT_VARIN,
	  INIT_VARIN, INIT_VAROUT, GET_NEXT_VAR, GET_NEXT_VAR, APPEND_VAR,
	  FINISH_INT_SINGLE, CLEAR_NOTHING)
func2(MTIMEtimestampdiff_year_ts_t, "timestampdiff_year",
	  timestamp, daytime, int, timestampdiff_year_timestamp_time,
	  func2_noexcept, DEC_VAR, DEC_VAR, DEC_VAR_R, DEC_INT, INIT_VARIN,
	  INIT_VARIN, INIT_VAROUT, GET_NEXT_VAR, GET_NEXT_VAR, APPEND_VAR,
	  FINISH_INT_SINGLE, CLEAR_NOTHING)

#include "mel.h"
static mel_func mtime_init_funcs[] = {
 command("mtime", "epoch", MTIMEseconds_since_epoch, false, "unix-time (epoch) support: seconds since epoch", args(1,2, arg("",int),arg("t",timestamp))),
 pattern("batmtime", "epoch", MTIMEseconds_since_epoch_bulk, false, "", args(1,2, batarg("",int),batarg("t",timestamp))),
 pattern("batmtime", "epoch", MTIMEseconds_since_epoch_bulk, false, "", args(1,3, batarg("",int),batarg("t",timestamp),batarg("s",oid))),
 command("mtime", "epoch", MTIMEtimestamp_fromsecond_epoch, false, "convert seconds since epoch into a timestamp", args(1,2, arg("",timestamp),arg("t",int))),
 pattern("batmtime", "epoch", MTIMEtimestamp_fromsecond_epoch_bulk, false, "", args(1,2, batarg("",timestamp),batarg("t",int))),
 pattern("batmtime", "epoch", MTIMEtimestamp_fromsecond_epoch_bulk, false, "", args(1,3, batarg("",timestamp),batarg("t",int),batarg("s",oid))),
 command("mtime", "epoch", MTIMEtimestamp_frommsec_epoch, false, "convert milliseconds since epoch into a timestamp", args(1,2, arg("",timestamp),arg("t",lng))),
 pattern("batmtime", "epoch", MTIMEtimestamp_frommsec_epoch_bulk, false, "", args(1,2, batarg("",timestamp),batarg("t",lng))),
 pattern("batmtime", "epoch", MTIMEtimestamp_frommsec_epoch_bulk, false, "", args(1,3, batarg("",timestamp),batarg("t",lng),batarg("s",oid))),
 command("mtime", "date_sub_msec_interval", MTIMEdate_sub_msec_interval, false, "", args(1,3, arg("",date),arg("t",date),arg("ms",lng))),
 pattern("batmtime", "date_sub_msec_interval", MTIMEdate_sub_msec_interval_bulk, false, "", args(1,3, batarg("",date),batarg("t",date),batarg("ms",lng))),
 pattern("batmtime", "date_sub_msec_interval", MTIMEdate_sub_msec_interval_bulk_p1, false, "", args(1,3, batarg("",date),arg("t",date),batarg("ms",lng))),
 pattern("batmtime", "date_sub_msec_interval", MTIMEdate_sub_msec_interval_bulk_p2, false, "", args(1,3, batarg("",date),batarg("t",date),arg("ms",lng))),
 pattern("batmtime", "date_sub_msec_interval", MTIMEdate_sub_msec_interval_bulk, false, "", args(1,5, batarg("",date),batarg("t",date),batarg("ms",lng),batarg("s1",oid),batarg("s2",oid))),
 pattern("batmtime", "date_sub_msec_interval", MTIMEdate_sub_msec_interval_bulk_p1, false, "", args(1,4, batarg("",date),arg("t",date),batarg("ms",lng),batarg("s",oid))),
 pattern("batmtime", "date_sub_msec_interval", MTIMEdate_sub_msec_interval_bulk_p2, false, "", args(1,4, batarg("",date),batarg("t",date),arg("ms",lng),batarg("s",oid))),
 command("mtime", "date_add_msec_interval", MTIMEdate_add_msec_interval, false, "", args(1,3, arg("",date),arg("t",date),arg("ms",lng))),
 pattern("batmtime", "date_add_msec_interval", MTIMEdate_add_msec_interval_bulk, false, "", args(1,3, batarg("",date),batarg("t",date),batarg("ms",lng))),
 pattern("batmtime", "date_add_msec_interval", MTIMEdate_add_msec_interval_bulk_p1, false, "", args(1,3, batarg("",date),arg("t",date),batarg("ms",lng))),
 pattern("batmtime", "date_add_msec_interval", MTIMEdate_add_msec_interval_bulk_p2, false, "", args(1,3, batarg("",date),batarg("t",date),arg("ms",lng))),
 pattern("batmtime", "date_add_msec_interval", MTIMEdate_add_msec_interval_bulk, false, "", args(1,5, batarg("",date),batarg("t",date),batarg("ms",lng),batarg("s1",oid),batarg("s2",oid))),
 pattern("batmtime", "date_add_msec_interval", MTIMEdate_add_msec_interval_bulk_p1, false, "", args(1,4, batarg("",date),arg("t",date),batarg("ms",lng),batarg("s",oid))),
 pattern("batmtime", "date_add_msec_interval", MTIMEdate_add_msec_interval_bulk_p2, false, "", args(1,4, batarg("",date),batarg("t",date),arg("ms",lng),batarg("s",oid))),
 command("mtime", "timestamp_sub_msec_interval", MTIMEtimestamp_sub_msec_interval, false, "", args(1,3, arg("",timestamp),arg("t",timestamp),arg("ms",lng))),
 pattern("batmtime", "timestamp_sub_msec_interval", MTIMEtimestamp_sub_msec_interval_bulk, false, "", args(1,3, batarg("",timestamp),batarg("t",timestamp),batarg("ms",lng))),
 pattern("batmtime", "timestamp_sub_msec_interval", MTIMEtimestamp_sub_msec_interval_bulk_p1, false, "", args(1,3, batarg("",timestamp),arg("t",timestamp),batarg("ms",lng))),
 pattern("batmtime", "timestamp_sub_msec_interval", MTIMEtimestamp_sub_msec_interval_bulk_p2, false, "", args(1,3, batarg("",timestamp),batarg("t",timestamp),arg("ms",lng))),
 pattern("batmtime", "timestamp_sub_msec_interval", MTIMEtimestamp_sub_msec_interval_bulk, false, "", args(1,5, batarg("",timestamp),batarg("t",timestamp),batarg("ms",lng),batarg("s1",oid),batarg("s2",oid))),
 pattern("batmtime", "timestamp_sub_msec_interval", MTIMEtimestamp_sub_msec_interval_bulk_p1, false, "", args(1,4, batarg("",timestamp),arg("t",timestamp),batarg("ms",lng),batarg("s",oid))),
 pattern("batmtime", "timestamp_sub_msec_interval", MTIMEtimestamp_sub_msec_interval_bulk_p2, false, "", args(1,4, batarg("",timestamp),batarg("t",timestamp),arg("ms",lng),batarg("s",oid))),
 command("mtime", "timestamp_add_msec_interval", MTIMEtimestamp_add_msec_interval, false, "", args(1,3, arg("",timestamp),arg("t",timestamp),arg("ms",lng))),
 pattern("batmtime", "timestamp_add_msec_interval", MTIMEtimestamp_add_msec_interval_bulk, false, "", args(1,3, batarg("",timestamp),batarg("t",timestamp),batarg("ms",lng))),
 pattern("batmtime", "timestamp_add_msec_interval", MTIMEtimestamp_add_msec_interval_bulk_p1, false, "", args(1,3, batarg("",timestamp),arg("t",timestamp),batarg("ms",lng))),
 pattern("batmtime", "timestamp_add_msec_interval", MTIMEtimestamp_add_msec_interval_bulk_p2, false, "", args(1,3, batarg("",timestamp),batarg("t",timestamp),arg("ms",lng))),
 pattern("batmtime", "timestamp_add_msec_interval", MTIMEtimestamp_add_msec_interval_bulk, false, "", args(1,5, batarg("",timestamp),batarg("t",timestamp),batarg("ms",lng),batarg("s1",oid),batarg("s2",oid))),
 pattern("batmtime", "timestamp_add_msec_interval", MTIMEtimestamp_add_msec_interval_bulk_p1, false, "", args(1,4, batarg("",timestamp),arg("t",timestamp),batarg("ms",lng),batarg("s",oid))),
 pattern("batmtime", "timestamp_add_msec_interval", MTIMEtimestamp_add_msec_interval_bulk_p2, false, "", args(1,4, batarg("",timestamp),batarg("t",timestamp),arg("ms",lng),batarg("s",oid))),
 command("mtime", "timestamp_sub_month_interval", MTIMEtimestamp_sub_month_interval, false, "Subtract months from a timestamp", args(1,3, arg("",timestamp),arg("t",timestamp),arg("s",int))),
 pattern("batmtime", "timestamp_sub_month_interval", MTIMEtimestamp_sub_month_interval_bulk, false, "", args(1,3, batarg("",timestamp),batarg("t",timestamp),batarg("s",int))),
 pattern("batmtime", "timestamp_sub_month_interval", MTIMEtimestamp_sub_month_interval_bulk_p1, false, "", args(1,3, batarg("",timestamp),arg("t",timestamp),batarg("s",int))),
 pattern("batmtime", "timestamp_sub_month_interval", MTIMEtimestamp_sub_month_interval_bulk_p2, false, "", args(1,3, batarg("",timestamp),batarg("t",timestamp),arg("s",int))),
 pattern("batmtime", "timestamp_sub_month_interval", MTIMEtimestamp_sub_month_interval_bulk, false, "", args(1,5, batarg("",timestamp),batarg("t",timestamp),batarg("s",int),batarg("s1",oid),batarg("s2",oid))),
 pattern("batmtime", "timestamp_sub_month_interval", MTIMEtimestamp_sub_month_interval_bulk_p1, false, "", args(1,4, batarg("",timestamp),arg("t",timestamp),batarg("s",int),batarg("s",oid))),
 pattern("batmtime", "timestamp_sub_month_interval", MTIMEtimestamp_sub_month_interval_bulk_p2, false, "", args(1,4, batarg("",timestamp),batarg("t",timestamp),arg("s",int),batarg("s",oid))),
 // --
 command("mtime", "timestamp_add_month_interval", MTIMEtimestamp_add_month_interval, false, "Add months to a timestamp", args(1,3, arg("",timestamp),arg("t",timestamp),arg("s",int))),
 pattern("batmtime", "timestamp_add_month_interval", MTIMEtimestamp_add_month_interval_bulk, false, "", args(1,3, batarg("",timestamp),batarg("t",timestamp),batarg("s",int))),
 pattern("batmtime", "timestamp_add_month_interval", MTIMEtimestamp_add_month_interval_bulk_p1, false, "", args(1,3, batarg("",timestamp),arg("t",timestamp),batarg("s",int))),
 pattern("batmtime", "timestamp_add_month_interval", MTIMEtimestamp_add_month_interval_bulk_p2, false, "", args(1,3, batarg("",timestamp),batarg("t",timestamp),arg("s",int))),
 pattern("batmtime", "timestamp_add_month_interval", MTIMEtimestamp_add_month_interval_bulk, false, "", args(1,5, batarg("",timestamp),batarg("t",timestamp),batarg("s",int),batarg("s1",oid),batarg("s2",oid))),
 pattern("batmtime", "timestamp_add_month_interval", MTIMEtimestamp_add_month_interval_bulk_p1, false, "", args(1,4, batarg("",timestamp),arg("t",timestamp),batarg("s",int),batarg("s",oid))),
 pattern("batmtime", "timestamp_add_month_interval", MTIMEtimestamp_add_month_interval_bulk_p2, false, "", args(1,4, batarg("",timestamp),batarg("t",timestamp),arg("s",int),batarg("s",oid))),
// odbc timestampadd corner cases
 command("mtime", "odbc_timestamp_add_msec_time", MTIMEodbc_timestamp_add_msec_interval_time, false, "", args(1,3, arg("",timestamp),arg("t",daytime),arg("ms",lng))),
 pattern("batmtime", "odbc_timestamp_add_msec_time", MTIMEodbc_timestamp_add_msec_interval_time_bulk, false, "", args(1,3, batarg("",timestamp),batarg("t",daytime),batarg("ms",lng))),
 pattern("batmtime", "odbc_timestamp_add_msec_time", MTIMEodbc_timestamp_add_msec_interval_time_bulk_p1, false, "", args(1,3, batarg("",timestamp),arg("t",daytime),batarg("ms",lng))),
 pattern("batmtime", "odbc_timestamp_add_msec_time", MTIMEodbc_timestamp_add_msec_interval_time_bulk_p2, false, "", args(1,3, batarg("",timestamp),batarg("t",daytime),arg("ms",lng))),
 pattern("batmtime", "odbc_timestamp_add_msec_time", MTIMEodbc_timestamp_add_msec_interval_time_bulk, false, "", args(1,5, batarg("",timestamp),batarg("t",daytime),batarg("ms",lng),batarg("s1",oid),batarg("s2",oid))),
 pattern("batmtime", "odbc_timestamp_add_msec_time", MTIMEodbc_timestamp_add_msec_interval_time_bulk_p1, false, "", args(1,4, batarg("",timestamp),arg("t",daytime),batarg("ms",lng),batarg("s",oid))),
 pattern("batmtime", "odbc_timestamp_add_msec_time", MTIMEodbc_timestamp_add_msec_interval_time_bulk_p2, false, "", args(1,4, batarg("",timestamp),batarg("t",daytime),arg("ms",lng),batarg("s",oid))),
 // --
 command("mtime", "odbc_timestamp_add_month_time", MTIMEodbc_timestamp_add_month_interval_time, false, "Add months to a time", args(1,3, arg("",timestamp),arg("t",daytime),arg("s",int))),
 pattern("batmtime", "odbc_timestamp_add_month_time", MTIMEodbc_timestamp_add_month_interval_time_bulk, false, "", args(1,3, batarg("",timestamp),batarg("t",daytime),batarg("s",int))),
 pattern("batmtime", "odbc_timestamp_add_month_time", MTIMEodbc_timestamp_add_month_interval_time_bulk_p1, false, "", args(1,3, batarg("",timestamp),arg("t",daytime),batarg("s",int))),
 pattern("batmtime", "odbc_timestamp_add_month_time", MTIMEodbc_timestamp_add_month_interval_time_bulk_p2, false, "", args(1,3, batarg("",timestamp),batarg("t",daytime),arg("s",int))),
 pattern("batmtime", "odbc_timestamp_add_month_time", MTIMEodbc_timestamp_add_month_interval_time_bulk, false, "", args(1,5, batarg("",timestamp),batarg("t",daytime),batarg("s",int),batarg("s1",oid),batarg("s2",oid))),
 pattern("batmtime", "odbc_timestamp_add_month_time", MTIMEodbc_timestamp_add_month_interval_time_bulk_p1, false, "", args(1,4, batarg("",timestamp),arg("t",daytime),batarg("s",int),batarg("s",oid))),
 pattern("batmtime", "odbc_timestamp_add_month_time", MTIMEodbc_timestamp_add_month_interval_time_bulk_p2, false, "", args(1,4, batarg("",timestamp),batarg("t",daytime),arg("s",int),batarg("s",oid))),
 // --
 command("mtime", "odbc_timestamp_add_msec_date", MTIMEodbc_timestamp_add_msec_interval_date, false, "", args(1,3, arg("",timestamp),arg("d",date),arg("ms",lng))),
 pattern("batmtime", "odbc_timestamp_add_msec_date", MTIMEodbc_timestamp_add_msec_interval_date_bulk, false, "", args(1,3, batarg("",timestamp),batarg("d",date),batarg("ms",lng))),
 pattern("batmtime", "odbc_timestamp_add_msec_date", MTIMEodbc_timestamp_add_msec_interval_date_bulk_p1, false, "", args(1,3, batarg("",timestamp),arg("d",date),batarg("ms",lng))),
 pattern("batmtime", "odbc_timestamp_add_msec_date", MTIMEodbc_timestamp_add_msec_interval_date_bulk_p2, false, "", args(1,3, batarg("",timestamp),batarg("d",date),arg("ms",lng))),
 pattern("batmtime", "odbc_timestamp_add_msec_date", MTIMEodbc_timestamp_add_msec_interval_date_bulk, false, "", args(1,5, batarg("",timestamp),batarg("d",date),batarg("ms",lng),batarg("s1",oid),batarg("s2",oid))),
 pattern("batmtime", "odbc_timestamp_add_msec_date", MTIMEodbc_timestamp_add_msec_interval_date_bulk_p1, false, "", args(1,4, batarg("",timestamp),arg("d",date),batarg("ms",lng),batarg("s",oid))),
 pattern("batmtime", "odbc_timestamp_add_msec_date", MTIMEodbc_timestamp_add_msec_interval_date_bulk_p2, false, "", args(1,4, batarg("",timestamp),batarg("d",date),arg("ms",lng),batarg("s",oid))),
// end odbc timestampadd corner cases
 command("mtime", "time_sub_msec_interval", MTIMEtime_sub_msec_interval, false, "Subtract seconds from a time", args(1,3, arg("",daytime),arg("t",daytime),arg("ms",lng))),
 pattern("batmtime", "time_sub_msec_interval", MTIMEtime_sub_msec_interval_bulk, false, "", args(1,3, batarg("",daytime),batarg("t",daytime),batarg("ms",lng))),
 pattern("batmtime", "time_sub_msec_interval", MTIMEtime_sub_msec_interval_bulk_p1, false, "", args(1,3, batarg("",daytime),arg("t",daytime),batarg("ms",lng))),
 pattern("batmtime", "time_sub_msec_interval", MTIMEtime_sub_msec_interval_bulk_p2, false, "", args(1,3, batarg("",daytime),batarg("t",daytime),arg("ms",lng))),
 pattern("batmtime", "time_sub_msec_interval", MTIMEtime_sub_msec_interval_bulk, false, "", args(1,5, batarg("",daytime),batarg("t",daytime),batarg("ms",lng),batarg("s1",oid),batarg("s2",oid))),
 pattern("batmtime", "time_sub_msec_interval", MTIMEtime_sub_msec_interval_bulk_p1, false, "", args(1,4, batarg("",daytime),arg("t",daytime),batarg("ms",lng),batarg("s",oid))),
 pattern("batmtime", "time_sub_msec_interval", MTIMEtime_sub_msec_interval_bulk_p2, false, "", args(1,4, batarg("",daytime),batarg("t",daytime),arg("ms",lng),batarg("s",oid))),
 command("mtime", "time_add_msec_interval", MTIMEtime_add_msec_interval, false, "Add seconds to a time", args(1,3, arg("",daytime),arg("t",daytime),arg("ms",lng))),
 pattern("batmtime", "time_add_msec_interval", MTIMEtime_add_msec_interval_bulk, false, "", args(1,3, batarg("",daytime),batarg("t",daytime),batarg("ms",lng))),
 pattern("batmtime", "time_add_msec_interval", MTIMEtime_add_msec_interval_bulk_p1, false, "", args(1,3, batarg("",daytime),arg("t",daytime),batarg("ms",lng))),
 pattern("batmtime", "time_add_msec_interval", MTIMEtime_add_msec_interval_bulk_p2, false, "", args(1,3, batarg("",daytime),batarg("t",daytime),arg("ms",lng))),
 pattern("batmtime", "time_add_msec_interval", MTIMEtime_add_msec_interval_bulk, false, "", args(1,5, batarg("",daytime),batarg("t",daytime),batarg("ms",lng),batarg("s1",oid),batarg("s2",oid))),
 pattern("batmtime", "time_add_msec_interval", MTIMEtime_add_msec_interval_bulk_p1, false, "", args(1,4, batarg("",daytime),arg("t",daytime),batarg("ms",lng),batarg("s",oid))),
 pattern("batmtime", "time_add_msec_interval", MTIMEtime_add_msec_interval_bulk_p2, false, "", args(1,4, batarg("",daytime),batarg("t",daytime),arg("ms",lng),batarg("s",oid))),
 command("mtime", "diff", MTIMEdaytime_diff_msec, false, "returns the number of msec between 'val1' and 'val2'.", args(1,3, arg("",lng),arg("val1",daytime),arg("val2",daytime))),
 pattern("batmtime", "diff", MTIMEdaytime_diff_msec_bulk, false, "", args(1,3, batarg("",lng),batarg("val1",daytime),batarg("val2",daytime))),
 pattern("batmtime", "diff", MTIMEdaytime_diff_msec_bulk_p1, false, "", args(1,3, batarg("",lng),arg("val1",daytime),batarg("val2",daytime))),
 pattern("batmtime", "diff", MTIMEdaytime_diff_msec_bulk_p2, false, "", args(1,3, batarg("",lng),batarg("val1",daytime),arg("val2",daytime))),
 pattern("batmtime", "diff", MTIMEdaytime_diff_msec_bulk, false, "", args(1,5, batarg("",lng),batarg("val1",daytime),batarg("val2",daytime),batarg("s1",oid),batarg("s2",oid))),
 pattern("batmtime", "diff", MTIMEdaytime_diff_msec_bulk_p1, false, "", args(1,4, batarg("",lng),arg("val1",daytime),batarg("val2",daytime),batarg("s",oid))),
 pattern("batmtime", "diff", MTIMEdaytime_diff_msec_bulk_p2, false, "", args(1,4, batarg("",lng),batarg("val1",daytime),arg("val2",daytime),batarg("s",oid))),
 command("mtime", "date_sub_month_interval", MTIMEdate_submonths, false, "Subtract months from a date", args(1,3, arg("",date),arg("t",date),arg("months",int))),
 pattern("batmtime", "date_sub_month_interval", MTIMEdate_submonths_bulk, false, "", args(1,3, batarg("",date),batarg("t",date),batarg("months",int))),
 pattern("batmtime", "date_sub_month_interval", MTIMEdate_submonths_bulk_p1, false, "", args(1,3, batarg("",date),arg("t",date),batarg("months",int))),
 pattern("batmtime", "date_sub_month_interval", MTIMEdate_submonths_bulk_p2, false, "", args(1,3, batarg("",date),batarg("t",date),arg("months",int))),
 pattern("batmtime", "date_sub_month_interval", MTIMEdate_submonths_bulk, false, "", args(1,5, batarg("",date),batarg("t",date),batarg("months",int),batarg("s1",oid),batarg("s2",oid))),
 pattern("batmtime", "date_sub_month_interval", MTIMEdate_submonths_bulk_p1, false, "", args(1,4, batarg("",date),arg("t",date),batarg("months",int),batarg("s",oid))),
 pattern("batmtime", "date_sub_month_interval", MTIMEdate_submonths_bulk_p2, false, "", args(1,4, batarg("",date),batarg("t",date),arg("months",int),batarg("s",oid))),
 command("mtime", "local_timezone", MTIMElocal_timezone_msec, false, "get the local timezone in seconds", args(1,1, arg("",lng))),
 command("mtime", "century", MTIMEdate_extract_century, false, "extracts century from date.", args(1,2, arg("",int),arg("d",date))),
 pattern("batmtime", "century", MTIMEdate_extract_century_bulk, false, "", args(1,2, batarg("",int),batarg("d",date))),
 pattern("batmtime", "century", MTIMEdate_extract_century_bulk, false, "", args(1,3, batarg("",int),batarg("d",date),batarg("s",oid))),
 command("mtime", "decade", MTIMEdate_extract_decade, false, "extracts decade from date.", args(1,2, arg("",int),arg("d",date))),
 pattern("batmtime", "decade", MTIMEdate_extract_decade_bulk, false, "", args(1,2, batarg("",int),batarg("d",date))),
 pattern("batmtime", "decade", MTIMEdate_extract_decade_bulk, false, "", args(1,3, batarg("",int),batarg("d",date),batarg("s",oid))),
 command("mtime", "year", MTIMEdate_extract_year, false, "extracts year from date.", args(1,2, arg("",int),arg("d",date))),
 pattern("batmtime", "year", MTIMEdate_extract_year_bulk, false, "", args(1,2, batarg("",int),batarg("d",date))),
 pattern("batmtime", "year", MTIMEdate_extract_year_bulk, false, "", args(1,3, batarg("",int),batarg("d",date),batarg("s",oid))),
 command("mtime", "quarter", MTIMEdate_extract_quarter, false, "extracts quarter from date", args(1,2, arg("",bte),arg("d",date))),
 pattern("batmtime", "quarter", MTIMEdate_extract_quarter_bulk, false, "", args(1,2, batarg("",bte),batarg("d",date))),
 pattern("batmtime", "quarter", MTIMEdate_extract_quarter_bulk, false, "", args(1,3, batarg("",bte),batarg("d",date),batarg("s",oid))),
 command("mtime", "month", MTIMEdate_extract_month, false, "extracts month from date", args(1,2, arg("",bte),arg("d",date))),
 pattern("batmtime", "month", MTIMEdate_extract_month_bulk, false, "", args(1,2, batarg("",bte),batarg("d",date))),
 pattern("batmtime", "month", MTIMEdate_extract_month_bulk, false, "", args(1,3, batarg("",bte),batarg("d",date),batarg("s",oid))),
 command("mtime", "day", MTIMEdate_extract_day, false, "extracts day from date ", args(1,2, arg("",bte),arg("d",date))),
 pattern("batmtime", "day", MTIMEdate_extract_day_bulk, false, "", args(1,2, batarg("",bte),batarg("d",date))),
 pattern("batmtime", "day", MTIMEdate_extract_day_bulk, false, "", args(1,3, batarg("",bte),batarg("d",date),batarg("s",oid))),
 command("mtime", "epoch_ms", MTIMEdate_extract_epoch_ms, false, "", args(1,2, arg("",lng),arg("d",date))),
 pattern("batmtime", "epoch_ms", MTIMEdate_extract_epoch_ms_bulk, false, "", args(1,2, batarg("",lng),batarg("d",date))),
 pattern("batmtime", "epoch_ms", MTIMEdate_extract_epoch_ms_bulk, false, "", args(1,3, batarg("",lng),batarg("d",date),batarg("s",oid))),
 command("mtime", "hours", MTIMEdaytime_extract_hours, false, "extracts hour from daytime", args(1,2, arg("",bte),arg("h",daytime))),
 pattern("batmtime", "hours", MTIMEdaytime_extract_hours_bulk, false, "", args(1,2, batarg("",bte),batarg("d",daytime))),
 pattern("batmtime", "hours", MTIMEdaytime_extract_hours_bulk, false, "", args(1,3, batarg("",bte),batarg("d",daytime),batarg("s",oid))),
 command("mtime", "minutes", MTIMEdaytime_extract_minutes, false, "extracts minutes from daytime", args(1,2, arg("",bte),arg("d",daytime))),
 pattern("batmtime", "minutes", MTIMEdaytime_extract_minutes_bulk, false, "", args(1,2, batarg("",bte),batarg("d",daytime))),
 pattern("batmtime", "minutes", MTIMEdaytime_extract_minutes_bulk, false, "", args(1,3, batarg("",bte),batarg("d",daytime),batarg("s",oid))),
 command("mtime", "sql_seconds", MTIMEdaytime_extract_sql_seconds, false, "extracts seconds (with fractional milliseconds) from daytime", args(1,2, arg("",int),arg("d",daytime))),
 pattern("batmtime", "sql_seconds", MTIMEdaytime_extract_sql_seconds_bulk, false, "", args(1,2, batarg("",int),batarg("d",daytime))),
 pattern("batmtime", "sql_seconds", MTIMEdaytime_extract_sql_seconds_bulk, false, "", args(1,3, batarg("",int),batarg("d",daytime),batarg("s",oid))),
 command("mtime", "epoch_ms", MTIMEdaytime_extract_epoch_ms, false, "", args(1,2, arg("",lng),arg("d",daytime))),
 pattern("batmtime", "epoch_ms", MTIMEdaytime_extract_epoch_ms_bulk, false, "", args(1,2, batarg("",lng),batarg("d",daytime))),
 pattern("batmtime", "epoch_ms", MTIMEdaytime_extract_epoch_ms_bulk, false, "", args(1,3, batarg("",lng),batarg("d",daytime),batarg("s",oid))),
 command("mtime", "addmonths", MTIMEdate_addmonths, false, "returns the date after a number of\nmonths (possibly negative).", args(1,3, arg("",date),arg("value",date),arg("months",int))),
 pattern("batmtime", "addmonths", MTIMEdate_addmonths_bulk, false, "", args(1,3, batarg("",date),batarg("value",date),batarg("months",int))),
 pattern("batmtime", "addmonths", MTIMEdate_addmonths_bulk_p1, false, "", args(1,3, batarg("",date),arg("value",date),batarg("months",int))),
 pattern("batmtime", "addmonths", MTIMEdate_addmonths_bulk_p2, false, "", args(1,3, batarg("",date),batarg("value",date),arg("months",int))),
 pattern("batmtime", "addmonths", MTIMEdate_addmonths_bulk, false, "", args(1,5, batarg("",date),batarg("value",date),batarg("months",int),batarg("s1",oid),batarg("s2",oid))),
 pattern("batmtime", "addmonths", MTIMEdate_addmonths_bulk_p1, false, "", args(1,4, batarg("",date),arg("value",date),batarg("months",int),batarg("s",oid))),
 pattern("batmtime", "addmonths", MTIMEdate_addmonths_bulk_p2, false, "", args(1,4, batarg("",date),batarg("value",date),arg("months",int),batarg("s",oid))),
 command("mtime", "diff", MTIMEdate_diff, false, "returns the number of days\nbetween 'val1' and 'val2'.", args(1,3, arg("",lng),arg("val1",date),arg("val2",date))),
 pattern("batmtime", "diff", MTIMEdate_diff_bulk, false, "", args(1,3, batarg("",lng),batarg("val1",date),batarg("val2",date))),
 pattern("batmtime", "diff", MTIMEdate_diff_bulk_p1, false, "", args(1,3, batarg("",lng),arg("val1",date),batarg("val2",date))),
 pattern("batmtime", "diff", MTIMEdate_diff_bulk_p2, false, "", args(1,3, batarg("",lng),batarg("val1",date),arg("val2",date))),
 pattern("batmtime", "diff", MTIMEdate_diff_bulk, false, "", args(1,5, batarg("",lng),batarg("val1",date),batarg("val2",date),batarg("s1",oid),batarg("s2",oid))),
 pattern("batmtime", "diff", MTIMEdate_diff_bulk_p1, false, "", args(1,4, batarg("",lng),arg("val1",date),batarg("val2",date),batarg("s",oid))),
 pattern("batmtime", "diff", MTIMEdate_diff_bulk_p2, false, "", args(1,4, batarg("",lng),batarg("val1",date),arg("val2",date),batarg("s",oid))),
 command("mtime", "dayofyear", MTIMEdate_extract_dayofyear, false, "Returns N where d is the Nth day\nof the year (january 1 returns 1)", args(1,2, arg("",sht),arg("d",date))),
 pattern("batmtime", "dayofyear", MTIMEdate_extract_dayofyear_bulk, false, "", args(1,2, batarg("",sht),batarg("d",date))),
 pattern("batmtime", "dayofyear", MTIMEdate_extract_dayofyear_bulk, false, "", args(1,3, batarg("",sht),batarg("d",date),batarg("s",oid))),
 command("mtime", "weekofyear", MTIMEdate_extract_weekofyear, false, "Returns the week number in the year.", args(1,2, arg("",bte),arg("d",date))),
 pattern("batmtime", "weekofyear", MTIMEdate_extract_weekofyear_bulk, false, "", args(1,2, batarg("",bte),batarg("d",date))),
 pattern("batmtime", "weekofyear", MTIMEdate_extract_weekofyear_bulk, false, "", args(1,3, batarg("",bte),batarg("d",date),batarg("s",oid))),
 command("mtime", "usweekofyear", MTIMEdate_extract_usweekofyear, false, "Returns the week number in the year, US style.", args(1,2, arg("",bte),arg("d",date))),
 pattern("batmtime", "usweekofyear", MTIMEdate_extract_usweekofyear_bulk, false, "", args(1,2, batarg("",bte),batarg("d",date))),
 pattern("batmtime", "usweekofyear", MTIMEdate_extract_usweekofyear_bulk, false, "", args(1,3, batarg("",bte),batarg("d",date),batarg("s",oid))),
 command("mtime", "dayofweek", MTIMEdate_extract_dayofweek, false, "Returns the current day of the week\nwhere 1=monday, .., 7=sunday", args(1,2, arg("",bte),arg("d",date))),
 pattern("batmtime", "dayofweek", MTIMEdate_extract_dayofweek_bulk, false, "", args(1,2, batarg("",bte),batarg("d",date))),
 pattern("batmtime", "dayofweek", MTIMEdate_extract_dayofweek_bulk, false, "", args(1,3, batarg("",bte),batarg("d",date),batarg("s",oid))),
 command("mtime", "diff", MTIMEtimestamp_diff_msec, false, "returns the number of milliseconds\nbetween 'val1' and 'val2'.", args(1,3, arg("",lng),arg("val1",timestamp),arg("val2",timestamp))),
 pattern("batmtime", "diff", MTIMEtimestamp_diff_msec_bulk, false, "", args(1,3, batarg("",lng),batarg("val1",timestamp),batarg("val2",timestamp))),
 pattern("batmtime", "diff", MTIMEtimestamp_diff_msec_bulk_p1, false, "", args(1,3, batarg("",lng),arg("val1",timestamp),batarg("val2",timestamp))),
 pattern("batmtime", "diff", MTIMEtimestamp_diff_msec_bulk_p2, false, "", args(1,3, batarg("",lng),batarg("val1",timestamp),arg("val2",timestamp))),
 pattern("batmtime", "diff", MTIMEtimestamp_diff_msec_bulk, false, "", args(1,5, batarg("",lng),batarg("val1",timestamp),batarg("val2",timestamp),batarg("s1",oid),batarg("s2",oid))),
 pattern("batmtime", "diff", MTIMEtimestamp_diff_msec_bulk_p1, false, "", args(1,4, batarg("",lng),arg("val1",timestamp),batarg("val2",timestamp),batarg("s",oid))),
 pattern("batmtime", "diff", MTIMEtimestamp_diff_msec_bulk_p2, false, "", args(1,4, batarg("",lng),batarg("val1",timestamp),arg("val2",timestamp),batarg("s",oid))),
 command("mtime", "str_to_date", MTIMEstr_to_date, false, "create a date from the string, using the specified format (see man strptime)", args(1,4, arg("",date),arg("s",str),arg("format",str),arg("tz_msec",lng))),
 pattern("batmtime", "str_to_date", MTIMEstr_to_date_bulk, false, "", args(1,4, batarg("",date),batarg("s",str),batarg("format",str),arg("tz_msec",lng))),
 pattern("batmtime", "str_to_date", MTIMEstr_to_date_bulk_p1, false, "", args(1,4, batarg("",date),arg("s",str),batarg("format",str),arg("tz_msec",lng))),
 pattern("batmtime", "str_to_date", MTIMEstr_to_date_bulk_p2, false, "", args(1,4, batarg("",date),batarg("s",str),arg("format",str),arg("tz_msec",lng))),
 pattern("batmtime", "str_to_date", MTIMEstr_to_date_bulk, false, "", args(1,6, batarg("",date),batarg("s",str),batarg("format",str),batarg("s1",oid),batarg("s2",oid),arg("tz_msec",lng))),
 pattern("batmtime", "str_to_date", MTIMEstr_to_date_bulk_p1, false, "", args(1,5, batarg("",date),arg("s",str),batarg("format",str),arg("tz_msec",lng),batarg("s",oid))),
 pattern("batmtime", "str_to_date", MTIMEstr_to_date_bulk_p2, false, "", args(1,5, batarg("",date),batarg("s",str),arg("format",str),arg("tz_msec",lng),batarg("s",oid))),
 command("mtime", "date_to_str", MTIMEdate_to_str, false, "create a string from the date, using the specified format (see man strftime)", args(1,3, arg("",str),arg("d",date),arg("format",str))),
 pattern("batmtime", "date_to_str", MTIMEdate_to_str_bulk, false, "", args(1,3, batarg("",str),batarg("d",date),batarg("format",str))),
 pattern("batmtime", "date_to_str", MTIMEdate_to_str_bulk_p1, false, "", args(1,3, batarg("",str),arg("d",date),batarg("format",str))),
 pattern("batmtime", "date_to_str", MTIMEdate_to_str_bulk_p2, false, "", args(1,3, batarg("",str),batarg("d",date),arg("format",str))),
 pattern("batmtime", "date_to_str", MTIMEdate_to_str_bulk, false, "", args(1,5, batarg("",str),batarg("d",date),batarg("format",str),batarg("s1",oid),batarg("s2",oid))),
 pattern("batmtime", "date_to_str", MTIMEdate_to_str_bulk_p1, false, "", args(1,4, batarg("",str),arg("d",date),batarg("format",str),batarg("s",oid))),
 pattern("batmtime", "date_to_str", MTIMEdate_to_str_bulk_p2, false, "", args(1,4, batarg("",str),batarg("d",date),arg("format",str),batarg("s",oid))),
 command("mtime", "str_to_time", MTIMEstr_to_time, false, "create a time from the string, using the specified format (see man strptime)", args(1,4, arg("",daytime),arg("s",str),arg("format",str),arg("tz_msec",lng))),
 pattern("batmtime", "str_to_time", MTIMEstr_to_time_bulk, false, "", args(1,4, batarg("",daytime),batarg("s",str),batarg("format",str),arg("tz_msec",lng))),
 pattern("batmtime", "str_to_time", MTIMEstr_to_time_bulk_p1, false, "", args(1,4, batarg("",daytime),arg("s",str),batarg("format",str),arg("tz_msec",lng))),
 pattern("batmtime", "str_to_time", MTIMEstr_to_time_bulk_p2, false, "", args(1,4, batarg("",daytime),batarg("s",str),arg("format",str),arg("tz_msec",lng))),
 pattern("batmtime", "str_to_time", MTIMEstr_to_time_bulk, false, "", args(1,6, batarg("",daytime),batarg("s",str),batarg("format",str),batarg("s1",oid),batarg("s2",oid),arg("tz_msec",lng))),
 pattern("batmtime", "str_to_time", MTIMEstr_to_time_bulk_p1, false, "", args(1,5, batarg("",daytime),arg("s",str),batarg("format",str),arg("tz_msec",lng),batarg("s",oid))),
 pattern("batmtime", "str_to_time", MTIMEstr_to_time_bulk_p2, false, "", args(1,5, batarg("",daytime),batarg("s",str),arg("format",str),arg("tz_msec",lng),batarg("s",oid))),
 command("mtime", "time_to_str", MTIMEtime_to_str, false, "create a string from the time, using the specified format (see man strftime)", args(1,3, arg("",str),arg("d",daytime),arg("format",str))),
 pattern("batmtime", "time_to_str", MTIMEtime_to_str_bulk, false, "", args(1,3, batarg("",str),batarg("d",daytime),batarg("format",str))),
 pattern("batmtime", "time_to_str", MTIMEtime_to_str_bulk_p1, false, "", args(1,3, batarg("",str),arg("d",daytime),batarg("format",str))),
 pattern("batmtime", "time_to_str", MTIMEtime_to_str_bulk_p2, false, "", args(1,3, batarg("",str),batarg("d",daytime),arg("format",str))),
 pattern("batmtime", "time_to_str", MTIMEtime_to_str_bulk, false, "", args(1,5, batarg("",str),batarg("d",daytime),batarg("format",str),batarg("s1",oid),batarg("s2",oid))),
 pattern("batmtime", "time_to_str", MTIMEtime_to_str_bulk_p1, false, "", args(1,4, batarg("",str),arg("d",daytime),batarg("format",str),batarg("s",oid))),
 pattern("batmtime", "time_to_str", MTIMEtime_to_str_bulk_p2, false, "", args(1,4, batarg("",str),batarg("d",daytime),arg("format",str),batarg("s",oid))),
 command("mtime", "timetz_to_str", MTIMEtimetz_to_str, false, "create a string from the time, using the specified format (see man strftime)", args(1,4, arg("",str),arg("d",daytime),arg("format",str),arg("tz_msec",lng))),
 pattern("batmtime", "timetz_to_str", MTIMEtimetz_to_str_bulk, false, "", args(1,4, batarg("",str),batarg("d",daytime),batarg("format",str),arg("tz_msec",lng))),
 pattern("batmtime", "timetz_to_str", MTIMEtimetz_to_str_bulk_p1, false, "", args(1,4, batarg("",str),arg("d",daytime),batarg("format",str),arg("tz_msec",lng))),
 pattern("batmtime", "timetz_to_str", MTIMEtimetz_to_str_bulk_p2, false, "", args(1,4, batarg("",str),batarg("d",daytime),arg("format",str),arg("tz_msec",lng))),
 pattern("batmtime", "timetz_to_str", MTIMEtimetz_to_str_bulk, false, "", args(1,6, batarg("",str),batarg("d",daytime),batarg("format",str),batarg("s1",oid),batarg("s2",oid),arg("tz_msec",lng))),
 pattern("batmtime", "timetz_to_str", MTIMEtimetz_to_str_bulk_p1, false, "", args(1,5, batarg("",str),arg("d",daytime),batarg("format",str),arg("tz_msec",lng),batarg("s",oid))),
 pattern("batmtime", "timetz_to_str", MTIMEtimetz_to_str_bulk_p2, false, "", args(1,5, batarg("",str),batarg("d",daytime),arg("format",str),arg("tz_msec",lng),batarg("s",oid))),
 command("mtime", "str_to_timestamp", MTIMEstr_to_timestamp, false, "create a timestamp from the string, using the specified format (see man strptime)", args(1,4, arg("",timestamp),arg("s",str),arg("format",str),arg("tz_msec",lng))),
 pattern("batmtime", "str_to_timestamp", MTIMEstr_to_timestamp_bulk, false, "", args(1,4, batarg("",timestamp),batarg("d",str),batarg("format",str),arg("tz_msec",lng))),
 pattern("batmtime", "str_to_timestamp", MTIMEstr_to_timestamp_bulk_p1, false, "", args(1,4, batarg("",timestamp),arg("s",str),batarg("format",str),arg("tz_msec",lng))),
 pattern("batmtime", "str_to_timestamp", MTIMEstr_to_timestamp_bulk_p2, false, "", args(1,4, batarg("",timestamp),batarg("s",str),arg("format",str),arg("tz_msec",lng))),
 pattern("batmtime", "str_to_timestamp", MTIMEstr_to_timestamp_bulk, false, "", args(1,6, batarg("",timestamp),batarg("d",str),batarg("format",str),batarg("s1",oid),batarg("s2",oid),arg("tz_msec",lng))),
 pattern("batmtime", "str_to_timestamp", MTIMEstr_to_timestamp_bulk_p1, false, "", args(1,5, batarg("",timestamp),arg("s",str),batarg("format",str),arg("tz_msec",lng),batarg("s",oid))),
 pattern("batmtime", "str_to_timestamp", MTIMEstr_to_timestamp_bulk_p2, false, "", args(1,5, batarg("",timestamp),batarg("s",str),arg("format",str),arg("tz_msec",lng),batarg("s",oid))),
 command("mtime", "timestamp_to_str", MTIMEtimestamp_to_str, false, "create a string from the time, using the specified format (see man strftime)", args(1,3, arg("",str),arg("d",timestamp),arg("format",str))),
 pattern("batmtime", "timestamp_to_str", MTIMEtimestamp_to_str_bulk, false, "", args(1,3, batarg("",str),batarg("d",timestamp),batarg("format",str))),
 pattern("batmtime", "timestamp_to_str", MTIMEtimestamp_to_str_bulk_p1, false, "", args(1,3, batarg("",str),arg("d",timestamp),batarg("format",str))),
 pattern("batmtime", "timestamp_to_str", MTIMEtimestamp_to_str_bulk_p2, false, "", args(1,3, batarg("",str),batarg("d",timestamp),arg("format",str))),
 pattern("batmtime", "timestamp_to_str", MTIMEtimestamp_to_str_bulk, false, "", args(1,5, batarg("",str),batarg("d",timestamp),batarg("format",str),batarg("s1",oid),batarg("s2",oid))),
 pattern("batmtime", "timestamp_to_str", MTIMEtimestamp_to_str_bulk_p1, false, "", args(1,4, batarg("",str),arg("d",timestamp),batarg("format",str),batarg("s",oid))),
 pattern("batmtime", "timestamp_to_str", MTIMEtimestamp_to_str_bulk_p2, false, "", args(1,4, batarg("",str),batarg("d",timestamp),arg("format",str),batarg("s",oid))),
 command("mtime", "timestamptz_to_str", MTIMEtimestamptz_to_str, false, "create a string from the time, using the specified format (see man strftime)", args(1,4, arg("",str),arg("d",timestamp),arg("format",str),arg("tz_msec",lng))),
 pattern("batmtime", "timestamptz_to_str", MTIMEtimestamptz_to_str_bulk, false, "", args(1,4, batarg("",str),batarg("d",timestamp),batarg("format",str),arg("tz_msec",lng))),
 pattern("batmtime", "timestamptz_to_str", MTIMEtimestamptz_to_str_bulk_p1, false, "", args(1,4, batarg("",str),arg("d",timestamp),batarg("format",str),arg("tz_msec",lng))),
 pattern("batmtime", "timestamptz_to_str", MTIMEtimestamptz_to_str_bulk_p2, false, "", args(1,4, batarg("",str),batarg("d",timestamp),arg("format",str),arg("tz_msec",lng))),
 pattern("batmtime", "timestamptz_to_str", MTIMEtimestamptz_to_str_bulk, false, "", args(1,6, batarg("",str),batarg("d",timestamp),batarg("format",str),batarg("s1",oid),batarg("s2",oid),arg("tz_msec",lng))),
 pattern("batmtime", "timestamptz_to_str", MTIMEtimestamptz_to_str_bulk_p1, false, "", args(1,5, batarg("",str),arg("d",timestamp),batarg("format",str),arg("tz_msec",lng),batarg("s",oid))),
 pattern("batmtime", "timestamptz_to_str", MTIMEtimestamptz_to_str_bulk_p2, false, "", args(1,5, batarg("",str),batarg("d",timestamp),arg("format",str),arg("tz_msec",lng),batarg("s",oid))),
 command("mtime", "current_timestamp", MTIMEcurrent_timestamp, false, "", args(1,1, arg("",timestamp))),
 command("mtime", "current_date", MTIMEcurrent_date, false, "", args(1,1, arg("",date))),
 command("mtime", "current_time", MTIMEcurrent_time, false, "", args(1,1, arg("",daytime))),
 command("mtime", "century", MTIMEtimestamp_century, false, "", args(1,2, arg("",int),arg("t",timestamp))),
 pattern("batmtime", "century", MTIMEtimestamp_century_bulk, false, "", args(1,2, batarg("",int),batarg("t",timestamp))),
 pattern("batmtime", "century", MTIMEtimestamp_century_bulk, false, "", args(1,3, batarg("",int),batarg("t",timestamp),batarg("s",oid))),
 command("mtime", "decade", MTIMEtimestamp_decade, false, "", args(1,2, arg("",int),arg("t",timestamp))),
 pattern("batmtime", "decade", MTIMEtimestamp_decade_bulk, false, "", args(1,2, batarg("",int),batarg("t",timestamp))),
 pattern("batmtime", "decade", MTIMEtimestamp_decade_bulk, false, "", args(1,3, batarg("",int),batarg("t",timestamp),batarg("s",oid))),
 command("mtime", "year", MTIMEtimestamp_year, false, "", args(1,2, arg("",int),arg("t",timestamp))),
 pattern("batmtime", "year", MTIMEtimestamp_year_bulk, false, "", args(1,2, batarg("",int),batarg("t",timestamp))),
 pattern("batmtime", "year", MTIMEtimestamp_year_bulk, false, "", args(1,3, batarg("",int),batarg("t",timestamp),batarg("s",oid))),
 command("mtime", "quarter", MTIMEtimestamp_quarter, false, "", args(1,2, arg("",bte),arg("t",timestamp))),
 pattern("batmtime", "quarter", MTIMEtimestamp_quarter_bulk, false, "", args(1,2, batarg("",bte),batarg("t",timestamp))),
 pattern("batmtime", "quarter", MTIMEtimestamp_quarter_bulk, false, "", args(1,3, batarg("",bte),batarg("t",timestamp),batarg("s",oid))),
 command("mtime", "month", MTIMEtimestamp_month, false, "", args(1,2, arg("",bte),arg("t",timestamp))),
 pattern("batmtime", "month", MTIMEtimestamp_month_bulk, false, "", args(1,2, batarg("",bte),batarg("t",timestamp))),
 pattern("batmtime", "month", MTIMEtimestamp_month_bulk, false, "", args(1,3, batarg("",bte),batarg("t",timestamp),batarg("s",oid))),
 command("mtime", "day", MTIMEtimestamp_day, false, "", args(1,2, arg("",bte),arg("t",timestamp))),
 pattern("batmtime", "day", MTIMEtimestamp_day_bulk, false, "", args(1,2, batarg("",bte),batarg("t",timestamp))),
 pattern("batmtime", "day", MTIMEtimestamp_day_bulk, false, "", args(1,3, batarg("",bte),batarg("t",timestamp),batarg("s",oid))),
 command("mtime", "hours", MTIMEtimestamp_hours, false, "", args(1,2, arg("",bte),arg("t",timestamp))),
 pattern("batmtime", "hours", MTIMEtimestamp_hours_bulk, false, "", args(1,2, batarg("",bte),batarg("t",timestamp))),
 pattern("batmtime", "hours", MTIMEtimestamp_hours_bulk, false, "", args(1,3, batarg("",bte),batarg("t",timestamp),batarg("s",oid))),
 command("mtime", "minutes", MTIMEtimestamp_minutes, false, "", args(1,2, arg("",bte),arg("t",timestamp))),
 pattern("batmtime", "minutes", MTIMEtimestamp_minutes_bulk, false, "", args(1,2, batarg("",bte),batarg("t",timestamp))),
 pattern("batmtime", "minutes", MTIMEtimestamp_minutes_bulk, false, "", args(1,3, batarg("",bte),batarg("t",timestamp),batarg("s",oid))),
 command("mtime", "sql_seconds", MTIMEtimestamp_sql_seconds, false, "", args(1,2, arg("",int),arg("t",timestamp))),
 pattern("batmtime", "sql_seconds", MTIMEtimestamp_sql_seconds_bulk, false, "", args(1,2, batarg("",int),batarg("d",timestamp))),
 pattern("batmtime", "sql_seconds", MTIMEtimestamp_sql_seconds_bulk, false, "", args(1,3, batarg("",int),batarg("d",timestamp),batarg("s",oid))),
 command("mtime", "epoch_ms", MTIMEtimestamp_extract_epoch_ms, false, "", args(1,2, arg("",lng),arg("t",timestamp))),
 pattern("batmtime", "epoch_ms", MTIMEtimestamp_extract_epoch_ms_bulk, false, "", args(1,2, batarg("",lng),batarg("t",timestamp))),
 pattern("batmtime", "epoch_ms", MTIMEtimestamp_extract_epoch_ms_bulk, false, "", args(1,3, batarg("",lng),batarg("t",timestamp),batarg("s",oid))),
 command("mtime", "year", MTIMEsql_year, false, "", args(1,2, arg("",int),arg("months",int))),
 pattern("batmtime", "year", MTIMEsql_year_bulk, false, "", args(1,2, batarg("",int),batarg("months",int))),
 pattern("batmtime", "year", MTIMEsql_year_bulk, false, "", args(1,3, batarg("",int),batarg("months",int),batarg("s",oid))),
 command("mtime", "month", MTIMEsql_month, false, "", args(1,2, arg("",int),arg("months",int))),
 pattern("batmtime", "month", MTIMEsql_month_bulk, false, "", args(1,2, batarg("",int),batarg("months",int))),
 pattern("batmtime", "month", MTIMEsql_month_bulk, false, "", args(1,3, batarg("",int),batarg("months",int),batarg("s",oid))),
 command("mtime", "day", MTIMEsql_day, false, "", args(1,2, arg("",lng),arg("msecs",lng))),
 pattern("batmtime", "day", MTIMEsql_day_bulk, false, "", args(1,2, batarg("",lng),batarg("msecs",lng))),
 pattern("batmtime", "day", MTIMEsql_day_bulk, false, "", args(1,3, batarg("",lng),batarg("msecs",lng),batarg("s",oid))),
 command("mtime", "hours", MTIMEsql_hours, false, "", args(1,2, arg("",int),arg("msecs",lng))),
 pattern("batmtime", "hours", MTIMEsql_hours_bulk, false, "", args(1,2, batarg("",int),batarg("msecs",lng))),
 pattern("batmtime", "hours", MTIMEsql_hours_bulk, false, "", args(1,3, batarg("",int),batarg("msecs",lng),batarg("s",oid))),
 command("mtime", "minutes", MTIMEsql_minutes, false, "", args(1,2, arg("",int),arg("msecs",lng))),
 pattern("batmtime", "minutes", MTIMEsql_minutes_bulk, false, "", args(1,2, batarg("",int),batarg("msecs",lng))),
 pattern("batmtime", "minutes", MTIMEsql_minutes_bulk, false, "", args(1,3, batarg("",int),batarg("msecs",lng),batarg("s",oid))),
 command("mtime", "seconds", MTIMEsql_seconds, false, "", args(1,2, arg("",int),arg("msecs",lng))),
 pattern("batmtime", "seconds", MTIMEsql_seconds_bulk, false, "", args(1,2, batarg("",int),batarg("msecs",lng))),
 pattern("batmtime", "seconds", MTIMEsql_seconds_bulk, false, "", args(1,3, batarg("",int),batarg("msecs",lng),batarg("s",oid))),
 command("mtime", "epoch_ms", MTIMEmsec_extract_epoch_ms, false, "", args(1,2, arg("",lng),arg("msecs",lng))),
 pattern("batmtime", "epoch_ms", MTIMEmsec_extract_epoch_ms_bulk, false, "", args(1,2, batarg("",lng),batarg("msecs",lng))),
 pattern("batmtime", "epoch_ms", MTIMEmsec_extract_epoch_ms_bulk, false, "", args(1,3, batarg("",lng),batarg("msecs",lng),batarg("s",oid))),
 command("calc", "date", MTIMEdate_fromstr, false, "", args(1,2, arg("",date),arg("s",str))),
 command("calc", "date", MTIMEdate_date, false, "", args(1,2, arg("",date),arg("d",date))),
 command("calc", "date", MTIMEtimestamp_extract_date, false, "", args(1,2, arg("",date),arg("t",timestamp))),
 command("calc", "datetz", MTIMEtimestamp_tz_extract_date, false, "", args(1,3, arg("",date),arg("t",timestamp),arg("tz_msec",lng))),
 command("calc", "timestamp", MTIMEtimestamp_fromstr, false, "", args(1,2, arg("",timestamp),arg("s",str))),
 command("calc", "timestamp", MTIMEtimestamp_timestamp, false, "", args(1,2, arg("",timestamp),arg("t",timestamp))),
 command("calc", "timestamp", MTIMEtimestamp_fromdate, false, "", args(1,2, arg("",timestamp),arg("d",date))),
 command("calc", "timestamp", MTIMEtimestamp_fromsecond, false, "", args(1,2, arg("",timestamp),arg("secs",int))),
 command("calc", "timestamp", MTIMEtimestamp_frommsec, false, "", args(1,2, arg("",timestamp),arg("msecs",lng))),
 command("calc", "daytime", MTIMEdaytime_fromstr, false, "", args(1,2, arg("",daytime),arg("s",str))),
 command("calc", "daytime", MTIMEdaytime_daytime, false, "", args(1,2, arg("",daytime),arg("d",daytime))),
 command("calc", "daytime", MTIMEdaytime_fromseconds, false, "", args(1,2, arg("",daytime),arg("s",lng))),
 command("calc", "daytime", MTIMEtimestamp_extract_daytime, false, "", args(1,2, arg("",daytime),arg("t",timestamp))),
// pattern("batcalc", "date", MTIMEdate_fromstr_bulk, false, "", args(1,2, batarg("",date),batarg("s",str))),
 pattern("batcalc", "date", MTIMEdate_fromstr_bulk, false, "", args(1,3, batarg("",date),batarg("s",str),batarg("s",oid))),
// pattern("batcalc", "date", MTIMEdate_date_bulk, false, "", args(1,2, batarg("",date),batarg("d",date))),
 pattern("batcalc", "date", MTIMEdate_date_bulk, false, "", args(1,3, batarg("",date),batarg("d",date),batarg("s",oid))),
// pattern("batcalc", "date", MTIMEtimestamp_extract_date_bulk, false, "", args(1,2, batarg("",date),batarg("t",timestamp))),
 pattern("batcalc", "date", MTIMEtimestamp_extract_date_bulk, false, "", args(1,3, batarg("",date),batarg("t",timestamp),batarg("s",oid))),
 pattern("batcalc", "datetz", MTIMEtimestamp_tz_extract_date_bulk, false, "", args(1,3, batarg("",date),batarg("t",timestamp),batarg("tz_msec",lng))),
 pattern("batcalc", "datetz", MTIMEtimestamp_tz_extract_date_bulk, false, "", args(1,5, batarg("",date),batarg("t",timestamp),batarg("tz_msec",lng),batarg("s1",oid),batarg("s2",oid))),
 pattern("batcalc", "datetz", MTIMEtimestamp_tz_extract_date_bulk_p1, false, "", args(1,3, batarg("",date),arg("t",timestamp),batarg("tz_msec",lng))),
 pattern("batcalc", "datetz", MTIMEtimestamp_tz_extract_date_bulk_p1, false, "", args(1,4, batarg("",date),arg("t",timestamp),batarg("tz_msec",lng),batarg("s",oid))),
 pattern("batcalc", "datetz", MTIMEtimestamp_tz_extract_date_bulk_p2, false, "", args(1,3, batarg("",date),batarg("t",timestamp),arg("tz_msec",lng))),
 pattern("batcalc", "datetz", MTIMEtimestamp_tz_extract_date_bulk_p2, false, "", args(1,4, batarg("",date),batarg("t",timestamp),arg("tz_msec",lng),batarg("s",oid))),
// pattern("batcalc", "timestamp", MTIMEtimestamp_fromstr_bulk, false, "", args(1,2, batarg("",timestamp),batarg("s",str))),
 pattern("batcalc", "timestamp", MTIMEtimestamp_fromstr_bulk, false, "", args(1,3, batarg("",timestamp),batarg("s",str),batarg("s",oid))),
// pattern("batcalc", "timestamp", MTIMEtimestamp_timestamp_bulk, false, "", args(1,2, batarg("",timestamp),batarg("t",timestamp))),
 pattern("batcalc", "timestamp", MTIMEtimestamp_timestamp_bulk, false, "", args(1,3, batarg("",timestamp),batarg("t",timestamp),batarg("s",oid))),
// pattern("batcalc", "timestamp", MTIMEtimestamp_fromdate_bulk, false, "", args(1,2, batarg("",timestamp),batarg("d",date))),
 pattern("batcalc", "timestamp", MTIMEtimestamp_fromdate_bulk, false, "", args(1,3, batarg("",timestamp),batarg("d",date),batarg("s",oid))),
// pattern("batcalc", "timestamp", MTIMEtimestamp_fromsecond_bulk, false, "", args(1,2, batarg("",timestamp),batarg("secs",int))),
 pattern("batcalc", "timestamp", MTIMEtimestamp_fromsecond_bulk, false, "", args(1,3, batarg("",timestamp),batarg("secs",int),batarg("s",oid))),
// pattern("batcalc", "timestamp", MTIMEtimestamp_frommsec_bulk, false, "", args(1,2, batarg("",timestamp),batarg("msecs",lng))),
 pattern("batcalc", "timestamp", MTIMEtimestamp_frommsec_bulk, false, "", args(1,3, batarg("",timestamp),batarg("msecs",lng),batarg("s",oid))),
// pattern("batcalc", "daytime", MTIMEdaytime_fromstr_bulk, false, "", args(1,2, batarg("",daytime),batarg("s",str))),
 pattern("batcalc", "daytime", MTIMEdaytime_fromstr_bulk, false, "", args(1,3, batarg("",daytime),batarg("s",str),batarg("s",oid))),
// pattern("batcalc", "daytime", MTIMEdaytime_daytime_bulk, false, "", args(1,2, batarg("",daytime),batarg("d",daytime))),
 pattern("batcalc", "daytime", MTIMEdaytime_daytime_bulk, false, "", args(1,3, batarg("",daytime),batarg("d",daytime),batarg("s",oid))),
// pattern("batcalc", "daytime", MTIMEdaytime_fromseconds_bulk, false, "", args(1,2, batarg("",daytime),batarg("s",lng))),
 pattern("batcalc", "daytime", MTIMEdaytime_fromseconds_bulk, false, "", args(1,3, batarg("",daytime),batarg("s",lng),batarg("s",oid))),
// pattern("batcalc", "daytime", MTIMEtimestamp_extract_daytime_bulk, false, "", args(1,2, batarg("",daytime),batarg("t",timestamp))),
 pattern("batcalc", "daytime", MTIMEtimestamp_extract_daytime_bulk, false, "", args(1,3, batarg("",daytime),batarg("t",timestamp),batarg("s",oid))),
 // -- odbc timestampdiff variants
 command("mtime", "timestampdiff_sec", MTIMEtimestampdiff_sec, false, "diff in seconds between 'val1' and 'val2'.", args(1,3, arg("",lng),arg("val1",timestamp),arg("val2",timestamp))),
 pattern("batmtime", "timestampdiff_sec", MTIMEtimestampdiff_sec_bulk, false, "", args(1,3, batarg("",lng),batarg("val1",timestamp),batarg("val2",timestamp))),
 pattern("batmtime", "timestampdiff_sec", MTIMEtimestampdiff_sec_bulk_p1, false, "", args(1,3, batarg("",lng),arg("val1",timestamp),batarg("val2",timestamp))),
 pattern("batmtime", "timestampdiff_sec", MTIMEtimestampdiff_sec_bulk_p2, false, "", args(1,3, batarg("",lng),batarg("val1",timestamp),arg("val2",timestamp))),
 pattern("batmtime", "timestampdiff_sec", MTIMEtimestampdiff_sec_bulk, false, "", args(1,5, batarg("",lng),batarg("val1",timestamp),batarg("val2",timestamp),batarg("s1",oid),batarg("s2",oid))),
 pattern("batmtime", "timestampdiff_sec", MTIMEtimestampdiff_sec_bulk_p1, false, "", args(1,4, batarg("",lng),arg("val1",timestamp),batarg("val2",timestamp),batarg("s",oid))),
 pattern("batmtime", "timestampdiff_sec", MTIMEtimestampdiff_sec_bulk_p2, false, "", args(1,4, batarg("",lng),batarg("val1",timestamp),arg("val2",timestamp),batarg("s",oid))),
 // --
 command("mtime", "timestampdiff_sec", MTIMEtimestampdiff_sec_d_ts, false, "diff in seconds between 'val1' and 'val2'.", args(1,3, arg("",lng),arg("val1",date),arg("val2",timestamp))),
 pattern("batmtime", "timestampdiff_sec", MTIMEtimestampdiff_sec_d_ts_bulk, false, "", args(1,3, batarg("",lng),batarg("val1",date),batarg("val2",timestamp))),
 pattern("batmtime", "timestampdiff_sec", MTIMEtimestampdiff_sec_d_ts_bulk_p1, false, "", args(1,3, batarg("",lng),arg("val1",date),batarg("val2",timestamp))),
 pattern("batmtime", "timestampdiff_sec", MTIMEtimestampdiff_sec_d_ts_bulk_p2, false, "", args(1,3, batarg("",lng),batarg("val1",date),arg("val2",timestamp))),
 pattern("batmtime", "timestampdiff_sec", MTIMEtimestampdiff_sec_d_ts_bulk, false, "", args(1,5, batarg("",lng),batarg("val1",date),batarg("val2",timestamp),batarg("s1",oid),batarg("s2",oid))),
 pattern("batmtime", "timestampdiff_sec", MTIMEtimestampdiff_sec_d_ts_bulk_p1, false, "", args(1,4, batarg("",lng),arg("val1",date),batarg("val2",timestamp),batarg("s",oid))),
 pattern("batmtime", "timestampdiff_sec", MTIMEtimestampdiff_sec_d_ts_bulk_p2, false, "", args(1,4, batarg("",lng),batarg("val1",date),arg("val2",timestamp),batarg("s",oid))),
 // --
 command("mtime", "timestampdiff_sec", MTIMEtimestampdiff_sec_ts_d, false, "diff in seconds between 'val1' and 'val2'.", args(1,3, arg("",lng),arg("val1",timestamp),arg("val2",date))),
 pattern("batmtime", "timestampdiff_sec", MTIMEtimestampdiff_sec_ts_d_bulk, false, "", args(1,3, batarg("",lng),batarg("val1",timestamp),batarg("val2",date))),
 pattern("batmtime", "timestampdiff_sec", MTIMEtimestampdiff_sec_ts_d_bulk_p1, false, "", args(1,3, batarg("",lng),arg("val1",timestamp),batarg("val2",date))),
 pattern("batmtime", "timestampdiff_sec", MTIMEtimestampdiff_sec_ts_d_bulk_p2, false, "", args(1,3, batarg("",lng),batarg("val1",timestamp),arg("val2",date))),
 pattern("batmtime", "timestampdiff_sec", MTIMEtimestampdiff_sec_ts_d_bulk, false, "", args(1,5, batarg("",lng),batarg("val1",timestamp),batarg("val2",date),batarg("s1",oid),batarg("s2",oid))),
 pattern("batmtime", "timestampdiff_sec", MTIMEtimestampdiff_sec_ts_d_bulk_p1, false, "", args(1,4, batarg("",lng),arg("val1",timestamp),batarg("val2",date),batarg("s",oid))),
 pattern("batmtime", "timestampdiff_sec", MTIMEtimestampdiff_sec_ts_d_bulk_p2, false, "", args(1,4, batarg("",lng),batarg("val1",timestamp),arg("val2",date),batarg("s",oid))),
 // --
 command("mtime", "timestampdiff_min", MTIMEtimestampdiff_min, false, "diff in min between 'val1' and 'val2'.", args(1,3, arg("",lng),arg("val1",timestamp),arg("val2",timestamp))),
 pattern("batmtime", "timestampdiff_min", MTIMEtimestampdiff_min_bulk, false, "", args(1,3, batarg("",lng),batarg("val1",timestamp),batarg("val2",timestamp))),
 pattern("batmtime", "timestampdiff_min", MTIMEtimestampdiff_min_bulk_p1, false, "", args(1,3, batarg("",lng),arg("val1",timestamp),batarg("val2",timestamp))),
 pattern("batmtime", "timestampdiff_min", MTIMEtimestampdiff_min_bulk_p2, false, "", args(1,3, batarg("",lng),batarg("val1",timestamp),arg("val2",timestamp))),
 pattern("batmtime", "timestampdiff_min", MTIMEtimestampdiff_min_bulk, false, "", args(1,5, batarg("",lng),batarg("val1",timestamp),batarg("val2",timestamp),batarg("s1",oid),batarg("s2",oid))),
 pattern("batmtime", "timestampdiff_min", MTIMEtimestampdiff_min_bulk_p1, false, "", args(1,4, batarg("",lng),arg("val1",timestamp),batarg("val2",timestamp),batarg("s",oid))),
 pattern("batmtime", "timestampdiff_min", MTIMEtimestampdiff_min_bulk_p2, false, "", args(1,4, batarg("",lng),batarg("val1",timestamp),arg("val2",timestamp),batarg("s",oid))),
 // --
 command("mtime", "timestampdiff_min", MTIMEtimestampdiff_min_d_ts, false, "diff in min between 'val1' and 'val2'.", args(1,3, arg("",lng),arg("val1",date),arg("val2",timestamp))),
 pattern("batmtime", "timestampdiff_min", MTIMEtimestampdiff_min_d_ts_bulk, false, "", args(1,3, batarg("",lng),batarg("val1",date),batarg("val2",timestamp))),
 pattern("batmtime", "timestampdiff_min", MTIMEtimestampdiff_min_d_ts_bulk_p1, false, "", args(1,3, batarg("",lng),arg("val1",date),batarg("val2",timestamp))),
 pattern("batmtime", "timestampdiff_min", MTIMEtimestampdiff_min_d_ts_bulk_p2, false, "", args(1,3, batarg("",lng),batarg("val1",date),arg("val2",timestamp))),
 pattern("batmtime", "timestampdiff_min", MTIMEtimestampdiff_min_d_ts_bulk, false, "", args(1,5, batarg("",lng),batarg("val1",date),batarg("val2",timestamp),batarg("s1",oid),batarg("s2",oid))),
 pattern("batmtime", "timestampdiff_min", MTIMEtimestampdiff_min_d_ts_bulk_p1, false, "", args(1,4, batarg("",lng),arg("val1",date),batarg("val2",timestamp),batarg("s",oid))),
 pattern("batmtime", "timestampdiff_min", MTIMEtimestampdiff_min_d_ts_bulk_p2, false, "", args(1,4, batarg("",lng),batarg("val1",date),arg("val2",timestamp),batarg("s",oid))),
 // --
 command("mtime", "timestampdiff_min", MTIMEtimestampdiff_min_ts_d, false, "diff in min between 'val1' and 'val2'.", args(1,3, arg("",lng),arg("val1",timestamp),arg("val2",date))),
 pattern("batmtime", "timestampdiff_min", MTIMEtimestampdiff_min_ts_d_bulk, false, "", args(1,3, batarg("",lng),batarg("val1",timestamp),batarg("val2",date))),
 pattern("batmtime", "timestampdiff_min", MTIMEtimestampdiff_min_ts_d_bulk_p1, false, "", args(1,3, batarg("",lng),arg("val1",timestamp),batarg("val2",date))),
 pattern("batmtime", "timestampdiff_min", MTIMEtimestampdiff_min_ts_d_bulk_p2, false, "", args(1,3, batarg("",lng),batarg("val1",timestamp),arg("val2",date))),
 pattern("batmtime", "timestampdiff_min", MTIMEtimestampdiff_min_ts_d_bulk, false, "", args(1,5, batarg("",lng),batarg("val1",timestamp),batarg("val2",date),batarg("s1",oid),batarg("s2",oid))),
 pattern("batmtime", "timestampdiff_min", MTIMEtimestampdiff_min_ts_d_bulk_p1, false, "", args(1,4, batarg("",lng),arg("val1",timestamp),batarg("val2",date),batarg("s",oid))),
 pattern("batmtime", "timestampdiff_min", MTIMEtimestampdiff_min_ts_d_bulk_p2, false, "", args(1,4, batarg("",lng),batarg("val1",timestamp),arg("val2",date),batarg("s",oid))),
 // --
 command("mtime", "timestampdiff_hour", MTIMEtimestampdiff_hour, false, "diff in hours between 'val1' and 'val2'.", args(1,3, arg("",lng),arg("val1",timestamp),arg("val2",timestamp))),
 pattern("batmtime", "timestampdiff_hour", MTIMEtimestampdiff_hour_bulk, false, "", args(1,3, batarg("",lng),batarg("val1",timestamp),batarg("val2",timestamp))),
 pattern("batmtime", "timestampdiff_hour", MTIMEtimestampdiff_hour_bulk_p1, false, "", args(1,3, batarg("",lng),arg("val1",timestamp),batarg("val2",timestamp))),
 pattern("batmtime", "timestampdiff_hour", MTIMEtimestampdiff_hour_bulk_p2, false, "", args(1,3, batarg("",lng),batarg("val1",timestamp),arg("val2",timestamp))),
 pattern("batmtime", "timestampdiff_hour", MTIMEtimestampdiff_hour_bulk, false, "", args(1,5, batarg("",lng),batarg("val1",timestamp),batarg("val2",timestamp),batarg("s1",oid),batarg("s2",oid))),
 pattern("batmtime", "timestampdiff_hour", MTIMEtimestampdiff_hour_bulk_p1, false, "", args(1,4, batarg("",lng),arg("val1",timestamp),batarg("val2",timestamp),batarg("s",oid))),
 pattern("batmtime", "timestampdiff_hour", MTIMEtimestampdiff_hour_bulk_p2, false, "", args(1,4, batarg("",lng),batarg("val1",timestamp),arg("val2",timestamp),batarg("s",oid))),
 // --
 command("mtime", "timestampdiff_hour", MTIMEtimestampdiff_hour_d_ts, false, "diff in hours between 'val1' and 'val2'.", args(1,3, arg("",lng),arg("val1",date),arg("val2",timestamp))),
 pattern("batmtime", "timestampdiff_hour", MTIMEtimestampdiff_hour_d_ts_bulk, false, "", args(1,3, batarg("",lng),batarg("val1",date),batarg("val2",timestamp))),
 pattern("batmtime", "timestampdiff_hour", MTIMEtimestampdiff_hour_d_ts_bulk_p1, false, "", args(1,3, batarg("",lng),arg("val1",date),batarg("val2",timestamp))),
 pattern("batmtime", "timestampdiff_hour", MTIMEtimestampdiff_hour_d_ts_bulk_p2, false, "", args(1,3, batarg("",lng),batarg("val1",date),arg("val2",timestamp))),
 pattern("batmtime", "timestampdiff_hour", MTIMEtimestampdiff_hour_d_ts_bulk, false, "", args(1,5, batarg("",lng),batarg("val1",date),batarg("val2",timestamp),batarg("s1",oid),batarg("s2",oid))),
 pattern("batmtime", "timestampdiff_hour", MTIMEtimestampdiff_hour_d_ts_bulk_p1, false, "", args(1,4, batarg("",lng),arg("val1",date),batarg("val2",timestamp),batarg("s",oid))),
 pattern("batmtime", "timestampdiff_hour", MTIMEtimestampdiff_hour_d_ts_bulk_p2, false, "", args(1,4, batarg("",lng),batarg("val1",date),arg("val2",timestamp),batarg("s",oid))),
 // --
 command("mtime", "timestampdiff_hour", MTIMEtimestampdiff_hour_ts_d, false, "diff in hours between 'val1' and 'val2'.", args(1,3, arg("",lng),arg("val1",timestamp),arg("val2",date))),
 pattern("batmtime", "timestampdiff_hour", MTIMEtimestampdiff_hour_ts_d_bulk, false, "", args(1,3, batarg("",lng),batarg("val1",timestamp),batarg("val2",date))),
 pattern("batmtime", "timestampdiff_hour", MTIMEtimestampdiff_hour_ts_d_bulk_p1, false, "", args(1,3, batarg("",lng),arg("val1",timestamp),batarg("val2",date))),
 pattern("batmtime", "timestampdiff_hour", MTIMEtimestampdiff_hour_ts_d_bulk_p2, false, "", args(1,3, batarg("",lng),batarg("val1",timestamp),arg("val2",date))),
 pattern("batmtime", "timestampdiff_hour", MTIMEtimestampdiff_hour_ts_d_bulk, false, "", args(1,5, batarg("",lng),batarg("val1",timestamp),batarg("val2",date),batarg("s1",oid),batarg("s2",oid))),
 pattern("batmtime", "timestampdiff_hour", MTIMEtimestampdiff_hour_ts_d_bulk_p1, false, "", args(1,4, batarg("",lng),arg("val1",timestamp),batarg("val2",date),batarg("s",oid))),
 pattern("batmtime", "timestampdiff_hour", MTIMEtimestampdiff_hour_ts_d_bulk_p2, false, "", args(1,4, batarg("",lng),batarg("val1",timestamp),arg("val2",date),batarg("s",oid))),
 // --
 command("mtime", "timestampdiff_day", MTIMEtimestampdiff_day, false, "diff in days between 'val1' and 'val2'.", args(1,3, arg("",int),arg("val1",timestamp),arg("val2",timestamp))),
 pattern("batmtime", "timestampdiff_day", MTIMEtimestampdiff_day_bulk, false, "", args(1,3, batarg("",int),batarg("val1",timestamp),batarg("val2",timestamp))),
 pattern("batmtime", "timestampdiff_day", MTIMEtimestampdiff_day_bulk_p1, false, "", args(1,3, batarg("",int),arg("val1",timestamp),batarg("val2",timestamp))),
 pattern("batmtime", "timestampdiff_day", MTIMEtimestampdiff_day_bulk_p2, false, "", args(1,3, batarg("",int),batarg("val1",timestamp),arg("val2",timestamp))),
 pattern("batmtime", "timestampdiff_day", MTIMEtimestampdiff_day_bulk, false, "", args(1,5, batarg("",int),batarg("val1",timestamp),batarg("val2",timestamp),batarg("s1",oid),batarg("s2",oid))),
 pattern("batmtime", "timestampdiff_day", MTIMEtimestampdiff_day_bulk_p1, false, "", args(1,4, batarg("",int),arg("val1",timestamp),batarg("val2",timestamp),batarg("s",oid))),
 pattern("batmtime", "timestampdiff_day", MTIMEtimestampdiff_day_bulk_p2, false, "", args(1,4, batarg("",int),batarg("val1",timestamp),arg("val2",timestamp),batarg("s",oid))),
 // --
 command("mtime", "timestampdiff_day", MTIMEtimestampdiff_day_t_ts, false, "diff in days between 'val1' and 'val2'.", args(1,3, arg("",int),arg("val1",daytime),arg("val2",timestamp))),
 pattern("batmtime", "timestampdiff_day", MTIMEtimestampdiff_day_t_ts_bulk, false, "", args(1,3, batarg("",int),batarg("val1",daytime),batarg("val2",timestamp))),
 pattern("batmtime", "timestampdiff_day", MTIMEtimestampdiff_day_t_ts_bulk_p1, false, "", args(1,3, batarg("",int),arg("val1",daytime),batarg("val2",timestamp))),
 pattern("batmtime", "timestampdiff_day", MTIMEtimestampdiff_day_t_ts_bulk_p2, false, "", args(1,3, batarg("",int),batarg("val1",daytime),arg("val2",timestamp))),
 pattern("batmtime", "timestampdiff_day", MTIMEtimestampdiff_day_t_ts_bulk, false, "", args(1,5, batarg("",int),batarg("val1",daytime),batarg("val2",timestamp),batarg("s1",oid),batarg("s2",oid))),
 pattern("batmtime", "timestampdiff_day", MTIMEtimestampdiff_day_t_ts_bulk_p1, false, "", args(1,4, batarg("",int),arg("val1",daytime),batarg("val2",timestamp),batarg("s",oid))),
 pattern("batmtime", "timestampdiff_day", MTIMEtimestampdiff_day_t_ts_bulk_p2, false, "", args(1,4, batarg("",int),batarg("val1",daytime),arg("val2",timestamp),batarg("s",oid))),
 // --
 command("mtime", "timestampdiff_day", MTIMEtimestampdiff_day_ts_t, false, "diff in days between 'val1' and 'val2'.", args(1,3, arg("",int),arg("val1",timestamp),arg("val2",daytime))),
 pattern("batmtime", "timestampdiff_day", MTIMEtimestampdiff_day_ts_t_bulk, false, "", args(1,3, batarg("",int),batarg("val1",timestamp),batarg("val2",daytime))),
 pattern("batmtime", "timestampdiff_day", MTIMEtimestampdiff_day_ts_t_bulk_p1, false, "", args(1,3, batarg("",int),arg("val1",timestamp),batarg("val2",daytime))),
 pattern("batmtime", "timestampdiff_day", MTIMEtimestampdiff_day_ts_t_bulk_p2, false, "", args(1,3, batarg("",int),batarg("val1",timestamp),arg("val2",daytime))),
 pattern("batmtime", "timestampdiff_day", MTIMEtimestampdiff_day_ts_t_bulk, false, "", args(1,5, batarg("",int),batarg("val1",timestamp),batarg("val2",daytime),batarg("s1",oid),batarg("s2",oid))),
 pattern("batmtime", "timestampdiff_day", MTIMEtimestampdiff_day_ts_t_bulk_p1, false, "", args(1,4, batarg("",int),arg("val1",timestamp),batarg("val2",daytime),batarg("s",oid))),
 pattern("batmtime", "timestampdiff_day", MTIMEtimestampdiff_day_ts_t_bulk_p2, false, "", args(1,4, batarg("",int),batarg("val1",timestamp),arg("val2",daytime),batarg("s",oid))),
 // --
 command("mtime", "timestampdiff_week", MTIMEtimestampdiff_week, false, "diff in weeks between 'val1' and 'val2'.", args(1,3, arg("",int),arg("val1",timestamp),arg("val2",timestamp))),
 pattern("batmtime", "timestampdiff_week", MTIMEtimestampdiff_week_bulk, false, "", args(1,3, batarg("",int),batarg("val1",timestamp),batarg("val2",timestamp))),
 pattern("batmtime", "timestampdiff_week", MTIMEtimestampdiff_week_bulk_p1, false, "", args(1,3, batarg("",int),arg("val1",timestamp),batarg("val2",timestamp))),
 pattern("batmtime", "timestampdiff_week", MTIMEtimestampdiff_week_bulk_p2, false, "", args(1,3, batarg("",int),batarg("val1",timestamp),arg("val2",timestamp))),
 pattern("batmtime", "timestampdiff_week", MTIMEtimestampdiff_week_bulk, false, "", args(1,5, batarg("",int),batarg("val1",timestamp),batarg("val2",timestamp),batarg("s1",oid),batarg("s2",oid))),
 pattern("batmtime", "timestampdiff_week", MTIMEtimestampdiff_week_bulk_p1, false, "", args(1,4, batarg("",int),arg("val1",timestamp),batarg("val2",timestamp),batarg("s",oid))),
 pattern("batmtime", "timestampdiff_week", MTIMEtimestampdiff_week_bulk_p2, false, "", args(1,4, batarg("",int),batarg("val1",timestamp),arg("val2",timestamp),batarg("s",oid))),
 // --
 command("mtime", "timestampdiff_week", MTIMEtimestampdiff_week_t_ts, false, "diff in weeks between 'val1' and 'val2'.", args(1,3, arg("",int),arg("val1",daytime),arg("val2",timestamp))),
 pattern("batmtime", "timestampdiff_week", MTIMEtimestampdiff_week_t_ts_bulk, false, "", args(1,3, batarg("",int),batarg("val1",daytime),batarg("val2",timestamp))),
 pattern("batmtime", "timestampdiff_week", MTIMEtimestampdiff_week_t_ts_bulk_p1, false, "", args(1,3, batarg("",int),arg("val1",daytime),batarg("val2",timestamp))),
 pattern("batmtime", "timestampdiff_week", MTIMEtimestampdiff_week_t_ts_bulk_p2, false, "", args(1,3, batarg("",int),batarg("val1",daytime),arg("val2",timestamp))),
 pattern("batmtime", "timestampdiff_week", MTIMEtimestampdiff_week_t_ts_bulk, false, "", args(1,5, batarg("",int),batarg("val1",daytime),batarg("val2",timestamp),batarg("s1",oid),batarg("s2",oid))),
 pattern("batmtime", "timestampdiff_week", MTIMEtimestampdiff_week_t_ts_bulk_p1, false, "", args(1,4, batarg("",int),arg("val1",daytime),batarg("val2",timestamp),batarg("s",oid))),
 pattern("batmtime", "timestampdiff_week", MTIMEtimestampdiff_week_t_ts_bulk_p2, false, "", args(1,4, batarg("",int),batarg("val1",daytime),arg("val2",timestamp),batarg("s",oid))),
 // --
 command("mtime", "timestampdiff_week", MTIMEtimestampdiff_week_ts_t, false, "diff in weeks between 'val1' and 'val2'.", args(1,3, arg("",int),arg("val1",timestamp),arg("val2",daytime))),
 pattern("batmtime", "timestampdiff_week", MTIMEtimestampdiff_week_ts_t_bulk, false, "", args(1,3, batarg("",int),batarg("val1",timestamp),batarg("val2",daytime))),
 pattern("batmtime", "timestampdiff_week", MTIMEtimestampdiff_week_ts_t_bulk_p1, false, "", args(1,3, batarg("",int),arg("val1",timestamp),batarg("val2",daytime))),
 pattern("batmtime", "timestampdiff_week", MTIMEtimestampdiff_week_ts_t_bulk_p2, false, "", args(1,3, batarg("",int),batarg("val1",timestamp),arg("val2",daytime))),
 pattern("batmtime", "timestampdiff_week", MTIMEtimestampdiff_week_ts_t_bulk, false, "", args(1,5, batarg("",int),batarg("val1",timestamp),batarg("val2",daytime),batarg("s1",oid),batarg("s2",oid))),
 pattern("batmtime", "timestampdiff_week", MTIMEtimestampdiff_week_ts_t_bulk_p1, false, "", args(1,4, batarg("",int),arg("val1",timestamp),batarg("val2",daytime),batarg("s",oid))),
 pattern("batmtime", "timestampdiff_week", MTIMEtimestampdiff_week_ts_t_bulk_p2, false, "", args(1,4, batarg("",int),batarg("val1",timestamp),arg("val2",daytime),batarg("s",oid))),
 // --
 command("mtime", "timestampdiff_month", MTIMEtimestampdiff_month, false, "diff in months between 'val1' and 'val2'.", args(1,3, arg("",int),arg("val1",timestamp),arg("val2",timestamp))),
 pattern("batmtime", "timestampdiff_month", MTIMEtimestampdiff_month_bulk, false, "", args(1,3, batarg("",int),batarg("val1",timestamp),batarg("val2",timestamp))),
 pattern("batmtime", "timestampdiff_month", MTIMEtimestampdiff_month_bulk_p1, false, "", args(1,3, batarg("",int),arg("val1",timestamp),batarg("val2",timestamp))),
 pattern("batmtime", "timestampdiff_month", MTIMEtimestampdiff_month_bulk_p2, false, "", args(1,3, batarg("",int),batarg("val1",timestamp),arg("val2",timestamp))),
 pattern("batmtime", "timestampdiff_month", MTIMEtimestampdiff_month_bulk, false, "", args(1,5, batarg("",int),batarg("val1",timestamp),batarg("val2",timestamp),batarg("s1",oid),batarg("s2",oid))),
 pattern("batmtime", "timestampdiff_month", MTIMEtimestampdiff_month_bulk_p1, false, "", args(1,4, batarg("",int),arg("val1",timestamp),batarg("val2",timestamp),batarg("s",oid))),
 pattern("batmtime", "timestampdiff_month", MTIMEtimestampdiff_month_bulk_p2, false, "", args(1,4, batarg("",int),batarg("val1",timestamp),arg("val2",timestamp),batarg("s",oid))),
 // --
 command("mtime", "timestampdiff_month", MTIMEtimestampdiff_month_t_ts, false, "diff in months between 'val1' and 'val2'.", args(1,3, arg("",int),arg("val1",daytime),arg("val2",timestamp))),
 pattern("batmtime", "timestampdiff_month", MTIMEtimestampdiff_month_t_ts_bulk, false, "", args(1,3, batarg("",int),batarg("val1",daytime),batarg("val2",timestamp))),
 pattern("batmtime", "timestampdiff_month", MTIMEtimestampdiff_month_t_ts_bulk_p1, false, "", args(1,3, batarg("",int),arg("val1",daytime),batarg("val2",timestamp))),
 pattern("batmtime", "timestampdiff_month", MTIMEtimestampdiff_month_t_ts_bulk_p2, false, "", args(1,3, batarg("",int),batarg("val1",daytime),arg("val2",timestamp))),
 pattern("batmtime", "timestampdiff_month", MTIMEtimestampdiff_month_t_ts_bulk, false, "", args(1,5, batarg("",int),batarg("val1",daytime),batarg("val2",timestamp),batarg("s1",oid),batarg("s2",oid))),
 pattern("batmtime", "timestampdiff_month", MTIMEtimestampdiff_month_t_ts_bulk_p1, false, "", args(1,4, batarg("",int),arg("val1",daytime),batarg("val2",timestamp),batarg("s",oid))),
 pattern("batmtime", "timestampdiff_month", MTIMEtimestampdiff_month_t_ts_bulk_p2, false, "", args(1,4, batarg("",int),batarg("val1",daytime),arg("val2",timestamp),batarg("s",oid))),
 // --
 command("mtime", "timestampdiff_month", MTIMEtimestampdiff_month_ts_t, false, "diff in months between 'val1' and 'val2'.", args(1,3, arg("",int),arg("val1",timestamp),arg("val2",daytime))),
 pattern("batmtime", "timestampdiff_month", MTIMEtimestampdiff_month_ts_t_bulk, false, "", args(1,3, batarg("",int),batarg("val1",timestamp),batarg("val2",daytime))),
 pattern("batmtime", "timestampdiff_month", MTIMEtimestampdiff_month_ts_t_bulk_p1, false, "", args(1,3, batarg("",int),arg("val1",timestamp),batarg("val2",daytime))),
 pattern("batmtime", "timestampdiff_month", MTIMEtimestampdiff_month_ts_t_bulk_p2, false, "", args(1,3, batarg("",int),batarg("val1",timestamp),arg("val2",daytime))),
 pattern("batmtime", "timestampdiff_month", MTIMEtimestampdiff_month_ts_t_bulk, false, "", args(1,5, batarg("",int),batarg("val1",timestamp),batarg("val2",daytime),batarg("s1",oid),batarg("s2",oid))),
 pattern("batmtime", "timestampdiff_month", MTIMEtimestampdiff_month_ts_t_bulk_p1, false, "", args(1,4, batarg("",int),arg("val1",timestamp),batarg("val2",daytime),batarg("s",oid))),
 pattern("batmtime", "timestampdiff_month", MTIMEtimestampdiff_month_ts_t_bulk_p2, false, "", args(1,4, batarg("",int),batarg("val1",timestamp),arg("val2",daytime),batarg("s",oid))),
 // --
 command("mtime", "timestampdiff_quarter", MTIMEtimestampdiff_quarter, false, "diff in months between 'val1' and 'val2'.", args(1,3, arg("",int),arg("val1",timestamp),arg("val2",timestamp))),
 pattern("batmtime", "timestampdiff_quarter", MTIMEtimestampdiff_quarter_bulk, false, "", args(1,3, batarg("",int),batarg("val1",timestamp),batarg("val2",timestamp))),
 pattern("batmtime", "timestampdiff_quarter", MTIMEtimestampdiff_quarter_bulk_p1, false, "", args(1,3, batarg("",int),arg("val1",timestamp),batarg("val2",timestamp))),
 pattern("batmtime", "timestampdiff_quarter", MTIMEtimestampdiff_quarter_bulk_p2, false, "", args(1,3, batarg("",int),batarg("val1",timestamp),arg("val2",timestamp))),
 pattern("batmtime", "timestampdiff_quarter", MTIMEtimestampdiff_quarter_bulk, false, "", args(1,5, batarg("",int),batarg("val1",timestamp),batarg("val2",timestamp),batarg("s1",oid),batarg("s2",oid))),
 pattern("batmtime", "timestampdiff_quarter", MTIMEtimestampdiff_quarter_bulk_p1, false, "", args(1,4, batarg("",int),arg("val1",timestamp),batarg("val2",timestamp),batarg("s",oid))),
 pattern("batmtime", "timestampdiff_quarter", MTIMEtimestampdiff_quarter_bulk_p2, false, "", args(1,4, batarg("",int),batarg("val1",timestamp),arg("val2",timestamp),batarg("s",oid))),
 // --
 command("mtime", "timestampdiff_quarter", MTIMEtimestampdiff_quarter_t_ts, false, "diff in months between 'val1' and 'val2'.", args(1,3, arg("",int),arg("val1",daytime),arg("val2",timestamp))),
 pattern("batmtime", "timestampdiff_quarter", MTIMEtimestampdiff_quarter_t_ts_bulk, false, "", args(1,3, batarg("",int),batarg("val1",daytime),batarg("val2",timestamp))),
 pattern("batmtime", "timestampdiff_quarter", MTIMEtimestampdiff_quarter_t_ts_bulk_p1, false, "", args(1,3, batarg("",int),arg("val1",daytime),batarg("val2",timestamp))),
 pattern("batmtime", "timestampdiff_quarter", MTIMEtimestampdiff_quarter_t_ts_bulk_p2, false, "", args(1,3, batarg("",int),batarg("val1",daytime),arg("val2",timestamp))),
 pattern("batmtime", "timestampdiff_quarter", MTIMEtimestampdiff_quarter_t_ts_bulk, false, "", args(1,5, batarg("",int),batarg("val1",daytime),batarg("val2",timestamp),batarg("s1",oid),batarg("s2",oid))),
 pattern("batmtime", "timestampdiff_quarter", MTIMEtimestampdiff_quarter_t_ts_bulk_p1, false, "", args(1,4, batarg("",int),arg("val1",daytime),batarg("val2",timestamp),batarg("s",oid))),
 pattern("batmtime", "timestampdiff_quarter", MTIMEtimestampdiff_quarter_t_ts_bulk_p2, false, "", args(1,4, batarg("",int),batarg("val1",daytime),arg("val2",timestamp),batarg("s",oid))),
 // --
 command("mtime", "timestampdiff_quarter", MTIMEtimestampdiff_quarter_ts_t, false, "diff in months between 'val1' and 'val2'.", args(1,3, arg("",int),arg("val1",timestamp),arg("val2",daytime))),
 pattern("batmtime", "timestampdiff_quarter", MTIMEtimestampdiff_quarter_ts_t_bulk, false, "", args(1,3, batarg("",int),batarg("val1",timestamp),batarg("val2",daytime))),
 pattern("batmtime", "timestampdiff_quarter", MTIMEtimestampdiff_quarter_ts_t_bulk_p1, false, "", args(1,3, batarg("",int),arg("val1",timestamp),batarg("val2",daytime))),
 pattern("batmtime", "timestampdiff_quarter", MTIMEtimestampdiff_quarter_ts_t_bulk_p2, false, "", args(1,3, batarg("",int),batarg("val1",timestamp),arg("val2",daytime))),
 pattern("batmtime", "timestampdiff_quarter", MTIMEtimestampdiff_quarter_ts_t_bulk, false, "", args(1,5, batarg("",int),batarg("val1",timestamp),batarg("val2",daytime),batarg("s1",oid),batarg("s2",oid))),
 pattern("batmtime", "timestampdiff_quarter", MTIMEtimestampdiff_quarter_ts_t_bulk_p1, false, "", args(1,4, batarg("",int),arg("val1",timestamp),batarg("val2",daytime),batarg("s",oid))),
 pattern("batmtime", "timestampdiff_quarter", MTIMEtimestampdiff_quarter_ts_t_bulk_p2, false, "", args(1,4, batarg("",int),batarg("val1",timestamp),arg("val2",daytime),batarg("s",oid))),
 // --
 command("mtime", "timestampdiff_year", MTIMEtimestampdiff_year, false, "diff in months between 'val1' and 'val2'.", args(1,3, arg("",int),arg("val1",timestamp),arg("val2",timestamp))),
 pattern("batmtime", "timestampdiff_year", MTIMEtimestampdiff_year_bulk, false, "", args(1,3, batarg("",int),batarg("val1",timestamp),batarg("val2",timestamp))),
 pattern("batmtime", "timestampdiff_year", MTIMEtimestampdiff_year_bulk_p1, false, "", args(1,3, batarg("",int),arg("val1",timestamp),batarg("val2",timestamp))),
 pattern("batmtime", "timestampdiff_year", MTIMEtimestampdiff_year_bulk_p2, false, "", args(1,3, batarg("",int),batarg("val1",timestamp),arg("val2",timestamp))),
 pattern("batmtime", "timestampdiff_year", MTIMEtimestampdiff_year_bulk, false, "", args(1,5, batarg("",int),batarg("val1",timestamp),batarg("val2",timestamp),batarg("s1",oid),batarg("s2",oid))),
 pattern("batmtime", "timestampdiff_year", MTIMEtimestampdiff_year_bulk_p1, false, "", args(1,4, batarg("",int),arg("val1",timestamp),batarg("val2",timestamp),batarg("s",oid))),
 pattern("batmtime", "timestampdiff_year", MTIMEtimestampdiff_year_bulk_p2, false, "", args(1,4, batarg("",int),batarg("val1",timestamp),arg("val2",timestamp),batarg("s",oid))),
 // --
 command("mtime", "timestampdiff_year", MTIMEtimestampdiff_year_t_ts, false, "diff in months between 'val1' and 'val2'.", args(1,3, arg("",int),arg("val1",daytime),arg("val2",timestamp))),
 pattern("batmtime", "timestampdiff_year", MTIMEtimestampdiff_year_t_ts_bulk, false, "", args(1,3, batarg("",int),batarg("val1",daytime),batarg("val2",timestamp))),
 pattern("batmtime", "timestampdiff_year", MTIMEtimestampdiff_year_t_ts_bulk_p1, false, "", args(1,3, batarg("",int),arg("val1",daytime),batarg("val2",timestamp))),
 pattern("batmtime", "timestampdiff_year", MTIMEtimestampdiff_year_t_ts_bulk_p2, false, "", args(1,3, batarg("",int),batarg("val1",daytime),arg("val2",timestamp))),
 pattern("batmtime", "timestampdiff_year", MTIMEtimestampdiff_year_t_ts_bulk, false, "", args(1,5, batarg("",int),batarg("val1",daytime),batarg("val2",timestamp),batarg("s1",oid),batarg("s2",oid))),
 pattern("batmtime", "timestampdiff_year", MTIMEtimestampdiff_year_t_ts_bulk_p1, false, "", args(1,4, batarg("",int),arg("val1",daytime),batarg("val2",timestamp),batarg("s",oid))),
 pattern("batmtime", "timestampdiff_year", MTIMEtimestampdiff_year_t_ts_bulk_p2, false, "", args(1,4, batarg("",int),batarg("val1",daytime),arg("val2",timestamp),batarg("s",oid))),
 // --
 command("mtime", "timestampdiff_year", MTIMEtimestampdiff_year_ts_t, false, "diff in months between 'val1' and 'val2'.", args(1,3, arg("",int),arg("val1",timestamp),arg("val2",daytime))),
 pattern("batmtime", "timestampdiff_year", MTIMEtimestampdiff_year_ts_t_bulk, false, "", args(1,3, batarg("",int),batarg("val1",timestamp),batarg("val2",daytime))),
 pattern("batmtime", "timestampdiff_year", MTIMEtimestampdiff_year_ts_t_bulk_p1, false, "", args(1,3, batarg("",int),arg("val1",timestamp),batarg("val2",daytime))),
 pattern("batmtime", "timestampdiff_year", MTIMEtimestampdiff_year_ts_t_bulk_p2, false, "", args(1,3, batarg("",int),batarg("val1",timestamp),arg("val2",daytime))),
 pattern("batmtime", "timestampdiff_year", MTIMEtimestampdiff_year_ts_t_bulk, false, "", args(1,5, batarg("",int),batarg("val1",timestamp),batarg("val2",daytime),batarg("s1",oid),batarg("s2",oid))),
 pattern("batmtime", "timestampdiff_year", MTIMEtimestampdiff_year_ts_t_bulk_p1, false, "", args(1,4, batarg("",int),arg("val1",timestamp),batarg("val2",daytime),batarg("s",oid))),
 pattern("batmtime", "timestampdiff_year", MTIMEtimestampdiff_year_ts_t_bulk_p2, false, "", args(1,4, batarg("",int),batarg("val1",timestamp),arg("val2",daytime),batarg("s",oid))),
 { .imp=NULL }
};
#include "mal_import.h"
#ifdef _MSC_VER
#undef read
#pragma section(".CRT$XCU",read)
#endif
LIB_STARTUP_FUNC(init_mtime_mal)
{ mal_module("mtime", NULL, mtime_init_funcs); }
