#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>
#include <stdint.h>

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

