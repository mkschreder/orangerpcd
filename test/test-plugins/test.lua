local function test_echo(args)
	return args; 
end

local function test_delay_echo(args)
	os.execute("sleep 1"); 
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

local function test_error_code(args)
	return args.code or -1; -- test returning an error code
end

local function test_exit(args)
	CORE.__interrupt(args.code or 0); 
	--os.exit(args.code or 0); 
end

return {
	echo = test_echo, 
	delay_echo = test_delay_echo,
	test_c_calls = test_c_calls,
	error_code = test_error_code,
	exit = test_exit
}

