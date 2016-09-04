-- this is a lua plugin that is in a subdirectory (tests loading of subdirs)

local function test_echo(args)
	return args; 
end

return {
	echo = test_echo
}

