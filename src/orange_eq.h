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
/*
	This is an event queue for passing system events to connected clients locally. 
	A local program can send event blobs to this queue. 

	An event blob, expressed as json, looks like this: 
	[ "<name>", {..data..} ]
*/

#pragma once

#include <mqueue.h>
#include <stdbool.h>

struct blob; 

struct orange_eq {
	mqd_t mq;  
	struct mq_attr attr; 
	char *buf; 
}; 

int orange_eq_open(struct orange_eq *self, const char *queue_name, bool server); 
int orange_eq_close(struct orange_eq *self); 
int orange_eq_send(struct orange_eq *self, struct blob *in); 
int orange_eq_recv(struct orange_eq *self, struct blob *out); 

