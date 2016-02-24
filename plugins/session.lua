#!/usr/bin/lua

-- JUCI Lua Backend Server API
-- Copyright (c) 2016 Martin Schröder <mkschreder.uk@gmail.com>. All rights reserved. 
-- This module is distributed under GNU GPLv3 with additional permission for signed images.
-- See LICENSE file for more details. 

local json = require("juci/json"); 

local function session_access(opts)
	print(json.encode(opts)); 
	return { access = SESSION.access(opts.scope, opts.object, opts.method, opts.perms) }  
end

return {
	access = session_access
}
