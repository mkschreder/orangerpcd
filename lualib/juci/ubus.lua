local juci = require("juci/core"); 

local function ubus_call(o, m, opts)
	local result = juci.shell("ubus call "..o.." "..m.." '"..json.encode(opts).."'"); 
	return JSON.parse(result); 
end

local function ubus_bind(o, obj)
	local ret = {}; 
	print("binding "..json.encode(obj)); 
	for _,m in ipairs(obj) do
		ret[m] = function(opts)
			print("ubus call "..o.." "..m.." '"..json.encode(opts).."'"); 
			return ubus_call(o, m, opts);  
		end
	end
	return ret; 
end

return {
	bind = ubus_bind, 
	call = ubus_call
}; 
