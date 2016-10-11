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

#include <fcntl.h>
#include <sys/stat.h>
#include <blobpack/blobpack.h>

#include "orange_eq.h"
#include "util.h"

int orange_eq_open(struct orange_eq *self, const char *queue_name, bool server){
	memset(self, 0, sizeof(struct orange_eq)); 
	if(!queue_name) queue_name = "/orangerpcd-events"; 
	if(server){
		mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
		self->mq = mq_open(queue_name, O_RDONLY | O_CREAT, mode, NULL); 
	} else {
		self->mq = mq_open(queue_name, O_WRONLY); 
	}
	if(self->mq == -1) return -1; 
	mq_getattr(self->mq, &self->attr); 
	self->buf = malloc(self->attr.mq_msgsize); 
	if(!self->buf) return -ENOMEM; 
	return 0; 
}

int orange_eq_close(struct orange_eq *self){
	if(self->mq != -1){
		mq_close(self->mq);
		self->mq = -1; 
	}
	if(self->buf) free(self->buf); 
	memset(&self->attr, 0, sizeof(self->attr)); 
	return 0; 
}

int orange_eq_send(struct orange_eq *self, struct blob *in){
	if(!self->buf || self->mq == -1) return -EINVAL; 
	return mq_send(self->mq, (void*)blob_head(in), blob_size(in), 0); 
}

int orange_eq_recv(struct orange_eq *self, struct blob *out){
	if(!self->buf || self->mq == -1) return -EINVAL; 
	struct timespec ts; 
	timespec_from_now_us(&ts, 5000000UL); 
	int rsize = mq_timedreceive(self->mq, self->buf, self->attr.mq_msgsize, NULL, &ts);  
	if(rsize <= 0) return -EAGAIN; 
	blob_init(out, self->buf, rsize); 
	return 1; 
}

