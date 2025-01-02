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

#ifndef _REL_SEQUENCE_H_
#define _REL_SEQUENCE_H_

#include "sql_symbol.h"
#include "store_sequence.h"
#include "sql_query.h"

extern sql_rel *rel_sequences(sql_query *query, symbol *s);
extern char* sql_next_seq_name(mvc *sql);

#endif /*_REL_SEQUENCE_H_*/
