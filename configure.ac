dnl Process this file with autoconf to produce a configure script
AC_INIT(jucid,0.1,mkschreder.uk@gmail.com)
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_SRCDIR(src/main.c)

AM_INIT_AUTOMAKE

AC_PROG_CC

AC_CHECK_HEADERS([lua5.2/lua.h])
AC_CHECK_HEADERS([lua.h])
AC_CHECK_LIB(lua5.2,lua_close,[LIBLUA_LINK="-llua5.2"]) 
AC_CHECK_LIB(lua,lua_close,[LIBLUA_LINK="-llua"]) 
AC_SUBST(LIBLUA_LINK) 

AC_OUTPUT(Makefile src/Makefile)

