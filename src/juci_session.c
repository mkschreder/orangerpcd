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
#include <string.h>
#define _GNU_SOURCE	/* crypt() */

#include <blobpack/blobpack.h>
#include <libutype/avl-cmp.h>
#include <libutype/list.h>
#include <fnmatch.h>
#include <glob.h>
#include <uci.h>
#include <limits.h>
#include <ctype.h>
#include <crypt.h>

#ifdef HAVE_SHADOW
#include <shadow.h>
#else 
#include <sys/types.h>
#include <pwd.h>
#endif

#include "internal.h"

#include "juci_session.h"

struct juci_session_data {
	struct avl_node avl;
	struct blob_field *attr;
};

struct juci_session_acl_scope {
	struct avl_node avl;
	struct avl_tree acls;
};

struct juci_session_acl {
	struct avl_node avl;
	const char *object;
	const char *function;
	int sort_len;
};

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

struct juci_session *juci_session_new(struct juci_user *user){
	struct juci_session *self = calloc(1, sizeof(struct juci_session)); 
	assert(self); 
	
	_generate_sid(self->sid); 

	self->avl.key = self->sid; 

	avl_init(&self->acl_scopes, avl_strcmp, false, NULL);
	avl_init(&self->data, avl_strcmp, false, NULL);

	self->user = user; 

	return self; 
}

void juci_session_delete(struct juci_session **self){
	assert(*self); 
	free(*self); 
	*self = NULL; 
}

bool juci_session_can_access(struct juci_session *self, const char *scope, const char *object, const char *method){
	assert(self && scope && object && method); 
	DEBUG("check session access to %s %s %s\n", scope, object, method); 
	return true; 
}

