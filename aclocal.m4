dnl
dnl AC_PROMPT_USER_NO_CACHE(VARIABLE,PROMPT,[DEFAULT])
dnl
dnl
AC_DEFUN(AC_PROMPT_USER_NO_CACHE,
[
if test "x$defaults" = "xno"; then
echo $ac_n "$2 ($3): $ac_c"
read tmpinput
if test "$tmpinput" = "" -a "$3" != ""; then
  tmpinput="$3"
fi
eval $1=\"$tmpinput\"
else
tmpinput="$3"
eval $1=\"$tmpinput\"
fi
]) dnl done AC_PROMPT_USER


dnl
dnl AC_PROMPT_USER(VARIABLE,PROMPT,[DEFAULT])
dnl
dnl
AC_DEFUN(AC_PROMPT_USER,
[
MSG_CHECK=`echo "$2" | tail -1`
AC_CACHE_CHECK($MSG_CHECK, ac_cv_user_prompt_$1,
[echo ""
AC_PROMPT_USER_NO_CACHE($1,[$2],$3)
eval ac_cv_user_prompt_$1=\$$1
echo $ac_n "setting $MSG_CHECK to...  $ac_c"
])
]) dnl done AC_PROMPT_USER_FOR_DEFINE

