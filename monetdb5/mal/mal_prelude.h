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

#ifndef _MAL_PRELUDE_H
#define _MAL_PRELUDE_H

#include "mal_exception.h"
#include "mal_client.h"

#include "mel.h"

mal_export int mal_startup(void);

mal_export void mal_module(const char *name, mel_atom * atoms,
						   mel_func * funcs);
mal_export void mal_module2(const char *name, mel_atom * atoms,
							mel_func * funcs, mel_init initfunc,
							const char *code);

mal_export str malIncludeModules(Client c, char *modules[], int listing,
								 bool embedded, const char *initpasswd);

#endif /*  _MAL_PRELUDE_H */
