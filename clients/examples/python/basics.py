# SPDX-License-Identifier: MPL-2.0
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0.  If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# Copyright 2024, 2025 MonetDB Foundation;
# Copyright August 2008 - 2023 MonetDB B.V.;
# Copyright 1997 - July 2008 CWI.

import logging

#configure the logger, so we can see what is happening
logging.basicConfig(level=logging.DEBUG)
logger = logging.getLogger('monetdb')

import pymonetdb

x = pymonetdb.connect(username="monetdb", password="monetdb", hostname="localhost", database="demo")
c = x.cursor()

# some basic query
c.arraysize=100
c.execute('select * from tables')
results = c.fetchall()
x.commit()
print(results)

c.close()
x.close()
