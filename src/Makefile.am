bin_PROGRAMS=orangerpcd
orangerpcd_SOURCES=base64.c json_check.c orange_luaobject.c orange_session.c orange_message.c orange_id.c orange_lua.c orange.c orange_ws_server.c orange_user.c orange_uci.c sha1.c main.c
orangerpcd_CFLAGS=-std=gnu99 -Wall -Werror
orangerpcd_LDADD=-lblobpack -lusys -lutype -lpthread -lwebsockets -lcrypt -luci @LIBLUA_LINK@
