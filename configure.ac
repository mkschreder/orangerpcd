dnl Process this file with autoconf to produce a configure script
AC_INIT(orangerpcd,[0.1.0],mkschreder.uk@gmail.com)
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_SRCDIR(src/main.c)

AC_CONFIG_MACRO_DIR([m4])

m4_pattern_allow([AM_DEFAULT_VERBOSITY])

AM_INIT_AUTOMAKE
AX_CODE_COVERAGE

AX_VALGRIND_CHECK
AC_SUBST(VALGRIND_CHECK_RULES)

LT_INIT 
AC_PROG_CC

AC_CHECK_HEADERS([lua5.1/lua.h],[LIBLUA_LINK="-llua5.1"])
AC_CHECK_HEADERS([lua5.2/lua.h],[LIBLUA_LINK="-llua5.2"])
AC_CHECK_HEADERS([lua.h],[LIBLUA_LINK="-llua"])
AC_SUBST(LIBLUA_LINK) 

AC_CHECK_HEADERS([uci.h],[LIBUCI_LINK="-luci"])
AC_SUBST(LIBUCI_LINK) 

AC_ARG_ENABLE([parallel],
	AC_HELP_STRING([--disable-parallel], [Disables parallel execution of lua scripts. By default the server will handle requests in parallel. Supplying this option makes main thread responsible for handling all requests. Note that this makes your server as slow as the slowest running rpc call. Use this for debugging.]),,enable_parallel=yes)

if test x$enable_parallel = xyes; then 
	CONFIG_CFLAGS="$CONFIG_CFLAGS -DCONFIG_THREADS"; 
fi

AC_SUBST(CONFIG_CFLAGS) 

AC_OUTPUT(Makefile src/Makefile test/Makefile)

