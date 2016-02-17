/*
	JUCI Backend Websocket API Server

	Copyright (C) 2016 Martin K. Schröder <mkschreder.uk@gmail.com>

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version. (Please read LICENSE file on special
	permission to include this software in signed images). 

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
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
