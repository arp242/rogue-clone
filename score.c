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
 * @(#)score.c 8.1 (Berkeley) 5/31/93
 * $FreeBSD: src/games/rogue/score.c,v 1.4 1999/11/30 03:49:27 billf Exp $
 *
 * This source herein may be modified and/or distributed by anybody who
 * so desires, with the following restrictions:
 *    1.)  No portion of this notice shall be removed.
 *    2.)  Credit shall not be taken for the creation of this source.
 *    3.)  This code is not to be traded, sold, or used for personal
 *         gain or profit.
 */

#include <stdio.h>
#include <errno.h>
#include "rogue.h"

static void center(short, const char *);
static int get_value(const object *);
static void id_all(void);
static void score_reason(char[128], const object *, short);
static void sell_pack(void);
static void sf_error(const char *);

void killed_by(const object *monster, short other) {
	md_ignore_signals();

	if (other != QUIT)
		rogue.gold = ((rogue.gold * 9) / 10);

	char buf[128];
	if (other) {
		switch (other) {
		case HYPOTHERMIA: strcpy(buf, "died of hypothermia"); break;
		case STARVATION:  strcpy(buf, "died of starvation");  break;
		case POISON_DART: strcpy(buf, "killed by a dart");    break;
		case QUIT:        strcpy(buf, "quit");                break;
		case KFIRE:       strcpy(buf, "killed by fire");      break;
		}
	} else {
		strcpy(buf, "Killed by ");
		if (is_vowel(m_names[monster->m_char - 'A'][0]))
			strcat(buf, "an ");
		else
			strcat(buf, "a ");
		strcat(buf, m_names[monster->m_char - 'A']);
	}
	strcat(buf, " with ");
	sprintf(buf + strlen(buf), "%ld gold", rogue.gold);
	if (!other && !no_skull) {
		clear();
		mvaddstr(4, 32, "__---------__");
		mvaddstr(5, 30, "_~             ~_");
		mvaddstr(6, 29, "/                 \\");
		mvaddstr(7, 28, "~                   ~");
		mvaddstr(8, 27, "/                     \\");
		mvaddstr(9, 27, "|    XXXX     XXXX    |");
		mvaddstr(10, 27, "|    XXXX     XXXX    |");
		mvaddstr(11, 27, "|    XXX       XXX    |");
		mvaddstr(12, 28, "\\         @         /");
		mvaddstr(13, 29, "--\\     @@@     /--");
		mvaddstr(14, 30, "| |    @@@    | |");
		mvaddstr(15, 30, "| |           | |");
		mvaddstr(16, 30, "| vvVvvvvvvvVvv |");
		mvaddstr(17, 30, "|  ^^^^^^^^^^^  |");
		mvaddstr(18, 31, "\\_           _/");
		mvaddstr(19, 33, "~---------~");
		center(21, nick_name);
		center(22, buf);
	} else
		message(buf, 0);
	message("", 0);
	put_scores(monster, other);
}

void win(void) {
	unwield(rogue.weapon);  // disarm and relax
	unwear(rogue.armor);
	un_put_on(rogue.left_ring);
	un_put_on(rogue.right_ring);

	clear();
	mvaddstr(10, 11, "@   @  @@@   @   @      @  @  @   @@@   @   @   @");
	mvaddstr(11, 11, " @ @  @   @  @   @      @  @  @  @   @  @@  @   @");
	mvaddstr(12, 11, "  @   @   @  @   @      @  @  @  @   @  @ @ @   @");
	mvaddstr(13, 11, "  @   @   @  @   @      @  @  @  @   @  @  @@");
	mvaddstr(14, 11, "  @    @@@    @@@        @@ @@    @@@   @   @   @");
	mvaddstr(17, 11, "Congratulations,  you have  been admitted  to  the");
	mvaddstr(18, 11, "Fighters' Guild.   You return home,  sell all your");
	mvaddstr(19, 11, "treasures at great profit and retire into comfort.");
	message("", 0);
	message("", 0);
	id_all();
	sell_pack();
	put_scores(NULL, WIN);
}

void quit(bool from_intrpt) {
	md_ignore_signals();

	bool mc = FALSE;
	short orow = 0, ocol = 0;
	char buf[128];
	if (from_intrpt) {
		orow = rogue.row;
		ocol = rogue.col;

		mc = msg_cleared;

		for (short i = 0; i < DCOLS; i++)
			buf[i] = (short)mvinch(0, i);
	}
	check_message();
	message("really quit?", 1);
	if (rgetchar() != 'y') {
		md_heed_signals();
		check_message();
		if (from_intrpt) {
			for (short i = 0; i < DCOLS; i++)
				mvaddch(0, i, buf[i]);
			msg_cleared = mc;
			move(orow, ocol);
			refresh();
		}
		return;
	}
	if (from_intrpt)
		clean_up(byebye_string);
	check_message();
	killed_by(NULL, QUIT);
}

struct scoreline {
	short rank;
	int   gold;
	char  nick[31];
	char  reason[128];
};

void put_scores(const object *monster, short other) {
	bool pause = score_only;  // score_only is the -s flag from the CLI.

	md_lock(1);
	const char *file = md_scorefile();
	FILE *fp;
	if ((fp = fopen(file, "r+")) == NULL && (fp = fopen(file, "w+")) == NULL) {
		char errmsg[1024];
	    snprintf(errmsg, 1024, "cannot read/write score file %s: %s", file, strerror(errno));
		message(errmsg, 0);
		sf_error(errmsg);
	}
	rewind(fp);

	struct scoreline scores[NUM_SCORES] = {{0}};
	int n = fread(&scores, sizeof(char), sizeof(scores), fp);
	if (n < sizeof(scores) && n != 0)
		sf_error(NULL);

	short rank = 0;
	if (!score_only) {
		for (short i = 0; i < NUM_SCORES; i++) {
			if (rogue.gold > scores[i].gold) {
				rank = i + 1;
				break;
			}
		}

		if (rank >= 1) {
			for (short i = NUM_SCORES; i >= rank - 1; i--) {
				if (scores[i].rank == 0)
					continue;
				scores[i].rank = i + 2;
				scores[i + 1] = scores[i];
			}
			struct scoreline s = {rank, rogue.gold, "Jan 12 '23", ""};
			snprintf(s.nick, sizeof(s.nick) - 1, "%s", nick_name);
			score_reason(s.reason, monster, other);
			scores[rank - 1] = s;
		}
	}

	clear();
	mvaddstr(1, 30, "Top  Rogueists");
	mvaddstr(2, 0, "Rank   Score    Name");

	short offset = 0;
	for (short i = 0; i < NUM_SCORES; i++) {
		if (scores[i].rank == 0)
			break;

		char buf[128];
		snprintf(buf, 128, "%2d    %6d    %s: %s\n",
			scores[i].rank, scores[i].gold, scores[i].nick, scores[i].reason);

		if (i == rank - 1)
			standout();
		/// Wrap to next line with indent if >80 chars. 64 characters available
		/// – needs to indent 16.
		int l = 80;
		if (strlen(buf) > l + 1) {
			char buf2[80] = {0};
			while (buf[l] != ' ')
				l--;
			strncpy(buf2, buf, l);
			mvaddstr(i + offset + 3, 0, buf2);

			buf2[0] = 0;
			strncpy(buf2, &buf[l+1], 80);
			mvaddstr(i + offset + 4, 0, "                ");
			mvaddstr(i + offset + 4, 16, buf2);
			offset++;
		} else
			mvaddstr(i + offset + 3, 0, buf);
		if (i == rank - 1)
			standend();
	}

	md_ignore_signals();
	if (!score_only && rank >= 1) {
		rewind(fp);
		fwrite((char *)&scores, sizeof(char), sizeof(scores), fp);
	}

	md_lock(0);
	refresh();
	fclose(fp);
	message("", 0);
	if (pause)
		message("", 0);
	clean_up("");
}

static void score_reason(char buf[128], const object *monster, short other) {
	if (other) {
		switch (other) {
		case HYPOTHERMIA: strncat(buf, "died of hypothermia", 128); break;
		case STARVATION:  strncat(buf, "died of starvation", 128);  break;
		case POISON_DART: strncat(buf, "killed by a dart", 128);    break;
		case QUIT:        strncat(buf, "quit", 128);                break;
		case WIN:         strncat(buf, "a total winner", 128);      break;
		case KFIRE:       strncpy(buf, "killed by fire", 128);      break;
		}
	}
	else {
		snprintf(buf, 128, "killed by a%s %s",
			is_vowel(m_names[monster->m_char - 'A'][0]) ? "n" : "",
			m_names[monster->m_char - 'A']);
		//strcat(buf, "killed by ");
		//strcat(buf, is_vowel(m_names[monster->m_char - 'A'][0]) ? "an " : "a ");
		//strcat(buf, m_names[monster->m_char - 'A']);
	}
	sprintf(buf + strlen(buf), " on %s at level %d ", "Jun 19 '23", max_level);
	if (other != WIN && has_amulet())
		strncat(buf, "with amulet", 128);
}

bool is_vowel(short ch) {
	return ch == 'a' || ch == 'e' || ch == 'i' || ch == 'o' || ch == 'u';
}

static void sell_pack(void) {
	clear();
	mvaddstr(1, 0, "Value      Item");

	char buf[DCOLS];
	short row = 2;
	object *obj = rogue.pack.next_object;
	while (obj) {
		if (obj->what_is != FOOD) {
			obj->identified = 1;
			short val = get_value(obj);
			rogue.gold += val;

			if (row < DROWS) {
				sprintf(buf, "%5d      ", val);
				get_desc(obj, buf+11);
				mvaddstr(row++, 0, buf);
			}
		}
		obj = obj->next_object;
	}
	refresh();
	if (rogue.gold > MAX_GOLD)
		rogue.gold = MAX_GOLD;
	message("", 0);
}

static int get_value(const object *obj) {
	int val = 0;
	short wc = obj->which_kind;
	switch (obj->what_is) {
	case WEAPON:
		val = id_weapons[wc].value;
		if (wc == ARROW || wc == DAGGER || wc == SHURIKEN || wc == DART)
			val *= obj->quantity;
		val += (obj->d_enchant * 85);
		val += (obj->hit_enchant * 85);
		break;
	case ARMOR:
		val = id_armors[wc].value;
		val += (obj->d_enchant * 75);
		if (obj->is_protected)
			val += 200;
		break;
	case WAND:
		val = id_wands[wc].value * (obj->class + 1);
		break;
	case SCROL:
		val = id_scrolls[wc].value * obj->quantity;
		break;
	case POTION:
		val = id_potions[wc].value * obj->quantity;
		break;
	case AMULET:
		val = 5000;
		break;
	case RING:
		val = id_rings[wc].value * (obj->class + 1);
		break;
	}
	if (val <= 0)
		val = 10;
	return val;
}

static void id_all(void) {
	for (short i = 0; i < SCROLS; i++)
		id_scrolls[i].id_status = IDENTIFIED;
	for (short i = 0; i < WEAPONS; i++)
		id_weapons[i].id_status = IDENTIFIED;
	for (short i = 0; i < ARMORS; i++)
		id_armors[i].id_status = IDENTIFIED;
	for (short i = 0; i < WANDS; i++)
		id_wands[i].id_status = IDENTIFIED;
	for (short i = 0; i < POTIONS; i++)
		id_potions[i].id_status = IDENTIFIED;
}

static void center(short row, const char *buf) {
	short margin = ((DCOLS - strlen(buf)) / 2);
	mvaddstr(row, margin, buf);
}

static void sf_error(const char *msg) {
	md_lock(0);
	message("", 1);
	if (msg != NULL)
		clean_up(msg);
	else
		clean_up("sorry, score file is out of order");
}
