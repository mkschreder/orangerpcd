@CODE_COVERAGE_RULES@
check_PROGRAMS=json_check session sha1 id ws_server b64 orange
AM_CFLAGS=$(CODE_COVERAGE_CFLAGS) $(CONFIG_CFLAGS) -I../src/ -D_GNU_SOURCE -std=c99 -D_POSIX_C_SOURCE=201609L -D_BSD_SOURCE -D_XOPEN_SOURCE -D_XOPEN_SOUCE_EXTENDED -D_GNU_SOURCE -Wall -Werror -Wno-format-y2k -W -Wstrict-prototypes -Wmissing-prototypes \
-Wpointer-arith -Wreturn-type -Wwrite-strings -Wswitch \
-Wno-cast-align -Wchar-subscripts -Winline -Wtype-limits \
-Wnested-externs -Wredundant-decls -Wmissing-field-initializers -Wextra \
-Wformat=2 -Wno-format-nonliteral -Wpointer-arith -Wno-missing-braces \
-Wno-unused-parameter -Wno-unused-variable -Wno-inline
json_check_SOURCES=json_check.c
json_check_CFLAGS=$(AM_CFLAGS) 
json_check_LDFLAGS=$(CODE_COVERAGE_LDFLAGS) -L../src/.libs/ -lblobpack -lm
session_SOURCES=session.c
session_CFLAGS=$(AM_CFLAGS)
session_LDFLAGS=$(CODE_COVERAGE_LDFLAGS) -L../src/.libs/ -lblobpack -lorange -lpthread
sha1_SOURCES=sha1.c
sha1_CFLAGS=$(AM_CFLAGS)
sha1_LDFLAGS=$(CODE_COVERAGE_LDFLAGS) -L../src/.libs/ -lblobpack -lorange 
ws_server_SOURCES=ws_server.c
ws_server_CFLAGS=$(AM_CFLAGS)
ws_server_LDFLAGS=$(CODE_COVERAGE_LDFLAGS) -L../src/.libs/ -lblobpack -lorange 
id_SOURCES=id.c
id_CFLAGS=$(AM_CFLAGS)
id_LDFLAGS=$(CODE_COVERAGE_LDFLAGS) -L../src/.libs/ -lblobpack -lorange 
b64_SOURCES=b64.c
b64_CFLAGS=$(AM_CFLAGS)
b64_LDFLAGS=$(CODE_COVERAGE_LDFLAGS) -L../src/.libs/ -lblobpack -lorange 
orange_SOURCES=orange.c
orange_CFLAGS=$(AM_CFLAGS)
orange_LDFLAGS=$(CODE_COVERAGE_LDFLAGS) -L../src/.libs/ -lblobpack -lorange 
TESTS=$(check_PROGRAMS)
@VALGRIND_CHECK_RULES@
