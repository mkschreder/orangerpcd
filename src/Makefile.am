@CODE_COVERAGE_RULES@
includedir=$(prefix)/include/orangerpcd/
lib_LTLIBRARIES=liborange.la
include_HEADERS=orange.h orange_id.h orange_lua.h orange_luaobject.h orange_message.h orange_server.h orange_uci.h orange_user.h orange_ws_server.h sha1.h 
liborange_la_SOURCES=base64.c json_check.c orange_luaobject.c orange_session.c orange_message.c orange_id.c orange_lua.c orange.c orange_ws_server.c orange_user.c orange_uci.c sha1.c 
liborange_la_CFLAGS=$(CODE_COVERAGE_CFLAGS) -std=gnu99 -Wall -Werror
liborange_la_LIBADD=-lblobpack -lusys -lutype -lpthread -lwebsockets -lcrypt -luci -lorange @LIBLUA_LINK@
orangerpcd_SOURCES=main.c
orangerpcd_CFLAGS=-std=gnu99 -Wall -Werror
orangerpcd_LDADD=-lorange
