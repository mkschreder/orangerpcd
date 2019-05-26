/** :ms-top-comment
* +-------------------------------------------------------------------------------+
* |                      ____                  _ _     _                          |
* |                     / ___|_      _____  __| (_)___| |__                       |
* |                     \___ \ \ /\ / / _ \/ _` | / __| '_ \                      |
* |                      ___) \ V  V /  __/ (_| | \__ \ | | |                     |
* |                     |____/ \_/\_/ \___|\__,_|_|___/_| |_|                     |
* |                                                                               |
* |               _____           _              _     _          _               |
* |              | ____|_ __ ___ | |__   ___  __| | __| | ___  __| |              |
* |              |  _| | '_ ` _ \| '_ \ / _ \/ _` |/ _` |/ _ \/ _` |              |
* |              | |___| | | | | | |_) |  __/ (_| | (_| |  __/ (_| |              |
* |              |_____|_| |_| |_|_.__/ \___|\__,_|\__,_|\___|\__,_|              |
* |                                                                               |
* |                       We design hardware and program it                       |
* |                                                                               |
* |               If you have software that needs to be developed or              |
* |                      hardware that needs to be designed,                      |
* |                               then get in touch!                              |
* |                                                                               |
* |                            info@swedishembedded.com                           |
* +-------------------------------------------------------------------------------+
*
*                       This file is part of TheBoss Project
*
* FILE ............... src/orange.h
* AUTHOR ............. Martin K. Schröder
* VERSION ............ Not tagged
* DATE ............... 2019-05-26
* GIT ................ https://github.com/mkschreder/orangerpcd
* WEBSITE ............ http://swedishembedded.com
* LICENSE ............ Swedish Embedded Open Source License
*
*          Copyright (C) 2014-2019 Martin Schröder. All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy of
* this software and associated documentation files (the "Software"), to deal in
* the Software without restriction, including without limitation the rights to
* use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
* the Software, and to permit persons to whom the Software is furnished to do so,
* subject to the following conditions:
*
* The above copyright notice and this text, in full, shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
* FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
* COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
* IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
* CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**/
#pragma once

#include <blobpack/blobpack.h>
#include <utype/avl.h>
#include <utype/avl-cmp.h>
#include <pthread.h>

#include "orange_session.h"
#include "json_check.h"

struct orange_message; 

struct orange {
	struct avl_tree objects; 
	struct avl_tree sessions; 
	struct avl_tree users; 
	
	char *plugin_path; 	
	char *pwfile; 
	char *acl_path; 

	pthread_mutex_t lock; 
}; 

struct orange* orange_new(const char *plugin_path, const char *pwfile, const char *acl_dir); 
void orange_delete(struct orange **_self); 

void orange_add_user(struct orange *self, struct orange_user **user); 

int orange_login_plaintext(struct orange *self, const char *username, const char *password, struct orange_sid *sid); 
int orange_login(struct orange *self, const char *username, const char *challenge, const char *response, struct orange_sid *new_sid); 
int orange_logout(struct orange *self, const char *sid); 
bool orange_session_is_valid(struct orange *self, const char *sid); 
//struct orange_session* orange_find_session(struct orange *self, const char *sid); 
int orange_list(struct orange *self, const char *sid, const char *path, struct blob *out); 

int orange_call(struct orange *self, const char *sid, const char *object, const char *method, const struct blob_field *args, struct blob *out); 
  

