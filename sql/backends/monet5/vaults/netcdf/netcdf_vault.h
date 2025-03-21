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

#ifndef _NCDF_
#define _NCDF_
#include "mal.h"
#include "mal_client.h"

#ifdef WIN32
#ifndef LIBNCDF
#define netcdf_export extern __declspec(dllimport)
#else
#define netcdf_export extern __declspec(dllexport)
#endif
#else
#define netcdf_export extern
#endif

netcdf_export str NCDFtest(int *vars, str *fname);
netcdf_export str NCDFattach(Client cntxt, MalBlkPtr mb, MalStkPtr stk, InstrPtr pci);
netcdf_export str NCDFimportVarStmt(str *sciqlstmt, str *fname, int *varid);
netcdf_export str NCDFimportVariable(Client cntxt, MalBlkPtr mb, MalStkPtr stk, InstrPtr pci);
#endif
