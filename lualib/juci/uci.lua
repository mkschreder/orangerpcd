local juci = require("juci/core"); 

local function uci_load(config)
	print("dump: "..config); 
	local lines = juci:shell("uci show %s", config); 
	local result = {}; 
	for line in lines:gmatch("[^\r\n]+") do 
		--local config,section,option,value = line:match("([^\\.]+)\\.([^\\.]+)\\.([^=]+)=(.*)"); 
		local config,section,kind = line:match("^([^\\.]+).([^\\.=]+)=(.*)"); 
		if(config and section and kind) then 
			result[section] = { 
				[".name"] = section, 
				[".type"] = kind 
			}; 	
		else 
			local config,section,option,value = line:match("^([^\\.]+).([^\\.]+).([^=]+)='(.*)'"); 
			if(config and section and option and value and result[section]) then
				result[section][option] = value; 
			end
		end
	end
	return result; 
end

return {
	["load"] = uci_load
}; 
