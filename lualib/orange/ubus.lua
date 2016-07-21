-- JUCI Lua Backend Server API
-- Copyright (c) 2016 Martin Schröder <mkschreder.uk@gmail.com>. All rights reserved. 
-- This module is distributed under JUCI Genereal Public License as published
-- at https://github.com/mkschreder/orangerpcd/COPYING. See COPYING file for details. 

local orange = require("orange/core"); 

local function ubus_call(o, m, opts)
	local params = json.encode(opts); 
	if params == "[]" then params = '{}'; end; 
	local result = orange.shell("ubus call "..o.." "..m.." '"..params.."'"); 
	return JSON.parse(result); 
end

local function ubus_bind(o, obj)
	local ret = {}; 
	--print("binding "..json.encode(obj)); 
	for _,m in ipairs(obj) do
		ret[m] = function(opts)
			--print("ubus call "..o.." "..m.." '"..json.encode(opts).."'"); 
			return ubus_call(o, m, opts);  
		end
	end
	return ret; 
end

return {
	bind = ubus_bind, 
	call = ubus_call
}; 
