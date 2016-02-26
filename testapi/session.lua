#!/usr/bin/lua

local function session_test()
	-- check acl 
	local s = SESSION.get(); 
	print("Current user: "..s.username); 
	local a = SESSION.access("ubus","session","test","w"); -- success
	local b = not SESSION.access("ubus","abrakadabra","test","w"); -- fail
end

local function session_access(opts)
	return { access = SESSION.access(opts.scope, opts.object, opts.method, opts.perms) };  
end

return {
	test = session_test,
	access = session_access
}; 
