JUCI General Purpose Websocket RPC Server
-----------------------------------------

This project introduces a replacement rpc server for JUCI webgui embedded
router user interface, based on libwebsockets. It replaces the previous rpc
system heavily dependent on rpcd, uhttpd, ubus and ubus-scriptd with a single
server. The server features an RPC interface facing the user, it\'s own private
access control system independent of rpcd, fast calls into precompiled and
preloaded lua backend scripts and all features present in the previous system
(including possibility to call ubus methods).  

Directory Structure
-------------------

- /usr/lib/orange/lib/: shared lua libraries
	./orange/...
- /usr/lib/orange/rpc/: all code accessible through websocket rpc.
	./orange/...
	uci.lua
	session.lua
- /usr/lib/orange/acl/: all plugin specific access control lists
	some-plugin.acl
	...


