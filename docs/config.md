Orange Config
=============

The main config file is installed into uci config directory (usually
/etc/config/) and is called 'orange'. Valid sections are outlined below. 

login
-----

This section defines a login user. Passwords are set in /etc/orange/shadow.
Under login you can specify a list of acl files that will be loaded for that
particular user (note that this is not individual acls, but rather acl files
that contain actual acl definitions). All acls are loaded from
/usr/lib/orange/acl/ directory. Wildcards are allowed.  

````
config login admin
	list acls 'juci*'
	list acls 'user-admin'
````

The above section defines a user 'admin' and first adds all acl files that
start with 'juci' (juci acl files in this case) and the second line adds
special acls that are defined for admin user on the system. You can add as many
acls as you like. The order of acls matter in this case because we can 'negate'
acls in subsequent files (ie disable them) by using the '!' character in the
acl file (see documentation about acls). 

