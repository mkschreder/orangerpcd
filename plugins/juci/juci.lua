#!/usr/bin/lua

local juci = require("juci/core"); 
local json = require("juci/json"); 

local function install_juci()
	local result = { stdout = "" }
	result.stdout = juci.shell("juci-shell install"); 	
	print(json.encode(result)); 
end

return {
	["install"] = install_juci
}; 
