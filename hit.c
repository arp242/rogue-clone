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
 * @(#)hit.c 8.1 (Berkeley) 5/31/93
 * $FreeBSD: src/games/rogue/hit.c,v 1.6 1999/11/30 03:49:22 billf Exp $
 *
 * This source herein may be modified and/or distributed by anybody who
 * so desires, with the following restrictions:
 *    1.)  No portion of this notice shall be removed.
 *    2.)  Credit shall not be taken for the creation of this source.
 *    3.)  This code is not to be traded, sold, or used for personal
 *         gain or profit.
 */

#include "rogue.h"

static short damage_for_strength(void);
static int get_w_damage(const object *);
static int to_hit(const object *);

static object *fight_monster = NULL;
char hit_message[HIT_MESSAGE_SIZE] = "";

static void disappear(object *);
static void drain_life(void);
static void drop_level(void);
static void freeze(object *);
static short get_dir(short, short, short, short);
static bool gold_at(short, short);
static void steal_gold(object *);
static void steal_item(object *);
static void sting(object *);
static bool try_to_cough(short, short, object *);

short less_hp = 0;
bool being_held;

void mon_hit(object *monster) {
	if (fight_monster && monster != fight_monster)
		fight_monster = NULL;

	short hit_chance;

	monster->trow = NO_ROOM;
	if (cur_level >= (AMULET_LEVEL * 2))
		hit_chance = 100;
	else {
		hit_chance = monster->m_hit_chance - (((2 * rogue.exp) + (2 * ring_exp)) - r_rings);
	}
	if (wizard)
		hit_chance /= 2;

	if (!fight_monster)
		interrupted = 1;

	const char *mn = mon_name(monster);
	if (!rand_percent(hit_chance)) {
		if (!fight_monster) {
			sprintf(hit_message + strlen(hit_message), "the %s misses", mn);
			message(hit_message, 1);
			hit_message[0] = 0;
		}
		return;
	}
	if (!fight_monster) {
		sprintf(hit_message + strlen(hit_message), "the %s hit", mn);
		message(hit_message, 1);
		hit_message[0] = 0;
	}

	short damage;
	if (!(monster->m_flags & STATIONARY)) {
		damage = get_damage(monster->m_damage, 1);
		float minus;
		if (cur_level >= (AMULET_LEVEL * 2))
			minus = (float)((AMULET_LEVEL * 2) - cur_level);
		else {
			minus = (float)get_armor_class(rogue.armor) * 3.00;
			minus = minus / 100.00 * (float)damage;
		}
		damage -= (short)minus;
	} else
		damage = monster->stationary_damage++;

	if (wizard)
		damage /= 3;
	if (damage > 0)
		rogue_damage(damage, monster, 0);
	if (monster->m_flags & SPECIAL_HIT)
		special_hit(monster);
}

void rogue_hit(object *monster, bool force_hit) {
	if (!monster)
		return;
	if (check_imitator(monster))
		return;

	short hit_chance = force_hit ? 100 : get_hit_chance(rogue.weapon);
	if (wizard)
		hit_chance *= 2;
	if (!rand_percent(hit_chance)) {
		if (!fight_monster)
			strcpy(hit_message, "you miss  ");
		goto RET;
	}
	short damage = get_weapon_damage(rogue.weapon);
	if (wizard)
		damage *= 3;
	if (con_mon)
		s_con_mon(monster);
	if (mon_damage(monster, damage)) {  // still alive?
		if (!fight_monster)
			strcpy(hit_message, "you hit  ");
	}

RET:
	check_gold_seeker(monster);
	wake_up(monster);
}

void rogue_damage(short d, object *monster, short other) {
	if (d >= rogue.hp_current) {
		rogue.hp_current = 0;
		print_stats(STAT_HP);
		killed_by(monster, other);
	}
	if (d > 0) {
		rogue.hp_current -= d;
		print_stats(STAT_HP);
	}
}

int get_damage(const char *ds, bool r) {
	int i = 0, total = 0;
	while (ds[i]) {
		int n = get_number(ds+i);
		while (ds[i++] != 'd')
			;
		int d = get_number(ds+i);
		while ((ds[i] != '/') && ds[i])
			i++;

		for (int j = 0; j < n; j++) {
			if (r)
				total += get_rand(1, d);
			else
				total += d;
		}
		if (ds[i] == '/')
			i++;
	}
	return total;
}

static int get_w_damage(const object *obj) {
	if ((!obj) || (obj->what_is != WEAPON))
		return -1;

	int t_hit = get_number(obj->damage) + obj->hit_enchant;
	int i = 0;
	while (obj->damage[i++] != 'd')
		;
	int damage = get_number(obj->damage + i) + obj->d_enchant;

	char new_damage[12];
	sprintf(new_damage, "%dd%d", t_hit, damage);
	return get_damage(new_damage, 1);
}

int get_number(const char *s) {
	int i = 0, total = 0;
	while (s[i] >= '0' && s[i] <= '9') {
		total = (10 * total) + (s[i] - '0');
		i++;
	}
	return total;
}

long lget_number(const char *s) {
	short i = 0;
	long total = 0;
	while (s[i] >= '0' && s[i] <= '9') {
		total = (10 * total) + (s[i] - '0');
		i++;
	}
	return total;
}

unsigned long ulget_number(const char *s) {
	short i = 0;
	unsigned long total = 0;
	while (s[i] >= '0' && s[i] <= '9') {
		total = (10 * total) + (s[i] - '0');
		i++;
	}
	return total;
}

static int to_hit(const object *obj) {
	if (!obj)
		return 1;
	return get_number(obj->damage) + obj->hit_enchant;
}

static short damage_for_strength(void) {
	short strength = rogue.str_current + add_strength;
	if (strength <= 6)  return strength - 5;
	if (strength <= 14) return 1;
	if (strength <= 17) return 3;
	if (strength <= 18) return 4;
	if (strength <= 20) return 5;
	if (strength <= 21) return 6;
	if (strength <= 30) return 7;
	return 8;
}

bool mon_damage(object *monster, short damage) {
	monster->hp_to_kill -= damage;
	if (monster->hp_to_kill <= 0) {
		short row = monster->row;
		short col = monster->col;
		dungeon[row][col] &= ~MONSTER;
		mvaddch(row, col, (int)get_dungeon_char(row, col));

		fight_monster = NULL;
		cough_up(monster);
		const char *mn = mon_name(monster);
		sprintf(hit_message + strlen(hit_message), "defeated the %s", mn);
		message(hit_message, 1);
		hit_message[0] = 0;
		add_exp(monster->kill_exp, 1);
		take_from_pack(monster, &level_monsters);

		if (monster->m_flags & HOLDS)
			being_held = 0;
		free_object(monster);
		return 0;
	}
	return 1;
}

void fight(bool to_the_death) {
	message("fight direction? ", 0);

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
	if (ch == CANCEL)
		return;
	short row = rogue.row;
	short col = rogue.col;
	get_dir_rc(d, &row, &col, 0);

	short c = (short)mvinch(row, col);
	if (c < 'A' || c > 'Z' || !can_move(rogue.row, rogue.col, row, col)) {
		message("I see no monster there", 0);
		return;
	}
	if (!(fight_monster = object_at(&level_monsters, row, col)))
		return;

	short possible_damage;
	if (!(fight_monster->m_flags & STATIONARY))
		possible_damage = ((get_damage(fight_monster->m_damage, 0) * 2) / 3);
	else
		possible_damage = fight_monster->stationary_damage - 1;

	while (fight_monster) {
		one_move_rogue(ch, 0);
		if ((!to_the_death && rogue.hp_current <= possible_damage) || interrupted || (!(dungeon[row][col] & MONSTER)))
			fight_monster = NULL;
		else {
			object *monster = object_at(&level_monsters, row, col);
			if (monster != fight_monster)
				fight_monster = NULL;
		}
	}
}

void get_dir_rc(short dir, short *row, short *col, short allow_off_screen) {
	switch (dir) {
	case LEFT:
		if (allow_off_screen || (*col > 0))
			(*col)--;
		break;
	case DOWN:
		if (allow_off_screen || (*row < (DROWS - 2)))
			(*row)++;
		break;
	case UPWARD:
		if (allow_off_screen || (*row > MIN_ROW))
			(*row)--;
		break;
	case RIGHT:
		if (allow_off_screen || (*col < (DCOLS - 1)))
			(*col)++;
		break;
	case UPLEFT:
		if (allow_off_screen || ((*row > MIN_ROW) && (*col > 0))) {
			(*row)--;
			(*col)--;
		}
		break;
	case UPRIGHT:
		if (allow_off_screen || ((*row > MIN_ROW) && (*col < (DCOLS - 1)))) {
			(*row)--;
			(*col)++;
		}
		break;
	case DOWNRIGHT:
		if (allow_off_screen || ((*row < (DROWS - 2)) && (*col < (DCOLS - 1)))) {
			(*row)++;
			(*col)++;
		}
		break;
	case DOWNLEFT:
		if (allow_off_screen || ((*row < (DROWS - 2)) && (*col > 0))) {
			(*row)++;
			(*col)--;
		}
		break;
	}
}

short get_hit_chance(const object *weapon) {
	return 40 + (3 * to_hit(weapon)) + ((2*rogue.exp + 2*ring_exp) - r_rings);
}

short get_weapon_damage(const object *weapon) {
	return get_w_damage(weapon) + damage_for_strength() +
		((((rogue.exp + ring_exp) - r_rings) + 1) / 2);
}

void s_con_mon(object *monster) {
	if (con_mon) {
		monster->m_flags |= CONFUSED;
		monster->moves_confused += get_rand(12, 22);
		message("the monster appears confused", 0);
		con_mon = 0;
	}
}

void special_hit(object *monster) {
	if ((monster->m_flags & CONFUSED) && rand_percent(66))
		return;

	if (monster->m_flags & RUSTS)
		rust(monster);
	if ((monster->m_flags & HOLDS) && !levitate)
		being_held = 1;
	if (monster->m_flags & FREEZES)
		freeze(monster);
	if (monster->m_flags & STINGS)
		sting(monster);
	if (monster->m_flags & DRAINS_LIFE)
		drain_life();
	if (monster->m_flags & DROPS_LEVEL)
		drop_level();

	if (monster->m_flags & STEALS_GOLD)
		steal_gold(monster);
	else if (monster->m_flags & STEALS_ITEM)
		steal_item(monster);
}

void rust(object *monster) {
	if ((!rogue.armor) || (get_armor_class(rogue.armor) <= 1) ||
		(rogue.armor->which_kind == LEATHER)) {
		return;
	}
	if ((rogue.armor->is_protected) || maintain_armor) {
		if (monster && (!(monster->m_flags & RUST_VANISHED))) {
			message("the rust vanishes instantly", 0);
			monster->m_flags |= RUST_VANISHED;
		}
	} else {
		rogue.armor->d_enchant--;
		message("your armor weakens", 0);
		print_stats(STAT_ARMOR);
	}
}

static void freeze(object *monster) {
	if (rand_percent(12))
		return;

	short freeze_percent = 99 -
		(rogue.str_current + rogue.str_current/2) -
		((rogue.exp + ring_exp) * 4) -
		(get_armor_class(rogue.armor) * 5) -
		(rogue.hp_max / 3);
	if (freeze_percent > 10) {
		monster->m_flags |= FREEZING_ROGUE;
		message("you are frozen", 1);

		short n = get_rand(4, 8);
		for (short i = 0; i < n; i++)
			mv_mons();
		if (rand_percent(freeze_percent)) {
			for (short i = 0; i < 50; i++)
				mv_mons();
			killed_by(NULL, HYPOTHERMIA);
		}
		message(you_can_move_again, 1);
		monster->m_flags &= (~FREEZING_ROGUE);
	}
}

static void steal_gold(object *monster) {
	if ((rogue.gold <= 0) || rand_percent(10))
		return;

	int amount = get_rand((cur_level * 10), (cur_level * 30));
	if (amount > rogue.gold)
		amount = rogue.gold;
	rogue.gold -= amount;
	message("your purse feels lighter", 0);
	print_stats(STAT_GOLD);
	disappear(monster);
}

static void steal_item(object *monster) {
	if (rand_percent(15))
		return;

	object *obj = rogue.pack.next_object;
	if (!obj)
		goto DSPR;

	bool has_something = 0;
	while (obj) {
		if (!(obj->in_use_flags & BEING_USED)) {
			has_something = 1;
			break;
		}
		obj = obj->next_object;
	}
	if (!has_something)
		goto DSPR;

	short n = get_rand(0, MAX_PACK_COUNT);
	obj = rogue.pack.next_object;
	for (short i = 0; i <= n; i++) {
		obj = obj->next_object;
		while (!obj || (obj->in_use_flags & BEING_USED)) {
			if (!obj)
				obj = rogue.pack.next_object;
			else
				obj = obj->next_object;
		}
	}

	short t = 0;
	if (obj->what_is != WEAPON) {
		t = obj->quantity;
		obj->quantity = 1;
	}
	messagef(0, "she stole %s", get_desc_str(obj));

	obj->quantity = ((obj->what_is != WEAPON) ? t : 1);

	vanish(obj, 0, &rogue.pack);
DSPR:
	disappear(monster);
}

static void disappear(object *monster) {
	short row = monster->row;
	short col = monster->col;

	dungeon[row][col] &= ~MONSTER;
	if (rogue_can_see(row, col))
		mvaddch(row, col, get_dungeon_char(row, col));
	take_from_pack(monster, &level_monsters);
	free_object(monster);
	mon_disappeared = 1;
}

void cough_up(object *monster) {
	if (cur_level < max_level)
		return;

	object *obj;
	if (monster->m_flags & STEALS_GOLD) {
		obj = alloc_object();
		obj->what_is = GOLD;
		obj->quantity = get_rand((cur_level * 15), (cur_level * 30));
	} else {
		if (!rand_percent((int)monster->drop_percent))
			return;
		obj = gr_object();
	}
	short row = monster->row;
	short col = monster->col;

	for (short n = 0; n <= 5; n++) {
		for (short i = -n; i <= n; i++) {
			if (try_to_cough(row + n, col + i, obj))
				return;
			if (try_to_cough(row - n, col + i, obj))
				return;
		}
		for (short i = -n; i <= n; i++) {
			if (try_to_cough(row + i, col - n, obj))
				return;
			if (try_to_cough(row + i, col + n, obj))
				return;
		}
	}
	free_object(obj);
}

static bool try_to_cough(short row, short col, object *obj) {
	if (row < MIN_ROW || row > (DROWS - 2) || col < 0 || col > (DCOLS - 1))
		return 0;

	if ((!(dungeon[row][col] & (OBJECT | STAIRS | TRAP))) && (dungeon[row][col] & (TUNNEL | FLOOR | DOOR))) {
		place_at(obj, row, col);
		if (((row != rogue.row) || (col != rogue.col)) && (!(dungeon[row][col] & MONSTER)))
			mvaddch(row, col, get_dungeon_char(row, col));
		return 1;
	}
	return 0;
}

bool seek_gold(object *monster) {
	short rn = get_room_number(monster->row, monster->col);
	if (rn < 0)
		return 0;

	for (short i = rooms[rn].top_row + 1; i < rooms[rn].bottom_row; i++) {
		for (short j = rooms[rn].left_col + 1; j < rooms[rn].right_col; j++) {
			if (gold_at(i, j) && !(dungeon[i][j] & MONSTER)) {
				monster->m_flags |= CAN_FLIT;
				short s = mon_can_go(monster, i, j);
				monster->m_flags &= (~CAN_FLIT);
				if (s) {
					move_mon_to(monster, i, j);
					monster->m_flags |= ASLEEP;
					monster->m_flags &= (~(WAKENS | SEEKS_GOLD));
					return 1;
				}
				monster->m_flags &= (~SEEKS_GOLD);
				monster->m_flags |= CAN_FLIT;
				mv_1_monster(monster, i, j);
				monster->m_flags &= (~CAN_FLIT);
				monster->m_flags |= SEEKS_GOLD;
				return 1;
			}
		}
	}
	return 0;
}

static bool gold_at(short row, short col) {
	if (dungeon[row][col] & OBJECT) {
		object *obj;
		if ((obj = object_at(&level_objects, row, col)) && (obj->what_is == GOLD))
			return 1;
	}
	return 0;
}

void check_gold_seeker(object *monster) {
	monster->m_flags &= (~SEEKS_GOLD);
}

bool check_imitator(object *monster) {
	if (monster->m_flags & IMITATES) {
		wake_up(monster);
		if (!blind) {
			mvaddch(monster->row, monster->col, get_dungeon_char(monster->row, monster->col));
			check_message();
			messagef(0, "wait, that's a %s!", mon_name(monster));
		}
		return 1;
	}
	return 0;
}

bool imitating(short row, short col) {
	if (dungeon[row][col] & MONSTER) {
		object *monster;
		if ((monster = object_at(&level_monsters, row, col)) != NULL) {
			if (monster->m_flags & IMITATES)
				return 1;
		}
	}
	return 0;
}

static void sting(object *monster) {
	if ((rogue.str_current <= 3) || sustain_strength)
		return;

	short sting_chance = 35 + (6 * (6 - get_armor_class(rogue.armor)));
	if ((rogue.exp + ring_exp) > 8)
		sting_chance -= (6 * ((rogue.exp + ring_exp) - 8));

	if (rand_percent(sting_chance)) {
		messagef(0, "the %s's bite has weakened you", mon_name(monster));
		rogue.str_current--;
		print_stats(STAT_STRENGTH);
	}
}

static void drop_level(void) {
	if (rand_percent(80) || (rogue.exp <= 5))
		return;

	rogue.exp_points = level_points[rogue.exp-2] - get_rand(9, 29);
	rogue.exp -= 2;
	int hp = hp_raise();
	if (rogue.hp_current -= hp <= 0)
		rogue.hp_current = 1;
	if (rogue.hp_max -= hp <= 0)
		rogue.hp_max = 1;
	add_exp(1, 0);
}

static void drain_life(void) {
	if (rand_percent(60) || rogue.hp_max <= 30 || rogue.hp_current < 10)
		return;

	short n = get_rand(1, 3);  // 1 Hp, 2 Str, 3 both
	if (n != 2 || !sustain_strength)
		message("you feel weaker", 0);

	if (n != 2) {
		rogue.hp_max--;
		rogue.hp_current--;
		less_hp++;
	}
	if (n != 1) {
		if ((rogue.str_current > 3) && (!sustain_strength)) {
			rogue.str_current--;
			if (coin_toss())
				rogue.str_max--;
		}
	}
	print_stats((STAT_STRENGTH | STAT_HP));
}

bool m_confuse(object *monster) {
	if (!rogue_can_see(monster->row, monster->col))
		return 0;

	if (rand_percent(45)) {
		monster->m_flags &= (~CONFUSES);  // will not confuse the rogue
		return 0;
	}
	if (rand_percent(55)) {
		monster->m_flags &= (~CONFUSES);
		messagef(1, "the gaze of the %s has confused you", mon_name(monster));
		cnfs();
		return 1;
	}
	return 0;
}

bool flame_broil(object *monster) {
	if ((!mon_sees(monster, rogue.row, rogue.col)) || coin_toss())
		return 0;

	short row = rogue.row - monster->row;
	short col = rogue.col - monster->col;
	if (row < 0)
		row = -row;
	if (col < 0)
		col = -col;
	if (((row != 0) && (col != 0) && (row != col)) || ((row > 7) || (col > 7)))
		return 0;

	short dir = get_dir(monster->row, monster->col, row, col);
	bounce(FIRE, dir, monster->row, monster->col, 0);

	return 1;
}

static short get_dir(short srow, short scol, short drow, short dcol) {
	if (srow == drow) {
		if (scol < dcol)
			return RIGHT;
		else
			return LEFT;
	}
	if (scol == dcol) {
		if (srow < drow)
			return DOWN;
		else
			return UPWARD;
	}
	if ((srow > drow) && (scol > dcol))
		return UPLEFT;
	if ((srow < drow) && (scol < dcol))
		return DOWNRIGHT;
	if ((srow < drow) && (scol > dcol))
		return DOWNLEFT;
	// if ((srow > drow) && (scol < dcol))
	return UPRIGHT;
}
