local function echo(args)
	return { msg = args.msg }; 
end

return {
	echo = echo
}

