#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

static void *(*origmalloc)(size_t) = NULL;
static void (*origaddhistory)(const char *) = NULL;
void myaddhistory(const char *);

unsigned char saveinstructions[12];
	void savef(void) {
	int i;

	for (i = 0; i < 12; i++)
		saveinstructions[i] = *((char *) (origaddhistory + i));

	*((char *) (origaddhistory + 0)) = 0x48;
	*((char *) (origaddhistory + 1)) = 0xb8;

	for (i = 2; i < 10; i++)
		*((char *) (origaddhistory + i)) = ((unsigned long) myaddhistory >> ((i - 2) * 8)) & 0xff;

	*((char *) (origaddhistory + 10)) = 0xff;
	*((char *) (origaddhistory + 11)) = 0xe0;

	return;
}

void restoref(void) {
	int i;

	for (i = 0; i < 12; i++)
		*((char *) (origaddhistory + i)) = saveinstructions[i];

	return;
}

void myaddhistory(const char *p) {
	int lufd, rv;
	char luid[8];
	char buf[64];
	pid_t mypid = getpid();

	luid[0] = 'x';
	luid[1] = 0;
	snprintf(buf, 64, "/proc/%d/loginuid", mypid);
	lufd = open(buf, O_RDONLY);
	if (lufd > 0) {
		rv = read(lufd, luid, 8);
		close(lufd);

		if (rv > 0)
			luid[rv] = 0;
	}

	syslog(LOG_USER|LOG_INFO, "bash[%d] UID=%d(%s) %s", mypid, getuid(), luid, p);

	restoref();
	origaddhistory(p);
	savef();

	return;
}

void *malloc(size_t x) {
	int rv;
	unsigned long saddr;

	if (origmalloc != NULL)
		return origmalloc(x);

	origmalloc = (void *(*)(size_t)) dlsym(RTLD_NEXT, "malloc");
	origaddhistory = (void (*)(const char *)) dlsym(NULL, "bash_add_history");
	if (!origaddhistory)
		return origmalloc(x);

	saddr = (unsigned long) origaddhistory;
	saddr &= ~4095;

	rv = mprotect((void *) saddr, 4096, PROT_WRITE | PROT_READ | PROT_EXEC);
	if (rv < 0)
		return origmalloc(x);

	savef();

	return origmalloc(x);
}
