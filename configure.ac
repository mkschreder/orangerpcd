dnl Process this file with autoconf to produce a configure script
AC_INIT(oranged,[0.1.0],mkschreder.uk@gmail.com)
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_SRCDIR(src/main.c)

AM_INIT_AUTOMAKE

AC_PROG_CC

AC_CHECK_HEADERS([lua5.1/lua.h],[LIBLUA_LINK="-llua5.1"])
AC_CHECK_HEADERS([lua5.2/lua.h],[LIBLUA_LINK="-llua5.2"])
AC_CHECK_HEADERS([lua.h],[LIBLUA_LINK="-llua"])
AC_SUBST(LIBLUA_LINK) 

AC_CHECK_HEADERS([uci.h],[LIBUCI_LINK="-luci"])
AC_SUBST(LIBLUA_LINK) 

AC_OUTPUT(Makefile src/Makefile)

