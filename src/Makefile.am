bin_PROGRAMS=revorpcd
revorpcd_SOURCES=juci_luaobject.c juci_session.c juci_message.c juci_id.c juci_lua.c juci.c juci_ws_server.c juci_user.c juci_uci.c sha1.c main.c
revorpcd_CFLAGS=-std=gnu99 -Wall -Werror
revorpcd_LDADD=-lblobpack -lusys -lutype -lpthread -lwebsockets -lcrypt -luci @LIBLUA_LINK@
