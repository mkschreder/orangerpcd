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
* FILE ............... src/orange_uci.c
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
#if 0
#ifdef HAVE_UCI_H
#include <uci.h>
#else
#error "You need libuci installed to build this file!"
#endif

#include <blobpack/blobpack.h>

static void _uci_option_to_blob(struct uci_option *o, struct blob *buf){
	struct uci_element *e;

	switch (o->type){
	case UCI_TYPE_STRING:
		blob_put_string(buf, o->v.string);
		break;
	case UCI_TYPE_LIST: {
		blob_offset_t c = blob_open_array(buf);

		uci_foreach_element(&o->v.list, e)
			blob_put_string(buf, e->name);

		blob_close_array(buf, c);
	} break;
	default:
		break;
	}
}

static void _uci_section_to_blob(struct uci_section *s, const char *name, int index, struct blob *buf){
	struct uci_option *o;
	struct uci_element *e;

	blob_offset_t c = blob_open_table(buf);

	blob_put_string(buf, ".anonymous"); 
	blob_put_bool(buf, s->anonymous); 
	blob_put_string(buf, ".type"); 
	blob_put_string(buf, s->type);
	blob_put_string(buf, ".name"); 
	blob_put_string(buf, s->e.name);

	if (index >= 0) {
		blob_put_string(buf, ".index"); 
		blob_put_int(buf, index);
	}

	uci_foreach_element(&s->options, e){
		o = uci_to_option(e);
		blob_put_string(buf, o->e.name); 
		_uci_option_to_blob(o, buf);
	}

	blob_close_table(buf, c);
}

static void _uci_config_to_blob(struct uci_package *p, struct blob *buf){
	struct uci_element *e;
	int i = 0;

	blob_offset_t c = blob_open_table(buf);

	uci_foreach_element(&p->sections, e){
		_uci_section_to_blob(uci_to_section(e), e->name, i, buf);
		i++;
	}

	blob_close_table(buf, c);
}

int orange_uci_load_config(const char *config, struct blob *buf){
	struct uci_package *p = NULL; 
	struct uci_ptr ptr = { .package = config }; 
	struct uci_context *uci = uci_alloc_context(); 
	assert(uci); 
	
	uci_load(uci, ptr.package, &p); 
	
	if(!p) return -EACCES; 
	
	_uci_config_to_blob(p, buf); 

	uci_free_context(uci); 
	return 0; 
}
#endif
