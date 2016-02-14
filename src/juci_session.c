/*
	JUCI Backend Server

	Copyright (C) 2016 Martin K. Schröder <mkschreder.uk@gmail.com>

	Based on code by: 
	Copyright (C) 2013 Felix Fietkau <nbd@openwrt.org>
	Copyright (C) 2013-2014 Jo-Philipp Wich <jow@openwrt.org>

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "juci_session.h"

static int _generate_sid(juci_sid_t dest){
	unsigned char buf[16] = { 0 };
	FILE *f;
	int i;
	int ret;

	f = fopen("/dev/urandom", "r");
	if (!f)
		return -1;

	ret = fread(buf, 1, sizeof(buf), f);
	fclose(f);

	if (ret < 0)
		return ret;

	for (i = 0; i < sizeof(buf); i++)
		sprintf(dest + (i<<1), "%02x", buf[i]);

	return 0;
}

struct juci_session *juci_session_new(){
	struct juci_session *self = calloc(1, sizeof(struct juci_session)); 
	assert(self); 
	_generate_sid(self->sid); 	
	return self; 
}

void juci_session_delete(struct juci_session **self){
	assert(*self); 
	free(*self); 
	*self = NULL; 
}
