local json = require("juci/json"); 

local function ubus_bind(o, obj)
	local ret = {}; 
	print("binding "..json.encode(obj)); 
	for _,m in ipairs(obj) do
		ret[m] = function(opts)
			print("call "..o.."."..m); 
			return {}; 
		end
	end
	return ret; 
end

return {
	bind = ubus_bind
}; 
