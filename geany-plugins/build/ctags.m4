AC_DEFUN([GP_CHECK_CTAGS],
[
    GP_ARG_DISABLE([ctags], [yes])
    GP_STATUS_PLUGIN_ADD([ctags], [$enable_ctags])
    AC_CONFIG_FILES([
        ctags/Makefile
        ctags/src/Makefile
    ])
])
