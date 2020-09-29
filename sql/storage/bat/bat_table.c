/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0.  If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright 1997 - July 2008 CWI, August 2008 - 2020 MonetDB B.V.
 */

#include "monetdb_config.h"
#include "bat_table.h"
#include "bat_utils.h"
#include "bat_storage.h"

static BAT *
_delta_cands(sql_trans *tr, sql_table *t)
{
	sql_column *c = t->columns.set->h->data;
	/* create void,void bat with length and oid's set */
	size_t nr = store_funcs.count_col(tr, c, 0), dcnt = 0;
	BAT *tids = BATdense(0, 0, (BUN) nr);

	if (!tids)
		return NULL;

	if ((dcnt=store_funcs.count_del(tr, t, 0)) > 0 || store_funcs.count_del(tr, t, 2) > 0) {
		BAT *d;

		if ((d = store_funcs.bind_del(tr, t, RDONLY)) != NULL) {
			if (!d)
				return NULL;
			if (store_funcs.count_del(tr, t, 2) > 0) {
				BAT *nd = COLcopy(d, d->ttype, true, TRANSIENT);
				BAT *ui = store_funcs.bind_del(tr, t, RD_UPD_ID);
				BAT *uv = store_funcs.bind_del(tr, t, RD_UPD_VAL);

				BBPunfix(d->batCacheid);
	    		if (!nd || !ui || !uv || BATreplace(nd, ui, uv, true) != GDK_SUCCEED) {
					if (nd) BBPunfix(nd->batCacheid);
					if (ui) BBPunfix(ui->batCacheid);
					if (uv) BBPunfix(uv->batCacheid);
					return NULL;
				}
				BBPunfix(ui->batCacheid);
				BBPunfix(uv->batCacheid);
				d = nd;
			}
			BAT *del_ids = BATunmask(d);
			if (!del_ids) {
				bat_destroy(d);
				bat_destroy(tids);
				return NULL;
			}
			bat_destroy(d);
			gdk_return ret = BATnegcands(tids, del_ids);
			BBPunfix(del_ids->batCacheid);
			if (ret != GDK_SUCCEED) {
				BBPreclaim(del_ids);
				bat_destroy(tids);
				return NULL;
			}
		} else {
			bat_destroy(tids);
			return NULL;
		}
	}
	return tids;
}

static BAT *
delta_cands(sql_trans *tr, sql_table *t)
{
	BAT *tids;

	if (!t->data) {
		sql_table *ot = tr_find_table(tr->parent, t);
		t->data = timestamp_dbat(ot->data, tr->stime);
	}
	tids = _delta_cands(tr, t);
	return tids;
}

static BAT *
full_column(sql_trans *tr, sql_column *c)
{
	/* TODO use bat_storage via callbacks */
	if (!c->data) {
		sql_column *oc = tr_find_column(tr->parent, c);
		c->data = timestamp_delta(oc->data, tr->stime);
	}
	/* return full normalized column bat
	 * 	b := b.copy()
		b := b.append(i);
		b := b.replace(u);
	*/
	BAT *b, *ui, *uv;
	int temp = isTemp(c);
	sql_delta *bat = c->data;

	b = temp_descriptor(bat->cs.bid);
	if (temp)
		return b;
	if (b && bat->cs.uibid && bat->cs.ucnt) {
		ui = temp_descriptor(bat->cs.uibid);
		uv = temp_descriptor(bat->cs.uvbid);
		if (ui && BATcount(ui)) {
			BAT *r = COLcopy(b, b->ttype, true, TRANSIENT);

			bat_destroy(b);
			b = r;
	    		if (!b || !ui || !uv || BATreplace(b, ui, uv, true) != GDK_SUCCEED) {
				if (b) BBPunfix(b->batCacheid);
				if (ui) BBPunfix(ui->batCacheid);
				if (uv) BBPunfix(uv->batCacheid);
				return NULL;
			}
		}
		bat_destroy(ui);
		bat_destroy(uv);
	}
	(void)c;
	return b;
}

static void
full_destroy(sql_column *c, BAT *b)
{
	(void)c;
	bat_destroy(b);
}

static oid
column_find_row(sql_trans *tr, sql_column *c, const void *value, ...)
{
	va_list va;
	BAT *b = NULL, *s = NULL, *r = NULL;
	oid rid = oid_nil;
	sql_column *n = NULL;

	va_start(va, value);
	s = delta_cands(tr, c->t);
	if (!s)
		goto return_nil;
	b = full_column(tr, c);
	if (!b) {
		bat_destroy(s);
		goto return_nil;
	}
	r = BATselect(b, s, value, NULL, true, false, false);
	bat_destroy(s);
	full_destroy(c, b);
	if (!r)
		goto return_nil;
	s = r;
	while ((n = va_arg(va, sql_column *)) != NULL) {
		value = va_arg(va, void *);
		c = n;

		b = full_column(tr, c);
		if (!b) {
			bat_destroy(s);
			goto return_nil;
		}
		r = BATselect(b, s, value, NULL, true, false, false);
		bat_destroy(s);
		full_destroy(c, b);
		if (!r)
			goto return_nil;
		s = r;
	}
	va_end(va);
	if (BATcount(s) == 1) {
		rid = BUNtoid(s, 0);
	}
	bat_destroy(s);
	return rid;
  return_nil:
	va_end(va);
	return oid_nil;
}

static void *
column_find_value(sql_trans *tr, sql_column *c, oid rid)
{
	BUN q = BUN_NONE;
	BAT *b;
	void *res = NULL;

	b = full_column(tr, c);
	if (b) {
		if (rid < b->hseqbase || rid >= b->hseqbase + BATcount(b))
			q = BUN_NONE;
		else
			q = rid - b->hseqbase;
	}
	if (q != BUN_NONE) {
		BATiter bi = bat_iterator(b);
		const void *r;
		size_t sz;

		r = BUNtail(bi, q);
		sz = ATOMlen(b->ttype, r);
		res = GDKmalloc(sz);
		if (res)
			memcpy(res, r, sz);
	}
	full_destroy(c, b);
	return res;
}

static sqlid
column_find_sqlid(sql_trans *tr, sql_column *c, oid rid)
{
	BUN q = BUN_NONE;
	BAT *b;
	sqlid res = -1;

	b = full_column(tr, c);
	if (b) {
		if (rid < b->hseqbase || rid >= b->hseqbase + BATcount(b))
			q = BUN_NONE;
		else
			q = rid - b->hseqbase;
	}
	if (q != BUN_NONE) {
		BATiter bi = bat_iterator(b);

		res = *(sqlid*)BUNtail(bi, q);
	}
	full_destroy(c, b);
	return res;
}

static sqlid
column_find_int(sql_trans *tr, sql_column *c, oid rid)
{
	BUN q = BUN_NONE;
	BAT *b;
	int res = -1;

	b = full_column(tr, c);
	if (b) {
		if (rid < b->hseqbase || rid >= b->hseqbase + BATcount(b))
			q = BUN_NONE;
		else
			q = rid - b->hseqbase;
	}
	if (q != BUN_NONE) {
		BATiter bi = bat_iterator(b);

		res = *(int*)BUNtail(bi, q);
	}
	full_destroy(c, b);
	return res;
}

static int
column_update_value(sql_trans *tr, sql_column *c, oid rid, void *value)
{
	assert(!is_oid_nil(rid));

	return store_funcs.update_col(tr, c, &rid, value, c->type.type->localtype);
}

static int
table_insert(sql_trans *tr, sql_table *t, ...)
{
	va_list va;
	node *n = cs_first_node(&t->columns);
	void *val = NULL;
	int cnt = 0;
	int ok = LOG_OK;

	va_start(va, t);
	size_t offset = store_funcs.claim_tab(tr, t, 1);
	for (; n; n = n->next) {
		sql_column *c = n->data;
		val = va_arg(va, void *);
		if (!val)
			break;
		ok = store_funcs.append_col(tr, c, offset, val, c->type.type->localtype);
		if (ok != LOG_OK) {
			va_end(va);
			return ok;
		}
		cnt++;
	}
	va_end(va);
	if (n) {
		// This part of the code should never get reached
		TRC_ERROR(SQL_STORE, "Called table_insert(%s) with wrong number of args (%d,%d)\n", t->base.name, list_length(t->columns.set), cnt);
		assert(0);
		return LOG_ERR;
	}
	return LOG_OK;
}

static int
table_delete(sql_trans *tr, sql_table *t, oid rid)
{
	assert(!is_oid_nil(rid));

	return store_funcs.delete_tab(tr, t, &rid, TYPE_oid);
}



/* returns table rids, for the given select ranges */
static rids *
rids_select( sql_trans *tr, sql_column *key, const void *key_value_low, const void *key_value_high, ...)
{
	va_list va;
	BAT *b = NULL, *r = NULL, *s = NULL;
	rids *rs = ZNEW(rids);
	const void *kvl = key_value_low, *kvh = key_value_high;
	/* if pointers are equal, make it an inclusive select */
	bool hi = key_value_low == key_value_high;

	if(!rs)
		return NULL;
	s = delta_cands(tr, key->t);
	if (s == NULL) {
		GDKfree(rs);
		return NULL;
	}
	b = full_column(tr, key);
	if (b == NULL) {
		bat_destroy(s);
		GDKfree(rs);
		return NULL;
	}
	if (!kvl)
		kvl = ATOMnilptr(b->ttype);
	if (!kvh && kvl != ATOMnilptr(b->ttype))
		kvh = ATOMnilptr(b->ttype);
	if (key_value_low) {
		BAThash(b);
		r = BATselect(b, s, kvl, kvh, true, hi, false);
		bat_destroy(s);
		s = r;
	}
	full_destroy(key, b);
	if (s == NULL) {
		GDKfree(rs);
		return NULL;
	}
	if (key_value_low || key_value_high) {
		va_start(va, key_value_high);
		while ((key = va_arg(va, sql_column *)) != NULL) {
			kvl = va_arg(va, void *);
			kvh = va_arg(va, void *);

			b = full_column(tr, key);
			if (!kvl)
				kvl = ATOMnilptr(b->ttype);
			if (!kvh && kvl != ATOMnilptr(b->ttype))
				kvh = ATOMnilptr(b->ttype);
			assert(kvh);
			r = BATselect(b, s, kvl, kvh, true, hi, false);
			bat_destroy(s);
			s = r;
			full_destroy(key, b);
			if (s == NULL) {
				GDKfree(rs);
				va_end(va);
				return NULL;
			}
		}
		va_end(va);
	}
	rs->data = s;
	rs->cur = 0;
	return rs;
}

/* order rids by orderby_column values */
static rids *
rids_orderby(sql_trans *tr, rids *r, sql_column *orderby_col)
{
	BAT *b, *s, *o;

	b = full_column(tr, orderby_col);
	s = BATproject(r->data, b);
	full_destroy(orderby_col, b);
	if (BATsort(NULL, &o, NULL, s, NULL, NULL, false, false, false) != GDK_SUCCEED) {
		bat_destroy(s);
		return NULL;
	}
	bat_destroy(s);
	s = BATproject(o, r->data);
	bat_destroy(o);
	if (s == NULL)
		return NULL;
	bat_destroy(r->data);
	r->data = s;
	return r;
}


/* return table rids from result of rids_select, return (oid_nil) when done */
static oid
rids_next(rids *r)
{
	if (r->cur < BATcount((BAT *) r->data)) {
		return BUNtoid((BAT *) r->data, r->cur++);
	}
	return oid_nil;
}

/* clean up the resources taken by the result of rids_select */
static void
rids_destroy(rids *r)
{
	bat_destroy(r->data);
	_DELETE(r);
}

static int
rids_empty(rids *r )
{
	BAT *b = r->data;
	return BATcount(b) <= 0;
}

static rids *
rids_join(sql_trans *tr, rids *l, sql_column *lc, rids *r, sql_column *rc)
{
	BAT *lcb, *rcb, *s = NULL, *d = NULL;
	gdk_return ret;

	lcb = full_column(tr, lc);
	rcb = full_column(tr, rc);
	ret = BATjoin(&s, &d, lcb, rcb, l->data, r->data, false, BATcount(lcb));
	bat_destroy(l->data);
	bat_destroy(d);
	if (ret != GDK_SUCCEED) {
		l->data = NULL;
	} else {
		l->data = s;
	}
	full_destroy(lc, lcb);
	full_destroy(rc, rcb);
	return l;
}

static subrids *
subrids_create(sql_trans *tr, rids *t1, sql_column *rc, sql_column *lc, sql_column *obc)
{
	/* join t1.rc with lc order by obc */
	subrids *r;
	BAT *lcb, *rcb, *s, *obb, *d = NULL, *o, *g, *ids, *rids = NULL;
	gdk_return ret;

	lcb = full_column(tr, lc);
	rcb = full_column(tr, rc);
	s = delta_cands(tr, lc->t);
	if (lcb == NULL || rcb == NULL || s == NULL) {
		if (lcb)
			full_destroy(rc, lcb);
		if (rcb)
			full_destroy(rc, rcb);
		bat_destroy(s);
		return NULL;
	}

	ret = BATjoin(&rids, &d, lcb, rcb, s, t1->data, false, BATcount(lcb));
	bat_destroy(s);
	full_destroy(rc, rcb);
	if (ret != GDK_SUCCEED) {
		full_destroy(rc, lcb);
		return NULL;
	}
	bat_destroy(d);

	s = BATproject(rids, lcb);
	full_destroy(lc, lcb);
	if (s == NULL) {
		bat_destroy(rids);
		return NULL;
	}
	lcb = s;

	if ((obb = full_column(tr, obc)) == NULL) {
		bat_destroy(lcb);
		bat_destroy(rids);
		return NULL;
	}
	s = BATproject(rids, obb);
	full_destroy(obc, obb);
	if (s == NULL) {
		bat_destroy(lcb);
		bat_destroy(rids);
		return NULL;
	}
	obb = s;

	/* need id, obc */
	ids = o = g = NULL;
	ret = BATsort(&ids, &o, &g, lcb, NULL, NULL, false, false, false);
	bat_destroy(lcb);
	if (ret != GDK_SUCCEED) {
		bat_destroy(obb);
		bat_destroy(rids);
		return NULL;
	}

	s = NULL;
	ret = BATsort(NULL, &s, NULL, obb, o, g, false, false, false);
	bat_destroy(obb);
	bat_destroy(o);
	bat_destroy(g);
	if (ret != GDK_SUCCEED) {
		bat_destroy(ids);
		bat_destroy(rids);
		return NULL;
	}

	o = BATproject(s, rids);
	bat_destroy(rids);
	bat_destroy(s);
	if (o == NULL) {
		bat_destroy(ids);
		return NULL;
	}
	rids = o;

	assert(ids->ttype == TYPE_int && ATOMtype(rids->ttype) == TYPE_oid);
	r = ZNEW(subrids);
	if (r == NULL) {
		bat_destroy(ids);
		bat_destroy(rids);
		return NULL;
	}
	r->id = 0;
	r->pos = 0;
	r->ids = ids;
	r->rids = rids;
	return r;
}

static oid
subrids_next(subrids *r)
{
	if (r->pos < BATcount((BAT *) r->ids)) {
		BATiter ii = bat_iterator((BAT *) r->ids);
		sqlid id = *(sqlid*)BUNtloc(ii, r->pos);
		if (id == r->id)
			return BUNtoid((BAT *) r->rids, r->pos++);
	}
	return oid_nil;
}

static sqlid
subrids_nextid(subrids *r)
{
	if (r->pos < BATcount((BAT *) r->ids)) {
		BATiter ii = bat_iterator((BAT *) r->ids);
		r->id = *(sqlid*)BUNtloc(ii, r->pos);
		return r->id;
	}
	return -1;
}

static void
subrids_destroy(subrids *r )
{
	if (r->ids)
		bat_destroy(r->ids);
	if (r->rids)
		bat_destroy(r->rids);
	_DELETE(r);
}

/* get the non - join results */
static rids *
rids_diff(sql_trans *tr, rids *l, sql_column *lc, subrids *r, sql_column *rc )
{
	BAT *lcb = full_column(tr, lc), *s, *d, *rids, *diff;
	BAT *rcb = full_column(tr, rc);
	gdk_return ret;

	if (lcb == NULL || rcb == NULL) {
		if (lcb)
			full_destroy(rc, lcb);
		if (rcb)
			full_destroy(rc, rcb);
		return NULL;
	}
	s = BATproject(r->rids, rcb);
	full_destroy(rc, rcb);
	if (s == NULL) {
		full_destroy(rc, lcb);
		return NULL;
	}
	rcb = s;

	s = BATproject(l->data, lcb);
	if (s == NULL) {
		full_destroy(rc, lcb);
		bat_destroy(rcb);
		return NULL;
	}

	diff = BATdiff(s, rcb, NULL, NULL, false, false, BUN_NONE);
	bat_destroy(rcb);
	if (diff == NULL) {
		full_destroy(rc, lcb);
		bat_destroy(s);
		return NULL;
	}

	ret = BATjoin(&rids, &d, lcb, s, NULL, diff, false, BATcount(s));
	bat_destroy(diff);
	full_destroy(lc, lcb);
	bat_destroy(s);
	if (ret != GDK_SUCCEED)
		return NULL;

	bat_destroy(d);
	bat_destroy(l->data);
	l->data = rids;
	return l;
}

static int
table_vacuum(sql_trans *tr, sql_table *t)
{
	BAT *tids = delta_cands(tr, t);
	BAT **cols;
	node *n;
	size_t cnt = 0;

	if (!tids)
		return SQL_ERR;
	cnt = BATcount(tids);
	cols = NEW_ARRAY(BAT*, cs_size(&t->columns));
	if (!cols) {
		bat_destroy(tids);
		return SQL_ERR;
	}
	for (n = t->columns.set->h; n; n = n->next) {
		sql_column *c = n->data;
		BAT *v = store_funcs.bind_col(tr, c, RDONLY);

		if (v == NULL ||
		    (cols[c->colnr] = BATproject(tids, v)) == NULL) {
			BBPunfix(tids->batCacheid);
			for (n = t->columns.set->h; n; n = n->next) {
				if (n->data == c)
					break;
				bat_destroy(cols[((sql_column *) n->data)->colnr]);
			}
			bat_destroy(v);
			_DELETE(cols);
			return SQL_ERR;
		}
		BBPunfix(v->batCacheid);
	}
	BBPunfix(tids->batCacheid);
	sql_trans_clear_table(tr, t);
	size_t offset = store_funcs.claim_tab(tr, t, cnt);
	assert(offset == 0);
	for (n = t->columns.set->h; n; n = n->next) {
		sql_column *c = n->data;
		int ok;

		ok = store_funcs.append_col(tr, c, offset, cols[c->colnr], TYPE_bat);
		BBPunfix(cols[c->colnr]->batCacheid);
		if (ok != LOG_OK) {
			for (n = n->next; n; n = n->next) {
				c = n->data;
				BBPunfix(cols[c->colnr]->batCacheid);
			}
			_DELETE(cols);
			return SQL_ERR;
		}
	}
	_DELETE(cols);
	return SQL_OK;
}

void
bat_table_init( table_functions *tf )
{
	tf->column_find_row = column_find_row;
	tf->column_find_value = column_find_value;
	tf->column_find_sqlid = column_find_sqlid;
	tf->column_find_int = column_find_int;

	tf->column_update_value = column_update_value;
	tf->table_insert = table_insert;
	tf->table_delete = table_delete;
	tf->table_vacuum = table_vacuum;

	tf->rids_select = rids_select;
	tf->rids_orderby = rids_orderby;
	tf->rids_join = rids_join;
	tf->rids_next = rids_next;
	tf->rids_destroy = rids_destroy;
	tf->rids_empty = rids_empty;

	tf->subrids_create = subrids_create;
	tf->subrids_next = subrids_next;
	tf->subrids_nextid = subrids_nextid;
	tf->subrids_destroy = subrids_destroy;
	tf->rids_diff = rids_diff;
}
