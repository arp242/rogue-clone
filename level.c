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
 * @(#)level.c 8.1 (Berkeley) 5/31/93
 * $FreeBSD: src/games/rogue/level.c,v 1.3 1999/11/30 03:49:23 billf Exp $
 *
 * This source herein may be modified and/or distributed by anybody who
 * so desires, with the following restrictions:
 *    1.)  No portion of this notice shall be removed.
 *    2.)  Credit shall not be taken for the creation of this source.
 *    3.)  This code is not to be traded, sold, or used for personal
 *         gain or profit.
 */

#include "rogue.h"

#define SWAP(x,y) {short t = (x); (x) = (y); (y) = t;}

static void add_mazes(void);
static bool connect_rooms(short, short);
static void draw_simple_passage(short, short, short, short, short);
static void fill_it(int, bool);
static void fill_out_level(void);
static short get_exp_level(long);
static void hide_boxed_passage(short, short, short, short, short);
static void make_maze(short, short, short, short, short, short);
static void make_room(short, short, short, short);
static bool mask_room(short, short *, short *, unsigned short);
static void mix_random_rooms(void);
static void put_door(room *, short, short *, short *);
static void recursive_deadend(short, const short *, short, short);
static bool same_col(short, short);
static bool same_row(short, short);

short cur_level = 0;
short max_level = 1;
short cur_room;
const char *new_level_message = NULL;
short party_room = NO_ROOM;

static short r_de;

const long level_points[MAX_EXP_LEVEL] = {
		  10L,
		  20L,
		  40L,
		  80L,
		 160L,
		 320L,
		 640L,
		1300L,
		2600L,
		5200L,
	   10000L,
	   20000L,
	   40000L,
	   80000L,
	  160000L,
	  320000L,
	 1000000L,
	 3333333L,
	 6666666L,
	  MAX_EXP,
	99900000L
};

static short random_rooms[MAXROOMS] = {3, 7, 5, 2, 0, 6, 1, 4, 8};

void make_level(void) {
	if (cur_level < LAST_DUNGEON)
		cur_level++;
	if (cur_level > max_level)
		max_level = cur_level;

	short must_1 = get_rand(0, 5), must_2 = 0, must_3 = 0;
	switch (must_1) {
	case 0: must_1 = 0; must_2 = 1; must_3 = 2; break;
	case 1: must_1 = 3; must_2 = 4; must_3 = 5; break;
	case 2: must_1 = 6; must_2 = 7; must_3 = 8; break;
	case 3: must_1 = 0; must_2 = 3; must_3 = 6; break;
	case 4: must_1 = 1; must_2 = 4; must_3 = 7; break;
	case 5: must_1 = 2; must_2 = 5; must_3 = 8; break;
	}
	if (rand_percent(8))
		party_room = 0;

	bool big_room = ((party_room != NO_ROOM) && rand_percent(1));
	if (big_room) {
		make_room(BIG_ROOM, 0, 0, 0);
	} else {
		for (short i = 0; i < MAXROOMS; i++)
			make_room(i, must_1, must_2, must_3);
	}
	if (!big_room) {
		add_mazes();
		mix_random_rooms();
		for (short j = 0; j < MAXROOMS; j++) {
			short i = random_rooms[j];
			if (i < (MAXROOMS-1))
				connect_rooms(i, i+1);
			if (i < (MAXROOMS-3))
				connect_rooms(i, i+3);
			if (i < (MAXROOMS-2)) {
				if (rooms[i+1].is_room & R_NOTHING) {
					if (connect_rooms(i, i+2))
						rooms[i+1].is_room = R_CROSS;
				}
			}
			if (i < (MAXROOMS-6)) {
				if (rooms[i+3].is_room & R_NOTHING) {
					if (connect_rooms(i, i+6))
						rooms[i+3].is_room = R_CROSS;
				}
			}
			if (is_all_connected())
				break;
		}
		fill_out_level();
	}
	if (!has_amulet() && cur_level >= AMULET_LEVEL)
		put_amulet();
}

static void make_room(short rn, short r1, short r2, short r3) {
	short left_col = 0, right_col = 0, top_row = 0, bottom_row = 0;
	switch (rn) {
	case 0:
		left_col = 0;
		right_col = COL1-1;
		top_row = MIN_ROW;
		bottom_row = ROW1-1;
		break;
	case 1:
		left_col = COL1+1;
		right_col = COL2-1;
		top_row = MIN_ROW;
		bottom_row = ROW1-1;
		break;
	case 2:
		left_col = COL2+1;
		right_col = DCOLS-1;
		top_row = MIN_ROW;
		bottom_row = ROW1-1;
		break;
	case 3:
		left_col = 0;
		right_col = COL1-1;
		top_row = ROW1+1;
		bottom_row = ROW2-1;
		break;
	case 4:
		left_col = COL1+1;
		right_col = COL2-1;
		top_row = ROW1+1;
		bottom_row = ROW2-1;
		break;
	case 5:
		left_col = COL2+1;
		right_col = DCOLS-1;
		top_row = ROW1+1;
		bottom_row = ROW2-1;
		break;
	case 6:
		left_col = 0;
		right_col = COL1-1;
		top_row = ROW2+1;
		bottom_row = DROWS - 2;
		break;
	case 7:
		left_col = COL1+1;
		right_col = COL2-1;
		top_row = ROW2+1;
		bottom_row = DROWS - 2;
		break;
	case 8:
		left_col = COL2+1;
		right_col = DCOLS-1;
		top_row = ROW2+1;
		bottom_row = DROWS - 2;
		break;
	case BIG_ROOM:
		top_row = get_rand(MIN_ROW, MIN_ROW+5);
		bottom_row = get_rand(DROWS-7, DROWS-2);
		left_col = get_rand(0, 10);
		right_col = get_rand(DCOLS-11, DCOLS-1);
		rn = 0;
		goto B;
	}

	short height = get_rand(4, (bottom_row - top_row + 1));
	short width = get_rand(7, (right_col - left_col - 2));
	short row_offset = get_rand(0, ((bottom_row - top_row) - height + 1));
	short col_offset = get_rand(0, ((right_col - left_col) - width + 1));

	top_row += row_offset;
	bottom_row = top_row + height - 1;
	left_col += col_offset;
	right_col = left_col + width - 1;

	if (rn != r1 && rn != r2 && rn != r3 && rand_percent(40))
		goto END;
B:
	rooms[rn].is_room = R_ROOM;

	for (short i = top_row; i <= bottom_row; i++) {
		for (short j = left_col; j <= right_col; j++) {
			short ch;
			if (i == top_row || i == bottom_row)
				ch = HORWALL;
			else if ((i != top_row && i != bottom_row) && (j == left_col || j == right_col))
				ch = VERTWALL;
			else
				ch = FLOOR;
			dungeon[i][j] = ch;
		}
	}
END:
	rooms[rn].top_row = top_row;
	rooms[rn].bottom_row = bottom_row;
	rooms[rn].left_col = left_col;
	rooms[rn].right_col = right_col;
}

static bool connect_rooms(short room1, short room2) {
	short row1, col1, row2, col2, dir;
	if ((!(rooms[room1].is_room & (R_ROOM | R_MAZE))) || (!(rooms[room2].is_room & (R_ROOM | R_MAZE))))
		return 0;
	if (same_row(room1, room2) && (rooms[room1].left_col > rooms[room2].right_col)) {
		put_door(&rooms[room1], LEFT, &row1, &col1);
		put_door(&rooms[room2], RIGHT, &row2, &col2);
		dir = LEFT;
	} else if (same_row(room1, room2) && (rooms[room2].left_col > rooms[room1].right_col)) {
		put_door(&rooms[room1], RIGHT, &row1, &col1);
		put_door(&rooms[room2], LEFT, &row2, &col2);
		dir = RIGHT;
	} else if (same_col(room1, room2) && (rooms[room1].top_row > rooms[room2].bottom_row)) {
		put_door(&rooms[room1], UPWARD, &row1, &col1);
		put_door(&rooms[room2], DOWN, &row2, &col2);
		dir = UPWARD;
	} else if (same_col(room1, room2) && (rooms[room2].top_row > rooms[room1].bottom_row)) {
		put_door(&rooms[room1], DOWN, &row1, &col1);
		put_door(&rooms[room2], UPWARD, &row2, &col2);
		dir = DOWN;
	} else
		return 0;

	do {
		draw_simple_passage(row1, col1, row2, col2, dir);
	} while (rand_percent(4));

	rooms[room1].doors[dir/2].oth_room = room2;
	rooms[room1].doors[dir/2].oth_row = row2;
	rooms[room1].doors[dir/2].oth_col = col2;

	rooms[room2].doors[((dir + 4) % DIRS) / 2].oth_room = room1;
	rooms[room2].doors[((dir + 4) % DIRS) / 2].oth_row = row1;
	rooms[room2].doors[((dir + 4) % DIRS) / 2].oth_col = col1;
	return 1;
}

void clear_level(void) {
	for (short i = 0; i < MAXROOMS; i++) {
		rooms[i].is_room = R_NOTHING;
		for (short j = 0; j < 4; j++)
			rooms[i].doors[j].oth_room = NO_ROOM;
	}

	for (short i = 0; i < MAX_TRAPS; i++)
		traps[i].trap_type = NO_TRAP;
	for (short i = 0; i < DROWS; i++) {
		for (short j = 0; j < DCOLS; j++)
			dungeon[i][j] = NOTHING;
	}
	detect_monster = see_invisible = 0;
	bear_trap = being_held = 0;
	party_room = NO_ROOM;
	rogue.row = rogue.col = -1;
	clear();
}

static void put_door(room *rm, short dir, short *row, short *col) {
	short wall_width = (rm->is_room & R_MAZE) ? 0 : 1;
	switch (dir) {
	case UPWARD:
	case DOWN:
		*row = (dir == UPWARD ? rm->top_row : rm->bottom_row);
		do {
			*col = get_rand(rm->left_col+wall_width, rm->right_col-wall_width);
		} while (!(dungeon[*row][*col] & (HORWALL | TUNNEL)));
		break;
	case RIGHT:
	case LEFT:
		*col = (dir == LEFT ? rm->left_col : rm->right_col);
		do {
			*row = get_rand(rm->top_row+wall_width, rm->bottom_row-wall_width);
		} while (!(dungeon[*row][*col] & (VERTWALL | TUNNEL)));
		break;
	}
	if (rm->is_room & R_ROOM)
		dungeon[*row][*col] = DOOR;
	if (cur_level > 2 && rand_percent(HIDE_PERCENT))
		dungeon[*row][*col] |= HIDDEN;
	rm->doors[dir / 2].door_row = *row;
	rm->doors[dir / 2].door_col = *col;
}

static void draw_simple_passage(short row1, short col1, short row2, short col2, short dir) {
	short middle;
	if ((dir == LEFT) || (dir == RIGHT)) {
		if (col1 > col2) {
			SWAP(row1, row2);
			SWAP(col1, col2);
		}
		middle = get_rand(col1+1, col2-1);
		for (short i = col1 + 1; i != middle; i++)
			dungeon[row1][i] = TUNNEL;
		for (short i = row1; i != row2; i += (row1 > row2) ? -1 : 1)
			dungeon[i][middle] = TUNNEL;
		for (short i = middle; i != col2; i++)
			dungeon[row2][i] = TUNNEL;
	} else {
		if (row1 > row2) {
			SWAP(row1, row2);
			SWAP(col1, col2);
		}
		middle = get_rand(row1+1, row2-1);
		for (short i = row1 + 1; i != middle; i++)
			dungeon[i][col1] = TUNNEL;
		for (short i = col1; i != col2; i += (col1 > col2) ? -1 : 1)
			dungeon[middle][i] = TUNNEL;
		for (short i = middle; i != row2; i++)
			dungeon[i][col2] = TUNNEL;
	}
	if (rand_percent(HIDE_PERCENT))
		hide_boxed_passage(row1, col1, row2, col2, 1);
}

static bool same_row(short room1, short room2) {
	return (room1 / 3) == (room2 / 3);
}

static bool same_col(short room1, short room2) {
	return (room1 % 3) == (room2 % 3);
}

static void add_mazes(void) {
	if (cur_level <= 1)
		return;

	short maze_percent = (cur_level * 5) / 4;
	if (cur_level > 15)
		maze_percent += cur_level;

	short start = get_rand(0, (MAXROOMS-1));
	for (short i = 0; i < MAXROOMS; i++) {
		short j = ((start + i) % MAXROOMS);
		if (rooms[j].is_room & R_NOTHING) {
			if (rand_percent(maze_percent)) {
				rooms[j].is_room = R_MAZE;
				make_maze(get_rand(rooms[j].top_row+1, rooms[j].bottom_row-1),
					get_rand(rooms[j].left_col+1, rooms[j].right_col-1),
					rooms[j].top_row, rooms[j].bottom_row,
					rooms[j].left_col, rooms[j].right_col);
				hide_boxed_passage(rooms[j].top_row, rooms[j].left_col,
					rooms[j].bottom_row, rooms[j].right_col,
					get_rand(0, 2));
			}
		}
	}
}

static void fill_out_level(void) {
	mix_random_rooms();
	r_de = NO_ROOM;

	for (short i = 0; i < MAXROOMS; i++) {
		short rn = random_rooms[i];
		if ((rooms[rn].is_room & R_NOTHING) || ((rooms[rn].is_room & R_CROSS) && coin_toss()))
			fill_it(rn, 1);
	}
	if (r_de != NO_ROOM)
		fill_it(r_de, 0);
}

static void fill_it(int rn, bool do_rec_de) {
	static short offsets[4] = {-1, 1, 3, -3};
	short srow, scol;
	for (short i = 0; i < 10; i++) {
		srow = get_rand(0, 3);
		scol = get_rand(0, 3);
		short t = offsets[srow];
		offsets[srow] = offsets[scol];
		offsets[scol] = t;
	}

	bool did_this = 0;
	short rooms_found = 0;
	for (short i = 0; i < 4; i++) {
		short target_room = rn + offsets[i];

		if ((target_room < 0 || target_room >= MAXROOMS) ||
			(!(same_row(rn,target_room) || same_col(rn,target_room))) ||
			(!(rooms[target_room].is_room & (R_ROOM | R_MAZE)))
		) {
			continue;
		}
		short tunnel_dir;
		if (same_row(rn, target_room))
			tunnel_dir = (rooms[rn].left_col < rooms[target_room].left_col) ? RIGHT : LEFT;
		else
			tunnel_dir = (rooms[rn].top_row < rooms[target_room].top_row) ? DOWN : UPWARD;
		short door_dir = ((tunnel_dir + 4) % DIRS);
		if (rooms[target_room].doors[door_dir/2].oth_room != NO_ROOM)
			continue;

		if ((!do_rec_de || did_this) || !mask_room(rn, &srow, &scol, TUNNEL)) {
			srow = (rooms[rn].top_row + rooms[rn].bottom_row) / 2;
			scol = (rooms[rn].left_col + rooms[rn].right_col) / 2;
		}

		short drow, dcol;
		put_door(&rooms[target_room], door_dir, &drow, &dcol);
		rooms_found++;
		draw_simple_passage(srow, scol, drow, dcol, tunnel_dir);
		rooms[rn].is_room = R_DEADEND;
		dungeon[srow][scol] = TUNNEL;

		if (i < 3 && !did_this) {
			did_this = 1;
			if (coin_toss())
				continue;
		}
		if (rooms_found < 2 && do_rec_de)
			recursive_deadend(rn, offsets, srow, scol);
		break;
	}
}

static void recursive_deadend(short rn, const short *offsets, short srow, short scol) {
	rooms[rn].is_room = R_DEADEND;
	dungeon[srow][scol] = TUNNEL;

	for (short i = 0; i < 4; i++) {
		short de = rn + offsets[i];
		if ((de < 0 || de >= MAXROOMS) || (!(same_row(rn, de) || same_col(rn, de))))
			continue;
		if (!(rooms[de].is_room & R_NOTHING))
			continue;

		short drow = (rooms[de].top_row + rooms[de].bottom_row) / 2;
		short dcol = (rooms[de].left_col + rooms[de].right_col) / 2;
		short tunnel_dir;
		if (same_row(rn, de))
			tunnel_dir = (rooms[rn].left_col < rooms[de].left_col) ? RIGHT : LEFT;
		else
			tunnel_dir = (rooms[rn].top_row < rooms[de].top_row) ? DOWN : UPWARD;
		draw_simple_passage(srow, scol, drow, dcol, tunnel_dir);
		r_de = de;
		recursive_deadend(de, offsets, drow, dcol);
	}
}

static bool mask_room(short rn, short *row, short *col, unsigned short mask) {
	for (short i = rooms[rn].top_row; i <= rooms[rn].bottom_row; i++) {
		for (short j = rooms[rn].left_col; j <= rooms[rn].right_col; j++) {
			if (dungeon[i][j] & mask) {
				*row = i;
				*col = j;
				return 1;
			}
		}
	}
	return 0;
}

static void make_maze(short r, short c, short tr, short br, short lc, short rc) {
	dungeon[r][c] = TUNNEL;

	char dirs[4] = {UPWARD, DOWN, LEFT, RIGHT};
	if (rand_percent(20)) {
		for (short i = 0; i < 10; i++)
			SWAP(dirs[get_rand(0, 3)], dirs[get_rand(0, 3)]);
	}
	for (short i = 0; i < 4; i++) {
		switch (dirs[i]) {
		case UPWARD:
			if (((r-1) >= tr) &&
				(dungeon[r-1][c] != TUNNEL) &&
				(dungeon[r-1][c-1] != TUNNEL) &&
				(dungeon[r-1][c+1] != TUNNEL) &&
				(dungeon[r-2][c] != TUNNEL)) {
				make_maze((r-1), c, tr, br, lc, rc);
			}
			break;
		case DOWN:
			if (((r+1) <= br) &&
				(dungeon[r+1][c] != TUNNEL) &&
				(dungeon[r+1][c-1] != TUNNEL) &&
				(dungeon[r+1][c+1] != TUNNEL) &&
				(dungeon[r+2][c] != TUNNEL)) {
				make_maze((r+1), c, tr, br, lc, rc);
			}
			break;
		case LEFT:
			if (((c-1) >= lc) &&
				(dungeon[r][c-1] != TUNNEL) &&
				(dungeon[r-1][c-1] != TUNNEL) &&
				(dungeon[r+1][c-1] != TUNNEL) &&
				(dungeon[r][c-2] != TUNNEL)) {
				make_maze(r, (c-1), tr, br, lc, rc);
			}
			break;
		case RIGHT:
			if (((c+1) <= rc) &&
				(dungeon[r][c+1] != TUNNEL) &&
				(dungeon[r-1][c+1] != TUNNEL) &&
				(dungeon[r+1][c+1] != TUNNEL) &&
				(dungeon[r][c+2] != TUNNEL)) {
				make_maze(r, (c+1), tr, br, lc, rc);
			}
			break;
		}
	}
}

static void hide_boxed_passage(short row1, short col1, short row2, short col2, short n) {
	if (cur_level <= 2)
		return;

	if (row1 > row2)
		SWAP(row1, row2);
	if (col1 > col2)
		SWAP(col1, col2);

	short h = row2 - row1;
	short w = col2 - col1;
	if (w >= 5 || h >= 5) {
		short row_cut = h >= 2 ? 1 : 0;
		short col_cut = w >= 2 ? 1 : 0;

		for (short i = 0; i < n; i++) {
			for (short j = 0; j < 10; j++) {
				short row = get_rand(row1 + row_cut, row2 - row_cut);
				short col = get_rand(col1 + col_cut, col2 - col_cut);
				if (dungeon[row][col] == TUNNEL) {
					dungeon[row][col] |= HIDDEN;
					break;
				}
			}
		}
	}
}

// try not to put in this room
void put_player(short nr) {
	short rn = nr;
	short row, col;
	for (short misses = 0; ((misses < 2) && (rn == nr)); misses++) {
		gr_row_col(&row, &col, (FLOOR | TUNNEL | OBJECT | STAIRS));
		rn = get_room_number(row, col);
	}
	rogue.row = row;
	rogue.col = col;

	if (dungeon[rogue.row][rogue.col] & TUNNEL)
		cur_room = PASSAGE;
	else
		cur_room = rn;

	if (cur_room != PASSAGE)
		light_up_room(cur_room);
	else
		light_passage(rogue.row, rogue.col);

	rn = get_room_number(rogue.row, rogue.col);
	wake_room(rn, 1, rogue.row, rogue.col);
	if (new_level_message) {
		message(new_level_message, 0);
		new_level_message = NULL;
	}
	mvaddch(rogue.row, rogue.col, rogue.fchar);
}

bool drop_check(void) {
	if (wizard)
		return 1;

	if (dungeon[rogue.row][rogue.col] & STAIRS) {
		if (levitate) {
			message("you're floating in the air!", 0);
			return 0;
		}
		return 1;
	}
	message("I see no way down", 0);
	return 0;
}

bool check_up(void) {
	if (!wizard) {
		if (!(dungeon[rogue.row][rogue.col] & STAIRS)) {
			message("I see no way up", 0);
			return 0;
		}
		if (!has_amulet()) {
			message("your way is magically blocked", 0);
			return 0;
		}
	}
	new_level_message = "you feel a wrenching sensation in your gut";
	if (cur_level == 1)
		win();
	else {
		cur_level -= 2;
		return 1;
	}
	return 0;
}

void add_exp(int e, bool promotion) {
	rogue.exp_points += e;

	if (rogue.exp_points >= level_points[rogue.exp - 1]) {
		short new_exp = get_exp_level(rogue.exp_points);
		if (rogue.exp_points > MAX_EXP)
			rogue.exp_points = MAX_EXP + 1;

		for (short i = rogue.exp+1; i <= new_exp; i++) {
			messagef(0, "welcome to level %d", i);
			if (promotion) {
				short hp = hp_raise();
				rogue.hp_current += hp;
				rogue.hp_max += hp;
			}
			rogue.exp = i;
			print_stats(STAT_HP | STAT_EXP);
		}
	} else
		print_stats(STAT_EXP);
}

static short get_exp_level(long e) {
	short i;
	for (i = 0; i < (MAX_EXP_LEVEL - 1); i++) {
		if (level_points[i] > e)
			break;
	}
	return i + 1;
}

int hp_raise(void) {
	return wizard ? 10 : get_rand(3, 10);
}

void show_average_hp(void) {
	float real_avg = 0, effective_avg = 0;
	if (rogue.exp > 1) {
		real_avg      = (float)((rogue.hp_max - extra_hp - INIT_HP) + less_hp) / (rogue.exp - 1);
		effective_avg = (float)(rogue.hp_max - INIT_HP) / (rogue.exp - 1);
	}
	messagef(0, "R-Hp: %.2f, E-Hp: %.2f (!: %d, V: %d)", real_avg, effective_avg, extra_hp, less_hp);
}

static void mix_random_rooms(void) {
	short x, y;
	for (short i = 0; i < (3 * MAXROOMS); i++) {
		do {
			x = get_rand(0, (MAXROOMS-1));
			y = get_rand(0, (MAXROOMS-1));
		} while (x == y);
		SWAP(random_rooms[x], random_rooms[y]);
	}
}
