/*-
 * Copyright (c) 1983, 1988, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
static char copyright[] =
"@(#) Copyright (c) 1983, 1988, 1989, 1993\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)rlogind.c	8.2 (Berkeley) 4/28/95";
#endif /* not lint */

/*
 * relmote login server:  the following data is sent across the network
 * connection by the rcmd() function that the rlogin client uses:
 *	\0
 *	remuser\0
 *	locuser\0
 *	terminal_type/speed\0
 *	data
 * Define OLD_LOGIN for compatibility with the 4.2BSD and 4.3BSD /bin/login.
 * If this iisn't defined, a newer protocol is used  whereby rlogind does
 * the user verification.  This newer version of login is on the Berkeley
 * Networking Release 1.0 tape.
 * Alain: we don't use the OLD_LOGIN stuff anymore, by default we'll
 * call do_rlogin() that call ruserok() to do the authentication.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#ifdef HAVE_SYS_FILIO_H
#include <sys/filio.h>
#endif
#include <signal.h>
#include <termios.h>
#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif
#ifdef HAVE_SYS_STREAM_H
#include <sys/stream.h>
#endif
#ifdef HAVE_SYS_TTY_H
#include <sys/tty.h>
#endif
#ifdef HAVE_SYS_PTYVAR_H
#include <sys/ptyvar.h>
#endif

#include <sys/socket.h>
#include <netinet/in.h>
#ifdef HAVE_NETINET_IN_SYSTM_H
#include <netinet/in_systm.h>
#endif
#ifdef HAVE_NETINET_IP_H
#include <netinet/ip.h>
#endif
#include <arpa/inet.h>
#include <netdb.h>

#include <pwd.h>
#include <syslog.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#ifndef TIOCPKT_WINDOW
#define TIOCPKT_WINDOW 0x80
#endif

/* `defaults' for tty settings.  */
#ifndef TTYDEF_IFLAG
#define	TTYDEF_IFLAG	(BRKINT | ISTRIP | ICRNL | IMAXBEL | IXON | IXANY)
#endif
#ifndef TTYDEF_OFLAG
#ifndef OXTABS
#define OXTABS 0
#endif
#define TTYDEF_OFLAG	(OPOST | ONLCR | OXTABS)
#endif
#ifndef TTYDEF_LFLAG
#define TTYDEF_LFLAG	(ECHO | ICANON | ISIG | IEXTEN | ECHOE|ECHOKE|ECHOCTL)
#endif

#ifdef	KERBEROS
#include <kerberosIV/des.h>
#include <kerberosIV/krb.h>
#define	SECURE_MESSAGE "This rlogin session is using DES encryption for all transmissions.\r\n"

AUTH_DAT	*kdata;
KTEXT		ticket;
u_char		auth_buf[sizeof(AUTH_DAT)];
u_char		tick_buf[sizeof(KTEXT_ST)];
Key_schedule	schedule;
int		doencrypt, retval, use_kerberos, vacuous;

#define		ARGSTR			"alnkvx"
#else
#define		ARGSTR			"aln"
#endif	/* KERBEROS */

char	*env[2];
#define	NMAX 30
char	lusername[NMAX+1], rusername[NMAX+1];
static	char term[64] = "TERM=";
#define	ENVSIZE	(sizeof("TERM=")-1)	/* skip null for concatenation */
int	keepalive = 1;
int	check_all = 0;

struct	passwd *pwd;

void	doit __P((int, struct sockaddr_in *));
int	control __P((int, char *, int));
void	protocol __P((int, int));
void	cleanup __P((int));
void	fatal __P((int, char *, int));
int	do_rlogin __P((struct sockaddr_in *));
void	getstr __P((char *, int, char *));
void	setup_term __P((int));
int	do_krb_login __P((struct sockaddr_in *));
void	usage __P((void));
int	local_domain __P((char *));
char	*topdomain __P((char *));

int
main(int argc, char *argv[])
{
	struct sockaddr_in from;
	int ch, fromlen, on;

	openlog("rlogind", LOG_PID | LOG_CONS, LOG_AUTH);

	opterr = 0;
	while ((ch = getopt(argc, argv, ARGSTR)) != EOF)
		switch (ch) {
		case 'a':
			check_all = 1;
			break;
		case 'l':
			{
				extern int __check_rhosts_file;
				/* don't check .rhosts file */
				__check_rhosts_file = 0;
			}
			break;
		case 'n':
			keepalive = 0; /* don't enable SO_KEEPALIVE */
			break;
#ifdef KERBEROS
		case 'k':
			use_kerberos = 1;
			break;
		case 'v':
			vacuous = 1;
			break;
#ifdef CRYPT
		case 'x':
			doencrypt = 1;
			break;
#endif
#endif
		case '?':
		default:
			usage();
			break;
		}
	argc -= optind;
	argv += optind;

#ifdef	KERBEROS
	if (use_kerberos && vacuous) {
		usage();
		fatal(STDERR_FILENO, "only one of -k and -v allowed", 0);
	}
#endif

	/*
	 * We assusme we're incoked by inetd, so the socket that the connection
	 * is on, is open on descriptor 0, 1 and 2.
	 * STD{IN,OUT,ERR}_FILENO
	 *
	 * First get the Internet address o the client process.
	 * This is required for all the authentication we perform.
	 */
	fromlen = sizeof (from);
	if (getpeername(STDIN_FILENO, (struct sockaddr *)&from, &fromlen) < 0) {
		syslog(LOG_ERR,"Can't get peer name of remote host: %m");
		fatal(STDERR_FILENO, "Can't get peer name of remote host", 1);
	}

	on = 1;
	if (keepalive &&
	    setsockopt(STDIN_FILENO, SOL_SOCKET, SO_KEEPALIVE, (char *) &on,
		       sizeof (on)) < 0)
		syslog(LOG_WARNING, "setsockopt (SO_KEEPALIVE): %m");

#if defined (IP_TOS) && defined (IPPROTO_IP) && defined (IPTOS_LOWDELAY)
	on = IPTOS_LOWDELAY;
	if (setsockopt(STDIN_FILENO, IPPROTO_IP, IP_TOS, (char *)&on,
		sizeof(int)) < 0)
		syslog(LOG_WARNING, "setsockopt (IP_TOS): %m");
#endif

	doit(STDIN_FILENO, &from);
}

int	child;
int	netf;
char	line[1024];		/* XXX */
int	confirmed;

struct winsize win = { 0, 0, 0, 0 };


void
doit(int f, struct sockaddr_in *fromp)
{
	int master, pid, on = 1;
	int authenticated = 0;
	register struct hostent *hp;
	char *hostname;
	const char *raw_hostname;
	char c;

	/*
	 * Read the null byte from the client.  This byte is really
	 * written by the rcmd() function as the secondary port number.
	 * However, the rlogin client calls rcmd() specifying a secondary
	 * port of 0, so all that rcmd() sends is the null byte.
	 * We set a timer of 60 seconds to do this read, else we assume
	 * something is wrong.
	 */
	alarm(60);
	read(f, &c, 1);

	if (c != 0)
		exit(1);
#ifdef	KERBEROS
	if (vacuous)
		fatal(f, "Remote host requires Kerberos authentication", 0);
#endif

	alarm(0);
	/*
	 * Try to look up the client's name, given its internet
	 * address, since we use the name for the authentication.
	 */
	fromp->sin_port = ntohs((u_short)fromp->sin_port);
	hp = gethostbyaddr((char *)&fromp->sin_addr, sizeof(struct in_addr),
	    fromp->sin_family);
	if (hp)
		raw_hostname = hp->h_name;
	else
		raw_hostname = inet_ntoa(fromp->sin_addr);
	hostname = malloc (strlen (raw_hostname) + 1);
	if (! hostname)
		fatal (f, "Out of memory", 0);
	strcpy (hostname, raw_hostname);

#ifdef	KERBEROS
	if (use_kerberos) {
		retval = do_krb_login(fromp);
		if (retval == 0)
			authenticated++;
		else if (retval > 0)
			fatal(f, krb_err_txt[retval], 0);
		write(f, &c, 1);
		confirmed = 1;		/* we sent the null! */
	} else
#endif
	{
		if (fromp->sin_family != AF_INET ||
		    fromp->sin_port >= IPPORT_RESERVED ||
		    fromp->sin_port < IPPORT_RESERVED/2) {
			syslog(LOG_NOTICE, "Connection from %s on illegal port",
				inet_ntoa(fromp->sin_addr));
			fatal(f, "Permission denied", 0);
		}
#ifdef IP_OPTIONS
		{
		u_char optbuf[BUFSIZ/3], *cp;
		char lbuf[BUFSIZ], *lp;
		int optsize = sizeof(optbuf), ipproto;
		struct protoent *ip;

		if ((ip = getprotobyname("ip")) != NULL)
			ipproto = ip->p_proto;
		else
			ipproto = IPPROTO_IP;
		if (getsockopt(0, ipproto, IP_OPTIONS, (char *)optbuf,
		    &optsize) == 0 && optsize != 0) {
			lp = lbuf;
			for (cp = optbuf; optsize > 0; cp++, optsize--, lp += 3)
				sprintf(lp, " %2.2x", *cp);
			syslog(LOG_NOTICE,
			    "Connection received using IP options (ignored):%s",
			    lbuf);
			if (setsockopt(0, ipproto, IP_OPTIONS,
			    (char *)NULL, optsize) != 0) {
				syslog(LOG_ERR,
				    "setsockopt IP_OPTIONS NULL: %m");
				exit(1);
			}
		}
		}
#endif
		if (do_rlogin(fromp) == 0)
			authenticated++;
	}
	if (confirmed == 0) {
		write(f, "", 1);
		confirmed = 1;		/* we sent the null! */
	}
#ifdef	KERBEROS
#ifdef	CRYPT
	if (doencrypt)
		(void) des_write(f, SECURE_MESSAGE, sizeof(SECURE_MESSAGE) - 1);
#endif
#endif
	netf = f;

	pid = forkpty(&master, line, NULL, &win);
	if (pid < 0) {
		if (errno == ENOENT)
			fatal(f, "Out of ptys", 0);
		else
			fatal(f, "Forkpty", 1);
	}
	if (pid == 0) {
		if (f > 2)	/* f should always be 0, but... */
			(void) close(f);
		setup_term(0);
#ifdef UTMPX
		/* those system require utmpx entry for login to work */
		{
			char * utmp_ptsid();
			void utmp_init();
			char * ut_id = utmp_ptsid(line, "rl");
			utmp_init(line + sizeof("/dev/") -1, ".rlogin",
				ut_id);
		}
#endif
		if (authenticated) {
#ifdef	KERBEROS
			if (use_kerberos && (pwd->pw_uid == 0))
				syslog(LOG_INFO|LOG_AUTH,
				    "ROOT Kerberos login from %s.%s@%s on %s\n",
				    kdata->pname, kdata->pinst, kdata->prealm,
				    hostname);
#endif
#ifdef SOLARIS
 			execle(PATH_LOGIN, "login", "-p", 
 			    "-h", hostname, term, "-f", "--", lusername, NULL, env); 
#else
			execle(PATH_LOGIN, "login", "-p",
			    "-h", hostname, "-f", "--", lusername, NULL, env);
#endif
		} else {
#ifdef SOLARIS
 			execle(PATH_LOGIN, "login", "-p", 
 			    "-h", hostname, term, "--", lusername, NULL, env);
#else
			execle(PATH_LOGIN, "login", "-p",
			    "-h", hostname, "--", lusername, NULL, env);
#endif
      }       
		fatal(STDERR_FILENO, PATH_LOGIN, 1);
		/*NOTREACHED*/
	}
#ifdef	CRYPT
#ifdef	KERBEROS
	/*
	 * If encrypted, don't turn on NBIO or the des read/write
	 * routines will croak.
	 */

	if (!doencrypt)
#endif
#endif
		ioctl(f, FIONBIO, &on);
	ioctl(master, FIONBIO, &on);
	ioctl(master, TIOCPKT, &on);
	signal(SIGCHLD, cleanup);
	protocol(f, master);
	signal(SIGCHLD, SIG_IGN);
	cleanup(0);
}

char	magic[2] = { 0377, 0377 };
char	oobdata[] = {TIOCPKT_WINDOW};

/*
 * Handle a "control" request (signaled by magic being present)
 * in the data stream.  For now, we are only willing to handle
 * window size changes.
 */
int
control(int pty, char *cp, int n)
{
	struct winsize w;

	if (n < 4+sizeof (w) || cp[2] != 's' || cp[3] != 's')
		return (0);
	oobdata[0] &= ~TIOCPKT_WINDOW;	/* we know he heard */
	memmove(&w, cp+4, sizeof(w));
	w.ws_row = ntohs(w.ws_row);
	w.ws_col = ntohs(w.ws_col);
	w.ws_xpixel = ntohs(w.ws_xpixel);
	w.ws_ypixel = ntohs(w.ws_ypixel);
	(void)ioctl(pty, TIOCSWINSZ, &w);
	return (4+sizeof (w));
}

/*
 * rlogin "protocol" machine.
 */
void
protocol(register int f, register int p)
{
	char pibuf[1024+1], fibuf[1024], *pbp, *fbp;
	register pcc = 0, fcc = 0;
	int cc, nfd, n;
	char cntl;

	/*
	 * Must ignore SIGTTOU, otherwise we'll stop
	 * when we try and set slave pty's window shape
	 * (our controlling tty is the master pty).
	 */
	(void) signal(SIGTTOU, SIG_IGN);
	send(f, oobdata, 1, MSG_OOB);	/* indicate new rlogin */
	if (f > p)
		nfd = f + 1;
	else
		nfd = p + 1;
	if (nfd > FD_SETSIZE) {
		syslog(LOG_ERR, "select mask too small, increase FD_SETSIZE");
		fatal(f, "internal error (select mask too small)", 0);
	}
	for (;;) {
		fd_set ibits, obits, ebits, *omask;

		FD_ZERO(&ebits);
		FD_ZERO(&ibits);
		FD_ZERO(&obits);
		omask = (fd_set *)NULL;
		if (fcc) {
			FD_SET(p, &obits);
			omask = &obits;
		} else
			FD_SET(f, &ibits);
		if (pcc >= 0)
			if (pcc) {
				FD_SET(f, &obits);
				omask = &obits;
			} else
				FD_SET(p, &ibits);
		FD_SET(p, &ebits);
		if ((n = select(nfd, &ibits, omask, &ebits, 0)) < 0) {
			if (errno == EINTR)
				continue;
			fatal(f, "select", 1);
		}
		if (n == 0) {
			/* shouldn't happen... */
			sleep(5);
			continue;
		}
#define	pkcontrol(c)	((c)&(TIOCPKT_FLUSHWRITE|TIOCPKT_NOSTOP|TIOCPKT_DOSTOP))
		if (FD_ISSET(p, &ebits)) {
			cc = read(p, &cntl, 1);
			if (cc == 1 && pkcontrol(cntl)) {
				cntl |= oobdata[0];
				send(f, &cntl, 1, MSG_OOB);
				if (cntl & TIOCPKT_FLUSHWRITE) {
					pcc = 0;
					FD_CLR(p, &ibits);
				}
			}
		}
		if (FD_ISSET(f, &ibits)) {
#ifdef	CRYPT
#ifdef	KERBEROS
			if (doencrypt)
				fcc = des_read(f, fibuf, sizeof(fibuf));
			else
#endif
#endif
				fcc = read(f, fibuf, sizeof(fibuf));
			if (fcc < 0 && errno == EWOULDBLOCK)
				fcc = 0;
			else {
				register char *cp;
				int left, n;

				if (fcc <= 0)
					break;
				fbp = fibuf;

			top:
				for (cp = fibuf; cp < fibuf+fcc-1; cp++)
					if (cp[0] == magic[0] &&
					    cp[1] == magic[1]) {
						left = fcc - (cp-fibuf);
						n = control(p, cp, left);
						if (n) {
							left -= n;
							if (left > 0)
								bcopy(cp+n, cp, left);
							fcc -= n;
							goto top; /* n^2 */
						}
					}
				FD_SET(p, &obits);		/* try write */
			}
		}

		if (FD_ISSET(p, &obits) && fcc > 0) {
			cc = write(p, fbp, fcc);
			if (cc > 0) {
				fcc -= cc;
				fbp += cc;
			}
		}

		if (FD_ISSET(p, &ibits)) {
			pcc = read(p, pibuf, sizeof (pibuf));
			pbp = pibuf;
			if (pcc < 0 && errno == EWOULDBLOCK)
				pcc = 0;
			else if (pcc <= 0)
				break;
			else if (pibuf[0] == 0) {
				pbp++, pcc--;
#ifdef	CRYPT
#ifdef	KERBEROS
				if (!doencrypt)
#endif
#endif
					FD_SET(f, &obits);	/* try write */
			} else {
				if (pkcontrol(pibuf[0])) {
					pibuf[0] |= oobdata[0];
					send(f, &pibuf[0], 1, MSG_OOB);
				}
				pcc = 0;
			}
		}
		if ((FD_ISSET(f, &obits)) && pcc > 0) {
#ifdef	CRYPT
#ifdef	KERBEROS
			if (doencrypt)
				cc = des_write(f, pbp, pcc);
			else
#endif
#endif
				cc = write(f, pbp, pcc);
			if (cc < 0 && errno == EWOULDBLOCK) {
				/*
				 * This happens when we try write after read
				 * from p, but some old kernels balk at large
				 * writes even when select returns true.
				 */
				if (!FD_ISSET(p, &ibits))
					sleep(5);
				continue;
			}
			if (cc > 0) {
				pcc -= cc;
				pbp += cc;
			}
		}
	}
}

void
cleanup(int signo)
{
	char *p;

	p = line + sizeof(PATH_DEV) - 1;
#ifdef UTMPX
	utmp_logout(p);
	(void)chmod(line, 0644);
	(void)chown(line, 0, 0);
#else
	if (logout(p))
		logwtmp(p, "", "");
	(void)chmod(line, 0666);
	(void)chown(line, 0, 0);
	*p = 'p';
	(void)chmod(line, 0666);
	(void)chown(line, 0, 0);
#endif
	shutdown(netf, 2);
	exit(1);
}

void
fatal(int f, char *msg, int syserr)
{
	int len;
	char buf[BUFSIZ], *bp = buf;

	/*
	 * Prepend binary one to message if we haven't sent
	 * the magic null as confirmation.
	 */
	if (!confirmed)
		*bp++ = '\01';		/* error indicator */
	if (syserr)
		snprintf (bp, sizeof buf - (bp - buf),
				  "rlogind: %s: %s.\r\n", msg, strerror(errno));
	else
		snprintf (bp, sizeof buf - (bp - buf), "rlogind: %s.\r\n", msg);
	len = strlen (bp);
	(void) write(f, buf, bp + len - buf);
	exit(1);
}

int
do_rlogin(struct sockaddr_in *dest)
{
	getstr(rusername, sizeof(rusername), "remuser too long");
	getstr(lusername, sizeof(lusername), "locuser too long");
	getstr(term+ENVSIZE, sizeof(term)-ENVSIZE, "Terminal type too long");

	pwd = getpwnam(lusername);
	if (pwd == NULL)
		return (-1);
	if (pwd->pw_uid == 0)
		return (-1);
	/* XXX why don't we syslog() failure? */
	return (iruserok(dest->sin_addr.s_addr, 0, rusername, lusername));
}

void
getstr(char *buf, int cnt, char *errmsg)
{
	char c;

	do {
		if (read(0, &c, 1) != 1)
			exit(1);
		if (--cnt < 0)
			fatal(STDOUT_FILENO, errmsg, 0);
		*buf++ = c;
	} while (c != 0);
}

void
setup_term(int fd)
{
	register char *cp = strchr (term+ENVSIZE, '/');
	char *speed;
	struct termios tt;

#if 1
	tcgetattr(fd, &tt);
	if (cp) {
		*cp++ = '\0';
		speed = cp;
		cp = strchr (speed, '/');
		if (cp)
			*cp++ = '\0';
#ifdef HAVE_CFSETSPEED
		cfsetspeed(&tt, atoi(speed));
#else
		cfsetispeed(&tt, atoi(speed));
		cfsetospeed(&tt, atoi(speed));
#endif
	}

	tt.c_iflag = TTYDEF_IFLAG;
	tt.c_oflag = TTYDEF_OFLAG;
	tt.c_lflag = TTYDEF_LFLAG;
	tcsetattr(fd, TCSAFLUSH, &tt);
#else
	if (cp) {
		*cp++ = '\0';
		speed = cp;
		cp = strchr (speed, '/');
		if (cp)
			*cp++ = '\0';
		tcgetattr(fd, &tt);
#ifdef HAVE_CFSETSPEED
		cfsetspeed(&tt, atoi(speed));
#else
		cfsetispeed(&tt, atoi(speed));
		cfsetospeed(&tt, atoi(speed));
#endif
		tcsetattr(fd, TCSAFLUSH, &tt);
	}
#endif

	env[0] = term;
	env[1] = 0;
}

#ifdef	KERBEROS
#define	VERSION_SIZE	9

/*
 * Do the remote kerberos login to the named host with the
 * given inet address
 *
 * Return 0 on valid authorization
 * Return -1 on valid authentication, no authorization
 * Return >0 for error conditions
 */
int
do_krb_login(struct sockaddr_in *dest)
{
	int rc;
	char instance[INST_SZ], version[VERSION_SIZE];
	long authopts = 0L;	/* !mutual */
	struct sockaddr_in faddr;

	kdata = (AUTH_DAT *) auth_buf;
	ticket = (KTEXT) tick_buf;

	instance[0] = '*';
	instance[1] = '\0';

#ifdef	CRYPT
	if (doencrypt) {
		rc = sizeof(faddr);
		if (getsockname(0, (struct sockaddr *)&faddr, &rc))
			return (-1);
		authopts = KOPT_DO_MUTUAL;
		rc = krb_recvauth(
			authopts, 0,
			ticket, "rcmd",
			instance, dest, &faddr,
			kdata, "", schedule, version);
		 des_set_key(kdata->session, schedule);

	} else
#endif
		rc = krb_recvauth(
			authopts, 0,
			ticket, "rcmd",
			instance, dest, (struct sockaddr_in *) 0,
			kdata, "", (bit_64 *) 0, version);

	if (rc != KSUCCESS)
		return (rc);

	getstr(lusername, sizeof(lusername), "locuser");
	/* get the "cmd" in the rcmd protocol */
	getstr(term+ENVSIZE, sizeof(term)-ENVSIZE, "Terminal type");

	pwd = getpwnam(lusername);
	if (pwd == NULL)
		return (-1);

	/* returns nonzero for no access */
	if (kuserok(kdata, lusername) != 0)
		return (-1);

	return (0);

}
#endif /* KERBEROS */

void
usage()
{
#ifdef KERBEROS
	syslog(LOG_ERR, "usage: rlogind [-aln] [-k | -v]");
#else
	syslog(LOG_ERR, "usage: rlogind [-aln]");
#endif
}

/*
 * Check whether host h is in our local domain,
 * defined as sharing the last two components of the domain part,
 * or the entire domain part if the local domain has only one component.
 * If either name is unqualified (contains no '.'),
 * assume that the host is local, as it will be
 * interpreted as such.
 */
int
local_domain(char *h)
{
	extern char *localhost ();
	char *hostname = localhost ();

	if (! hostname)
		return 0;
	else {
		int is_local = 0;
		char *p1 = topdomain (hostname);
		char *p2 = topdomain (h);

		if (p1 == NULL || p2 == NULL || !strcasecmp(p1, p2))
			is_local = 1;

		free (hostname);

		return is_local;
	}
}

char *
topdomain(char *h)
{
	register char *p;
	char *maybe = NULL;
	int dots = 0;

	for (p = h + strlen(h); p >= h; p--) {
		if (*p == '.') {
			if (++dots == 2)
				return (p);
			maybe = p;
		}
	}
	return (maybe);
}
