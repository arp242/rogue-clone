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
 * @(#)pack.c 8.1 (Berkeley) 5/31/93
 * $FreeBSD: src/games/rogue/pack.c,v 1.8 1999/11/30 03:49:25 billf Exp $
 *
 * This source herein may be modified and/or distributed by anybody who
 * so desires, with the following restrictions:
 *    1.)  No portion of this notice shall be removed.
 *    2.)  Credit shall not be taken for the creation of this source.
 *    3.)  This code is not to be traded, sold, or used for personal
 *         gain or profit.
 */

#include <unistd.h>
#include "rogue.h"

const char curse_message[] = "you can't, it appears to be cursed";

static object *check_duplicate(object *, object *);
static bool is_pack_letter(int *, unsigned short *);
static bool mask_pack(const object *, unsigned short);
static int next_avail_ichar(void);

object * add_to_pack(object *obj, object *pack, int condense) {
	object *op;
	if (condense) {
		if ((op = check_duplicate(obj, pack)) != NULL) {
			free_object(obj);
			return op;
		} else
			obj->ichar = next_avail_ichar();
	}
	if (pack->next_object == 0)
		pack->next_object = obj;
	else {
		op = pack->next_object;
		while (op->next_object)
			op = op->next_object;
		op->next_object = obj;
	}
	obj->next_object = 0;
	return obj;
}

void take_from_pack(object *obj, object *pack) {
	while (pack->next_object != obj)
		pack = pack->next_object;
	pack->next_object = pack->next_object->next_object;
}

// Note: *status is set to 0 if the rogue attempts to pick up a scroll of
// scare-monster and it turns to dust.  *status is otherwise set to 1.
object * pick_up(int row, int col, short *status) {
	if (levitate) {
		message("you're floating in the air!", 0);
		return NULL;
	}

	object *obj = object_at(&level_objects, row, col);
	if (!obj) {
		message("pick_up(): inconsistent", 1);
		return obj;
	}

	*status = 1;
	if (obj->what_is == SCROL && obj->which_kind == SCARE_MONSTER && obj->picked_up) {
		message("the scroll turns to dust as you pick it up", 0);
		dungeon[row][col] &= (~OBJECT);
		vanish(obj, 0, &level_objects);
		*status = 0;
		if (id_scrolls[SCARE_MONSTER].id_status == UNIDENTIFIED) {
			id_scrolls[SCARE_MONSTER].id_status = IDENTIFIED;
		}
		return NULL;
	}
	if (obj->what_is == GOLD) {
		rogue.gold += obj->quantity;
		dungeon[row][col] &= ~(OBJECT);
		take_from_pack(obj, &level_objects);
		print_stats(STAT_GOLD);
		return obj;  // obj will be free_object()ed in caller
	}
	if (pack_count(obj) >= MAX_PACK_COUNT) {
		message("pack too full", 1);
		return NULL;
	}
	dungeon[row][col] &= ~(OBJECT);
	take_from_pack(obj, &level_objects);
	obj = add_to_pack(obj, &rogue.pack, 1);
	obj->picked_up = 1;
	return obj;
}

void drop(void) {
	if (dungeon[rogue.row][rogue.col] & (OBJECT | STAIRS | TRAP)) {
		message("there's already something there", 0);
		return;
	}
	if (!rogue.pack.next_object) {
		message("you have nothing to drop", 0);
		return;
	}
	short ch;
	if ((ch = pack_letter("drop what?", ALL_OBJECTS)) == CANCEL)
		return;

	object *obj = get_letter_object(ch);
	if (!obj) {
		message("no such item.", 0);
		return;
	}
	if (obj->in_use_flags & BEING_WIELDED) {
		if (obj->is_cursed) {
			message(curse_message, 0);
			return;
		}
		unwield(rogue.weapon);
	} else if (obj->in_use_flags & BEING_WORN) {
		if (obj->is_cursed) {
			message(curse_message, 0);
			return;
		}
		mv_aquatars();
		unwear(rogue.armor);
		print_stats(STAT_ARMOR);
	} else if (obj->in_use_flags & ON_EITHER_HAND) {
		if (obj->is_cursed) {
			message(curse_message, 0);
			return;
		}
		un_put_on(obj);
	}
	obj->row = rogue.row;
	obj->col = rogue.col;

	if ((obj->quantity > 1) && (obj->what_is != WEAPON)) {
		obj->quantity--;
		object *new = alloc_object();
		*new = *obj;
		new->quantity = 1;
		obj = new;
	} else {
		obj->ichar = 'L';
		take_from_pack(obj, &rogue.pack);
	}
	place_at(obj, rogue.row, rogue.col);
	messagef(0, "dropped %s", get_desc_str(obj));
	reg_move();
}

static object * check_duplicate(object *obj, object *pack) {
	if (!(obj->what_is & (WEAPON | FOOD | SCROL | POTION)))
		return 0;
	if (obj->what_is == FOOD && obj->which_kind == FRUIT)
		return 0;

	object *op = pack->next_object;
	while (op) {
		if (op->what_is == obj->what_is && op->which_kind == obj->which_kind) {
			if (obj->what_is != WEAPON || (
					obj->what_is == WEAPON && (
						obj->which_kind == ARROW || obj->which_kind == DAGGER ||
						obj->which_kind == DART || obj->which_kind == SHURIKEN
					) &&
					obj->quiver == op->quiver
				)
			) {
				op->quantity += obj->quantity;
				return op;
			}
		}
		op = op->next_object;
	}
	return 0;
}

static int next_avail_ichar(void) {
	bool ichars[26];
	for (int i = 0; i < 26; i++)
		ichars[i] = 0;

	object *obj = rogue.pack.next_object;
	while (obj) {
		if (obj->ichar >= 'a' && obj->ichar <= 'z')
			ichars[(obj->ichar - 'a')] = 1;
		obj = obj->next_object;
	}
	for (int i = 0; i < 26; i++) {
		if (!ichars[i])
			return i + 'a';
	}
	return '?';
}

void wait_for_ack(void) {
	if (!isatty(0) || !isatty(1))
	    return;
	while (true) {
		int c = rgetchar();
		if (c == ' ' || c == 0x1b || c == 0x0d)
			break;
	}
}

short pack_letter(const char *prompt, unsigned short mask) {
	if (!mask_pack(&rogue.pack, mask)) {
		message("nothing appropriate", 0);
		return CANCEL;
	}

	message(prompt, 0);
	inventory(&rogue.pack, mask, 1);
	for (;;) {
		int ch = rgetchar();
		if (is_pack_letter(&ch, &mask)) {
			check_message();
			clear_inventory();
			return ch;
		}
		sound_bell();
	}
}

void take_off(void) {
	if (rogue.armor) {
		if (rogue.armor->is_cursed)
			message(curse_message, 0);
		else {
			mv_aquatars();
			object *obj = rogue.armor;  // Save so get_desc() gets right message.
			unwear(rogue.armor);
			messagef(0, "was wearing %s", get_desc_str(obj));
			print_stats(STAT_ARMOR);
			reg_move();
		}
	} else {
		message("not wearing any", 0);
	}
}

void wear(void) {
	if (rogue.armor) {
		message("you're already wearing some", 0);
		return;
	}

	short ch = pack_letter("wear what?", ARMOR);
	if (ch == CANCEL)
		return;

	object *obj = get_letter_object(ch);
	if (!obj) {
		message("no such item.", 0);
		return;
	}
	if (obj->what_is != ARMOR) {
		message("you can't wear that", 0);
		return;
	}
	obj->identified = 1;
	messagef(0, "wearing %s", get_desc_str(obj));
	do_wear(obj);
	print_stats(STAT_ARMOR);
	reg_move();
}

void unwear(object *obj) {
	if (obj)
		obj->in_use_flags &= (~BEING_WORN);
	rogue.armor = NULL;
}

void do_wear(object *obj) {
	rogue.armor = obj;
	obj->in_use_flags |= BEING_WORN;
	obj->identified = 1;
}

void wield(void) {
	if (rogue.weapon && rogue.weapon->is_cursed) {
		message(curse_message, 0);
		return;
	}

	short ch = pack_letter("wield what?", WEAPON);
	if (ch == CANCEL)
		return;

	object *obj = get_letter_object(ch);
	if (!obj) {
		message("No such item.", 0);
		return;
	}
	if (obj->what_is & (ARMOR | RING)) {
		messagef(0, "you can't wield %s", ((obj->what_is == ARMOR) ? "armor" : "rings"));
		return;
	}
	if (obj->in_use_flags & BEING_WIELDED)
		message("in use", 0);
	else {
		unwield(rogue.weapon);
		messagef(0, "wielding %s", get_desc_str(obj));
		do_wield(obj);
		reg_move();
	}
}

void do_wield(object *obj) {
	rogue.weapon = obj;
	obj->in_use_flags |= BEING_WIELDED;
}

void unwield(object *obj) {
	if (obj)
		obj->in_use_flags &= (~BEING_WIELDED);
	rogue.weapon = NULL;
}

void call_it(void) {
	short ch = pack_letter("call what?", (SCROL | POTION | WAND | RING));
	if (ch == CANCEL)
		return;

	object *obj = get_letter_object(ch);
	if (!obj) {
		message("no such item.", 0);
		return;
	}
	if (!(obj->what_is & (SCROL | POTION | WAND | RING))) {
		message("surely you already know what that's called", 0);
		return;
	}

	char buf[MAX_TITLE_LENGTH+2];
	struct id *id_table = get_id_table(obj);
	if (get_input_line("call it:", "", buf, 1)) {
		id_table[obj->which_kind].id_status = CALLED;
		strcpy(id_table[obj->which_kind].title, buf);
	}
	message(id_table[obj->which_kind].title, 0);
}

short pack_count(const object *new_obj) {
	short count = 0;
	object *obj = rogue.pack.next_object;
	while (obj) {
		if (obj->what_is != WEAPON)
			count += obj->quantity;
		else if (!new_obj)
			count++;
		else if (new_obj->what_is != WEAPON ||
			(obj->which_kind != ARROW && obj->which_kind != DAGGER && obj->which_kind != DART && obj->which_kind != SHURIKEN) ||
			new_obj->which_kind != obj->which_kind || obj->quiver != new_obj->quiver
		) {
			count++;
		}
		obj = obj->next_object;
	}
	return count;
}

static bool mask_pack(const object *pack, unsigned short mask) {
	while (pack->next_object) {
		pack = pack->next_object;
		if (pack->what_is & mask)
			return 1;
	}
	return 0;
}

static bool is_pack_letter(int *c, unsigned short *mask) {
	switch (*c) {
	case '?': *mask = SCROL;  return 1;
	case '!': *mask = POTION; return 1;
	case ':': *mask = FOOD;   return 1;
	case '=': *mask = RING;   return 1;
	case ')': *mask = WEAPON; return 1;
	case ']': *mask = ARMOR;  return 1;
	case '/': *mask = WAND;   return 1;
	case ',': *mask = AMULET; return 1;
	}
	return (*c >= 'a' && *c <= 'z') || *c == CANCEL;
}

bool has_amulet(void) {
	return mask_pack(&rogue.pack, AMULET);
}

void kick_into_pack(void) {
	if (!(dungeon[rogue.row][rogue.col] & OBJECT)) {
		message("nothing here", 0);
		return;
	}

	short stat = 0;
	object *obj = pick_up(rogue.row, rogue.col, &stat);
	if (obj != NULL) {
		if (obj->what_is == GOLD) {
			message(get_desc_str(obj), 0);
			free_object(obj);
		} else
			messagef(0, "%s (%c)", get_desc_str(obj), obj->ichar);
	}
	if (obj || !stat)
		reg_move();
}
