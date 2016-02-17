#!/usr/bin/lua

local juci = require("juci/core"); 

local function print_hello(opts)
	print("Hello World!"); 
	print("option: "..(opts.key or "nil")); 
	return { foo = "bar", opts = opts, field = "value" }; 
end

local function largejson(opts)
	return JSON.parse(juci.shell("cat example/large.json")); 
end

return { 
	print_hello = print_hello,
	largejson = largejson
}; 
