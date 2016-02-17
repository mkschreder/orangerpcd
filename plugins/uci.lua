#!/usr/bin/lua

-- JUCI Lua Backend Server API
-- Copyright (c) 2016 Martin Schröder <mkschreder.uk@gmail.com>. All rights reserved. 
-- This module is distributed under GNU GPLv3 with additional permission for signed images.
-- See LICENSE file for more details. 


local ubus = require("juci/ubus"); 

return ubus.bind("uci", {"get","set","add","configs","commit","revert","apply","rollback","delete"}); 
