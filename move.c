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
 * @(#)move.c 8.1 (Berkeley) 5/31/93
 * $FreeBSD: src/games/rogue/move.c,v 1.7 1999/11/30 03:49:24 billf Exp $
 *
 * This source herein may be modified and/or distributed by anybody who
 * so desires, with the following restrictions:
 *    1.)  No portion of this notice shall be removed.
 *    2.)  Credit shall not be taken for the creation of this source.
 *    3.)  This code is not to be traded, sold, or used for personal
 *         gain or profit.
 */

#include "rogue.h"

short m_moves = 0;
bool revshift = false;
const char you_can_move_again[] = "you can move again";

static bool can_turn(short, short);
static bool check_hunger(bool);
static short gr_dir(void);
static void heal(void);
static bool next_to_something(int, int);
static void turn_passage(short, bool);

short one_move_rogue(int dirch, short pickup) {
	if (confused)
		dirch = gr_dir();

	int d = -1;
	short row = rogue.row, col = rogue.col;
	is_direction(dirch, &d);
	get_dir_rc(d, &row, &col, 1);
	if (!can_move(rogue.row, rogue.col, row, col))
		return MOVE_FAILED;

	if (being_held || bear_trap) {
		if (!(dungeon[row][col] & MONSTER)) {
			if (being_held) {
				message("you are being held", 1);
			} else {
				message("you are still stuck in the bear trap", 0);
				reg_move();
			}
			return MOVE_FAILED;
		}
	}
	if (r_teleport) {
		if (rand_percent(R_TELE_PERCENT)) {
			tele();
			return STOPPED_ON_SOMETHING;
		}
	}
	if (dungeon[row][col] & MONSTER) {
		rogue_hit(object_at(&level_monsters, row, col), 0);
		reg_move();
		return MOVE_FAILED;
	}
	if (dungeon[row][col] & DOOR) {
		if (cur_room == PASSAGE) {
			cur_room = get_room_number(row, col);
			if (cur_room == NO_ROOM)
				clean_up("one_move_rogue: door to nowhere");
			light_up_room(cur_room);
			wake_room(cur_room, 1, row, col);
		} else {
			light_passage(row, col);
		}
	} else if ((dungeon[rogue.row][rogue.col] & DOOR) && (dungeon[row][col] & TUNNEL)) {
		light_passage(row, col);
		wake_room(cur_room, 0, rogue.row, rogue.col);
		darken_room(cur_room);
		cur_room = PASSAGE;
	} else if (dungeon[row][col] & TUNNEL)
		light_passage(row, col);

	mvaddch(rogue.row, rogue.col, get_dungeon_char(rogue.row, rogue.col));
	mvaddch(row, col, rogue.fchar);
	refresh();
	rogue.row = row;
	rogue.col = col;
	object *obj;
	char desc[DCOLS];
	if (dungeon[row][col] & OBJECT) {
		if (levitate && pickup)
			return STOPPED_ON_SOMETHING;

		if (pickup && !levitate) {
			short status;
			if ((obj = pick_up(row, col, &status)) != NULL) {
				get_desc(obj, desc);
				if (obj->what_is == GOLD) {
					free_object(obj);
					goto not_in_pack;
				}
			} else if (!status)
				goto moved;
			else
				goto move_on;
		} else {
move_on:
			obj = object_at(&level_objects, row, col);
			strcpy(desc, "moved onto ");
			get_desc(obj, desc + 11);
			goto not_in_pack;
		}
		short n = strlen(desc);
		desc[n] = '(';
		desc[n+1] = obj->ichar;
		desc[n+2] = ')';
		desc[n+3] = 0;
not_in_pack:
		message(desc, 1);
		reg_move();
		return STOPPED_ON_SOMETHING;
	}
	if (dungeon[row][col] & (DOOR | STAIRS | TRAP)) {
		if ((!levitate) && (dungeon[row][col] & TRAP))
			trap_player(row, col);
		reg_move();
		return STOPPED_ON_SOMETHING;
	}
moved:
	if (reg_move())  // fainted from hunger
		return STOPPED_ON_SOMETHING;
	return confused ? STOPPED_ON_SOMETHING : MOVED;
}

void multiple_move_rogue(short dirch) {
	if (revshift)
		dirch ^= 0x40;

	switch (dirch) {
	case 'H': case 'J': case 'K': case 'L': case 'B': case 'Y': case 'U': case 'N':
		while (!interrupted && one_move_rogue((dirch + 32), 1) == MOVED)
			;

		if (!interrupted && passgo && (dungeon[rogue.row][rogue.col] & TUNNEL))
			turn_passage(dirch + 32, 1);
		return;
	}

	short m, row, col;
	do {
		row = rogue.row;
		col = rogue.col;
		if ((m = one_move_rogue((dirch + 96), 1)) == MOVE_FAILED || m == STOPPED_ON_SOMETHING || interrupted)
			break;
	} while (!next_to_something(row, col));
	if (!interrupted && passgo && m == MOVE_FAILED && (dungeon[rogue.row][rogue.col] & TUNNEL))
		turn_passage(dirch + 96, 0);
}

bool is_passable(int row, int col) {
	if ((row < MIN_ROW) || (row > (DROWS - 2)) || (col < 0) || (col > (DCOLS-1)))
		return 0;
	if (dungeon[row][col] & HIDDEN)
		return (dungeon[row][col] & TRAP) ? 1 : 0;
	return dungeon[row][col] & (FLOOR | TUNNEL | DOOR | STAIRS | TRAP);
}

static bool next_to_something(int drow, int dcol) {
	if (confused)
		return 1;
	if (blind)
		return 0;

	short pass_count = 0;
	short i_end = (rogue.row < (DROWS-2)) ? 1 : 0;
	short j_end = (rogue.col < (DCOLS-1)) ? 1 : 0;

	for (short i = ((rogue.row > MIN_ROW) ? -1 : 0); i <= i_end; i++) {
		for (short j = ((rogue.col > 0) ? -1 : 0); j <= j_end; j++) {
			if (i == 0 && j == 0)
				continue;
			if (rogue.row + i == drow && rogue.col + j == dcol)
				continue;

			short row = rogue.row + i;
			short col = rogue.col + j;
			unsigned short s = dungeon[row][col];
			if (s & HIDDEN)
				continue;

			// If the rogue used to be right, up, left, down, or right of
			// row,col, and now isn't, then don't stop.
			if (s & (MONSTER | OBJECT | STAIRS)) {
				if ((row == drow || col == dcol) && (!(row == rogue.row || col == rogue.col)))
					continue;
				return 1;
			}
			if (s & TRAP) {
				if (!(s & HIDDEN)) {
					if (((row == drow) || (col == dcol)) && (!((row == rogue.row) || (col == rogue.col))))
						continue;
					return 1;
				}
			}
			if ((i - j == 1 || i - j == -1) && (s & TUNNEL)) {
				if (++pass_count > 1)
					return 1;
			}
			if ((s & DOOR) && ((i == 0) || (j == 0)))
				return 1;
		}
	}
	return 0;
}

bool can_move(short row1, short col1, short row2, short col2) {
	if (!is_passable(row2, col2))
		return 0;

	if (row1 != row2 && col1 != col2) {
		if ((dungeon[row1][col1] & DOOR) || (dungeon[row2][col2] & DOOR))
			return 0;
		if ((!dungeon[row1][col2]) || (!dungeon[row2][col1]))
			return 0;
	}
	return 1;
}

void move_onto(void) {
	message("move direction? ", 0);

	int ch, d;
	bool first_miss = 1;
	while (!is_direction(ch = rgetchar(), &d)) {
		sound_bell();
		if (first_miss) {
			message("direction? ", 0);
			first_miss = 0;
		}
	}
	check_message();
	if (ch != CANCEL)
		one_move_rogue(ch, 0);
}

bool is_direction(int c, int *d) {
	if (c >= '1' && c <= '9' && c != '5')
		c = "bjnh lyku"[c - '1'];
	switch (c) {
	case 'h': case KEY_LEFT:  *d = LEFT;      return 1;
	case 'j': case KEY_DOWN:  *d = DOWN;      return 1;
	case 'k': case KEY_UP:    *d = UPWARD;    return 1;
	case 'l': case KEY_RIGHT: *d = RIGHT;     return 1;
	case 'b':                 *d = DOWNLEFT;  return 1;
	case 'y':                 *d = UPLEFT;    return 1;
	case 'u':                 *d = UPRIGHT;   return 1;
	case 'n':                 *d = DOWNRIGHT; return 1;
	case CANCEL:                              return 1;
	default:                                  return 0;
	}
}

static bool check_hunger(bool msg_only) {
	if (rogue.moves_left == HUNGRY) {
		strcpy(hunger_str, "hungry");
		message(hunger_str, 0);
		print_stats(STAT_HUNGER);
	}
	if (rogue.moves_left == WEAK) {
		strcpy(hunger_str, "weak");
		message(hunger_str, 1);
		print_stats(STAT_HUNGER);
	}

	bool fainted = 0;
	if (rogue.moves_left <= FAINT) {
		if (rogue.moves_left == FAINT) {
			strcpy(hunger_str, "faint");
			message(hunger_str, 1);
			print_stats(STAT_HUNGER);
		}
		short n = get_rand(0, (FAINT - rogue.moves_left));
		if (n > 0) {
			fainted = 1;
			if (rand_percent(40))
				rogue.moves_left++;
			message("you faint", 1);
			for (short i = 0; i < n; i++) {
				if (coin_toss())
					mv_mons();
			}
			message(you_can_move_again, 1);
		}
	}
	if (msg_only)
		return fainted;
	if (rogue.moves_left <= STARVE)
		killed_by(NULL, STARVATION);

	switch (e_rings) {
	case -1:
		rogue.moves_left -= (rogue.moves_left % 2);
		break;
	case 0:
		rogue.moves_left--;
		break;
	case 1:
		rogue.moves_left--;
		check_hunger(1);
		rogue.moves_left -= (rogue.moves_left % 2);
		break;
	case 2:
		rogue.moves_left--;
		check_hunger(1);
		rogue.moves_left--;
		break;
	}
	return fainted;
}

bool reg_move(void) {
	bool fainted = 0;
	if (rogue.moves_left <= HUNGRY || cur_level >= max_level)
		fainted = check_hunger(0);

	mv_mons();

	if (++m_moves >= 120) {
		m_moves = 0;
		wanderer();
		if (!noautosave)
			save_into_file(save_file);
	}
	if (halluc) {
		if (!(--halluc))
			unhallucinate();
		else
			hallucinate();
	}
	if (blind) {
		if (!(--blind))
			unblind();
	}
	if (confused) {
		if (!(--confused))
			unconfuse();
	}
	if (bear_trap)
		bear_trap--;
	if (levitate) {
		if (!(--levitate)) {
			message("you float gently to the ground", 1);
			if (dungeon[rogue.row][rogue.col] & TRAP) {
				trap_player(rogue.row, rogue.col);
			}
		}
	}
	if (haste_self) {
		if (!(--haste_self))
			message("you feel yourself slowing down", 0);
	}
	heal();
	if (auto_search > 0)
		search(auto_search, auto_search);
	return fainted;
}

void rest(int count) {
	interrupted = 0;
	for (int i = 0; i < count; i++) {
		if (interrupted)
			break;
		reg_move();
	}
}

static short gr_dir(void) {
	switch (get_rand(1, 8)) {
	case 1:  return 'j';
	case 2:  return 'k';
	case 3:  return 'l';
	case 4:  return 'h';
	case 5:  return 'y';
	case 6:  return 'u';
	case 7:  return 'b';
	case 8:  return 'n';
	default: return 0;  // Unreachable
	}
}

static void heal(void) {
	static short heal_exp = -1, n, c = 0;
	static bool alt;

	if (rogue.hp_current == rogue.hp_max) {
		c = 0;
		return;
	}
	if (rogue.exp != heal_exp) {
		heal_exp = rogue.exp;
		switch (heal_exp) {
		case 1:  n = 20; break;
		case 2:  n = 18; break;
		case 3:  n = 17; break;
		case 4:  n = 14; break;
		case 5:  n = 13; break;
		case 6:  n = 10; break;
		case 7:  n = 9;  break;
		case 8:  n = 8;  break;
		case 9:  n = 7;  break;
		case 10: n = 4;  break;
		case 11: n = 3;  break;
		case 12:
		default: n = 2;
		}
	}
	if (++c >= n) {
		c = 0;
		rogue.hp_current++;
		if ((alt = !alt) != 0)
			rogue.hp_current++;
		if ((rogue.hp_current += regeneration) > rogue.hp_max)
			rogue.hp_current = rogue.hp_max;
		print_stats(STAT_HP);
	}
}

static bool can_turn(short nrow, short ncol) {
	if ((dungeon[nrow][ncol] & TUNNEL) && is_passable(nrow, ncol))
		return 1;
	return 0;
}

static void turn_passage(short dir, bool fast) {
	short crow = rogue.row, ccol = rogue.col, turns = 0, ndir = 0;
	if (dir != 'h' && can_turn(crow, ccol + 1)) {
		turns++;
		ndir = 'l';
	}
	if (dir != 'l' && can_turn(crow, ccol - 1)) {
		turns++;
		ndir = 'h';
	}
	if (dir != 'k' && can_turn(crow + 1, ccol)) {
		turns++;
		ndir = 'j';
	}
	if (dir != 'j' && can_turn(crow - 1, ccol)) {
		turns++;
		ndir = 'k';
	}
	if (turns == 1)
		multiple_move_rogue(ndir - (fast ? 32 : 96));
}
