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
 * @(#)room.c 8.1 (Berkeley) 5/31/93
 * $FreeBSD: src/games/rogue/room.c,v 1.7 1999/11/30 03:49:26 billf Exp $
 *
 * This source herein may be modified and/or distributed by anybody who
 * so desires, with the following restrictions:
 *    1.)  No portion of this notice shall be removed.
 *    2.)  Credit shall not be taken for the creation of this source.
 *    3.)  This code is not to be traded, sold, or used for personal
 *         gain or profit.
 */

#include "rogue.h"

room rooms[MAXROOMS];

static bool rooms_visited[MAXROOMS];

#define NOPTS 9
static const struct option {
	const char *name;
	const char *prompt;
	bool       is_bool;
	bool       is_int;
	char       **strval;
	bool       *bval;
	int        *ival;
} options[NOPTS] = {
	{"passgo",     "Follow turnings in passageways",  1, 0, NULL,       &passgo,     NULL},
	{"noskull",    "Don't print skull when killed",   1, 0, NULL,       &no_skull,   NULL},
	{"askquit",    "Ask before exiting on SIGQUIT",   1, 0, NULL,       &ask_quit,   NULL},
	{"lowhealth",  "Low health warning below",        0, 1, NULL,       NULL,        &low_health_warn},
	{"fruit",      "Fruit name",                      0, 0, &fruit,     NULL,        NULL},
	{"noautosave", "Don't autosave",                  1, 0, NULL,       &noautosave, NULL},
	{"revshift",   "Reverse shift+dir and ctrl+dir",  1, 0, NULL,       &revshift,   NULL},
	// Keep these two last so there won't be any gaps when lock is set.
	{"name",       "Name",                            0, 0, &nick_name, NULL,        NULL},
	{"file",       "Save file",                       0, 0, &save_file, NULL,        NULL},
};

static bool get_oth_room(short, short *, short *);
static void opt_erase(int);
static void opt_go(int);
static void opt_show(int);
static void visit_rooms(int);

void light_up_room(int rn) {
	if (blind)
		return;

	for (short i = rooms[rn].top_row; i <= rooms[rn].bottom_row; i++) {
		for (short j = rooms[rn].left_col; j <= rooms[rn].right_col; j++) {
			if (dungeon[i][j] & MONSTER) {
				object *monster;
				if ((monster = object_at(&level_monsters, i, j))) {
					dungeon[monster->row][monster->col] &= (~MONSTER);
					monster->trail_char = get_dungeon_char(monster->row, monster->col);
					dungeon[monster->row][monster->col] |= MONSTER;
				}
			}
			mvaddch(i, j, get_dungeon_char(i, j));
		}
	}
	mvaddch(rogue.row, rogue.col, rogue.fchar);
}

void light_passage(int row, int col) {
	if (blind)
		return;

	short i_end = (row < (DROWS - 2)) ? 1 : 0;
	short j_end = (col < (DCOLS - 1)) ? 1 : 0;
	for (short i = ((row > MIN_ROW) ? -1 : 0); i <= i_end; i++) {
		for (short j = ((col > 0) ? -1 : 0); j <= j_end; j++) {
			if (can_move(row, col, row + i, col + j))
				mvaddch(row + i, col + j, get_dungeon_char(row + i, col + j));
		}
	}
}

void darken_room(short rn) {
	for (short i = rooms[rn].top_row + 1; i < rooms[rn].bottom_row; i++) {
		for (short j = rooms[rn].left_col + 1; j < rooms[rn].right_col; j++) {
			if (blind) {
				mvaddch(i, j, ' ');
				continue;
			}

			if (!(dungeon[i][j] & (OBJECT | STAIRS)) && !(detect_monster && (dungeon[i][j] & MONSTER))) {
				if (!imitating(i, j))
					mvaddch(i, j, ' ');
				if ((dungeon[i][j] & TRAP) && (!(dungeon[i][j] & HIDDEN)))
					mvaddch(i, j, '^');
			}
		}
	}
}

char get_dungeon_char(int row, int col) {
	unsigned short mask = dungeon[row][col];

	if (mask & MONSTER)
		return gmc_row_col(row, col);

	if (mask & OBJECT) {
		object *obj = object_at(&level_objects, row, col);
		return get_mask_char(obj->what_is);
	}
	if (mask & (TUNNEL | STAIRS | HORWALL | VERTWALL | FLOOR | DOOR)) {
		if ((mask & (TUNNEL| STAIRS)) && (!(mask & HIDDEN)))
			return ((mask & STAIRS) ? '%' : '#');
		if (mask & HORWALL)
			return '-';
		if (mask & VERTWALL)
			return '|';
		if (mask & FLOOR) {
			if (mask & TRAP) {
				if (!(dungeon[row][col] & HIDDEN))
					return '^';
			}
			return '.';
		}
		if (mask & DOOR) {
			if (mask & HIDDEN) {
				if ((col > 0 && (dungeon[row][col-1] & HORWALL)) || (col < (DCOLS-1) && (dungeon[row][col+1] & HORWALL)))
					return '-';
				else
					return '|';
			} else
				return '+';
		}
	}
	return ' ';
}

char get_mask_char(unsigned short mask) {
	switch (mask) {
	case SCROL:  return '?';
	case POTION: return '!';
	case GOLD:   return '*';
	case FOOD:   return ':';
	case WAND:   return '/';
	case ARMOR:  return ']';
	case WEAPON: return ')';
	case RING:   return '=';
	case AMULET: return ',';
	default:     return '~';  // unknown, something is wrong
	}
}

void gr_row_col(short *row, short *col, unsigned short mask) {
	short rn, r, c;
	do {
		r = get_rand(MIN_ROW, DROWS-2);
		c = get_rand(0, DCOLS-1);
		rn = get_room_number(r, c);
	} while (rn == NO_ROOM ||
		(!(dungeon[r][c] & mask)) ||
		(dungeon[r][c] & (~mask)) ||
		(!(rooms[rn].is_room & (R_ROOM | R_MAZE))) ||
		((r == rogue.row) && (c == rogue.col)));

	*row = r;
	*col = c;
}

short gr_room(void) {
	short i;
	do {
		i = get_rand(0, MAXROOMS-1);
	} while (!(rooms[i].is_room & (R_ROOM | R_MAZE)));
	return i;
}

short party_objects(short rn) {
	short N = ((rooms[rn].bottom_row - rooms[rn].top_row) - 1) * ((rooms[rn].right_col - rooms[rn].left_col) - 1);
	short n =  get_rand(5, 10);
	if (n > N)
		n = N - 2;

	short row = 0, col = 0;
	short nf = 0;
	bool found;
	for (short i = 0; i < n; i++) {
		for (short j = found = 0; ((!found) && (j < 250)); j++) {
			row = get_rand(rooms[rn].top_row + 1, rooms[rn].bottom_row - 1);
			col = get_rand(rooms[rn].left_col + 1, rooms[rn].right_col - 1);
			if (dungeon[row][col] == FLOOR || dungeon[row][col] == TUNNEL)
				found = 1;
		}
		if (found) {
			object *obj = gr_object();
			place_at(obj, row, col);
			nf++;
		}
	}
	return nf;
}

short get_room_number(int row, int col) {
	for (short i = 0; i < MAXROOMS; i++) {
		if (row >= rooms[i].top_row && row <= rooms[i].bottom_row && col >= rooms[i].left_col && col <= rooms[i].right_col)
			return i;
	}
	return NO_ROOM;
}

bool is_all_connected(void) {
	short starting_room = 0;
	for (short i = 0; i < MAXROOMS; i++) {
		rooms_visited[i] = 0;
		if (rooms[i].is_room & (R_ROOM | R_MAZE))
			starting_room = i;
	}
	visit_rooms(starting_room);

	for (short i = 0; i < MAXROOMS; i++) {
		if ((rooms[i].is_room & (R_ROOM | R_MAZE)) && !rooms_visited[i])
			return 0;
	}
	return 1;
}

static void visit_rooms(int rn) {
	rooms_visited[rn] = 1;
	for (short i = 0; i < 4; i++) {
		short oth_rn = rooms[rn].doors[i].oth_room;
		if (oth_rn >= 0 && !rooms_visited[oth_rn])
			visit_rooms(oth_rn);
	}
}

void draw_magic_map(void) {
	for (short i = 0; i < DROWS; i++) {
		for (short j = 0; j < DCOLS; j++) {
			unsigned short s = dungeon[i][j];
			if (s & (HORWALL | VERTWALL | DOOR | TUNNEL | TRAP | STAIRS | MONSTER)) {
				short ch = (short)mvinch(i, j);
				if ((ch == ' ') || ((ch >= 'A') && (ch <= 'Z')) || (s & (TRAP | HIDDEN))) {
					short och = ch;
					dungeon[i][j] &= (~HIDDEN);
					if (s & HORWALL)       ch = '-';
					else if (s & VERTWALL) ch = '|';
					else if (s & DOOR)     ch = '+';
					else if (s & TRAP)     ch = '^';
					else if (s & STAIRS)   ch = '%';
					else if (s & TUNNEL)   ch = '#';
					else                   continue;

					if ((!(s & MONSTER)) || och == ' ')
						addch(ch);

					if (s & MONSTER) {
						object *monster;
						if ((monster = object_at(&level_monsters, i, j)))
							monster->trail_char = ch;
					}
				}
			}
		}
	}
}

void dr_course(object *monster, bool entering, short row, short col) {
	monster->row = row;
	monster->col = col;
	if (mon_sees(monster, rogue.row, rogue.col)) {
		monster->trow = NO_ROOM;
		return;
	}

	short rn = get_room_number(row, col);
	if (entering) {  // entering room
		// look for door to some other room
		short r = get_rand(0, MAXROOMS-1);
		for (short i = 0; i < MAXROOMS; i++) {
			short rr = (r + i) % MAXROOMS;
			if ((!(rooms[rr].is_room & (R_ROOM | R_MAZE))) || rr == rn) {
				continue;
			}
			for (short k = 0; k < 4; k++) {
				if (rooms[rr].doors[k].oth_room == rn) {
					monster->trow = rooms[rr].doors[k].oth_row;
					monster->tcol = rooms[rr].doors[k].oth_col;
					if (monster->trow == row && monster->tcol == col)
						continue;
					return;
				}
			}
		}
		// look for door to dead end
		if (rn == NO_ROOM)
			clean_up("dr_course:  monster not in room");
		for (short i = rooms[rn].top_row; i <= rooms[rn].bottom_row; i++) {
			for (short j = rooms[rn].left_col; j <= rooms[rn].right_col; j++) {
				if (i != monster->row && j != monster->col && (dungeon[i][j] & DOOR)) {
					monster->trow = i;
					monster->tcol = j;
					return;
				}
			}
		}
		// return monster to room that he came from
		for (short i = 0; i < MAXROOMS; i++) {
			for (short j = 0; j < 4; j++) {
				if (rooms[i].doors[j].oth_room == rn) {
					for (short k = 0; k < 4; k++) {
						if (rooms[rn].doors[k].oth_room == i) {
							monster->trow = rooms[rn].doors[k].oth_row;
							monster->tcol = rooms[rn].doors[k].oth_col;
							return;
						}
					}
				}
			}
		}
		monster->trow = NO_ROOM;  // no place to send monster
	} else {  // exiting room
		if (rn == NO_ROOM || !get_oth_room(rn, &row, &col))
			monster->trow = NO_ROOM;
		else {
			monster->trow = row;
			monster->tcol = col;
		}
	}
}

static bool get_oth_room(short rn, short *row, short *col) {
	short d = -1;
	if (*row == rooms[rn].top_row)
		d = UPWARD / 2;
	else if (*row == rooms[rn].bottom_row)
		d = DOWN / 2;
	else if (*col == rooms[rn].left_col)
		d = LEFT / 2;
	else if (*col == rooms[rn].right_col)
		d = RIGHT / 2;

	if (d != -1 && rooms[rn].doors[d].oth_room >= 0) {
		*row = rooms[rn].doors[d].oth_row;
		*col = rooms[rn].doors[d].oth_col;
		return 1;
	}
	return 0;
}

void edit_opts(void) {
	char save[NOPTS + 1][DCOLS];
	for (short i = 0; i < NOPTS + (locked_down ? -1 : 1); i++) {
		for (short j = 0; j < DCOLS; j++)
			save[i][j] = (short)mvinch(i, j);
		if (i < NOPTS && (locked_down && i < NOPTS - 2))
			opt_show(i);
	}
	opt_go(0);

	short i = 0;
	short j;
	char buf[MAX_OPT_LEN + 2];
	bool done = 0;
	while (!done) {
		refresh();
		int ch = rgetchar();
CH:
		switch (ch) {
		case 0x1b:
			done = 1;
			break;
		case 0x0a: case 0x0d:
			if (i == NOPTS - 1 || (locked_down && i == NOPTS - 3)) {
				mvaddstr(locked_down ? NOPTS - 2 : NOPTS, 0, press_space);
				refresh();
				wait_for_ack();
				done = 1;
			} else {
				i++;
				opt_go(i);
			}
			break;
		case '-':
			if (i > 0)
				opt_go(--i);
			else
				sound_bell();
			break;
		case 't':
		case 'T':
		case 'f':
		case 'F':
			if (options[i].is_bool) {
				*(options[i].bval) = ((ch == 't' || ch == 'T') ? 1 : 0);
				opt_show(i);
				opt_go(++i);
				break;
			}
			// FALLTHROUGH
		default:
			if (options[i].is_bool) {
				sound_bell();
				break;
			}

			j = 0;
			if (ch == '\010' || (ch >= ' ' && ch <= '~')) {
				opt_erase(i);
				do {
					if (ch >= ' ' && ch <= '~' && j < MAX_OPT_LEN) {
						buf[j++] = ch;
						buf[j] = '\0';
						addch(ch);
					} else if (ch == '\010' && j > 0) {
						buf[--j] = '\0';
						move(i, j + strlen(options[i].prompt) + strlen(options[i].name) + 7);
						addch(' ');
						move(i, j + strlen(options[i].prompt) + strlen(options[i].name) + 7);
					}
					refresh();
					ch = rgetchar();
				} while (ch != '\012' && ch != '\015' && ch != '\033');
				if (j != 0) {
					// We rely on the option string being allocated to hold
					// MAX_OPT_LEN+2 bytes. This is arranged in init.c.
					if (options[i].is_int)
						*options[i].ival = get_number(buf);
					else
						strcpy(*(options[i].strval), buf);
				}
				opt_show(i);
				goto CH;
			} else {
				sound_bell();
			}
			break;
		}
	}

	for (i = 0; i < NOPTS + (locked_down ? -1 : 1); i++) {
		move(i, 0);
		for (j = 0; j < DCOLS; j++)
			addch(save[i][j]);
	}
}

static void opt_show(int i) {
	opt_erase(i);

	const struct option *opt = &options[i];
	const char *s;
	if (opt->is_bool)
		s = *(opt->bval) ? "True" : "False";
	else if (opt->is_int) {
		char buf[10];
		sprintf(buf, "%d", *opt->ival);
		s = &buf[0];
	}
	else
		s = *(opt->strval);
	addstr(s);
}

static void opt_erase(int i) {
	const struct option *opt = &options[i];
	mvaddstr(i, 0, opt->prompt);
	mvaddstr(i, strlen(opt->prompt), " (\"");
	mvaddstr(i, strlen(opt->prompt)+3, opt->name);
	mvaddstr(i, strlen(opt->prompt)+strlen(opt->name)+3, "\"): ");
	clrtoeol();
}

static void opt_go(int i) {
	move(i, strlen(options[i].prompt) + strlen(options[i].name) + 7);
}
