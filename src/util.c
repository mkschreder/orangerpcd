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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>
#include <stdint.h>
#include <time.h>
#include "util.h"

char* shell_command(const char *cmd, int *exit_code){
	FILE *pipe = 0;
	char buffer[256];
	
	int stdout_buf_size = 255; 
	char *stdout_buf = malloc(stdout_buf_size);
	memset(stdout_buf, 0, stdout_buf_size);

	if ((pipe = popen(cmd, "r"))){
		char *ptr = stdout_buf;
		while(fgets(buffer, sizeof(buffer), pipe)){
			int len = strlen(buffer);
			int consumed = (ptr - stdout_buf);
			if((stdout_buf_size - consumed) < len) {
				size_t size = stdout_buf_size * 2; // careful!
				stdout_buf = realloc(stdout_buf, size);
				if(!stdout_buf) {
					fprintf(stderr, "Could not allocate buffer of size %u\n", (uint32_t)size);
					return NULL; 
				}
				// zero the new space
				memset(stdout_buf + stdout_buf_size, 0, size - stdout_buf_size);
				stdout_buf_size = size;
				// update ptr to point to new buffer 
				ptr = stdout_buf + consumed;
			}
			strcpy(ptr, buffer);
			ptr+=len;
		}

		*exit_code = ((pclose(pipe)) & 0xff00) >> 8;

		// strip all new lines at the end of buffer
		ptr--;
		while(ptr > stdout_buf && *ptr == '\n'){
			*ptr = 0;
			ptr--;
		}

		if (ptr != stdout_buf)
			return (char*)stdout_buf;
		else
			return NULL;
	} else {
		return NULL;
	}
}

void timespec_now(struct timespec *t){
	clock_gettime(CLOCK_REALTIME, t); 
}

void timespec_from_now_us(struct timespec *t, unsigned long long timeout_us){
	timespec_now(t); 
	// figure out the exact timeout we should wait for the conditional 
	unsigned long long nsec = t->tv_nsec + timeout_us * 1000UL; 
	unsigned long long sec = nsec / 1000000000UL; 

	t->tv_nsec = nsec - sec * 1000000000UL; 
	t->tv_sec += sec; 
}

int timespec_before(struct timespec *a, struct timespec *before_b){
	if (a->tv_sec == before_b->tv_sec)
		return a->tv_nsec < before_b->tv_nsec;
	return a->tv_sec < before_b->tv_sec;
}	
	
