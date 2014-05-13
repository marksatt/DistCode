AC_PREREQ(2.58)
AC_INIT(dmucs, 0.6.1, vic.norman@gmail.com)
AM_INIT_AUTOMAKE([-Wall -Werror])

AC_CONFIG_SRCDIR([dmucs_msg.h])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_LIBTOOL

# Checks for libraries.
AC_CHECK_LIB([nsl], [gethostbyname])
AC_CHECK_LIB([nsl], [gethostbyaddr_r])
AC_CHECK_LIB([socket], [socket])
AC_CHECK_LIB([pthread], [pthread_mutex_lock])

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h netdb.h netinet/in.h sys/socket.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE

# Checks for library functions.
AC_FUNC_FORK
AC_HEADER_STDC
AC_CHECK_FUNCS([gethostbyaddr gethostbyaddr_r gethostbyname gethostname inet_ntoa putenv strerror])

dnl =================================================================
dnl Support for thread-safe versions of gethostbyaddr, gethostbyname,
dnl gmtime and localtime
dnl =================================================================

dnl Next line needed to find the gethost*_r functions on Solaris
AC_CHECK_LIB(nsl, gethostbyname)

AC_CHECK_FUNC(gethostbyaddr_r, [
  AC_MSG_CHECKING([signature of gethostbyaddr_r])
  AC_TRY_COMPILE([
#   include <netdb.h>
  ], [
    struct hostent *h, *hp;
    char *a, *b;
    int l, bl, t, e;
    (void) gethostbyaddr_r(a, l, t, h, b, bl, &hp, &e)
  ], [
    AC_DEFINE(HAVE_GETHOSTBYADDR_R_8_ARGS,1,
      [Define if gethostbyaddr_r() exists and takes 8 arguments])
    AC_MSG_RESULT([8 args])
  ], [
    AC_TRY_COMPILE([
#     include <netdb.h>
    ], [
      struct hostent *h;
      char *a, *b;
      int l, bl, t, e;
      (void) gethostbyaddr_r(a, l, t, h, b, bl, &e)
    ], [
      AC_DEFINE(HAVE_GETHOSTBYADDR_R_7_ARGS,1,
      [Define if gethostbyaddr_r() exists and takes 7 arguments])
      AC_MSG_RESULT([7 args])
    ], [
      AC_TRY_COMPILE([
#       include <netdb.h>
      ], [
        struct hostent_data *d;
        struct hostent *h;
        char a,
        int l, t;
        (void) gethostbyaddr_r(a, l, t, h, d)
      ], [
        AC_DEFINE(HAVE_GETHOSTBYADDR_R_5_ARGS,1,
      [Define if gethostbyaddr_r() exists and takes 5 arguments])
        AC_MSG_RESULT([5 args])
      ], [
        AC_MSG_RESULT(unrecognised)
      ])
    ])
  ])
], [
  AC_MSG_RESULT(no)
])

dnl FreeBSD -pthread check - Jonathan McDowell
AC_DEFUN([AC_PTHREAD_FREEBSD],
 [AC_MSG_CHECKING([if we need -pthread for threads])
          AC_CACHE_VAL(ac_ldflag_pthread,
                       [ac_save_LDFLAGS="$LDFLAGS"
                        LDFLAGS="-pthread $LDFLAGS"
                        AC_TRY_LINK([char pthread_create();],
                                    pthread_create();,
                                    eval "ac_ldflag_pthread=yes",
                                    eval "ac_ldflag_pthread=no"),
                                    LIBS="$ac_save_LDFLAGS"
                       ])
          if eval "test \"`echo $ac_ldflag_pthread`\" = yes"; then
             AC_MSG_RESULT(yes)
          else
            AC_MSG_RESULT(no)
          fi
        ])

AC_PTHREAD_FREEBSD


dnl @synopsis LIB_SOCKET_NSL
dnl
dnl This macro figures out what libraries are required on this platform
dnl to link sockets programs.
dnl
dnl The common cases are not to need any extra libraries, or to need
dnl -lsocket and -lnsl. We need to avoid linking with libnsl unless we
dnl need it, though, since on some OSes where it isn't necessary it
dnl will totally break networking. Unisys also includes gethostbyname()
dnl in libsocket but needs libnsl for socket().
dnl
dnl @category Misc
dnl @author Russ Allbery <rra@stanford.edu>
dnl @author Stepan Kasal <kasal@ucw.cz>
dnl @author Warren Young <warren@etr-usa.com>
dnl @version 2005-09-06
dnl @license AllPermissive
dnl (Got this code from autoconf-archive.cryp.to.)
AC_DEFUN([LIB_SOCKET_NSL],
[
      AC_SEARCH_LIBS([gethostbyname], [nsl])
      AC_SEARCH_LIBS([socket], [socket], [], [
              AC_CHECK_LIB([socket], [socket], [LIBS="-lsocket -lnsl $LIBS"],
              [], [-lnsl])])
])

LIB_SOCKET_NSL

AC_CONFIG_FILES([Makefile COSMIC/Makefile])
AC_OUTPUT
