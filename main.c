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
 * @(#)main.c 8.1 (Berkeley) 5/31/93
 * $NetBSD: main.c,v 1.9 2008/07/20 01:03:22 lukem Exp $
 *
 * This source herein may be modified and/or distributed by anybody who
 * so desires, with the following restrictions:
 *    1.)  No portion of this notice shall be removed.
 *    2.)  Credit shall not be taken for the creation of this source.
 *    3.)  This code is not to be traded, sold, or used for personal
 *         gain or profit.
 */

#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include "rogue.h"

int main(int argc, char *argv[]) {
	if (init(argc, argv))  // restored game
		goto play_level;

	for (;;) {
		if (!noautosave && rogue.hp_current > 0 && cur_level > 0)
			save_into_file(save_file);
		clear_level();
		make_level();
		put_objects();
		put_stairs();
		add_traps();
		put_mons();
		put_player(party_room);
		print_stats(STAT_ALL);
play_level:
		play_level();
		free_stuff(&level_objects);
		free_stuff(&level_monsters);
	}
}

static const char *const rnd_names[] = {
	"Arthur Dent", "Benjy Mouse", "Deep Thought", "Ford Prefect", "Frankie Mouse",
	"Marvin the Paranoid Android", "Slartibartfast", "Trillian", "Vogon Jeltz",
	"Zaphod Beeblebrox", "Zarniwoop",

	"Baldrick", "Bishop of Bath and Wells", "Bob", "Captain Darling", "Dougal MacAngus",
	"Edmund Blackadder", "Lord Flashheart", "Lord Percy Percy", "MacAdder", "Melchett",
	"Mrs Miggins", "Nursie", "Prince George", "Queenie", "William Pitt the Even Younger",

	"Adam Kenyon", "Angela Heaney", "Ben Swain", "Cal Richards", "Claire Ballentine", "Cliff Lawton",
	"Dan Miller", "Douglas Tickel", "Emma Messinger", "Fergus Williams", "Geoff Holhurst", "Glenn Cullen",
	"Helen Hatley", "Hugh Abbot", "Jamie McDonald", "John Duggan", "Julius Nicholson", "Malcolm Tucker",
	"Mary Drake", "Nick Hanway", "Nicola Murray", "Ollie Reeder", "Pat Morrissey", "Peter Mannion",
	"Phil Smith", "Robyn Murdoch", "Steve Fleming", "Stewart Pearson", "Terri Coverley",
};

static void do_args(int, char **);
static void do_opts(void);
static void env_get_value(char **, char *, bool);
static void init_str(char **, const char *);
static void player_init(void);

static bool init_curses = 0;

char *nick_name = NULL;
int low_health_warn = 0;
bool cant_int = 0;
bool did_int = 0;
bool score_only;
unsigned long game_seed = 0;
bool save_is_interactive = 1;
bool ask_quit = 1;
bool no_skull = 0;
bool passgo = 0;
bool locked_down = 0;
const char *error_file = "rogue.esave";
const char *byebye_string = "Okay, bye bye!";

bool init(int argc, char *argv[]) {
	do_args(argc, argv);
	do_opts();
	struct stat st;
	int exist = stat(save_file, &st);
	bool do_restore = false;
	if (exist == 0) {
		do_restore = true;
		printf("Restoring from %s", save_file);
		fflush(stdout);
	}

	if (!score_only && !do_restore) {
		printf("Hello %s, just a moment while I dig the dungeon...", nick_name);
		fflush(stdout);
	}

	initscr();
	if (LINES < DROWS || COLS < DCOLS)
		clean_up("must be played on 24 x 80 screen");
	start_window();
	init_curses = 1;

	md_heed_signals();

	if (score_only)
		put_scores(NULL, 0);

	if (game_seed == 0)
		game_seed = seed();
	srand(game_seed);

	if (do_restore) {
		restore(save_file);
		return 1;
	}
	mix_colors();
	get_wand_and_ring_materials();
	make_scroll_titles();

	level_objects.next_object = NULL;
	level_monsters.next_monster = NULL;
	player_init();
	ring_stats(0);
	return 0;
}

// https://stackoverflow.com/a/323302
unsigned long seed(void) {
	unsigned long a = clock();
	unsigned long b = time(NULL);
	unsigned long c = getpid();
    a=a-b;  a=a-c;  a=a^(c >> 13);
    b=b-c;  b=b-a;  b=b^(a << 8);
    c=c-a;  c=c-b;  c=c^(b >> 13);
    a=a-b;  a=a-c;  a=a^(c >> 12);
    b=b-c;  b=b-a;  b=b^(a << 16);
    c=c-a;  c=c-b;  c=c^(b >> 5);
    a=a-b;  a=a-c;  a=a^(c >> 3);
    b=b-c;  b=b-a;  b=b^(a << 10);
    c=c-a;  c=c-b;  c=c^(b >> 15);
    return c;
}

static void player_init(void) {
	rogue.pack.next_object = NULL;

	object *obj = alloc_object();
	get_food(obj, 1);
	add_to_pack(obj, &rogue.pack, 1);

	obj = alloc_object();  // initial armor
	obj->what_is = ARMOR;
	obj->which_kind = RINGMAIL;
	obj->class = RINGMAIL+2;
	obj->is_protected = 0;
	obj->d_enchant = 1;
	add_to_pack(obj, &rogue.pack, 1);
	do_wear(obj);

	obj = alloc_object();  // initial weapons
	obj->what_is = WEAPON;
	obj->which_kind = MACE;
	strcpy(&obj->damage[0], "2d3");
	obj->hit_enchant = obj->d_enchant = 1;
	obj->identified = 1;
	add_to_pack(obj, &rogue.pack, 1);
	do_wield(obj);

	obj = alloc_object();
	obj->what_is = WEAPON;
	obj->which_kind = BOW;
	strcpy(&obj->damage[0],"1d2");
	obj->hit_enchant = 1;
	obj->d_enchant = 0;
	obj->identified = 1;
	add_to_pack(obj, &rogue.pack, 1);

	obj = alloc_object();
	obj->what_is = WEAPON;
	obj->which_kind = ARROW;
	obj->quantity = get_rand(25, 35);
	strcpy(&obj->damage[0], "1d2");
	obj->hit_enchant = 0;
	obj->d_enchant = 0;
	obj->identified = 1;
	add_to_pack(obj, &rogue.pack, 1);
}

void clean_up(const char *estr) {
	if (save_is_interactive) {
		if (init_curses) {
			move(DROWS - 1, 0);
			refresh();
			stop_window();
		}
		printf("\n%s\n", estr);
	}
	exit(0);
}

void start_window(void) {
	cbreak();
	noecho();
	nonl();
	keypad(stdscr, true);
}

void stop_window(void) {
	endwin();
}

void byebye(int sig) {
	md_ignore_signals();
	if (ask_quit)
		quit(1);
	else
		clean_up(byebye_string);
	md_heed_signals();
}

void onintr(int sig) {
	md_ignore_signals();
	if (cant_int)
		did_int = 1;
	else {
		check_message();
		message("interrupt", 1);
	}
	md_heed_signals();
}

void error_save(int sig) {
	save_is_interactive = 0;
	save_into_file(error_file);
	clean_up("");
}

static void do_args(int argc, char *argv[]) {
	int opt;
	while ((opt = getopt(argc, argv, "sS:")) != -1) {
		switch (opt) {
		case 's':
			score_only = 1;
			break;
		case 'S':
			game_seed = ulget_number(optarg);
			break;
		default:
			exit(1);
		}
	}
	if (argc >= optind)
		save_file = argv[optind];
}

static void do_opts(void) {
	char *eptr;
	if ((eptr = getenv("ROGUE_CLONE_OPTS")) != NULL) {
		for (;;) {
			while ((*eptr) == ' ')
				eptr++;
			if (!(*eptr))
				break;
			if (!strncmp(eptr, "fruit=", 6)) {
				eptr += 6;
				env_get_value(&fruit, eptr, 1);
			} else if (!strncmp(eptr, "file=", 5)) {
				eptr += 5;
				env_get_value(&save_file, eptr, 0);
			} else if (!strncmp(eptr, "name=", 5)) {
				eptr += 5;
				env_get_value(&nick_name, eptr, 0);
				if (nick_name != NULL && strcmp(nick_name, "*") == 0) {
					srand(seed());  // Make sure it's seeded; will apply -S/seed() later for the actual game.
					free(nick_name);
					nick_name = (char *)rnd_names[get_rand(0, sizeof(rnd_names)/sizeof(rnd_names[0])) - 1];
				}
			} else if (!strncmp(eptr, "lowhealth=", 5)) {
				eptr += 10;
				char *buf = NULL;
				env_get_value(&buf, eptr, 0);
				low_health_warn = get_number(buf);
				free(buf);
			} else if (!strncmp(eptr, "noaskquit", 9))
				ask_quit = 0;
			else if (!strncmp(eptr, "noskull", 7) || !strncmp(eptr, "notomb", 6))
				no_skull = 1;
			else if (!strncmp(eptr, "passgo", 6))
				passgo = 1;
			else if (!strncmp(eptr, "lock", 4))
				locked_down = 1;
			else if (!strncmp(eptr, "noautosave", 10))
				noautosave = true;
			else if (!strncmp(eptr, "revshift", 8))
				revshift = true;

			while ((*eptr) && (*eptr != ','))
				eptr++;
			if (!(*(eptr++)))
				break;
		}
	}

	// If some strings have not been set through ROGUE_CLONE_OPTS, assign defaults
	// to them so that the options editor has data to work with.
	init_str(&nick_name, md_user());
	init_str(&fruit, "slime-mold");
	char *f = md_savefile();
	init_str(&save_file, f);
	free(f);
}

static void env_get_value(char **s, char *e, bool add_blank) {
	short i = 0;
	const char *t = e;
	while ((*e) && (*e != ',')) {
		if (*e == ':')
			*e = ';';  // ':' reserved for score file purposes
		e++;
		if (++i >= MAX_OPT_LEN)
			break;
	}
	// note: edit_opts() in room.c depends on this being the right size
	*s = malloc(MAX_OPT_LEN + 2);
	if (*s == NULL)
		clean_up("out of memory");
	strncpy(*s, t, i);
	if (add_blank)
		(*s)[i++] = ' ';
	(*s)[i] = '\0';
}

static void init_str(char **str, const char *dflt) {
	if (!(*str)) {
		// note: edit_opts() in room.c depends on this size
		*str = calloc(MAX_OPT_LEN + 2, sizeof(char));
		if (*str == NULL)
			clean_up("out of memory");
		strcpy(*str, dflt);
	}
}
