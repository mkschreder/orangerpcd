local function echo(args)
	return args; 
end

local function test_c_calls(args)
	local t = JSON.parse("{\"foo\":\"bar\"}"); 	
	if ( t.foo ~= "bar" ) then return -1; end
	if (not SESSION.access("rpc", "/test", "test_c_calls", "x")) then return -2; end
	if (SESSION.access("rpc", "/test", "noexist", "x")) then return -2; end
	local s = SESSION.get(); 
	if (s.username ~= "admin") then return -3; end
	local b = CORE.b64_encode("Hello World!"); 
	if (b ~= "SGVsbG8gV29ybGQh") then return -4; end
	return {}; 
end

return {
	echo = echo, 
	test_c_calls = test_c_calls
}

