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
 * @(#)inventory.c 8.1 (Berkeley) 5/31/93
 * $FreeBSD: src/games/rogue/inventory.c,v 1.4 1999/11/30 03:49:23 billf Exp $
 *
 * This source herein may be modified and/or distributed by anybody who
 * so desires, with the following restrictions:
 *    1.)  No portion of this notice shall be removed.
 *    2.)  Credit shall not be taken for the creation of this source.
 *    3.)  This code is not to be traded, sold, or used for personal
 *         gain or profit.
 */

#include "rogue.h"

bool is_wood[WANDS];
const char *press_space = " --press space to continue--";

static const char *const wand_materials[WAND_MATERIALS] = {
	"steel ",    "bronze ", "gold ",      "silver ", "copper ", "nickel ",    "cobalt ",
	"tin ",      "iron ",   "magnesium ", "chrome ", "carbon ", "platinum  ", "silicon ",
	"titanium ", "teak ",   "oak ",       "cherry ", "birch ",  "pine ",      "cedar ",
	"balsa ",    "ivory ",  "walnut ",    "maple ",  "mahogany  ", "elm ",    "palm ",
	"redwood ",  "wooden "
};

static const char *const gems[GEMS] = {
	"diamond ",  "stibotantalite ", "lapi-lazuli ", "ruby ", "emerald ", "sapphire ",
	"amethyst ", "quartz ",         "tiger-eye ",   "opal ", "agate ",   "turquoise ",
	"pearl ",    "garnet "
};

static const char *const syllables[MAXSYLLABLES] = {
	"blech ", "foo ",   "barf ",  "rech ",  "bar ",   "blech ", "quo ",   "bloto ", "oh ",
	"caca ",  "blorp ", "erp ",   "festr ", "rot ",   "slie ",  "snorf ", "iky ",   "yuky ",
	"ooze ",  "ah ",    "bahl ",  "zep ",   "druhl ", "flem ",  "behil ", "arek ",  "mep ",
	"zihr ",  "grit ",  "kona ",  "kini ",  "ichi ",  "tims ",  "ogr ",   "oo ",    "ighr ",
	"coph ",  "swerr ", "mihln ", "poxi "
};

#define COMS 47

struct show_help_s {
	short com_char;
	const char *com_desc;
};

static const struct show_help_s com_id_tab[COMS] = {
	{ '?',		"?       prints help" },
	{ 'r',		"r       read scroll" },
	{ '/',		"/       identify object" },
	{ 'e',		"e       eat food" },
	{ 'h',		"h       left " },
	{ 'w',		"w       wield a weapon" },
	{ 'j',		"j       down" },
	{ 'W',		"W       wear armor" },
	{ 'k',		"k       up" },
	{ 'T',		"T       take armor off" },
	{ 'l',		"l       right" },
	{ 'P',		"P       put on ring" },
	{ 'y',		"y       up & left" },
	{ 'R',		"R       remove ring" },
	{ 'u',		"u       up & right" },
	{ 'd',		"d       drop object" },
	{ 'b',		"b       down & left" },
	{ 'c',		"c       call object" },
	{ 'n',		"n       down & right" },
	{ '\0',		"M or <SHIFT><dir>: run that way" },
	{ ')',		")       print current weapon" },
	{ '\0',		"^m or <CTRL><dir>: run till adjacent" },
	{ ']',		"]       print current armor" },
	{ 'f',		"f<dir>  fight till death or near death" },
	{ '=',		"=       print current rings" },
	{ 't',		"t<dir>  throw something" },
	{ '\001',	"^A      print Hp-raise average" },
	{ 'm',		"m<dir>  move onto without picking up" },
	{ 'z',		"z<dir>  zap a wand in a direction" },
	{ 'o',		"o       examine/set options" },
	{ '^',		"^<dir>  identify trap type" },
	{ '\022',	"^R      redraw screen" },
	{ '&',		"&       save screen into 'rogue.screen'" },
	{ 's',		"s       search for trap/secret door" },
	{ '\020',	"^P      repeat last message" },
	{ '>',		">       go down a staircase" },
	{ '\033',	"^[      cancel command" },
	{ '<',		"<       go up a staircase" },
	{ 'S',		"S       save game" },
	{ '.',		".       rest for a turn" },
	{ 'Q',		"Q       quit" },
	{ ',',		",       pick something up" },
	{ 'i',		"i       inventory" },
	{ 'F',		"F<dir>  fight till either of you dies" },
	{ 'I',		"I       inventory single item" },
	{ 'v',		"v       print version number" },
	{ 'q',		"q       quaff potion" }
};

// Store this as globals so we can set it in inventory() and clear the
// screen again in clear_inventory() – this is a fast but a bit of a hacky way
// to add the openinv option.
short last_inv_i, last_inv_j, last_inv_col;
char inv_descs[MAX_PACK_COUNT+1][DCOLS];

void inventory(const object *pack, unsigned short mask, short dont_wait) {
	object *obj = pack->next_object;
	if (!obj) {
		message("your pack is empty", 0);
		return;
	}

	short i = 0, j = 0, maxlen = 0, n;
	while (obj) {
		if (obj->what_is & mask) {
			inv_descs[i][0] = ' ';
			inv_descs[i][1] = obj->ichar;
			inv_descs[i][2] = ((obj->what_is & ARMOR) && obj->is_protected) ? '}' : ')';
			inv_descs[i][3] = ' ';
			get_desc(obj, inv_descs[i]+4);
			if ((n = strlen(inv_descs[i])) > maxlen)
				maxlen = n;
			i++;
		}
		obj = obj->next_object;
	}
	if (dont_wait)
		strcpy(inv_descs[i++], "--select item--");
	else
		strcpy(inv_descs[i++], press_space);
	if (maxlen < 27)
		maxlen = 27;

	short col = DCOLS - (maxlen + 2);
	for (short row = 0; ((row < i) && (row < DROWS)); row++) {
		if (row > 0) {
			for (j = col; j < DCOLS; j++) {
				inv_descs[row-1][j-col] = (short)mvinch(row, j);
			}
			inv_descs[row-1][j-col] = 0;
		}
		mvaddstr(row, col, inv_descs[row]);
		clrtoeol();
	}
	refresh();
	last_inv_i = i;
	last_inv_j = j;
	last_inv_col = col;
	if (!dont_wait) {
		wait_for_ack();
		clear_inventory();
	}
}

void clear_inventory(void) {
	move(0, 0);
	clrtoeol();
	for (last_inv_j = 1; ((last_inv_j < last_inv_i) && (last_inv_j < DROWS)); last_inv_j++)
		mvaddstr(last_inv_j, last_inv_col, inv_descs[last_inv_j-1]);
}

void show_help(void) {
	check_message();

	short rows = (((COMS / 2) + (COMS % 2)) + 1);
	bool need_two_screens = FALSE;
	if (rows > LINES) {
		need_two_screens = 1;
		rows = LINES;
	}

	char save[(((COMS / 2) + (COMS % 2)) + 1)][DCOLS];
	for (short i = 0; i < rows; i++) {
		for (short j = 0; j < DCOLS; j++)
			save[i][j] = (short)mvinch(i, j);
	}

	short k = 0;
next_page:
	for (short i = 0; i < rows; i++) {
		move(i, 0);
		clrtoeol();
	}
	for (short i = 0; i < (rows-1); i++) {
		if (i < (LINES-1)) {
			if (((i + i) < COMS) && ((i + i + k) < COMS))
				mvaddstr(i, 0, com_id_tab[i + i + k].com_desc);
			if (((i + i + 1) < COMS) && ((i + i + k + 1) < COMS))
				mvaddstr(i, (DCOLS/2), com_id_tab[i + i + k + 1].com_desc);
		}
	}
	mvaddstr(rows - 1, 0, need_two_screens ? more : press_space);
	refresh();
	wait_for_ack();

	if (need_two_screens) {
		k += ((rows - 1) * 2);
		need_two_screens = 0;
		goto next_page;
	}

	for (short i = 0; i < rows; i++) {
		move(i, 0);
		for (short j = 0; j < DCOLS; j++)
			addch(save[i][j]);
	}
}

void mix_colors(void) {
	char *t[MAX_ID_TITLE_LEN];
	for (short i = 0; i <= 32; i++) {
		short j = get_rand(0, (POTIONS - 1));
		short k = get_rand(0, (POTIONS - 1));
		memcpy(t, id_potions[j].title, MAX_ID_TITLE_LEN);
		memcpy(id_potions[j].title, id_potions[k].title, MAX_ID_TITLE_LEN);
		memcpy(id_potions[k].title, t, MAX_ID_TITLE_LEN);
	}
}

void make_scroll_titles(void) {
	for (short i = 0; i < SCROLS; i++) {
		short sylls = get_rand(2, 5);
		strcpy(id_scrolls[i].title, "'");

		for (short j = 0; j < sylls; j++) {
			short s = get_rand(1, (MAXSYLLABLES-1));
			strcat(id_scrolls[i].title, syllables[s]);
		}
		short n = strlen(id_scrolls[i].title);
		strcpy(id_scrolls[i].title+(n-1), "' ");
	}
}

const char * get_desc_str(const object *obj) {
	char *buf = calloc(128, sizeof(char));
	get_desc(obj, buf);
	return buf;
}

void get_desc(const object *obj, char *desc) {
	if (!obj)
		return;
	if (obj->what_is == AMULET) {
		strcpy(desc, "the amulet of Yendor ");
		return;
	}
	if (obj->what_is == GOLD) {
		sprintf(desc, "%d pieces of gold", obj->quantity);
		return;
	}

	if (obj->what_is != ARMOR) {
		if (obj->quantity == 1)
			strcpy(desc, "a ");
		else
			sprintf(desc, "%d ", obj->quantity);
	}

	char more_info[32];
	const char *item_name = name_of(obj);
	if (obj->what_is == FOOD) {
		if (obj->which_kind == RATION) {
			if (obj->quantity > 1)
				sprintf(desc, "%d rations of ", obj->quantity);
			else
				strcpy(desc, "some ");
		} else
			strcpy(desc, "a ");
		strcat(desc, item_name);
		goto ANA;
	}

	struct id *id_table = get_id_table(obj);

	if (wizard)
		goto ID;
	if (obj->what_is & (WEAPON | ARMOR | WAND | RING))
		goto CHECK;

	switch (id_table[obj->which_kind].id_status) {
	case UNIDENTIFIED:
CHECK:
		switch (obj->what_is) {
		case SCROL:
			strcat(desc, item_name);
			strcat(desc, "entitled: ");
			strcat(desc, id_table[obj->which_kind].title);
			break;
		case POTION:
			strcat(desc, id_table[obj->which_kind].title);
			strcat(desc, item_name);
			break;
		case WAND:
		case RING:
			if (obj->identified || (id_table[obj->which_kind].id_status == IDENTIFIED))
				goto ID;
			if (id_table[obj->which_kind].id_status == CALLED)
				goto CALL;
			strcat(desc, id_table[obj->which_kind].title);
			strcat(desc, item_name);
			break;
		case ARMOR:
			if (obj->identified)
				goto ID;
			strcpy(desc, id_table[obj->which_kind].title);
			break;
		case WEAPON:
			if (obj->identified)
				goto ID;
			strcat(desc, name_of(obj));
			break;
		}
		break;
	case CALLED:
CALL:
		switch (obj->what_is) {
		case SCROL:
		case POTION:
		case WAND:
		case RING:
			strcat(desc, item_name);
			strcat(desc, "called ");
			strcat(desc, id_table[obj->which_kind].title);
			break;
		}
		break;
	case IDENTIFIED:
ID:
		switch (obj->what_is) {
		case SCROL:
		case POTION:
			strcat(desc, item_name);
			strcat(desc, id_table[obj->which_kind].real);
			break;
		case RING:
			if (wizard || obj->identified) {
				if ((obj->which_kind == DEXTERITY) ||
					(obj->which_kind == ADD_STRENGTH)) {
					sprintf(more_info, "%s%d ", ((obj->class > 0) ? "+" : ""),
						obj->class);
					strcat(desc, more_info);
				}
			}
			strcat(desc, item_name);
			strcat(desc, id_table[obj->which_kind].real);
			break;
		case WAND:
			strcat(desc, item_name);
			strcat(desc, id_table[obj->which_kind].real);
			if (wizard || obj->identified) {
				sprintf(more_info, "[%d]", obj->class);
				strcat(desc, more_info);
			}
			break;
		case ARMOR:
			sprintf(desc, "%s%d ", ((obj->d_enchant >= 0) ? "+" : ""),
			obj->d_enchant);
			strcat(desc, id_table[obj->which_kind].title);
			sprintf(more_info, "[%d] ", get_armor_class(obj));
			strcat(desc, more_info);
			break;
		case WEAPON:
			sprintf(desc+strlen(desc), "%s%d,%s%d ",
			((obj->hit_enchant >= 0) ? "+" : ""), obj->hit_enchant,
			((obj->d_enchant >= 0) ? "+" : ""), obj->d_enchant);
			strcat(desc, name_of(obj));
			break;
		}
		break;
	}
ANA:
	if (!strncmp(desc, "a ", 2)) {
		if (is_vowel(desc[2])) {
			for (short i = strlen(desc) + 1; i > 1; i--)
				desc[i] = desc[i-1];
			desc[1] = 'n';
		}
	}
	if (obj->in_use_flags & BEING_WIELDED)
		strcat(desc, "in hand");
	else if (obj->in_use_flags & BEING_WORN)
		strcat(desc, "being worn");
	else if (obj->in_use_flags & ON_LEFT_HAND)
		strcat(desc, "on left hand");
	else if (obj->in_use_flags & ON_RIGHT_HAND)
		strcat(desc, "on right hand");
}

void get_wand_and_ring_materials(void) {
	short j;
	bool used[WAND_MATERIALS];

	for (short i = 0; i < WAND_MATERIALS; i++)
		used[i] = 0;
	for (short i = 0; i < WANDS; i++) {
		do {
			j = get_rand(0, WAND_MATERIALS - 1);
		} while (used[j]);
		used[j] = 1;
		strcpy(id_wands[i].title, wand_materials[j]);
		is_wood[i] = (j > MAX_METAL);
	}

	for (short i = 0; i < GEMS; i++)
		used[i] = 0;
	for (short i = 0; i < RINGS; i++) {
		do {
			j = get_rand(0, GEMS - 1);
		} while (used[j]);
		used[j] = 1;
		strcpy(id_rings[i].title, gems[j]);
	}
}

void single_inv(short ichar) {
	short ch = ichar ? ichar : pack_letter("inventory what?", ALL_OBJECTS);
	if (ch == CANCEL)
		return;

	object *obj;
	if (!(obj = get_letter_object(ch))) {
		message("no such item.", 0);
		return;
	}
	messagef(0, "%c%c %s", ch, ((obj->what_is & ARMOR) && obj->is_protected) ? '}' : ')', get_desc_str(obj));
}

struct id * get_id_table(const object *obj) {
	switch (obj->what_is) {
	case SCROL:  return id_scrolls;
	case POTION: return id_potions;
	case WAND:   return id_wands;
	case RING:   return id_rings;
	case WEAPON: return id_weapons;
	case ARMOR:  return id_armors;
	}
	return NULL;
}

void inv_armor_weapon(bool is_weapon) {
	if (is_weapon) {
		if (rogue.weapon)
			single_inv(rogue.weapon->ichar);
		else
			message("not wielding anything", 0);
	} else {
		if (rogue.armor)
			single_inv(rogue.armor->ichar);
		else
			message("not wearing anything", 0);
	}
}

void id_type(void) {
	message("what do you want identified?", 0);

	const char *id;
	int ch = rgetchar();
	if (ch >= 'A' && ch <= 'Z') {
		id = m_names[ch-'A'];
	} else if (ch < 32) {
		check_message();
		return;
	} else {
		switch (ch) {
		case '@': id = "you";                   break;
		case '%': id = "staircase";             break;
		case '^': id = "trap";                  break;
		case '+': id = "door";                  break;
		case '-':
		case '|': id = "wall of a room";        break;
		case '.': id = "floor";                 break;
		case '#': id = "passage";               break;
		case ' ': id = "solid rock";            break;
		case '=': id = "ring";                  break;
		case '?': id = "scroll";                break;
		case '!': id = "potion";                break;
		case '/': id = "wand or staff";         break;
		case ')': id = "weapon";                break;
		case ']': id = "armor";                 break;
		case '*': id = "gold";                  break;
		case ':': id = "food";                  break;
		case ',': id = "the Amulet of Yendor";  break;
		default:  id = "unknown character";     break;
		}
	}
	check_message();
	messagef(0, "'%c': %s", ch, id);
}
