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
 * @(#)monster.c 8.1 (Berkeley) 5/31/93
 * $FreeBSD: src/games/rogue/monster.c,v 1.6 1999/11/30 03:49:24 billf Exp $
 *
 * This source herein may be modified and/or distributed by anybody who
 * so desires, with the following restrictions:
 *    1.)  No portion of this notice shall be removed.
 *    2.)  Credit shall not be taken for the creation of this source.
 *    3.)  This code is not to be traded, sold, or used for personal
 *         gain or profit.
 */

#include "rogue.h"

object level_monsters;
bool mon_disappeared;

const char *const m_names[] = {"aquator", "bat", "centaur", "dragon", "emu",
	"venus fly-trap", "griffin", "hobgoblin", "ice monster", "jabberwock",
	"kestrel", "leprechaun", "medusa", "nymph", "orc", "phantom", "quagga",
	"rattlesnake", "snake", "troll", "black unicorn", "vampire", "wraith",
	"xeroc", "yeti", "zombie"
};

static object mon_tab[MONSTERS] = {
	{(ASLEEP|WAKENS|WANDERS|RUSTS),        "0d0",       25, 'A',   20,  9,  18, 100, 0,   0, 0,0,0,0,0,0,0,0,0,0,0,0,0,NULL},
	{(ASLEEP|WANDERS|FLITS|FLIES),         "1d3",       10, 'B',    2,  1,   8,  60, 0,   0, 0,0,0,0,0,0,0,0,0,0,0,0,0,NULL},
	{(ASLEEP|WANDERS),                     "3d3/2d5",   32, 'C',   15,  7,  16,  85, 0,  10, 0,0,0,0,0,0,0,0,0,0,0,0,0,NULL},
	{(ASLEEP|WAKENS|FLAMES),               "4d6/4d9",  145, 'D', 5000, 21, 126, 100, 0,  90, 0,0,0,0,0,0,0,0,0,0,0,0,0,NULL},
	{(ASLEEP|WAKENS),                      "1d3",       11, 'E',    2,  1,   7,  65, 0,   0, 0,0,0,0,0,0,0,0,0,0,0,0,0,NULL},
	{(HOLDS|STATIONARY),                   "5d5",       73, 'F',   91, 12, 126,  80, 0,   0, 0,0,0,0,0,0,0,0,0,0,0,0,0,NULL},
	{(ASLEEP|WAKENS|WANDERS|FLIES),        "5d5/5d5",  115, 'G', 2000, 20, 126,  85, 0,  10, 0,0,0,0,0,0,0,0,0,0,0,0,0,NULL},
	{(ASLEEP|WAKENS|WANDERS),              "1d3/1d2",   15, 'H',    3,  1,  10,  67, 0,   0, 0,0,0,0,0,0,0,0,0,0,0,0,0,NULL},
	{(ASLEEP|FREEZES),                     "0d0",       15, 'I',    5,  2,  11,  68, 0,   0, 0,0,0,0,0,0,0,0,0,0,0,0,0,NULL},
	{(ASLEEP|WANDERS),                     "3d10/4d5", 132, 'J', 3000, 21, 126, 100, 0,   0, 0,0,0,0,0,0,0,0,0,0,0,0,0,NULL},
	{(ASLEEP|WAKENS|WANDERS|FLIES),        "1d4",       10, 'K',    2,  1,   6,  60, 0,   0, 0,0,0,0,0,0,0,0,0,0,0,0,0,NULL},
	{(ASLEEP|STEALS_GOLD),                 "0d0",       25, 'L',   21,  6,  16,  75, 0,   0, 0,0,0,0,0,0,0,0,0,0,0,0,0,NULL},
	{(ASLEEP|WAKENS|WANDERS|CONFUSES),     "4d4/3d7",   97, 'M',  250, 18, 126,  85, 0,  25, 0,0,0,0,0,0,0,0,0,0,0,0,0,NULL},
	{(ASLEEP|STEALS_ITEM),                 "0d0",       25, 'N',   39, 10,  19,  75, 0, 100, 0,0,0,0,0,0,0,0,0,0,0,0,0,NULL},
	{(ASLEEP|WANDERS|WAKENS|SEEKS_GOLD),   "1d6",       25, 'O',    5,  4,  13,  70, 0,  10, 0,0,0,0,0,0,0,0,0,0,0,0,0,NULL},
	{(ASLEEP|INVISIBLE|WANDERS|FLITS),     "5d4",       76, 'P',  120, 15,  24,  80, 0,  50, 0,0,0,0,0,0,0,0,0,0,0,0,0,NULL},
	{(ASLEEP|WAKENS|WANDERS),              "3d5",       30, 'Q',   20,  8,  17,  78, 0,  20, 0,0,0,0,0,0,0,0,0,0,0,0,0,NULL},
	{(ASLEEP|WAKENS|WANDERS|STINGS),       "2d5",       19, 'R',   10,  3,  12,  70, 0,   0, 0,0,0,0,0,0,0,0,0,0,0,0,0,NULL},
	{(ASLEEP|WAKENS|WANDERS),              "1d3",        8, 'S',    2,  1,   9,  50, 0,   0, 0,0,0,0,0,0,0,0,0,0,0,0,0,NULL},
	{(ASLEEP|WAKENS|WANDERS),              "4d6/1d4",   75, 'T',  125, 13,  22,  75, 0,  33, 0,0,0,0,0,0,0,0,0,0,0,0,0,NULL},
	{(ASLEEP|WAKENS|WANDERS),              "4d10",      90, 'U',  200, 17,  26,  85, 0,  33, 0,0,0,0,0,0,0,0,0,0,0,0,0,NULL},
	{(ASLEEP|WAKENS|WANDERS|DRAINS_LIFE),  "1d14/1d4",  55, 'V',  350, 19, 126,  85, 0,  18, 0,0,0,0,0,0,0,0,0,0,0,0,0,NULL},
	{(ASLEEP|WANDERS|DROPS_LEVEL),         "2d8",       45, 'W',   55, 14,  23,  75, 0,   0, 0,0,0,0,0,0,0,0,0,0,0,0,0,NULL},
	{(ASLEEP|IMITATES),                    "4d6",       42, 'X',  110, 16,  25,  75, 0,   0, 0,0,0,0,0,0,0,0,0,0,0,0,0,NULL},
	{(ASLEEP|WANDERS),                     "3d6",       35, 'Y',   50, 11,  20,  80, 0,  20, 0,0,0,0,0,0,0,0,0,0,0,0,0,NULL},
	{(ASLEEP|WAKENS|WANDERS),              "1d7",       21, 'Z',    8,  5,  14,  69, 0,   0, 0,0,0,0,0,0,0,0,0,0,0,0,0,NULL}
};

static void aim_monster(object *);
static bool flit(object *);
static bool move_confused(object *);
static bool mtry(object *, short, short);
static bool no_room_for_monster(int);
static void put_m_at(short, short, object *);
static short rogue_is_around(int, int);

void put_mons(void) {
	short n = get_rand(4, 6);
	for (short i = 0; i < n; i++) {
		object *monster = gr_monster(NULL, 0);
		if ((monster->m_flags & WANDERS) && coin_toss())
			wake_up(monster);

		short row, col;
		gr_row_col(&row, &col, (FLOOR | TUNNEL | STAIRS | OBJECT));
		put_m_at(row, col, monster);
	}
}

object * gr_monster(object *monster, int mn) {
	if (!monster) {
		monster = alloc_object();
		for (;;) {
			mn = get_rand(0, MONSTERS-1);
			if ((cur_level >= mon_tab[mn].first_level) && (cur_level <= mon_tab[mn].last_level))
				break;
		}
	}

	*monster = mon_tab[mn];
	if (monster->m_flags & IMITATES)
		monster->disguise = gr_obj_char();
	if (cur_level > (AMULET_LEVEL + 2))
		monster->m_flags |= HASTED;
	monster->trow = NO_ROOM;
	return monster;
}

void mv_mons(void) {
	if (haste_self % 2)
		return;

	object *next_monster, *test_mons;
	object *monster = level_monsters.next_monster;
	while (monster) {
		next_monster = monster->next_monster;
		mon_disappeared = 0;
		if (monster->m_flags & HASTED) {
			mv_1_monster(monster, rogue.row, rogue.col);
			if (mon_disappeared)
				goto next_monster;
		} else if (monster->m_flags & SLOWED) {
			monster->slowed_toggle = !monster->slowed_toggle;
			if (monster->slowed_toggle)
				goto next_monster;
		}
		if ((monster->m_flags & CONFUSED) && move_confused(monster))
			goto next_monster;

		bool flew = 0;
		if ((monster->m_flags & FLIES) && !(monster->m_flags & NAPPING) && !mon_can_go(monster, rogue.row, rogue.col)) {
			flew = 1;
			mv_1_monster(monster, rogue.row, rogue.col);
			if (mon_disappeared)
				goto next_monster;
		}
		if (!(flew && mon_can_go(monster, rogue.row, rogue.col)))
			mv_1_monster(monster, rogue.row, rogue.col);

next_monster:
		test_mons = level_monsters.next_monster;
		monster = NULL;
		while (test_mons) {
			if (next_monster == test_mons) {
				monster = next_monster;
				break;
			}
			test_mons = test_mons -> next_monster;
		}
	}
}

void party_monsters(int rn, int n) {
	for (short i = 0; i < MONSTERS; i++)
		mon_tab[i].first_level -= (cur_level % 3);

	n += n;
	for (short i = 0; i < n; i++) {
		if (no_room_for_monster(rn))
			break;

		bool found;
		short row = 0, col = 0;
		for (short j = found = 0; ((!found) && (j < 250)); j++) {
			row = get_rand(rooms[rn].top_row  + 1, rooms[rn].bottom_row - 1);
			col = get_rand(rooms[rn].left_col + 1, rooms[rn].right_col  - 1);
			if ((!(dungeon[row][col] & MONSTER)) && (dungeon[row][col] & (FLOOR | TUNNEL)))
				found = 1;
		}
		if (found) {
			object *monster = gr_monster(NULL, 0);
			if (!(monster->m_flags & IMITATES))
				monster->m_flags |= WAKENS;
			put_m_at(row, col, monster);
		}
	}

	for (short i = 0; i < MONSTERS; i++)
		mon_tab[i].first_level += cur_level % 3;
}

short gmc_row_col(int row, int col) {
	object *monster;
	if ((monster = object_at(&level_monsters, row, col)) != NULL) {
		if ((!(detect_monster || see_invisible || r_see_invisible) && (monster->m_flags & INVISIBLE)) || blind)
			return monster->trail_char;
		if (monster->m_flags & IMITATES)
			return monster->disguise;
		return monster->m_char;
	} else
		return '&';  // BUG if this ever happens
}

short gmc(object *monster) {
	if ((!(detect_monster || see_invisible || r_see_invisible) && (monster->m_flags & INVISIBLE)) || blind)
		return monster->trail_char;
	if (monster->m_flags & IMITATES)
		return monster->disguise;
	return monster->m_char;
}

void mv_1_monster(object *monster, short row, short col) {
	if (monster->m_flags & ASLEEP) {
		if (monster->m_flags & NAPPING) {
			if (--monster->nap_length <= 0)
				monster->m_flags &= (~(NAPPING | ASLEEP));
			return;
		}
		if ((monster->m_flags & WAKENS) && rogue_is_around(monster->row, monster->col) &&
			rand_percent(stealthy > 0 ? (WAKE_PERCENT / (STEALTH_FACTOR + stealthy)) : WAKE_PERCENT)
		) {
			wake_up(monster);
		}
		return;
	} else if (monster->m_flags & ALREADY_MOVED) {
		monster->m_flags &= (~ALREADY_MOVED);
		return;
	}
	if ((monster->m_flags & FLITS) && flit(monster))
		return;
	if ((monster->m_flags & STATIONARY) && (!mon_can_go(monster, rogue.row, rogue.col)))
		return;
	if (monster->m_flags & FREEZING_ROGUE)
		return;
	if ((monster->m_flags & CONFUSES) && m_confuse(monster))
		return;
	if (mon_can_go(monster, rogue.row, rogue.col)) {
		mon_hit(monster);
		return;
	}
	if ((monster->m_flags & FLAMES) && flame_broil(monster))
		return;
	if ((monster->m_flags & SEEKS_GOLD) && seek_gold(monster))
		return;

	if (monster->trow == monster->row && monster->tcol == monster->col)
		monster->trow = NO_ROOM;
	else if (monster->trow != NO_ROOM) {
		row = monster->trow;
		col = monster->tcol;
	}
	if (monster->row > row)
		row = monster->row - 1;
	else if (monster->row < row)
		row = monster->row + 1;

	if ((dungeon[row][monster->col] & DOOR) && mtry(monster, row, monster->col))
		return;
	if (monster->col > col)
		col = monster->col - 1;
	else if (monster->col < col)
		col = monster->col + 1;

	if ((dungeon[monster->row][col] & DOOR) && mtry(monster, monster->row, col))
		return;
	if (mtry(monster, row, col))
		return;

	bool tried[6];
	for (short i = 0; i <= 5; i++)
		tried[i] = 0;

	for (short i = 0; i < 6; i++) {
		short n;
next_try:
		n = get_rand(0, 5);
		switch (n) {
		case 0:
			if (!tried[n] && mtry(monster, row, monster->col-1))
				goto O;
			break;
		case 1:
			if (!tried[n] && mtry(monster, row, monster->col))
				goto O;
			break;
		case 2:
			if (!tried[n] && mtry(monster, row, monster->col+1))
				goto O;
			break;
		case 3:
			if (!tried[n] && mtry(monster, monster->row-1, col))
				goto O;
			break;
		case 4:
			if (!tried[n] && mtry(monster, monster->row, col))
				goto O;
			break;
		case 5:
			if (!tried[n] && mtry(monster, monster->row+1, col))
				goto O;
			break;
		}
		if (!tried[n])
			tried[n] = 1;
		else
			goto next_try;
	}
O:
	if ((monster->row == monster->o_row) && (monster->col == monster->o_col)) {
		if (++(monster->o) > 4) {
			if ((monster->trow == NO_ROOM) && (!mon_sees(monster, rogue.row, rogue.col))) {
				monster->trow = get_rand(1, (DROWS - 2));
				monster->tcol = get_rand(0, (DCOLS - 1));
			} else {
				monster->trow = NO_ROOM;
				monster->o = 0;
			}
		}
	} else {
		monster->o_row = monster->row;
		monster->o_col = monster->col;
		monster->o = 0;
	}
}

static bool mtry(object *monster, short row, short col) {
	if (mon_can_go(monster, row, col)) {
		move_mon_to(monster, row, col);
		return 1;
	}
	return 0;
}

void move_mon_to(object *monster, short row, short col) {
	int mrow = monster->row;
	int mcol = monster->col;
	dungeon[mrow][mcol] &= ~MONSTER;
	dungeon[row][col] |= MONSTER;

	short c = (short)mvinch(mrow, mcol);
	if ((c >= 'A') && (c <= 'Z')) {
		if (!detect_monster)
			mvaddch(mrow, mcol, monster->trail_char);
		else {
			if (rogue_can_see(mrow, mcol))
				mvaddch(mrow, mcol, monster->trail_char);
			else {
				if (monster->trail_char == '.')
					monster->trail_char = ' ';
				mvaddch(mrow, mcol, monster->trail_char);
			}
		}
	}
	monster->trail_char = (short)mvinch(row, col);
	if (!blind && (detect_monster || rogue_can_see(row, col))) {
		if ((!(monster->m_flags & INVISIBLE) || (detect_monster || see_invisible || r_see_invisible)))
			mvaddch(row, col, gmc(monster));
	}
	if ((dungeon[row][col] & DOOR) && (get_room_number(row, col) != cur_room) && (dungeon[mrow][mcol] == FLOOR) && !blind)
		mvaddch(mrow, mcol, ' ');
	if (dungeon[row][col] & DOOR) {
		dr_course(monster, ((dungeon[mrow][mcol] & TUNNEL) ? 1 : 0), row, col);
	} else {
		monster->row = row;
		monster->col = col;
	}
}

bool mon_can_go(const object *monster, short row, short col) {
	short dr = monster->row - row; // check if move distance > 1
	if ((dr >= 2) || (dr <= -2))
		return 0;

	short dc = monster->col - col;
	if ((dc >= 2) || (dc <= -2))
		return 0;

	if ((!dungeon[monster->row][col]) || (!dungeon[row][monster->col]))
		return 0;
	if ((!is_passable(row, col)) || (dungeon[row][col] & MONSTER))
		return 0;
	if ((monster->row!=row)&&(monster->col!=col)&&((dungeon[row][col]&DOOR) || (dungeon[monster->row][monster->col]&DOOR)))
		return 0;

	if (!(monster->m_flags & (FLITS | CONFUSED | CAN_FLIT)) && (monster->trow == NO_ROOM)) {
		if ((monster->row < rogue.row) && (row < monster->row)) return 0;
		if ((monster->row > rogue.row) && (row > monster->row)) return 0;
		if ((monster->col < rogue.col) && (col < monster->col)) return 0;
		if ((monster->col > rogue.col) && (col > monster->col)) return 0;
	}
	if (dungeon[row][col] & OBJECT) {
		object *obj = object_at(&level_objects, row, col);
		if ((obj->what_is == SCROL) && (obj->which_kind == SCARE_MONSTER))
			return 0;
	}
	return 1;
}

void wake_up(object *monster) {
	if (!(monster->m_flags & NAPPING))
		monster->m_flags &= (~(ASLEEP | IMITATES | WAKENS));
}

void wake_room(short rn, bool entering, short row, short col) {
	short wake_percent = (rn == party_room) ? PARTY_WAKE_PERCENT : WAKE_PERCENT;
	if (stealthy > 0)
		wake_percent /= (STEALTH_FACTOR + stealthy);

	object *monster = level_monsters.next_monster;
	while (monster) {
		bool in_room = (rn == get_room_number(monster->row, monster->col));
		if (in_room) {
			if (entering)
				monster->trow = NO_ROOM;
			else {
				monster->trow = row;
				monster->tcol = col;
			}
		}
		if ((monster->m_flags & WAKENS) && (rn == get_room_number(monster->row, monster->col))) {
			if (rand_percent(wake_percent))
				wake_up(monster);
		}
		monster = monster->next_monster;
	}
}

const char * mon_name(const object *monster) {
	if (blind || ((monster->m_flags & INVISIBLE) && !(detect_monster || see_invisible || r_see_invisible)))
		return "something";
	if (halluc)
		return m_names[get_rand('A', 'Z') - 'A'];
	return m_names[monster->m_char - 'A'];
}

static short rogue_is_around(int row, int col) {
	short rdif = row - rogue.row;
	short cdif = col - rogue.col;
	return (rdif >= -1) && (rdif <= 1) && (cdif >= -1) && (cdif <= 1);
}

void wanderer(void) {
	object *monster;
	bool found = 0;
	for (short i = 0; i < 15 && !found; i++) {
		monster = gr_monster(NULL, 0);
		if (!(monster->m_flags & (WAKENS | WANDERS)))
			free_object(monster);
		else
			found = 1;
	}

	if (found) {
		found = 0;
		wake_up(monster);
		for (short i = 0; ((i < 25) && (!found)); i++) {
			short row, col;
			gr_row_col(&row, &col, (FLOOR | TUNNEL | STAIRS | OBJECT));
			if (!rogue_can_see(row, col)) {
				put_m_at(row, col, monster);
				found = 1;
			}
		}
		if (!found)
			free_object(monster);
	}
}

void show_monsters(void) {
	if (blind)
		return;

	detect_monster = 1;
	object *monster = level_monsters.next_monster;
	while (monster) {
		mvaddch(monster->row, monster->col, monster->m_char);
		if (monster->m_flags & IMITATES) {
			monster->m_flags &= (~IMITATES);
			monster->m_flags |= WAKENS;
		}
		monster = monster->next_monster;
	}
}

void create_monster(void) {
	bool found = 0;
	short row = rogue.row;
	short col = rogue.col;
	for (short i = 0; i < 9; i++) {
		rand_around(i, &row, &col);
		if ((row == rogue.row && col == rogue.col) || row < MIN_ROW || row > (DROWS - 2) || col < 0 || col > (DCOLS - 1))
			continue;
		if ((!(dungeon[row][col] & MONSTER)) && (dungeon[row][col] & (FLOOR|TUNNEL|STAIRS|DOOR))) {
			found = 1;
			break;
		}
	}

	if (found) {
		object *monster = gr_monster(NULL, 0);
		put_m_at(row, col, monster);
		mvaddch(row, col, gmc(monster));
		if (monster->m_flags & (WANDERS | WAKENS))
			wake_up(monster);
	} else
		message("you hear a faint cry of anguish in the distance", 0);
}

static void put_m_at(short row, short col, object *monster) {
	monster->row = row;
	monster->col = col;
	dungeon[row][col] |= MONSTER;
	monster->trail_char = (short)mvinch(row, col);
	add_to_pack(monster, &level_monsters, 0);
	aim_monster(monster);
}

static void aim_monster(object *monster) {
	short rn = get_room_number(monster->row, monster->col);
	if (rn == NO_ROOM)
		clean_up("aim_monster: monster not in room");

	short r = get_rand(0, 12);
	for (short i = 0; i < 4; i++) {
		short d = (r + i) % 4;
		if (rooms[rn].doors[d].oth_room != NO_ROOM) {
			monster->trow = rooms[rn].doors[d].door_row;
			monster->tcol = rooms[rn].doors[d].door_col;
			break;
		}
	}
}

int rogue_can_see(int row, int col) {
	return !blind && (
		(get_room_number(row, col) == cur_room && !(rooms[cur_room].is_room & R_MAZE)) ||
		rogue_is_around(row, col));
}

static bool move_confused(object *monster) {
	if (!(monster->m_flags & ASLEEP)) {
		if (--monster->moves_confused <= 0)
			monster->m_flags &= (~CONFUSED);
		if (monster->m_flags & STATIONARY)
			return coin_toss() ? 1 : 0;
		else if (rand_percent(15))
			return 1;

		short row = monster->row;
		short col = monster->col;
		for (short i = 0; i < 9; i++) {
			rand_around(i, &row, &col);
			if ((row == rogue.row) && (col == rogue.col))
				return 0;
			if (mtry(monster, row, col))
				return 1;
		}
	}
	return 0;
}

static bool flit(object *monster) {
	if (!rand_percent(FLIT_PERCENT + ((monster->m_flags & FLIES) ? 20 : 0)))
		return 0;
	if (rand_percent(10))
		return 1;

	short row = monster->row;
	short col = monster->col;
	for (short i = 0; i < 9; i++) {
		rand_around(i, &row, &col);
		if ((row == rogue.row) && (col == rogue.col))
			continue;
		if (mtry(monster, row, col))
			return 1;
	}
	return 1;
}

char gr_obj_char(void) {
	const char *rs = "%!?]=/):*";
	return rs[get_rand(0, 8)];
}

static bool no_room_for_monster(int rn) {
	for (short i = rooms[rn].top_row+1; i < rooms[rn].bottom_row; i++) {
		for (short j = rooms[rn].left_col+1; j < rooms[rn].right_col; j++) {
			if (!(dungeon[i][j] & MONSTER))
				return 0;
		}
	}
	return 1;
}

void aggravate(void) {
	message("you hear a high pitched humming noise", 0);

	object *monster = level_monsters.next_monster;
	while (monster) {
		wake_up(monster);
		monster->m_flags &= (~IMITATES);
		if (rogue_can_see(monster->row, monster->col))
			mvaddch(monster->row, monster->col, monster->m_char);
		monster = monster->next_monster;
	}
}

bool mon_sees(const object *monster, int row, int col) {
	short rn = get_room_number(row, col);
	if (rn != NO_ROOM && rn == get_room_number(monster->row, monster->col) && !(rooms[rn].is_room & R_MAZE))
		return 1;

	short rdif = row - monster->row;
	short cdif = col - monster->col;
	return rdif >= -1 && rdif <= 1 && cdif >= -1 && cdif <= 1;
}

void mv_aquatars(void) {
	object *monster = level_monsters.next_monster;
	while (monster) {
		if ((monster->m_char == 'A') && mon_can_go(monster, rogue.row, rogue.col)) {
			mv_1_monster(monster, rogue.row, rogue.col);
			monster->m_flags |= ALREADY_MOVED;
		}
		monster = monster->next_monster;
	}
}
