Lua Plugin API Interface
------------------------

This documentation covers the api interface from lua scripts that are plugins
to RevoRPC server and to the main server executable.

Plugins reside in their own separate containers (lua states). Each plugin also
gets a global environment with functions which are exported by the server and
through which the lua plugins can for example find a session and check user
access. 

::SESSION

	.access(scope, object, method, permission): check session access

::JSON
	.parse(jsonString): parse json and return lua object
	.stringify(luaObject): convert lua object into json string

