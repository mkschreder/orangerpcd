#!/usr/bin/lua

local function print_hello(opts)
	print("Hello World!"); 
	print("option: "..(opts.key or "nil")); 
	return { foo = "bar", opts = opts, field = "value" }; 
end

return { print_hello = print_hello }; 
