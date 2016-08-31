@CODE_COVERAGE_RULES@
check_PROGRAMS=json_check session
json_check_SOURCES=json_check.c
json_check_CFLAGS=$(CODE_COVERAGE_CFLAGS) -I../src/ -D_GNU_SOURCE -std=c99 
json_check_LDFLAGS=$(CODE_COVERAGE_LDFLAGS) -L../src/.libs/ -lblobpack -lm
session_SOURCES=session.c
session_CFLAGS=$(CODE_COVERAGE_CFLAGS) -I../src/ -D_GNU_SOURCE -std=c99 
session_LDFLAGS=$(CODE_COVERAGE_LDFLAGS) -L../src/.libs/ -lblobpack -lorange -lm -lblobpack -lusys -lutype -lpthread -lwebsockets -lcrypt -luci @LIBLUA_LINK@
TESTS=$(check_PROGRAMS)
