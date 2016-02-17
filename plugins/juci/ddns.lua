#!/usr/bin/lua

-- JUCI Lua Backend Server API
-- Copyright (c) 2016 Martin Schröder <mkschreder.uk@gmail.com>. All rights reserved. 
-- This module is distributed under GNU GPLv3 with additional permission for signed images.
-- See LICENSE file for more details. 

local juci = require("juci.core"); 
local json = require("juci.json"); 

function ddns_list_providers()
	local list = juci.shell("cat /usr/lib/ddns/services | awk '/\".*\"/{ gsub(\"\\\"\",\"\",$1); print($1);}'"); 
	local line; 
	local result = { providers = {} }; 
	for line in list:gmatch("%S+") do
		table.insert(result.providers, line); 
	end
	return result;
end

return {
	providers = ddns_list_providers
}; 
