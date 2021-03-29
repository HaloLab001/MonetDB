import os
import sys
from subprocess import run, PIPE, CalledProcessError

HOST=os.getenv('HOST')
MAPIPORT=os.getenv('MAPIPORT')
TSTDB=os.getenv('TSTDB')
TSTSRCBASE=os.getenv('TSTSRCBASE')
TSTDIR=os.getenv('TSTDIR')
CLIENT='org.monetdb.client.JdbcClient'
USER='monetdb'
PASSWORD='monetdb'


if __name__ == '__main__':
    cmd = ['java', CLIENT, '-h', HOST, '-p', MAPIPORT, '-d', TSTDB, '-f', os.path.join(TSTSRCBASE, TSTDIR, 'Tests/ValidateSystemCatalogTables.sql')]
    try:
        p = run(cmd, stdout=PIPE, stderr=PIPE, check=True, text=True)
        if p.stderr:
            raise SystemExit(p.stderr)
    except CalledProcessError as e:
        raise SystemExit(e.stderr)


##!/bin/sh
#
#cat << EOF > .monetdb
#user=monetdb
#password=monetdb
#EOF
#
#java org.monetdb.client.JdbcClient -h $HOST -p $MAPIPORT -d ${TSTDB} -e -f $TSTSRCBASE/$TSTDIR/Tests/ValidateSystemCatalogTables.sql
#
##rm -f .monetdb
