/* Replacement for tzfile.h */
#define DAYSPERNYEAR 365
#define SECSPERDAY 86400

/* BSD-specific function */
#ifndef HAVE_STRMODE
void strmode (mode_t mode, char *bp);
#endif /* !HAVE_STRMODE */

/* Unclassified */

#ifndef HAVE_USER_FROM_UID
char *user_from_uid (uid_t uid, int ignored);
#endif /* !HAVE_USER_FROM_UID */

#ifndef HAVE_GROUP_FROM_GID
char *group_from_gid (gid_t gid, int ignored);
#endif /* !HAVE_GROUP_FROM_GID */

/* SYSV does not define dirfd() */
#ifndef HAVE_DIRFD
# define dirfd(dir) ((dirp)->dd_fd)
#endif

#ifndef _D_EXACT_NAMLEN
# define _D_EXACT_NAMLEN(d) (strlen(d->d_name) + 1)
#endif
