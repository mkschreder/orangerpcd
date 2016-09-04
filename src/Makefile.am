@CODE_COVERAGE_RULES@
includedir=$(prefix)/include/orangerpcd/
lib_LTLIBRARIES=liborange.la
bin_PROGRAMS=orangerpcd
include_HEADERS=orange.h orange_id.h orange_lua.h orange_luaobject.h orange_message.h orange_server.h orange_uci.h orange_user.h orange_ws_server.h sha1.h 
AM_CFLAGS=-Wall -fPIC -Wno-format-y2k -W -Wstrict-prototypes -Wmissing-prototypes \
-Wpointer-arith -Wreturn-type -Wwrite-strings -Wswitch \
-Wcast-align -Wchar-subscripts -Winline \
-Wnested-externs -Wredundant-decls -Wmissing-field-initializers -Wextra \
-Wformat=2 -Wno-format-nonliteral -Wpointer-arith -Wno-missing-braces \
-Wno-unused-parameter -Wno-unused-variable -Wno-inline
liborange_la_SOURCES=base64.c json_check.c orange_luaobject.c orange_session.c orange_message.c orange_id.c orange_lua.c orange_ws_server.c orange_user.c orange_uci.c sha1.c orange.c orange_rpc.c 
liborange_la_CFLAGS=$(AM_CFLAGS) $(CODE_COVERAGE_CFLAGS) -std=gnu99 -fstack-protector-all -Wall -Werror
liborange_la_LIBADD=-lblobpack -lutype -lpthread -lwebsockets -lcrypt -lrt @LIBLUA_LINK@ @LIBUCI_LINK@
orangerpcd_SOURCES=main.c
orangerpcd_CFLAGS=$(AM_CFLAGS) -std=gnu99 -fstack-protector-all -Wall -Werror
orangerpcd_LDADD=-lutype -lblobpack -lorange -lrt 
