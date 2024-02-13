#[[
# SPDX-License-Identifier: MPL-2.0
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0.  If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# Copyright 2024 MonetDB Foundation;
# Copyright August 2008 - 2023 MonetDB B.V.;
# Copyright 1997 - July 2008 CWI.
#]]

find_program(RPMBUILD_EXECUTABLE NAMES rpmbuild
  DOC "Create rpm packages"
)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(Rpmbuild
  DEFAULT_MSG
  RPMBUILD_EXECUTABLE
)

mark_as_advanced(RPMBUILD_EXECUTABLE)
