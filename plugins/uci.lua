#!/usr/bin/lua

-- JUCI Lua Backend Server API
-- Copyright (c) 2016 Martin Schröder <mkschreder.uk@gmail.com>. All rights reserved. 
-- This module is distributed under GNU GPLv3 with additional permission for signed images.
-- See LICENSE file for more details. 

local core = require("orange/core");
local ubus = require("orange/ubus"); 

local uci = ubus.bind("uci", {"get","set","add","configs","commit","revert","apply","rollback","delete"}); 

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

-- DEFERRED SHELL EXPLANATION
--[[ 
	OpenWRT uci subsystem does not comply to ACID rules of database design. 

	- Atomicity: uci operations are not atomic (even with sessions, uci is not
	  atomic because it requires an explicit commit)
	- Consistency: uci is pretty much a total fail here because any process can
	  modify the database and commit changes and another process can do the
	  same - result is not consistent. 
	- Isolation: uci calls are not isolated because the effect of set, add and
	  delete are all dependent on subsequent call to commit
	- Durability: no comment

	SO, we want to at least somehow mitigate these shortcomings and to do that
	the new system puts it this way: assume that each set, add and delete takes
	effect instantly. In practice we can not make this happen because OpenWRT
	will diligently restart relevant services when we do a commit to a config
	so we really don't want to be doing that for each change. Instead, we defer
	the commit. Backend code will prevent multiple commits to the same config
	from running if another command that looks exactly the same is in the
	queue. 

]]--

local function uci_get(args)
	if(not SESSION.access("uci", args.config, "*", "r")) then return -1; end
	return uci.get(args); 
end

local function uci_set(args)
	if(not SESSION.access("uci", args.config, "*", "w")) then return -1; end
	CORE.deferredshell("ubus call uci commit '{\"config\":\""..args.config.."\"}'", 5000);
	return uci.set(args); 
end

local function uci_add(args)
	if(not SESSION.access("uci", args.config, "*", "w")) then return -1; end
	CORE.deferredshell("ubus call uci commit '{\"config\":\""..args.config.."\"}'", 5000);
	return uci.add(args); 
end

local function uci_delete(args)
	if(not SESSION.access("uci", args.config, "*", "w")) then return -1; end
	CORE.deferredshell("ubus call uci commit '{\"config\":\""..args.config.."\"}'", 5000);
	return uci.delete(args); 
end

local res = {}; 

-- NOTE: we do not change names of methods yet and just keep them as they are,
-- but in reality we want to actually have the select, update, and delete
-- semantics for this kind of interface. 

res.configs = uci_configs; -- describe, gets list of accessible configs
res.set = uci_set; -- update, used to set values in uci 
res.get = uci_get; -- select, used to retreive uci data 
res.add = uci_add; -- create, add a uci section to a config
res.delete = uci_delete; -- delete, remove a uci section from config

return res; 
