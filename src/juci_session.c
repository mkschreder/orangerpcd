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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define _GNU_SOURCE	/* crypt() */

#include <blobpack/blobpack.h>
#include <libutype/avl-cmp.h>
#include <libutype/list.h>
#include <fnmatch.h>
#include <glob.h>
#include <uci.h>
#include <limits.h>
#include <ctype.h>
#include <crypt.h>

#ifdef HAVE_SHADOW
#include <shadow.h>
#else 
#include <sys/types.h>
#include <pwd.h>
#endif

#include "juci_session.h"

static int _generate_sid(juci_sid_t dest){
	unsigned char buf[16] = { 0 };
	FILE *f;
	int i;
	int ret;

	f = fopen("/dev/urandom", "r");
	if (!f)
		return -1;

	ret = fread(buf, 1, sizeof(buf), f);
	fclose(f);

	if (ret < 0)
		return ret;

	for (i = 0; i < sizeof(buf); i++)
		sprintf(dest + (i<<1), "%02x", buf[i]);

	return 0;
}

struct juci_session *juci_session_new(){
	struct juci_session *self = calloc(1, sizeof(struct juci_session)); 
	assert(self); 
	_generate_sid(self->sid); 	

	avl_init(&self->acls, avl_strcmp, true, NULL);
	avl_init(&self->data, avl_strcmp, false, NULL);

	return self; 
}

void juci_session_delete(struct juci_session **self){
	assert(*self); 
	free(*self); 
	*self = NULL; 
}
#define DEBUG(...) printf(__VA_ARGS__)
static struct avl_tree sessions;
static struct blob buf;

static LIST_HEAD(create_callbacks);
static LIST_HEAD(destroy_callbacks);

/*
 * Keys in the AVL tree contain all pattern characters up to the first wildcard.
 * To look up entries, start with the last entry that has a key less than or
 * equal to the method name, then work backwards as long as the AVL key still
 * matches its counterpart in the object name
 */
#define uh_foreach_matching_acl_prefix(_acl, _avl, _obj, _func)		\
	for (_acl = avl_find_le_element(_avl, _obj, _acl, avl);			\
	     _acl;														\
	     _acl = avl_is_first(_avl, &(_acl)->avl) ? NULL :			\
		    avl_prev_element((_acl), avl))

#define uh_foreach_matching_acl(_acl, _avl, _obj, _func)			\
	uh_foreach_matching_acl_prefix(_acl, _avl, _obj, _func)			\
		if (!strncmp((_acl)->object, _obj, (_acl)->sort_len) &&		\
		    !fnmatch((_acl)->object, (_obj), FNM_NOESCAPE) &&		\
		    !fnmatch((_acl)->function, (_func), FNM_NOESCAPE))

static void
juci_session_dump_data(struct juci_session *ses, struct blob *b){
	/*struct juci_session_data *d;

	avl_for_each_element(&ses->data, d, avl) {
		blob_put_field(b, blob_field_type(d->attr), blobmsg_name(d->attr),
				  blobmsg_data(d->attr), blobmsg_data_len(d->attr));
	}*/
}

static void
juci_session_dump_acls(struct juci_session *ses, struct blob *b)
{
	struct juci_session_acl *acl;
	struct juci_session_acl_scope *acl_scope;
	const char *lastobj = NULL;
	const char *lastscope = NULL;
	void *c = NULL, *d = NULL;

	avl_for_each_element(&ses->acls, acl_scope, avl) {
		if (!lastscope || strcmp(acl_scope->avl.key, lastscope))
		{
			if (c) blob_close_table(b, c);
			blob_put_string(b, acl_scope->avl.key); 
			c = blob_open_table(b);
			lastobj = NULL;
		}

		d = NULL;

		avl_for_each_element(&acl_scope->acls, acl, avl) {
			if (!lastobj || strcmp(acl->object, lastobj))
			{
				if (d) blob_close_array(b, d);
				blob_put_string(b, acl->object); 
				d = blob_open_array(b);
			}

			blob_put_string(b, acl->function);
			lastobj = acl->object;
		}

		if (d) blob_close_array(b, d);
	}

	if (c) blob_close_table(b, c);
}

static void
juci_session_to_blob(struct juci_session *ses, struct blob *buf, bool acls)
{
	void *c;

	blob_put_string(buf, "ubus_juci_session"); 
	blob_put_string(buf, ses->sid);
	//blob_put_u32(buf, "timeout", ses->timeout);
	//blob_put_u32(buf, "expires", uloop_timeout_remaining(&ses->t) / 1000);

	if (acls) {
		blob_put_string(buf, "acls"); 
		c = blob_open_table(buf);
		juci_session_dump_acls(ses, buf);
		blob_close_table(buf, c);
	}
	blob_put_string(buf, "data"); 
	c = blob_open_table(buf);
	juci_session_dump_data(ses, buf);
	blob_close_table(buf, c);
}

static void
juci_session_dump(struct juci_session *ses, struct blob *buf){	
	DEBUG("session.dump"); 
	blob_reset(buf);
	juci_session_to_blob(ses, buf, true);
}

static void
rpc_touch_session(struct juci_session *ses){
	//if (ses->timeout > 0)
//		uloop_timeout_set(&ses->t, ses->timeout * 1000);
}

static void 
juci_session_destroy(struct juci_session *ses){
	struct juci_session_acl *acl, *nacl;
	struct juci_session_acl_scope *acl_scope, *nacl_scope;
	struct juci_session_data *data, *ndata;

	//list_for_each_entry(cb, &destroy_callbacks, list)
//		cb->cb(ses, cb->priv);

	//uloop_timeout_cancel(&ses->t);

	avl_for_each_element_safe(&ses->acls, acl_scope, avl, nacl_scope) {
		avl_remove_all_elements(&acl_scope->acls, acl, avl, nacl)
			free(acl);

		avl_delete(&ses->acls, &acl_scope->avl);
		free(acl_scope);
	}

	avl_remove_all_elements(&ses->data, data, avl, ndata)
		free(data);

	avl_delete(&sessions, &ses->avl);
	free(ses);
}
/*
static struct juci_session *
juci_session_create(int timeout)
{
	struct juci_session *ses;
	struct juci_session_cb *cb;

	ses = juci_session_new();

	if (!ses)
		return NULL;

	if (rpc_random(ses->id))
		return NULL;

	ses->timeout = timeout;

	avl_insert(&sessions, &ses->avl);

	rpc_touch_session(ses);

	list_for_each_entry(cb, &create_callbacks, list)
		cb->cb(ses, cb->priv);

	return ses;
}

static struct juci_session *
juci_session_get(const char *id)
{
	struct juci_session *ses;

	ses = avl_find_element(&sessions, id, ses, avl);
	if (!ses)
		return NULL;

	rpc_touch_session(ses);
	return ses;
}

static int
rpc_handle_create(struct ubus_context *ctx, struct ubus_object *obj,
                  struct ubus_request_data *req, const char *method,
                  struct blob_field *msg)
{
	struct juci_session *ses;
	struct blob_field *tb;
	int timeout = RPC_DEFAULT_SESSION_TIMEOUT;
	
	DEBUG("session.create\n"); 
	blobmsg_parse(new_policy, __RPC_SN_MAX, &tb, blob_data(msg), blob_len(msg));
	if (tb)
		timeout = blobmsg_get_u32(tb);

	ses = juci_session_create(timeout);
	if (ses)
		juci_session_dump(ses, ctx, req);

	return 0;
}

static int
rpc_handle_list(struct ubus_context *ctx, struct ubus_object *obj,
                struct ubus_request_data *req, const char *method,
                struct blob_field *msg)
{
	struct juci_session *ses;
	struct blob_field *tb;

	DEBUG("session.list\n"); 
	blobmsg_parse(sid_policy, __RPC_SI_MAX, &tb, blob_data(msg), blob_len(msg));

	if (!tb) {
		avl_for_each_element(&sessions, ses, avl)
			juci_session_dump(ses, ctx, req);
		return 0;
	}

	ses = juci_session_get(blobmsg_data(tb));
	if (!ses)
		return UBUS_STATUS_NOT_FOUND;

	juci_session_dump(ses, ctx, req);

	return 0;
}

static int
uh_id_len(const char *str)
{
	return strcspn(str, "*?[");
}

static int
juci_session_grant(struct juci_session *ses,
                  const char *scope, const char *object, const char *function)
{
	struct juci_session_acl *acl;
	struct juci_session_acl_scope *acl_scope;
	char *new_scope, *new_obj, *new_func, *new_id;
	int id_len;

	if (!object || !function)
		return UBUS_STATUS_INVALID_ARGUMENT;

	acl_scope = avl_find_element(&ses->acls, scope, acl_scope, avl);

	if (acl_scope) {
		uh_foreach_matching_acl_prefix(acl, &acl_scope->acls, object, function) {
			if (!strcmp(acl->object, object) &&
				!strcmp(acl->function, function))
				return 0;
		}
	}

	if (!acl_scope) {
		acl_scope = calloc_a(sizeof(*acl_scope),
		                     &new_scope, strlen(scope) + 1);

		if (!acl_scope)
			return UBUS_STATUS_UNKNOWN_ERROR;

		acl_scope->avl.key = strcpy(new_scope, scope);
		avl_init(&acl_scope->acls, avl_strcmp, true, NULL);
		avl_insert(&ses->acls, &acl_scope->avl);
	}

	id_len = uh_id_len(object);
	acl = calloc_a(sizeof(*acl),
		&new_obj, strlen(object) + 1,
		&new_func, strlen(function) + 1,
		&new_id, id_len + 1);

	if (!acl)
		return UBUS_STATUS_UNKNOWN_ERROR;

	acl->object = strcpy(new_obj, object);
	acl->function = strcpy(new_func, function);
	acl->avl.key = strncpy(new_id, object, id_len);
	avl_insert(&acl_scope->acls, &acl->avl);

	return 0;
}

static int
juci_session_revoke(struct juci_session *ses,
                   const char *scope, const char *object, const char *function)
{
	struct juci_session_acl *acl, *next;
	struct juci_session_acl_scope *acl_scope;
	int id_len;
	char *id;

	acl_scope = avl_find_element(&ses->acls, scope, acl_scope, avl);

	if (!acl_scope)
		return 0;

	if (!object && !function) {
		avl_remove_all_elements(&acl_scope->acls, acl, avl, next)
			free(acl);
		avl_delete(&ses->acls, &acl_scope->avl);
		free(acl_scope);
		return 0;
	}

	id_len = uh_id_len(object);
	id = alloca(id_len + 1);
	strncpy(id, object, id_len);
	id[id_len] = 0;

	acl = avl_find_element(&acl_scope->acls, id, acl, avl);
	while (acl) {
		if (!avl_is_last(&acl_scope->acls, &acl->avl))
			next = avl_next_element(acl, avl);
		else
			next = NULL;

		if (strcmp(id, acl->avl.key) != 0)
			break;

		if (!strcmp(acl->object, object) &&
		    !strcmp(acl->function, function)) {
			avl_delete(&acl_scope->acls, &acl->avl);
			free(acl);
		}
		acl = next;
	}

	if (avl_is_empty(&acl_scope->acls)) {
		avl_delete(&ses->acls, &acl_scope->avl);
		free(acl_scope);
	}

	return 0;
}


static int
rpc_handle_acl(struct ubus_context *ctx, struct ubus_object *obj,
               struct ubus_request_data *req, const char *method,
               struct blob_field *msg)
{
	struct juci_session *ses;
	struct blob_field *tb[__RPC_SA_MAX];
	struct blob_field *attr, *sattr;
	const char *object, *function;
	const char *scope = "ubus";
	int rem1, rem2;

	DEBUG("session.acl\n"); 
	int (*cb)(struct juci_session *ses,
	          const char *scope, const char *object, const char *function);

	blobmsg_parse(acl_policy, __RPC_SA_MAX, tb, blob_data(msg), blob_len(msg));

	if (!tb[RPC_SA_SID])
		return UBUS_STATUS_INVALID_ARGUMENT;

	ses = juci_session_get(blobmsg_data(tb[RPC_SA_SID]));
	if (!ses)
		return UBUS_STATUS_NOT_FOUND;

	if (tb[RPC_SA_SCOPE])
		scope = blobmsg_data(tb[RPC_SA_SCOPE]);

	if (!strcmp(method, "grant"))
		cb = juci_session_grant;
	else
		cb = juci_session_revoke;

	if (!tb[RPC_SA_OBJECTS])
		return cb(ses, scope, NULL, NULL);

	blobmsg_for_each_attr(attr, tb[RPC_SA_OBJECTS], rem1) {
		if (blobmsg_type(attr) != BLOBMSG_TYPE_ARRAY)
			continue;

		object = NULL;
		function = NULL;

		blobmsg_for_each_attr(sattr, attr, rem2) {
			if (blobmsg_type(sattr) != BLOBMSG_TYPE_STRING)
				continue;

			if (!object)
				object = blobmsg_data(sattr);
			else if (!function)
				function = blobmsg_data(sattr);
			else
				break;
		}

		if (object && function)
			cb(ses, scope, object, function);
	}

	return 0;
}
*/
static bool
juci_session_acl_allowed(struct juci_session *ses, const char *scope,
                        const char *obj, const char *fun)
{
	struct juci_session_acl *acl;
	struct juci_session_acl_scope *acl_scope;

	acl_scope = avl_find_element(&ses->acls, scope, acl_scope, avl);

	if (acl_scope) {
		uh_foreach_matching_acl(acl, &acl_scope->acls, obj, fun)
			return true;
	}

	return false;
}
/*
static int
rpc_handle_access(struct ubus_context *ctx, struct ubus_object *obj,
                  struct ubus_request_data *req, const char *method,
                  struct blob_field *msg)
{
	struct juci_session *ses;
	struct blob_field *tb[__RPC_SP_MAX];
	const char *scope = "ubus";
	bool allow;

	blobmsg_parse(perm_policy, __RPC_SP_MAX, tb, blob_data(msg), blob_len(msg));

	if (!tb[RPC_SP_SID])
		return UBUS_STATUS_INVALID_ARGUMENT;

	DEBUG("session.access %s\n", blobmsg_data(tb[RPC_SP_SID])); 
	
	ses = juci_session_get(blobmsg_data(tb[RPC_SP_SID]));
	if (!ses)
		return UBUS_STATUS_NOT_FOUND;

	blob_init(&buf, 0);

	if (tb[RPC_SP_OBJECT] && tb[RPC_SP_FUNCTION])
	{
		if (tb[RPC_SP_SCOPE])
			scope = blobmsg_data(tb[RPC_SP_SCOPE]);

		allow = juci_session_acl_allowed(ses, scope,
		                                blobmsg_data(tb[RPC_SP_OBJECT]),
		                                blobmsg_data(tb[RPC_SP_FUNCTION]));

		blob_put_u8(&buf, "access", allow);
	}
	else
	{
		//juci_session_dump_acls(ses, &buf);
		juci_session_to_blob(ses, &buf, true);
	}
	
	ubus_send_reply(ctx, req, buf.head);

	blob_free(&buf); 
	return 0;
}

static void
juci_session_set(struct juci_session *ses, const char *key, struct blob_field *val)
{
	struct juci_session_data *data;

	data = avl_find_element(&ses->data, key, data, avl);
	if (data) {
		avl_delete(&ses->data, &data->avl);
		free(data);
	}

	data = calloc(1, sizeof(*data) + blob_pad_len(val));
	if (!data)
		return;

	memcpy(data->attr, val, blob_pad_len(val));
	data->avl.key = blobmsg_name(data->attr);
	avl_insert(&ses->data, &data->avl);
}

static int
rpc_handle_set(struct ubus_context *ctx, struct ubus_object *obj,
               struct ubus_request_data *req, const char *method,
               struct blob_field *msg)
{
	struct juci_session *ses;
	struct blob_field *tb[__RPC_SS_MAX];
	struct blob_field *attr;
	int rem;

	DEBUG("session.set\n"); 
	blobmsg_parse(set_policy, __RPC_SS_MAX, tb, blob_data(msg), blob_len(msg));

	if (!tb[RPC_SS_SID] || !tb[RPC_SS_VALUES])
		return UBUS_STATUS_INVALID_ARGUMENT;

	ses = juci_session_get(blobmsg_data(tb[RPC_SS_SID]));
	if (!ses)
		return UBUS_STATUS_NOT_FOUND;

	blobmsg_for_each_attr(attr, tb[RPC_SS_VALUES], rem) {
		if (!blobmsg_name(attr)[0])
			continue;

		juci_session_set(ses, blobmsg_name(attr), attr);
	}

	return 0;
}

static int
rpc_handle_get(struct ubus_context *ctx, struct ubus_object *obj,
               struct ubus_request_data *req, const char *method,
               struct blob_field *msg)
{
	struct juci_session *ses;
	struct juci_session_data *data;
	struct blob_field *tb[__RPC_SG_MAX];
	struct blob_field *attr;
	void *c;
	int rem;

	DEBUG("session.get\n"); 
	blobmsg_parse(get_policy, __RPC_SG_MAX, tb, blob_data(msg), blob_len(msg));

	if (!tb[RPC_SG_SID])
		return UBUS_STATUS_INVALID_ARGUMENT;

	ses = juci_session_get(blobmsg_data(tb[RPC_SG_SID]));
	if (!ses)
		return UBUS_STATUS_NOT_FOUND;

	blob_init(&buf, 0);
	c = blob_open_table(&buf, "values");

	if (tb[RPC_SG_KEYS])
		blobmsg_for_each_attr(attr, tb[RPC_SG_KEYS], rem) {
			if (blobmsg_type(attr) != BLOBMSG_TYPE_STRING)
				continue;

			data = avl_find_element(&ses->data, blobmsg_data(attr), data, avl);
			if (!data)
				continue;

			blob_put_field(&buf, blobmsg_type(data->attr),
					  blobmsg_name(data->attr),
					  blobmsg_data(data->attr),
					  blobmsg_data_len(data->attr));
		}
	else
		juci_session_dump_data(ses, &buf);

	blob_close_table(&buf, c);
	ubus_send_reply(ctx, req, buf.head);

	blob_free(&buf); 
	return 0;
}

static int
rpc_handle_unset(struct ubus_context *ctx, struct ubus_object *obj,
                 struct ubus_request_data *req, const char *method,
                 struct blob_field *msg)
{
	struct juci_session *ses;
	struct juci_session_data *data, *ndata;
	struct blob_field *tb[__RPC_SA_MAX];
	struct blob_field *attr;
	int rem;

	DEBUG("session.unset\n"); 
	blobmsg_parse(get_policy, __RPC_SG_MAX, tb, blob_data(msg), blob_len(msg));

	if (!tb[RPC_SG_SID])
		return UBUS_STATUS_INVALID_ARGUMENT;

	ses = juci_session_get(blobmsg_data(tb[RPC_SG_SID]));
	if (!ses)
		return UBUS_STATUS_NOT_FOUND;

	if (!tb[RPC_SG_KEYS]) {
		avl_remove_all_elements(&ses->data, data, avl, ndata)
			free(data);
		return 0;
	}

	blobmsg_for_each_attr(attr, tb[RPC_SG_KEYS], rem) {
		if (blobmsg_type(attr) != BLOBMSG_TYPE_STRING)
			continue;

		data = avl_find_element(&ses->data, blobmsg_data(attr), data, avl);
		if (!data)
			continue;

		avl_delete(&ses->data, &data->avl);
		free(data);
	}

	return 0;
}

static int
rpc_handle_destroy(struct ubus_context *ctx, struct ubus_object *obj,
                   struct ubus_request_data *req, const char *method,
                   struct blob_field *msg)
{
	struct juci_session *ses;
	struct blob_field *tb;

	DEBUG("session.destroy\n"); 
	blobmsg_parse(sid_policy, __RPC_SI_MAX, &tb, blob_data(msg), blob_len(msg));

	if (!tb)
		return UBUS_STATUS_INVALID_ARGUMENT;

	if (!strcmp(blobmsg_get_string(tb), RPC_DEFAULT_SESSION_ID))
		return UBUS_STATUS_PERMISSION_DENIED;

	ses = juci_session_get(blobmsg_data(tb));
	if (!ses)
		return UBUS_STATUS_NOT_FOUND;

	juci_session_destroy(ses);

	return 0;
}
*/

static bool
rpc_login_test_password(const char *hash, const char *password){
	char *crypt_hash;

	/* password is not set */
	if (!hash || !*hash || !strcmp(hash, "!") || !strcmp(hash, "x"))
	{
		return true;
	}

	/* password hash refers to shadow/passwd */
	else if (!strncmp(hash, "$p$", 3))
	{
#ifdef HAVE_SHADOW
		struct spwd *sp = getspnam(hash + 3);

		if (!sp)
			return false;

		return rpc_login_test_password(sp->sp_pwdp, password);
#else
		struct passwd *pw = getpwnam(hash + 3);

		if (!pw)
			return false;

		return rpc_login_test_password(pw->pw_passwd, password);
#endif
	}

	crypt_hash = crypt(password, hash);

	return !strcmp(crypt_hash, hash);
}

static struct uci_section *
rpc_login_test_login(struct uci_context *uci,
                     const char *username, const char *password)
{
	struct uci_package *p = NULL;
	struct uci_section *s;
	struct uci_element *e;
	struct uci_ptr ptr = { .package = "rpcd" };

	uci_load(uci, ptr.package, &p);

	if (!p)
		return false;

	uci_foreach_element(&p->sections, e)
	{
		s = uci_to_section(e);

		if (strcmp(s->type, "login"))
			continue;

		ptr.section = s->e.name;
		ptr.s = NULL;

		/* test for matching username */
		ptr.option = "username";
		ptr.o = NULL;

		if (uci_lookup_ptr(uci, &ptr, NULL, true))
			continue;

		if (ptr.o->type != UCI_TYPE_STRING)
			continue;

		if (strcmp(ptr.o->v.string, username))
			continue;

		/* If password is NULL, we're restoring ACLs for an existing session,
		 * in this case do not check the password again. */
		if (!password)
			return ptr.s;

		/* test for matching password */
		ptr.option = "password";
		ptr.o = NULL;

		if (uci_lookup_ptr(uci, &ptr, NULL, true))
			continue;

		if (ptr.o->type != UCI_TYPE_STRING)
			continue;

		if (rpc_login_test_password(ptr.o->v.string, password))
			return ptr.s;
	}

	return NULL;
}

static bool
rpc_login_test_permission(struct uci_section *s,
                          const char *perm, const char *group)
{
	const char *p;
	struct uci_option *o;
	struct uci_element *e, *l;

	/* If the login section is not provided, we're setting up acls for the
	 * default session, in this case uncondionally allow access to the
	 * "unauthenticated" access group */
	if (!s) {
		return !strcmp(group, "unauthenticated");
	}

	uci_foreach_element(&s->options, e)
	{
		o = uci_to_option(e);

		if (o->type != UCI_TYPE_LIST)
			continue;

		if (strcmp(o->e.name, perm))
			continue;

		/* Match negative expressions first. If a negative expression matches
		 * the current group name then deny access. */
		uci_foreach_element(&o->v.list, l) {
			p = l->name;

			if (!p || *p != '!')
				continue;

			while (isspace(*++p));

			if (!*p)
				continue;

			if (!fnmatch(p, group, 0))
				return false;
		}

		uci_foreach_element(&o->v.list, l) {
			if (!l->name || !*l->name || *l->name == '!')
				continue;

			if (!fnmatch(l->name, group, 0))
				return true;
		}
	}

	/* make sure that write permission implies read permission */
	if (!strcmp(perm, "read"))
		return rpc_login_test_permission(s, "write", group);

	return false;
}

static void
rpc_login_setup_acl_scope(struct juci_session *ses,
                          struct blob_field *acl_perm,
                          struct blob_field *acl_scope)
{
	struct blob_field *acl_obj, *acl_func;

	/*
	 * Parse ACL scopes in table notation.
	 *
	 *	"<scope>": {
	 *		"<object>": [
	 *			"<function>",
	 *			"<function>",
	 *			...
	 *		]
	 *	}
	 */
	if (blob_field_type(acl_scope) == BLOB_FIELD_TABLE) {
		blob_field_for_each_child(acl_scope, acl_obj) {
			if (blob_field_type(acl_obj) != BLOB_FIELD_ARRAY)
				continue;

			blob_field_for_each_child(acl_obj, acl_func) {
				if (blob_field_type(acl_func) != BLOB_FIELD_STRING)
					continue;

				//juci_session_grant(ses, blobmsg_name(acl_scope),
				 //                      blobmsg_name(acl_obj),
				  //                     blobmsg_data(acl_func));
			}
		}
	}

	/*
	 * Parse ACL scopes in array notation. The permission ("read" or "write")
	 * will be used as function name for each object.
	 *
	 *	"<scope>": [
	 *		"<object>",
	 *		"<object>",
	 *		...
	 *	]
	 */
	else if (blob_field_type(acl_scope) == BLOB_FIELD_ARRAY) {
		blob_field_for_each_child(acl_scope, acl_obj) {
			if (blob_field_type(acl_obj) != BLOB_FIELD_STRING)
				continue;

			/*juci_session_grant(ses, blobmsg_name(acl_scope),
			                       blobmsg_data(acl_obj),
			                       blobmsg_name(acl_perm));
								   */
		}
	}
}

