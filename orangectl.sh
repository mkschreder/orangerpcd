#!/bin/sh

SHADOW=/etc/orange/shadow
HOOKS=/usr/lib/orange/hooks/

CONFIG=orange

if [ "$(which whoami)" != "" ] && [ $(whoami) != "root" ]; then
	echo "Need to be root!"; 
	exit
fi

fixup_env() {
	mkdir -p /etc/orange/
	if [ ! -d ${HOOKS} ]; then mkdir -p ${HOOKS}; fi
	if [ ! -f ${SHADOW} ]; then touch ${SHADOW}; fi

	chown root:root ${SHADOW}
	chown root:root ${HOOKS}
	chmod 600 ${HOOKS}
}

ensure_consistency(){
	if [ ! "$(which uci)" ]; then return; fi
	# ensure that all the users that are in shadow file are also in the uci file
	for user in $(cut -f 1 -d' ' ${SHADOW}); do
		if [ "$(uci get ${CONFIG}.$user)" ]; then continue; fi
		uci add ${CONFIG} login
		uci rename ${CONFIG}.@login[-1]=$user
		uci commit
	done
}

# check if user exists
# 1: username
check_user(){
	if [ "$(grep "^$1" ${SHADOW} | cut -f 1 -d' ')" ]; then 
		echo "$1"
	fi 
}

fixup_env
ensure_consistency

if [ "$#" = "0" ]; then 
	echo "Usage: orangectl <command> [args..]"
	echo ""
	echo "   - adduser <username> - adds a user"
	echo "   - rmuser <username|*> - remove a user or all users"
	echo "   - passwd <username> <password> - set a new password for a user"
	echo "   - addacl <username> <acl> - add an access control permission group to a user (wildcards allowed)"
	echo "        Note that <acl> here is matched against acl files, not actual permissions inside them." 
	echo "   - rmacl <username> <acl|*> - remove acl or all acls. Wildcards not implemented."
	echo ""
fi 

case $1 in 
	passwd)
		# ensure that there is a line in the shadow file for this user
		if [ "" != "$(grep "^$2" ${SHADOW} | cut -f 1 -d' ')" ]; then 
			if [ "$(which sha1sum)" = "" ]; then 
				echo "No sha1sum utility present on this system!"; 
				exit 1; 
			fi
			SHA1SUM=$(printf "$3" | sha1sum | cut -f 1 -d' ')
			sed -i "s/^$2.*$/$2 ${SHA1SUM}/g" ${SHADOW}
			echo "Password for user $2 changed!"; 
			for file in $(find ${HOOKS}/ -name "*on_passwd_changed*"); do . ${file} $2 $3; done
		else 
			echo "User does not exist!"
		fi
	;;
	adduser)
		if [ ! "$2" ]; then 
			echo "No username specified!"
			exit 1
		fi
		if [ $(check_user $2) ]; then
			echo "User already exists!"; 
			exit 1; 
		fi
		if [ $(which uci) ]; then
			uci -q add ${CONFIG} login > /dev/null
			uci rename ${CONFIG}.@login[-1]=$2
			uci commit
		fi
		echo "$2 -" >> ${SHADOW}
	;;
	rmuser)
		if [ ! "$(which uci)" ]; then exit 0; fi
		if [ "$2" = "*" ]; then
			while test "$(uci -q get orange.@login[0])"; do
				uci delete orange.@login[0]
				uci commit
			done
			echo "" > ${SHADOW}
		else
			uci delete orange.$2; 
			uci commit
			sed -i '/^$2[:space:].*/d' ${SHADOW}
		fi
	;;
	addacl)
		if [ ! "$2" ] || [ ! "$3" ]; then
			echo "Usage: addacl <user> <acl>"; 
			exit 1; 
		fi
		if [ ! $(check_user $2) ]; then
			echo "User does not exist!"
			exit 1; 
		fi
		if [ ! $(which uci) ]; then
			exit 0; 
		fi
		uci add_list ${CONFIG}.$2.acls=$3
		uci commit
	;;
	rmacl)
		if [ ! "$2" ] || [ ! "$3" ]; then
			echo "Usage: rmacl <user> [<acl>|*]"; 
			exit 1; 
		fi
		if [ ! $(check_user $2) ]; then
			echo "User does not exist!"
			exit 1; 
		fi
		if [ ! $(which uci) ]; then
			exit 0; # exit with success. Nothing to do. 
		fi
		if [ "$3" = "*" ]; then
			uci delete ${CONFIG}.$2
		else
			uci del_list ${CONFIG}.$2.acls=$3
		fi
		uci commit
	;;
esac

#while test "$#"; do
#	if [ "$#" -gt 0 ]; then shift; else break; fi
#done
