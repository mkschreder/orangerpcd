#!/usr/bin/lua

local function file_write(opts)
	fs.writeFragment(opts.filename, opts.seek, opts.length, opts.data64); 
	return {}; 
end

return {
	write = file_write
}; 
