
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
