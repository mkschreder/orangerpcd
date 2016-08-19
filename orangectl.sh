#!/bin/bash

SHADOW=/etc/orange/shadow

if [ $(whoami) != "root" ]; then
	echo "Need to be root!"; 
	exit
fi

while (( "$#" )); do
	if [ "$1" == "passwd" ]; then
		if [ "" != $(grep "^$2" ${SHADOW} | cut -f 1 -d' ') ]; then 
			SHA1SUM=$(printf "$3" | sha1sum | cut -f 1 -d' ')
			sed -i "s/^$2.*$/$2 ${SHA1SUM}/g" ${SHADOW}
			echo "Password for user $2 changed!"; 
		else 
			echo "User does not exist!"
		fi
	fi
	shift 
done
