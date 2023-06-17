/*-
 * Copyright (c) 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Timothy C. Stoehr.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 *
 * @(#)machdep.c	8.1 (Berkeley) 5/31/93
 * $FreeBSD: src/games/rogue/machdep.c,v 1.6.2.1 2001/12/17 12:43:23 phantom Exp $
 */

/*
 * machdep.c
 *
 * This source herein may be modified and/or distributed by anybody who
 * so desires, with the following restrictions:
 *    1.)  No portion of this notice shall be removed.
 *    2.)  Credit shall not be taken for the creation of this source.
 *    3.)  This code is not to be traded, sold, or used for personal
 *         gain or profit.
 *
 */


#include <stdio.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <time.h>
#include <fcntl.h>

#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include "rogue.h"

void md_heed_signals(void) {
	signal(SIGINT, onintr);
#ifndef WINDOWS
	signal(SIGQUIT, byebye);
	signal(SIGHUP, error_save);
#endif
}

void md_ignore_signals(void) {
	signal(SIGINT, SIG_IGN);
#ifndef WINDOWS
	signal(SIGQUIT, SIG_IGN);
	signal(SIGHUP, SIG_IGN);
#endif
}

int md_get_file_id(const char *fname) {
	struct stat sbuf;
	if (stat(fname, &sbuf))
		return -1;
	return((int)sbuf.st_ino);
}

int md_link_count(const char *fname) {
	struct stat sbuf;
	stat(fname, &sbuf);
	return((int)sbuf.st_nlink);
}

void md_gct(struct rogue_time *rt_buf) {
	struct tm *t;
	time_t seconds;

	time(&seconds);
	t = localtime(&seconds);

	rt_buf->year = t->tm_year;
	rt_buf->month = t->tm_mon + 1;
	rt_buf->day = t->tm_mday;
	rt_buf->hour = t->tm_hour;
	rt_buf->minute = t->tm_min;
	rt_buf->second = t->tm_sec;
}

void md_gfmt(const char *fname, struct rogue_time *rt_buf) {
	struct stat sbuf;
	time_t seconds;
	struct tm *t;

	stat(fname, &sbuf);
	seconds = sbuf.st_mtime;
	t = localtime(&seconds);

	rt_buf->year = t->tm_year;
	rt_buf->month = t->tm_mon + 1;
	rt_buf->day = t->tm_mday;
	rt_buf->hour = t->tm_hour;
	rt_buf->minute = t->tm_min;
	rt_buf->second = t->tm_sec;
}

char * md_user(void) {
#ifdef WINDOWS
	return getenv("USERNAME");
#else
	return getenv("USER");
#endif
}

char * md_homedir(void) {
#ifdef WINDOWS
	return getenv("USERPROFILE");
#else
	return getenv("HOME");
#endif
}

char * md_savedir(void) {
	char *base = getenv("XDG_DATA_HOME");
	if (!base || base[0] != '/') {
		char *home = md_homedir();
#ifdef WINDOWS
		if (!home)
			clean_up("md_scorefile: invalid $HOME\n");
		base = malloc(strlen(home) + 14);
		snprintf(base, strlen(home) + 8, "%s/_rogue", home);
#else
		if (!home || home[0] != '/')
			clean_up("md_scorefile: invalid $HOME\n");
		base = malloc(strlen(home) + 14);
		snprintf(base, strlen(home) + 14, "%s/.local/share", home);
#endif
	}

	char *dir = malloc(strlen(base) + 7);
	snprintf(dir, strlen(base) + 7, "%s/rogue", base);
#ifdef WINDOWS
	mkdir(dir);
#else
	mkdir(dir, 0755);
#endif

	free(base);
	return dir;
}

char * md_scorefile(void) {
	char *dir = md_savedir();

	char *file = malloc(strlen(dir) + 8);
	snprintf(file, strlen(dir) + 8, "%s/score", dir);

	free(dir);
	return file;
}

char * md_savefile(void) {
	char *dir = md_savedir();

	char *file = malloc(strlen(dir) + 6);
	snprintf(file, strlen(dir) + 8, "%s/save", dir);

	free(dir);
	return file;
}

// When the parameter 'l' is non-zero a lock is requested. Otherwise the lock is
// released.
void md_lock(boolean l) {
#ifdef WINDOWS
	// Nothing or now.
#else
	static int fd;
	short tries;

	if (l) {
		if ((fd = open(md_scorefile(), O_RDONLY)) < 1) {
			message("cannot lock score file", 0);
			return;
		}
		for (tries = 0; tries < 5; tries++)
			if (!flock(fd, LOCK_EX|LOCK_NB))
				return;
	} else {
		flock(fd, LOCK_NB);
		close(fd);
	}
#endif
}

int get_rand(int x, int y) {
	int r, t;
	long lr;

	if (x > y) {
		t = y;
		y = x;
		x = t;
	}
#ifdef WINDOWS
	lr = rand();
#else
	lr = random();
#endif
	lr &= 0x00003fffL;
	r = (int)lr;
	r = (r % ((y - x) + 1)) + x;
	return(r);
}

boolean rand_percent(int percentage) {
	return(get_rand(1, 100) <= percentage);
}

boolean coin_toss(void) {
#ifdef WINDOWS
	return(((rand() & 01) ? 1 : 0));
#else
	return(((random() & 01) ? 1 : 0));
#endif
}
