/* AUTOMATICALLY GENERATED; DO NOT EDIT! */ 
#ifndef _G_config_h
#define _G_config_h
#ifdef COHERENT
#include <sys/types.h>
#include <signal.h>
#endif
#define _G_LIB_VERSION "0.63"
#define _G_NAMES_HAVE_UNDERSCORE 0
#define _G_HAVE_ST_BLKSIZE 0
typedef long _G_clock_t;
typedef o_dev_t _G_dev_t;
typedef long _G_fpos_t;
typedef o_gid_t _G_gid_t;
typedef o_ino_t _G_ino_t;
typedef o_mode_t _G_mode_t;
typedef o_nlink_t _G_nlink_t;
typedef long _G_off_t;
typedef long _G_pid_t;
typedef int _G_ptrdiff_t;
typedef o_sigset_t _G_sigset_t;
typedef unsigned _G_size_t;
typedef long _G_time_t;
typedef o_uid_t _G_uid_t;
typedef unsigned short _G_wchar_t;
typedef int /* default */ _G_int32_t;
typedef unsigned int /* default */ _G_uint32_t;
typedef long _G_ssize_t;
typedef char       * _G_va_list;
#define _G_signal_return_type void
#define _G_sprintf_return_type int
#define _G_BUFSIZ  512 
#define _G_FOPEN_MAX  60 
#define _G_FILENAME_MAX 64 
#define _G_NULL 0 /* default */
#define _G_ARGS(ARGLIST) (...)
#define _G_HAVE_ATEXIT 1
#define _G_HAVE_SYS_RESOURCE 0
#define _G_HAVE_SYS_SOCKET 1
#define _G_HAVE_SYS_WAIT 1
#define _G_HAVE_UNISTD 1
#define _G_HAVE_DIRENT 1
#define _G_HAVE_CURSES 1
#define _G_MATH_H_INLINES 0
#endif /* !_G_config_h */
