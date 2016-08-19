#!/usr/bin/lua

-- JUCI Lua Backend Server API
-- Copyright (c) 2016 Martin Schröder <mkschreder.uk@gmail.com>. All rights reserved. 
-- This module is distributed under GNU GPLv3 with additional permission for signed images.
-- See LICENSE file for more details. 

local core = require("orange/core");
local ubus = require("orange/ubus"); 

local uci = ubus.bind("uci", {"get","set","add","configs","commit","revert","apply","rollback","delete"}); 

local function uci_transaction_open(args)
	local pass = core.readfile("/etc/orange/password"):match("(%S+)"); 
	local r = ubus.call("session", "login", { username = "orange", password = pass }); 
	local res = {}; 
	if(not r["ubus_rpc_session"]) then 
		res.error = "Unable to create session!";
	else	
		res.id = r["ubus_rpc_session"]; 
	end
	return res; 
end

local function uci_transaction_close(args)
	local res = {}; 
	ubus.call("session", "logout", { ubus_rpc_session = args.id }); 
	return res; 
end

local function uci_configs(args)
	local res = uci.configs(args); 
	local allow = {}; 
	-- filter out only the ones we allow
	for i,conf in ipairs(res.configs) do
		if(SESSION.access("uci", conf, "*", "r")) then table.insert(allow, conf); end 
	end
	res.configs = allow; 
	return res; 
end

local function uci_get(args)
	if(not SESSION.access("uci", args.config, "*", "r")) then return { error = "No uci permission to read config section!" }; end
	return uci.get(args); 
end

local function uci_set(args)
	if(not SESSION.access("uci", args.config, "*", "w")) then return { error = "No uci permission to write config section!" }; end
	return uci.set(args); 
end

local function uci_add(args)
	if(not SESSION.access("uci", args.config, "*", "w")) then return { error = "No uci permission to write config!" }; end
	return uci.add(args); 
end

local function uci_revert(args)
	if(not SESSION.access("uci", args.config, "*", "w")) then return { error = "No uci permission to write config!" }; end
	return uci.revert(args); 
end

local function uci_commit(args)
	return uci.commit(args); 
end

local res = {}; 
res.open_transaction = uci_transaction_open; -- creates a new context based transaction  
res.close_transaction = uci_transaction_close; -- deletes a transaction (will automatically timeout if not deleted) 
res.configs = uci_configs; -- gets list of accessible configs
res.set = uci_set; -- used to set values in uci 
res.get = uci_get; -- used to retreive uci data 
res.add = uci_add; -- add a uci section to a config
res.revert = uci_revert; -- revert changes that are part of current transaction
res.commit = uci_commit; -- commits changes that are part of current transaction

return res; 
