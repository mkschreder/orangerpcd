#!/usr/bin/lua

local function session_test()
	-- check acl 
	local s = SESSION.get(); 
	print("Current user: "..s.username); 
	local a = SESSION.access("ubus","session","test","w"); -- success
	local b = not SESSION.access("ubus","abrakadabra","test","w"); -- fail
	local c = not SESSION.access("ubus"); -- fail
end

return {
	test = session_test
}; 
