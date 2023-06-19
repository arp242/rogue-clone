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
 * @(#)throw.c 8.1 (Berkeley) 5/31/93
 * $FreeBSD: src/games/rogue/throw.c,v 1.3 1999/11/30 03:49:28 billf Exp $
 *
 * This source herein may be modified and/or distributed by anybody who
 * so desires, with the following restrictions:
 *    1.)  No portion of this notice shall be removed.
 *    2.)  Credit shall not be taken for the creation of this source.
 *    3.)  This code is not to be traded, sold, or used for personal
 *         gain or profit.
 */

#include "rogue.h"

static void flop_weapon(object *, short, short);
static object *get_thrown_at_monster(object *, short, short *, short *);
static bool throw_at_monster(object *, object *);

void throw(void) {
	message("throw direction? ", 0);

	bool first_miss = 1;
	int d, dir;
	while (!is_direction(dir = rgetchar(), &d)) {
		sound_bell();
		if (first_miss) {
			message("direction? ", 0);
			first_miss = 0;
		}
	}
	check_message();
	if (dir == CANCEL)
		return;

	short wch = pack_letter("throw what?", WEAPON);
	if (wch == CANCEL)
		return;
	check_message();

	object *weapon = get_letter_object(wch);
	if (!weapon) {
		message("no such item.", 0);
		return;
	}
	if ((weapon->in_use_flags & BEING_USED) && weapon->is_cursed) {
		message(curse_message, 0);
		return;
	}

	short row = rogue.row, col = rogue.col;

	if ((weapon->in_use_flags & BEING_WIELDED) && (weapon->quantity <= 1))
		unwield(rogue.weapon);
	else if (weapon->in_use_flags & BEING_WORN) {
		mv_aquatars();
		unwear(rogue.armor);
		print_stats(STAT_ARMOR);
	} else if (weapon->in_use_flags & ON_EITHER_HAND)
		un_put_on(weapon);

	object *monster = get_thrown_at_monster(weapon, d, &row, &col);
	mvaddch(rogue.row, rogue.col, rogue.fchar);
	refresh();

	if (rogue_can_see(row, col) && ((row != rogue.row) || (col != rogue.col)))
		mvaddch(row, col, get_dungeon_char(row, col));

	if (monster) {
		wake_up(monster);
		check_gold_seeker(monster);

		if (!throw_at_monster(monster, weapon))
			flop_weapon(weapon, row, col);
	} else
		flop_weapon(weapon, row, col);
	vanish(weapon, 1, &rogue.pack);
}

static bool throw_at_monster(object *monster, object *weapon) {
	short hit_chance = get_hit_chance(weapon);
	short damage = get_weapon_damage(weapon);
	if (weapon->which_kind == ARROW && (rogue.weapon && rogue.weapon->which_kind == BOW)) {
		damage += get_weapon_damage(rogue.weapon);
		damage = ((damage * 2) / 3);
		hit_chance += (hit_chance / 3);
	} else if ((weapon->in_use_flags & BEING_WIELDED) && (
				(weapon->which_kind == DAGGER) ||
				(weapon->which_kind == SHURIKEN) ||
				(weapon->which_kind == DART)
	)) {
		damage = ((damage * 3) / 2);
		hit_chance += (hit_chance / 3);
	}
	short t = weapon->quantity;
	weapon->quantity = 1;
	sprintf(hit_message, "the %s", name_of(weapon));
	weapon->quantity = t;

	if (!rand_percent(hit_chance)) {
		strcat(hit_message, "misses  ");
		return 0;
	}
	s_con_mon(monster);
	strcat(hit_message, "hit  ");
	mon_damage(monster, damage);
	return 1;
}

static object * get_thrown_at_monster(object *obj, short dir, short *row, short *col) {
	short orow = *row, ocol = *col;
	short ch = get_mask_char(obj->what_is);

	for (short i = 0; i < 24; i++) {
		get_dir_rc(dir, row, col, 0);
		if (((*col <= 0 || *col >= DCOLS - 1) || dungeon[*row][*col] == NOTHING) ||
			((dungeon[*row][*col] & (HORWALL | VERTWALL | HIDDEN)) && (!(dungeon[*row][*col] & TRAP)))
		) {
			*row = orow;
			*col = ocol;
			return 0;
		}
		if ((i != 0) && rogue_can_see(orow, ocol))
			mvaddch(orow, ocol, get_dungeon_char(orow, ocol));

		if (rogue_can_see(*row, *col)) {
			if (!(dungeon[*row][*col] & MONSTER))
				mvaddch(*row, *col, ch);
			refresh();
		}
		orow = *row; ocol = *col;
		if (dungeon[*row][*col] & MONSTER) {
			if (!imitating(*row, *col))
				return object_at(&level_monsters, *row, *col);
		}
		if (dungeon[*row][*col] & TUNNEL)
			i += 2;
	}
	return 0;
}

static void flop_weapon(object *weapon, short row, short col) {
	bool found = 0;
	short i = 0;
	while ((i < 9) && dungeon[row][col] & ~(FLOOR | TUNNEL | DOOR | MONSTER)) {
		rand_around(i++, &row, &col);
		if ((row > (DROWS - 2)) || (row < MIN_ROW) ||
			(col > (DCOLS - 1)) || (col < 0) || (!dungeon[row][col]) ||
			(dungeon[row][col] & ~(FLOOR | TUNNEL | DOOR | MONSTER))
		) {
			continue;
		}
		found = 1;
		break;
	}

	if (found || (i == 0)) {
		object *new_weapon = alloc_object();
		*new_weapon = *weapon;
		new_weapon->in_use_flags = NOT_USED;
		new_weapon->quantity = 1;
		new_weapon->ichar = 'L';
		place_at(new_weapon, row, col);
		if (rogue_can_see(row, col) && ((row != rogue.row) || (col != rogue.col))) {
			unsigned short mon = dungeon[row][col] & MONSTER;
			dungeon[row][col] &= (~MONSTER);
			short dch = get_dungeon_char(row, col);
			if (mon) {
				short mch = (short)mvinch(row, col);
				object *monster;
				if ((monster = object_at(&level_monsters,
				    row, col)) != NULL) {
					monster->trail_char = dch;
				}
				if ((mch < 'A') || (mch > 'Z'))
					mvaddch(row, col, dch);
			} else
				mvaddch(row, col, dch);
			dungeon[row][col] |= mon;
		}
	} else {
		short t = weapon->quantity;
		weapon->quantity = 1;
		messagef(0, "the %svanishes as it hits the ground", name_of(weapon));
		weapon->quantity = t;
	}
}

void rand_around(short i, short *r, short *c) {
	static char pos[] = "\010\007\001\003\004\005\002\006\0";
	static short row, col;
	if (i == 0) {
		row = *r;
		col = *c;

		short o = get_rand(1, 8);
		for (short j = 0; j < 5; j++) {
			short x = get_rand(0, 8);
			short y = (x + o) % 9;
			short t = pos[x];
			pos[x] = pos[y];
			pos[y] = t;
		}
	}
	switch ((short)pos[i]) {
	case 0: *r = row + 1;  *c = col + 1;  break;
	case 1: *r = row + 1;  *c = col - 1;  break;
	case 2: *r = row - 1;  *c = col + 1;  break;
	case 3: *r = row - 1;  *c = col - 1;  break;
	case 4: *r = row;      *c = col + 1;  break;
	case 5: *r = row + 1;  *c = col;      break;
	case 6: *r = row;      *c = col;      break;
	case 7: *r = row - 1;  *c = col;      break;
	case 8: *r = row;      *c = col - 1;  break;
	}
}
