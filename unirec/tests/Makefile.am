BUILT_SOURCES=fields.h fields.c

EXTRA_DIST=fields.c fields.h

fields.h: fields.c

fields.c:
	${top_srcdir}/ur_processor.sh -i ${top_srcdir} -o ./

check_PROGRAMS=test_basic test_creation test_ipaddr test_time test_macaddr test_iter test_speed test_speed_o test_speed_ur test_speed_uro test_template_cmp

TESTS = test_basic test_creation test_ipaddr test_time test_macaddr test_iter test_speed test_speed_o test_speed_ur test_speed_uro test_template_cmp

AM_LDFLAGS=-static ../libunirec.la
COM_CPPFLAGS=-I../../ -I../ -I${top_srcdir}/../../

test_basic_SOURCES=test_basic.c fields.c
test_basic_CPPFLAGS=$(COM_CPPFLAGS)

test_creation_SOURCES=test_creation.c fields.c
test_creation_CPPFLAGS=$(COM_CPPFLAGS)

test_ipaddr_SOURCES=test_ipaddr.c fields.c
test_ipaddr_CPPFLAGS=$(COM_CPPFLAGS)

test_time_SOURCES=test_time.c fields.c
test_time_CPPFLAGS=$(COM_CPPFLAGS)

test_macaddr_SOURCES=test_macaddr.c fields.c
test_macaddr_CPPFLAGS=$(COM_CPPFLAGS)

test_iter_SOURCES=test_iter.c
test_iter_CPPFLAGS=$(COM_CPPFLAGS)

test_speed_SOURCES=test_speed.c fields.c
test_speed_CPPFLAGS=$(COM_CPPFLAGS)

test_speed_o_SOURCES=test_speed.c fields.c
test_speed_o_CPPFLAGS=$(COM_CPPFLAGS)

test_speed_ur_SOURCES=test_speed.c fields.c
test_speed_ur_CFLAGS=-DUNIREC
test_speed_ur_CPPFLAGS=$(COM_CPPFLAGS)

test_speed_uro_SOURCES=test_speed.c fields.c
test_speed_uro_CFLAGS=-DUNIREC
test_speed_uro_CPPFLAGS=$(COM_CPPFLAGS)

test_template_cmp_SOURCES=test_template_cmp.c fields.c
test_template_cmp_CFLAGS=-DUNIREC
test_template_cmp_CPPFLAGS=$(COM_CPPFLAGS)

clean-local:
	rm -f fields.c fields.h

