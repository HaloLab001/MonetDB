@REM SPDX-License-Identifier: MPL-2.0
@REM
@REM This Source Code Form is subject to the terms of the Mozilla Public
@REM License, v. 2.0.  If a copy of the MPL was not distributed with this
@REM file, You can obtain one at http://mozilla.org/MPL/2.0/.
@REM
@REM Copyright 2024, 2025 MonetDB Foundation;
@REM Copyright August 2008 - 2023 MonetDB B.V.;
@REM Copyright 1997 - July 2008 CWI.

@rem figure out the folder name
@set MONETDB=%~dp0

@rem remove the final backslash from the path
@set MONETDB=%MONETDB:~0,-1%

@rem start the real client
@"%MONETDB%\mclient.bat" --language=sql %*
