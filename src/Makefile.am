bin_PROGRAMS=jucid
jucid_SOURCES=juci_luaobject.c juci_session.c juci_message.c juci_id.c juci.c juci_ws_server.c main.c
jucid_CFLAGS=-std=gnu99 -Wall 
jucid_LDADD=-lblobpack -lusys -lutype -lpthread -lwebsockets @LIBLUA_LINK@
