/* Copyright (c) 1988, 1993
 * The Regents of the University of California.  All rights reserved.
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
 * @(#)save.c 8.1 (Berkeley) 5/31/93
 * $FreeBSD: src/games/rogue/save.c,v 1.6 1999/11/30 03:49:27 billf Exp $
 *
 * This source herein may be modified and/or distributed by anybody who
 * so desires, with the following restrictions:
 *    1.)  No portion of this notice shall be removed.
 *    2.)  Credit shall not be taken for the creation of this source.
 *    3.)  This code is not to be traded, sold, or used for personal
 *         gain or profit.
 */

#include <stdio.h>
#include <unistd.h>
#include <strings.h>
#include <limits.h>
#include "rogue.h"

static void del_save_file(void);
static void r_read(FILE *, char *, int);
static void r_write(FILE *, const char *, int);
static void read_pack(object *, FILE *, bool);
static void read_string(char *, FILE *, size_t);
static void rw_dungeon(FILE *, bool);
static void rw_id(struct id *, FILE *, int, bool);
static void rw_rooms(FILE *, bool);
static void write_pack(const object *, FILE *);
static void write_string(char *, FILE *);

static bool write_failed = 0;
static char save_name[80];

char *save_file = NULL;
bool noautosave = false;

void save_game(void) {
	char fname[PATH_MAX];
	if (locked_down)
		strncpy(fname, save_file, PATH_MAX);
	else {
		if (!get_input_line("file name?", save_file, fname, 0)) {
			message("game not saved", 0);
			return;
		}
	}
	check_message();
	messagef(0, "savegame loaded from \"%s\"", fname);

	int ok = save_into_file(fname);
	if (ok) {
		save_name[0] = 0;  // Clear so it won't get removed on exit.
		clean_up("");
	}
}

bool save_into_file(const char *sfile) {
	if (sfile[0] == '~') {
		char *home = md_homedir();
		if (home != NULL) {
			size_t len = strlen(home) + strlen(sfile);
			char *name_buffer = calloc(len, sizeof(char));
			if (name_buffer == NULL) {
				message("out of memory for save file name", 0);
				sfile = error_file;
			} else {
				strcpy(name_buffer, home);
				strcat(name_buffer, sfile+1);
				sfile = name_buffer;
			}
			free(home);
		}
	}

	FILE *fp = fopen(sfile, "w");
	if (fp == NULL) {
		messagef(0, "problem accessing the save file %s", sfile);
		return false;
	}
	md_ignore_signals();
	write_failed = false;

	r_write(fp, (char *)&game_seed, sizeof(game_seed));
	r_write(fp, (char *)&detect_monster, sizeof(detect_monster));
	r_write(fp, (char *)&cur_level, sizeof(cur_level));
	r_write(fp, (char *)&max_level, sizeof(max_level));
	write_string(hunger_str, fp);
	write_string(nick_name, fp);
	r_write(fp, (char *) &party_room, sizeof(party_room));
	write_pack(&level_monsters, fp);
	write_pack(&level_objects, fp);
	rw_dungeon(fp, 1);
	r_write(fp, (char *) &foods, sizeof(foods));
	r_write(fp, (char *) &rogue, sizeof(fighter));
	write_pack(&rogue.pack, fp);
	rw_id(id_potions, fp, POTIONS, 1);
	rw_id(id_scrolls, fp, SCROLS, 1);
	rw_id(id_wands, fp, WANDS, 1);
	rw_id(id_rings, fp, RINGS, 1);
	r_write(fp, (char *) traps, (MAX_TRAPS * sizeof(trap)));
	r_write(fp, (char *) is_wood, (WANDS * sizeof(bool)));
	r_write(fp, (char *) &cur_room, sizeof(cur_room));
	rw_rooms(fp, 1);
	r_write(fp, (char *) &being_held, sizeof(being_held));
	r_write(fp, (char *) &bear_trap, sizeof(bear_trap));
	r_write(fp, (char *) &halluc, sizeof(halluc));
	r_write(fp, (char *) &blind, sizeof(blind));
	r_write(fp, (char *) &confused, sizeof(confused));
	r_write(fp, (char *) &levitate, sizeof(levitate));
	r_write(fp, (char *) &haste_self, sizeof(haste_self));
	r_write(fp, (char *) &see_invisible, sizeof(see_invisible));
	r_write(fp, (char *) &detect_monster, sizeof(detect_monster));
	r_write(fp, (char *) &wizard, sizeof(wizard));
	r_write(fp, (char *) &score_only, sizeof(score_only));
	r_write(fp, (char *) &m_moves, sizeof(m_moves));
	fclose(fp);

	if (write_failed)
		unlink(sfile);
	return !write_failed;
}

static void del_save_file(void) {
	if (!save_name[0])
		return;
	unlink(save_name);
}

void restore(const char *fname) {
	FILE *fp = fopen(fname, "r");
	if (fp == NULL)
		clean_up("cannot open file");

	r_read(fp, (char *)&game_seed, sizeof(game_seed));
	r_read(fp, (char *)&detect_monster, sizeof(detect_monster));
	r_read(fp, (char *)&cur_level, sizeof(cur_level));
	r_read(fp, (char *)&max_level, sizeof(max_level));
	read_string(hunger_str, fp, sizeof(hunger_str));
	nick_name = calloc(MAX_OPT_LEN, sizeof(char));
	read_string(nick_name, fp, MAX_OPT_LEN + 1);
	r_read(fp, (char *) &party_room, sizeof(party_room));
	read_pack(&level_monsters, fp, 0);
	read_pack(&level_objects, fp, 0);
	rw_dungeon(fp, 0);
	r_read(fp, (char *) &foods, sizeof(foods));
	r_read(fp, (char *) &rogue, sizeof(fighter));
	read_pack(&rogue.pack, fp, 1);
	rw_id(id_potions, fp, POTIONS, 0);
	rw_id(id_scrolls, fp, SCROLS, 0);
	rw_id(id_wands, fp, WANDS, 0);
	rw_id(id_rings, fp, RINGS, 0);
	r_read(fp, (char *) traps, (MAX_TRAPS * sizeof(trap)));
	r_read(fp, (char *) is_wood, (WANDS * sizeof(bool)));
	r_read(fp, (char *) &cur_room, sizeof(cur_room));
	rw_rooms(fp, 0);
	r_read(fp, (char *) &being_held, sizeof(being_held));
	r_read(fp, (char *) &bear_trap, sizeof(bear_trap));
	r_read(fp, (char *) &halluc, sizeof(halluc));
	r_read(fp, (char *) &blind, sizeof(blind));
	r_read(fp, (char *) &confused, sizeof(confused));
	r_read(fp, (char *) &levitate, sizeof(levitate));
	r_read(fp, (char *) &haste_self, sizeof(haste_self));
	r_read(fp, (char *) &see_invisible, sizeof(see_invisible));
	r_read(fp, (char *) &detect_monster, sizeof(detect_monster));
	r_read(fp, (char *) &wizard, sizeof(wizard));
	r_read(fp, (char *) &score_only, sizeof(score_only));
	r_read(fp, (char *) &m_moves, sizeof(m_moves));

	char buf[4];
	if (fread(buf, sizeof(char), 1, fp) > 0) {
		clear();
		clean_up("extra characters in file");
	}

	if (!wizard) {
		strcpy(save_name, fname);
		atexit(del_save_file);
	}
	msg_cleared = 0;
	ring_stats(0);
	fclose(fp);
}

static void write_pack(const object *pack, FILE *fp) {
	while ((pack = pack->next_object) != NULL)
		r_write(fp, (const char *)pack, sizeof(object));

	object t;
	t.ichar = t.what_is = 0;
	r_write(fp, (const char *) &t, sizeof(object));
}

static void read_pack(object *pack, FILE *fp, bool is_rogue) {
	for (;;) {
		object read_obj;
		r_read(fp, (char *) &read_obj, sizeof(object));
		if (read_obj.ichar == 0) {
			pack->next_object = NULL;
			break;
		}

		object *new_obj = alloc_object();
		*new_obj = read_obj;
		if (is_rogue) {
			if (new_obj->in_use_flags & BEING_WORN)
				do_wear(new_obj);
			else if (new_obj->in_use_flags & BEING_WIELDED)
				do_wield(new_obj);
			else if (new_obj->in_use_flags & (ON_EITHER_HAND))
				do_put_on(new_obj, (new_obj->in_use_flags & ON_LEFT_HAND) ? 1 : 0);
		}
		pack->next_object = new_obj;
		pack = new_obj;
	}
}

static void rw_dungeon(FILE *fp, bool rw) {
	for (short i = 0; i < DROWS; i++) {
		char buf[DCOLS];
		if (rw) {
			r_write(fp, (char *) dungeon[i], (DCOLS * sizeof(dungeon[0][0])));
			for (short j = 0; j < DCOLS; j++)
				buf[j] = (short)mvinch(i, j);
			r_write(fp, buf, DCOLS);
		} else {
			r_read(fp, (char *) dungeon[i], (DCOLS * sizeof(dungeon[0][0])));
			r_read(fp, buf, DCOLS);
			for (short j = 0; j < DCOLS; j++)
				mvaddch(i, j, buf[j]);
		}
	}
}

static void rw_id(struct id id_table[], FILE *fp, int n, bool wr) {
	for (short i = 0; i < n; i++) {
		if (wr) {
			r_write(fp, (const char *) &(id_table[i].value), sizeof(short));
			r_write(fp, (const char *) &(id_table[i].id_status), sizeof(unsigned short));
			write_string(id_table[i].title, fp);
		} else {
			r_read(fp, (char *) &(id_table[i].value), sizeof(short));
			r_read(fp, (char *) &(id_table[i].id_status), sizeof(unsigned short));
			read_string(id_table[i].title, fp, MAX_ID_TITLE_LEN);
		}
	}
}

static void write_string(char *s, FILE *fp) {
	short n = strlen(s) + 1;
	r_write(fp, (char *) &n, sizeof(short));
	r_write(fp, s, n);
}

static void read_string(char *s, FILE *fp, size_t len) {
	short n;
	r_read(fp, (char *) &n, sizeof(short));
	if (n < 0 || n > (short)len)
		clean_up("read_string: corrupt game file");
	r_read(fp, s, n);
	s[n - 1] = 0;  // ensure null termination
}

static void rw_rooms(FILE *fp, bool rw) {
	for (short i = 0; i < MAXROOMS; i++) {
		rw ? r_write(fp, (char *) (rooms + i), sizeof(room)) :
			r_read(fp, (char *) (rooms + i), sizeof(room));
	}
}

static void r_read(FILE *fp, char *buf, int n) {
	if (fread(buf, sizeof(char), n, fp) != (unsigned)n)
		clean_up("read() failed, don't know why");
}

static void r_write(FILE *fp, const char *buf, int n) {
	if (!write_failed) {
		if (fwrite(buf, sizeof(char), n, fp) != (unsigned)n) {
			message("write() failed, don't know why", 0);
			sound_bell();
			write_failed = true;
		}
	}
}
