#!/bin/bash

SHADOW=/etc/orange/shadow
HOOKS=/usr/lib/orange/hooks/

if [ $(whoami) != "root" ]; then
	echo "Need to be root!"; 
	exit
fi

if [ ! -d ${HOOKS} ]; then mkdir -p ${HOOKS}; fi
chown root:root ${HOOKS}
chmod 600 ${HOOKS}

while (( "$#" )); do
	if [ "$1" == "passwd" ]; then
		if [ "" != $(grep "^$2" ${SHADOW} | cut -f 1 -d' ') ]; then 
			SHA1SUM=$(printf "$3" | sha1sum | cut -f 1 -d' ')
			sed -i "s/^$2.*$/$2 ${SHA1SUM}/g" ${SHADOW}
			echo "Password for user $2 changed!"; 
			for file in $(find ${HOOKS}/ -name "*on_passwd_changed*"); do . ${file} $2 $3; done
		else 
			echo "User does not exist!"
		fi
	fi
	shift 
done
