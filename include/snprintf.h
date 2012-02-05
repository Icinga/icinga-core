/* include/snprintf.h. */
#ifndef INCLUDE_SNPRINTF_h__
#define INCLUDE_SNPRINTF_h__

#ifdef SNPRINTF_BROKEN
/* snprintf and friends are known broken at least in solaris
 *  * we assume functions are not there to take local one
 *   */
#undef HAVE_SNPRINTF
#undef HAVE_VSNPRINTF
#undef HAVE_C99_VSNPRINTF
#undef HAVE_ASPRINTF
#undef HAVE_VASPRINTF
#undef HAVE_VA_COPY
#endif

/* prototypes  */
#if !defined(HAVE_C99_VSNPRINTF)
int smb_vsnprintf(char *str, size_t count, const char *fmt, va_list args);
#define vsnprintf smb_vsnprintf
int smb_snprintf(char *str, size_t count, const char *fmt, ...);
#define snprintf smb_snprintf
#endif

#ifndef HAVE_VASPRINTF
int vasprintf(char **ptr, const char *format, va_list ap);
#endif
#ifndef HAVE_ASPRINTF
int asprintf(char **ptr, const char *format, ...);
#endif

#endif /* SNPRINTF */

