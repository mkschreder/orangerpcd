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

#if defined(HAVE_LUA5_2_LUA_H)
#include <lua5.2/lua.h>
#include <lua5.2/lauxlib.h>
#include <lua5.2/lualib.h>
#elif defined(HAVE_LUA5_1_LUA_H)
#include <lua5.1/lua.h>
#include <lua5.1/lauxlib.h>
#include <lua5.1/lualib.h>
#elif defined(HAVE_LUA_H)
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#else
#error "Lua headers not found!"
#endif

#define JUCI_DBG_NONE 0
#define JUCI_DBG_INFO 1
#define JUCI_DBG_DEBUG 2
#define JUCI_DBG_TRACE 3

extern int juci_debug_level; 
#define TRACE(...) { if(juci_debug_level >= JUCI_DBG_TRACE){ printf(__VA_ARGS__); } }
#define DEBUG(...) { if(juci_debug_level >= JUCI_DBG_DEBUG){ printf(__VA_ARGS__); } }
#define INFO(...) { if(juci_debug_level >= JUCI_DBG_INFO){ printf(__VA_ARGS__); } }
#define ERROR(...) { fprintf(stderr, __VA_ARGS__); }


