#!/usr/bin/lua

local ubus = require("juci/ubus"); 

return ubus.bind({
	["."] = { "board", "info" }
}); 
