/*
 * Copyright (C) 2016 Martin K. Schröder <mkschreder.uk@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 2.1
 * as published by the Free Software Foundation
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "juci_message.h"

struct ubus_message *ubus_message_new(){
	struct ubus_message *self = calloc(1, sizeof(struct ubus_message)); 
	blob_init(&self->buf, 0, 0); 
	INIT_LIST_HEAD(&self->list); 
	return self; 
}

void ubus_message_delete(struct ubus_message **self){
	blob_free(&(*self)->buf); 
	list_del_init(&(*self)->list); 
	free(*self); 
	*self = 0; 
}
