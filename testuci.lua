#!/usr/bin/lua

package.path = "./lualib/?.lua;"..package.path; 
local uci = require("./plugins/uci"); 
local json = require("./lualib/juci/json"); 

print(json.encode(uci.configs())); 
