#!/usr/bin/lua

-- JUCI Lua Backend Server API
-- Copyright (c) 2016 Martin Schröder <mkschreder.uk@gmail.com>. All rights reserved. 
-- This module is distributed under GNU GPLv3 with additional permission for signed images.
-- See LICENSE file for more details. 

local juci = require("juci/core"); 

function iptv_igmptable()
	local res = {}; 
	local tbl = {}; 
	local obj = {}; 
	local field_names = {"bridge", "device", "srcdev", "tags", "lantci", "wantci", "group", "mode", "rx_group", "source", "reporter", "timeout", "index", "excludpt"}; 
	
	local f = assert(io.popen("igmpinfo", "r")); 
	local line = f:read("*l"); 
	while line do
		local fields = {}
		local obj = {}; 
		for w in s:gmatch("[^\t]+") do table.insert(fields, w) end
		for i,v in ipairs(fields) do
			obj[field_names[i]] = v; 
		end
		table.insert(tbl, obj); 
		line = f:read("*l"); 
	end
	f:close(); 
	res["table"] = tbl; 
	return res; 
end

return {
	["igmptable"] = iptv_igmptable
};
