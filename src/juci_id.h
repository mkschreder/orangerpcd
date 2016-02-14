/*
 * Copyright (C) 2011 Felix Fietkau <nbd@openwrt.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 2.1
 * as published by the Free Software Foundation
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#pragma once

#include <libutype/avl.h>
#include <stdint.h>

struct ubus_id {
	struct avl_node avl;
	uint32_t id;
};

void ubus_id_string_tree_init(struct avl_tree *tree, bool dup); 
void ubus_id_tree_init(struct avl_tree *tree);
bool ubus_id_alloc(struct avl_tree *tree, struct ubus_id *id, uint32_t val);

void ubus_id_free(struct avl_tree *tree, struct ubus_id *id); 
struct ubus_id *ubus_id_find(struct avl_tree *tree, uint32_t id); 

