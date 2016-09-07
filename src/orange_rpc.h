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
#pragma once

#include "orange_server.h"
#include <pthread.h>

struct orange; 

struct orange_rpc{
	struct blob buf; 
	struct blob out; 
	orange_server_t server; 
	struct orange *ctx; 
	unsigned long long timeout_us; 
	pthread_t *threads; 
	pthread_mutex_t lock; 
	unsigned int num_workers; 
	int shutdown; 
}; 

void orange_rpc_init(struct orange_rpc *self, orange_server_t server, struct orange *ctx, unsigned long long timeout_us, unsigned int num_workers); 
void orange_rpc_deinit(struct orange_rpc *self); 
bool orange_rpc_running(struct orange_rpc *self); 
