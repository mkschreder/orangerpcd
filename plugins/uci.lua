#!/usr/bin/lua

local juci = require("juci/core"); 

local function uci_get(opts)
	return { values = opts }; 
end

local function uci_set()
	return {}; 
end

local function uci_add()
	return {}; 
end

local function uci_revert()
	return { foo = "bar" }; 
end

local function uci_apply()
	return {}; 
end

local function uci_configs()
	local configs = {}; 
	local lines = juci.shell("ls /etc/config"); 
	for line in lines:gmatch("%S+") do 
		table.insert(configs, line); 
	end
	return { configs = configs }; 
end

return {
	get = uci_get, 
	set = uci_set,
	add = uci_add, 
	configs = uci_configs,
	revert = uci_revert, 
	apply = uci_apply
}; 

