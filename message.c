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
 * @(#)message.c 8.1 (Berkeley) 5/31/93
 * $FreeBSD: src/games/rogue/message.c,v 1.7.2.1 2000/07/20 10:35:07 kris Exp $
 *
 * This source herein may be modified and/or distributed by anybody who
 * so desires, with the following restrictions:
 *    1.)  No portion of this notice shall be removed.
 *    2.)  Credit shall not be taken for the creation of this source.
 *    3.)  This code is not to be traded, sold, or used for personal
 *         gain or profit.
 */

#include <stdio.h>
#include <limits.h>
#include "rogue.h"

static char msgs[NMESSAGES][DCOLS] = {"", "", "", "", ""};
static short msg_col = 0, imsg = -1;
static bool rmsg = 0;

bool msg_cleared = 1;
char hunger_str[HUNGER_STR_LEN] = "";
const char *more = "-more-";

static void pad(const char *, short);
static void save_screen(void);

void message(const char *msg, bool interrupt_player) {
	cant_int = 1;

	if (!save_is_interactive)
		return;
	if (interrupt_player) {
		interrupted = 1;
		fflush(stdin);
	}

	if (!msg_cleared) {
		mvaddstr(MIN_ROW-1, msg_col, more);
		refresh();
		wait_for_ack();
		check_message();
	}
	if (!rmsg) {
		imsg = (imsg + 1) % NMESSAGES;
		strcpy(msgs[imsg], msg);
	}
	mvaddstr(MIN_ROW-1, 0, msg);
	addch(' ');
	refresh();
	msg_cleared = 0;
	msg_col = strlen(msg);

	cant_int = 0;

	if (did_int) {
		did_int = 0;
		onintr(0);
	}
}

void messagef(bool interrupt_player, const char *fmt, ...) {
	char buf[1024];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	message(buf, interrupt_player);
}

void remessage(short c) {
	if (imsg != -1) {
		check_message();
		rmsg = 1;
		while (c > imsg)
			c -= NMESSAGES;
		message(msgs[(imsg - c) % NMESSAGES], 0);
		rmsg = 0;
		move(rogue.row, rogue.col);
		refresh();
	}
}

void check_message(void) {
	if (msg_cleared)
		return;
	move(MIN_ROW - 1, 0);
	clrtoeol();
	refresh();
	msg_cleared = 1;
}

bool get_input_line(const char *prompt, const char *insert, char *buf, bool add_blank) {
	message(prompt, 0);
	short n = strlen(prompt);
	short i = 0;
	if (insert[0]) {
		mvaddstr(0, n + 1, insert);
		strcpy(buf, insert);
		i = strlen(insert);
		move(0, (n + i + 1));
		refresh();
	}

	int ch;
	while (true) {
		ch = rgetchar();
		if (ch == '\r' || ch == '\n' || ch == CANCEL)
			break;

		if (ch >= ' ' && ch <= '~' && i < PATH_MAX - 2) {
			buf[i++] = ch;
			addch(ch);
		}
		if (ch == KEY_BACKSPACE && i > 0) {
			mvaddch(0, i + n, ' ');
			move(MIN_ROW-1, i+n);
			i--;
		}
		refresh();
	}
	check_message();
	if (add_blank)
		buf[i++] = ' ';
	else {
		while ((i > 0) && (buf[i - 1] == ' '))
			i--;
	}
	buf[i] = 0;

	return !(ch == CANCEL || i == 0 || (i == 1 && add_blank));
}

int rgetchar(void) {
	for(;;) {
		int ch = getch();
		switch (ch) {
		case 'R' - 0x40: wrefresh(curscr); break;
		case '&':        save_screen();    break;
		default:         return ch;
		}
	}
}

// Level: 99 Gold: 999999 Hp: 999(999) Str: 99(99) Arm: 99 Exp: 21/10000000 Hungry
// 0    5    1    5    2    5    3    5    4    5    5    5    6    5    7    5
void print_stats(int stat_mask) {
	char buf[16];
	int row = DROWS - 1;
	bool label = (stat_mask & STAT_LABEL) ? 1 : 0;

	if (stat_mask & STAT_LEVEL) {
		if (label)
			mvaddstr(row, 0, "Level: ");
		// max level taken care of in make_level()
		sprintf(buf, "%d", cur_level);
		mvaddstr(row, 7, buf);
		pad(buf, 2);
	}
	if (stat_mask & STAT_GOLD) {
		if (label)
			mvaddstr(row, 10, "Gold: ");
		if (rogue.gold > MAX_GOLD)
			rogue.gold = MAX_GOLD;
		sprintf(buf, "%ld", rogue.gold);
		mvaddstr(row, 16, buf);
		pad(buf, 6);
	}
	if (stat_mask & STAT_HP) {
		if (rogue.hp_current < low_health_warn)
			attron(A_BOLD | A_REVERSE);
		// Always print this label, to apply or clear standout (could probably
		// just always print this entire line without this stat_mask business;
		// terminals are so fast compared to 1980 terminals it probably doesn't
		// matter much).
		mvaddstr(row, 23, "Hp: ");
		if (rogue.hp_max > MAX_HP) {
			rogue.hp_current -= (rogue.hp_max - MAX_HP);
			rogue.hp_max = MAX_HP;
		}
		sprintf(buf, "%d(%d)", rogue.hp_current, rogue.hp_max);
		mvaddstr(row, 27, buf);
		if (rogue.hp_current < low_health_warn)
			attroff(A_BOLD | A_REVERSE);
		pad(buf, 8);
	}
	if (stat_mask & STAT_STRENGTH) {
		if (label)
			mvaddstr(row, 36, "Str: ");
		if (rogue.str_max > MAX_STRENGTH) {
			rogue.str_current -= (rogue.str_max - MAX_STRENGTH);
			rogue.str_max = MAX_STRENGTH;
		}
		sprintf(buf, "%d(%d)", (rogue.str_current + add_strength), rogue.str_max);
		mvaddstr(row, 41, buf);
		pad(buf, 6);
	}
	if (stat_mask & STAT_ARMOR) {
		if (label)
			mvaddstr(row, 48, "Arm: ");
		if (rogue.armor && (rogue.armor->d_enchant > MAX_ARMOR))
			rogue.armor->d_enchant = MAX_ARMOR;
		sprintf(buf, "%d", get_armor_class(rogue.armor));
		mvaddstr(row, 53, buf);
		pad(buf, 2);
	}
	if (stat_mask & STAT_EXP) {
		if (label)
			mvaddstr(row, 56, "Exp: ");
		if (rogue.exp_points > MAX_EXP)
			rogue.exp_points = MAX_EXP;
		if (rogue.exp > MAX_EXP_LEVEL)
			rogue.exp = MAX_EXP_LEVEL;
		sprintf(buf, "%d/%ld", rogue.exp, rogue.exp_points);
		mvaddstr(row, 61, buf);
		pad(buf, 11);
	}
	if (stat_mask & STAT_HUNGER) {
		mvaddstr(row, 73, hunger_str);
		clrtoeol();
	}
	refresh();
}

static void pad(const char *s, short n) {
	for (short i = strlen(s); i < n; i++)
		addch(' ');
}

static void save_screen(void) {
	FILE *fp = fopen("rogue.screen", "w");
	if (!fp) {
		sound_bell();
		return;
	}

	for (short i = 0; i < DROWS; i++) {
		bool found_non_blank = 0;
		char buf[DCOLS + 2];
		for (short j = (DCOLS - 1); j >= 0; j--) {
			buf[j] = (short)mvinch(i, j);
			if (!found_non_blank) {
				if (buf[j] != ' ' || j == 0) {
					buf[j + ((j == 0) ? 0 : 1)] = 0;
					found_non_blank = 1;
				}
			}
		}
		fputs(buf, fp);
		putc('\n', fp);
	}
	fclose(fp);
}

void sound_bell(void) {
	putchar(7);
	fflush(stdout);
}

bool is_digit(short ch) {
	return (ch >= '0') && (ch <= '9');
}

int r_index(const char *str, int ch, bool last) {
	int i = 0;
	if (last) {
		for (i = strlen(str) - 1; i >= 0; i--) {
			if (str[i] == ch)
				return i;
		}
	} else {
		for (i = 0; str[i]; i++) {
			if (str[i] == ch)
				return i;
		}
	}
	return -1;
}
