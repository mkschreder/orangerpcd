#!/usr/bin/lua

-- JUCI Lua Backend Server API
-- Copyright (c) 2016 Martin Schröder <mkschreder.uk@gmail.com>. All rights reserved. 
-- This module is distributed under GNU GPLv3 with additional permission for signed images.
-- See LICENSE file for more details. 

local juci = require("juci/core"); 

local function print_hello(opts)
	print("Hello World!"); 
	print("option: "..(opts.key or "nil")); 
	return { foo = "bar", opts = opts, field = "value" }; 
end

local function largejson(opts)
	return JSON.parse(juci.shell("cat example/large.json")); 
end

local function noreturn(opts)
	print("this function does not have a return value!"); 	
end

return { 
	print_hello = print_hello,
	largejson = largejson,
	noreturn = noreturn
}; 
