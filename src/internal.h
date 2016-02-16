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

extern int juci_debug_level; 
#define TRACE(...) { if(juci_debug_level > 2){ printf(__VA_ARGS__); } }
#define DEBUG(...) { if(juci_debug_level > 1){ printf(__VA_ARGS__); } }
#define INFO(...) { if(juci_debug_level > 0){ printf(__VA_ARGS__); } }
#define ERROR(...) { fprintf(stderr, __VA_ARGS__); }


