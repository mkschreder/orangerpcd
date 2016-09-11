#pragma once

char* shell_command(const char *cmd, int *exit_code); 
void timespec_now(struct timespec *t); 
int timespec_before(struct timespec *a, struct timespec *before_b); 
void timespec_from_now_us(struct timespec *t, unsigned long long timeout_us); 
int timespec_expired(struct timespec *ts); 
