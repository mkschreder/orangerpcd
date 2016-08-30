#!/bin/sh

SHADOW=/etc/orange/shadow
HOOKS=/usr/lib/orange/hooks/

if [ "$(which whoami)" != "" ] && [ $(whoami) != "root" ]; then
	echo "Need to be root!"; 
	exit
fi

if [ ! -d ${HOOKS} ]; then mkdir -p ${HOOKS}; fi
if [ ! -f ${SHADOW} ]; then touch ${SHADOW}; fi

chown root:root ${SHADOW}
chown root:root ${HOOKS}
chmod 600 ${HOOKS}

while test "$#"; do
	if [ "$1" = "passwd" ]; then
		if [ "" != "$(grep "^$2" ${SHADOW} | cut -f 1 -d' ')" ]; then 
			if [ "$(which sha1sum)" = "" ]; then 
				echo "No sha1sum utility present on this system!"; 
				exit 1; 
			fi
			SHA1SUM=$(printf "$3" | sha1sum | cut -f 1 -d' ')
			sed -i "s/^$2.*$/$2 ${SHA1SUM}/g" ${SHADOW}
			echo "Password for user $2 changed!"; 
			for file in $(find ${HOOKS}/ -name "*on_passwd_changed*"); do . ${file} $2 $3; done
			exit 0
		else 
			echo "User does not exist!"
		fi
	elif [ "$1" = "adduser" ]; then
		if [ "$2" = "" ]; then 
			echo "No username specified!"
			exit 1
		fi
		if [ "" != "$(grep "^$2" ${SHADOW} | cut -f 1 -d' ')" ]; then
			echo "User already exists!"; 
			exit 1; 
		fi
		echo "$2 -" >> ${SHADOW}
	fi
	if [ "$#" -gt 0 ]; then shift; else break; fi
done
