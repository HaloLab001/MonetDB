try:
    from MonetDBtesting import process
except ImportError:
    import process
import os, sys
from MonetDBtesting.sqltest import SQLTestCase

dbfarm = os.getenv('GDK_DBFARM')
tstdb = os.getenv('TSTDB')

if not tstdb or not dbfarm:
    print('No TSTDB or GDK_DBFARM in environment')
    sys.exit(1)

dbname = tstdb

with process.server(dbname=dbname, stdin=process.PIPE, stdout=process.PIPE, stderr=process.PIPE) as s, \
        SQLTestCase() as tc:
    tc.connect(database=dbname)
    tc.execute("insert into tmp values(3,'blah'),(4,'bloh');")\
        .assertSucceeded()\
        .assertRowCount(2)
    tc.execute("select * from tmp;")\
        .assertSucceeded()\
        .assertDataResultMatch([(1, 'hello'), (2, 'world'), (3, 'blah'), (4, 'bloh')])

    sout, serr = s.communicate()


#with process.server(dbname=dbname, stdin=process.PIPE, stdout=process.PIPE, stderr=process.PIPE) as s, \
#     process.client('sql', server = s, stdin = process.PIPE, stdout = process.PIPE, stderr = process.PIPE) as c:
#
#    cout, cerr = c.communicate('''\
#insert into tmp values(3,'blah'),(4,'bloh');
#select * from tmp;
#''')
#
#    sout, serr = s.communicate()
#
#    sys.stdout.write(sout)
#    sys.stdout.write(cout)
#    sys.stderr.write(serr)
#    sys.stderr.write(cerr)

def listfiles(path):
    sys.stdout.write("#LISTING OF THE LOG FILES\n")
    for f in sorted(os.listdir(path)):
        if (f.find('wlc') >= 0 or f.find('wlr') >=0 ) and f != 'wlc_logs':
            file = path + os.path.sep + f
            sys.stdout.write('#' + file + "\n")
            try:
                x = open(file)
                s = x.read()
                lines = s.split('\n')
                for l in lines:
                    sys.stdout.write('#' + l + '\n')
                x.close()
            except IOError:
                sys.stderr.write('Failure to read file ' + file + '\n')

listfiles(dbfarm + os.path.sep + tstdb)
listfiles(dbfarm + os.path.sep + tstdb + os.path.sep + 'wlc_logs')
