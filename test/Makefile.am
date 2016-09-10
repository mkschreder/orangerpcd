@CODE_COVERAGE_RULES@
check_PROGRAMS=json_check session sha1 id ws_server b64 orange
json_check_SOURCES=json_check.c
json_check_CFLAGS=$(CODE_COVERAGE_CFLAGS) -I../src/ -D_GNU_SOURCE -std=c99 
json_check_LDFLAGS=$(CODE_COVERAGE_LDFLAGS) -L../src/.libs/ -lblobpack -lm
session_SOURCES=session.c
session_CFLAGS=$(CODE_COVERAGE_CFLAGS) -I../src/ -D_GNU_SOURCE -std=c99 
session_LDFLAGS=$(CODE_COVERAGE_LDFLAGS) -L../src/.libs/ -lblobpack -lorange -lpthread
sha1_SOURCES=sha1.c
sha1_CFLAGS=$(CODE_COVERAGE_CFLAGS) -I../src/ -D_GNU_SOURCE -std=c99 
sha1_LDFLAGS=$(CODE_COVERAGE_LDFLAGS) -L../src/.libs/ -lblobpack -lorange 
ws_server_SOURCES=ws_server.c
ws_server_CFLAGS=$(CODE_COVERAGE_CFLAGS) -I../src/ -D_GNU_SOURCE -std=c99 
ws_server_LDFLAGS=$(CODE_COVERAGE_LDFLAGS) -L../src/.libs/ -lblobpack -lorange 
id_SOURCES=id.c
id_CFLAGS=$(CODE_COVERAGE_CFLAGS) -I../src/ -D_GNU_SOURCE -std=c99 
id_LDFLAGS=$(CODE_COVERAGE_LDFLAGS) -L../src/.libs/ -lblobpack -lorange 
b64_SOURCES=b64.c
b64_CFLAGS=$(CODE_COVERAGE_CFLAGS) -I../src/ -D_GNU_SOURCE -std=c99 
b64_LDFLAGS=$(CODE_COVERAGE_LDFLAGS) -L../src/.libs/ -lblobpack -lorange 
orange_SOURCES=orange.c
orange_CFLAGS=$(CODE_COVERAGE_CFLAGS) -I../src/ -D_GNU_SOURCE -std=c99 
orange_LDFLAGS=$(CODE_COVERAGE_LDFLAGS) -L../src/.libs/ -lblobpack -lorange 
TESTS=$(check_PROGRAMS)
@VALGRIND_CHECK_RULES@
