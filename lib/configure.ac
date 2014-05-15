# Autoconf requirements
AC_INIT(eXdbm, [1.3], [Frederic_DOT_Peschanski_AT_lip6_DOT_fr])
AC_REVISION($Revision: 1.1 $)

# information on the package
AC_CONFIG_SRCDIR([eXdbm.c])  # beware of space MACRO_NAME(args) with no space
AC_CONFIG_HEADERS([config.h])

# checks for programs
AC_PROG_RANLIB
AC_ARG_VAR(DATEPROG,[The date program to generate dist archive])
AC_CHECK_PROG([DATEPROG],[date +%m%d%y],[date])
AC_ARG_VAR(GCCMAKEDEP,[The GNU program to track header dependancies])
AC_CHECK_PROG([GCCMAKEDEP],[gccmakedep],[gccmakedep],[echo "No header dependancy tool : "])

# checks for libraries
AC_CHECK_LIB(m,ceil)
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_STRTOD
AC_CHECK_FUNC(strtok)
AC_CHECK_FUNC(atol,[],[AC_MSG_ERROR([Cannot compile without atol])])

# checks for header files
AC_HEADER_STDC

# checks for types

# checks for structures

# checks for compiler characteristics

# checks for library functions

# checks for system services
AC_CONFIG_FILES(Makefile)
AC_OUTPUT