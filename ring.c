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
 * @(#)ring.c 8.1 (Berkeley) 5/31/93
 * $FreeBSD: src/games/rogue/ring.c,v 1.3 1999/11/30 03:49:26 billf Exp $
 *
 * This source herein may be modified and/or distributed by anybody who
 * so desires, with the following restrictions:
 *    1.)  No portion of this notice shall be removed.
 *    2.)  Credit shall not be taken for the creation of this source.
 *    3.)  This code is not to be traded, sold, or used for personal
 *         gain or profit.
 */

#include "rogue.h"

static const char left_or_right[] = "left or right hand?";

short stealthy;
short r_rings;
short add_strength;
short e_rings;
short regeneration;
short ring_exp;
short auto_search;
bool r_teleport;
bool r_see_invisible;
bool sustain_strength;
bool maintain_armor;

void put_on_ring(void) {
	if (r_rings == 2) {
		message("wearing two rings already", 0);
		return;
	}

	int ch = pack_letter("put on what?", RING);
	if (ch == CANCEL)
		return;

	object *ring = get_letter_object(ch);
	if (!ring) {
		message("no such item.", 0);
		return;
	}

	if (!(ring->what_is & RING)) {
		message("that's not a ring", 0);
		return;
	}
	if (ring->in_use_flags & (ON_LEFT_HAND | ON_RIGHT_HAND)) {
		message("that ring is already being worn", 0);
		return;
	}
	if (r_rings == 1)
		ch = (rogue.left_ring ? 'r' : 'l');
	else {
		message(left_or_right, 0);
		do {
			ch = rgetchar();
		} while (ch != CANCEL && ch != 'l' && ch != 'r' && ch != '\n' && ch != '\r');
	}
	if (ch != 'l' && ch != 'r') {
		check_message();
		return;
	}
	if ((ch == 'l' && rogue.left_ring) || (ch == 'r' && rogue.right_ring)) {
		check_message();
		message("there's already a ring on that hand", 0);
		return;
	}

	do_put_on(ring, ch == 'l');
	ring_stats(1);
	check_message();
	message(get_desc_str(ring), 0);
	reg_move();
}

// Do not call ring_stats() from within do_put_on(). It will cause serious
// problems when do_put_on() is called from read_pack() in restore().
void do_put_on(object *ring, bool on_left) {
	if (on_left) {
		ring->in_use_flags |= ON_LEFT_HAND;
		rogue.left_ring = ring;
	} else {
		ring->in_use_flags |= ON_RIGHT_HAND;
		rogue.right_ring = ring;
	}
}

void remove_ring(void) {
	if (r_rings == 0) {
		message("not wearing any rings", 0);
		return;
	}

	bool left = 0;
	if (rogue.left_ring && !rogue.right_ring)
		left = 1;
	else if (!rogue.left_ring && rogue.right_ring)
		left = 0;
	else {
		message(left_or_right, 0);
		int ch;
		do {
			ch = rgetchar();
		} while (ch != CANCEL && ch != 'l' && ch != 'r' && ch != '\n' && ch != '\r');
		left = (ch == 'l');
		check_message();
	}

	object *ring = left ? rogue.left_ring : rogue.right_ring;
	if (ring->is_cursed)
		message(curse_message, 0);
	else {
		un_put_on(ring);
		messagef(0, "removed %s", get_desc_str(ring));
		reg_move();
	}
}

void un_put_on(object *ring) {
	if (ring && (ring->in_use_flags & ON_LEFT_HAND)) {
		ring->in_use_flags &= (~ON_LEFT_HAND);
		rogue.left_ring = NULL;
	} else if (ring && (ring->in_use_flags & ON_RIGHT_HAND)) {
		ring->in_use_flags &= (~ON_RIGHT_HAND);
		rogue.right_ring = NULL;
	}
	ring_stats(1);
}

void gr_ring(object *ring, bool assign_wk) {
	ring->what_is = RING;
	if (assign_wk)
		ring->which_kind = get_rand(0, (RINGS - 1));
	ring->class = 0;

	switch (ring->which_kind) {
	case R_TELEPORT:
		ring->is_cursed = 1;
		break;
	case ADD_STRENGTH:
	case DEXTERITY:
		while ((ring->class = (get_rand(0, 4) - 2)) == 0)
			;
		ring->is_cursed = (ring->class < 0);
		break;
	case ADORNMENT:
		ring->is_cursed = coin_toss();
		break;
	}
}

void inv_rings(void) {
	if (r_rings == 0)
		message("not wearing any rings", 0);
	else {
		if (rogue.left_ring)
			message(get_desc_str(rogue.left_ring), 0);
		if (rogue.right_ring)
			message(get_desc_str(rogue.right_ring), 0);
	}
	if (wizard)
		messagef(0, "ste %hi, r_r %hi, e_r %hi, r_t %d, s_s %d"
			", a_s %hi, reg %hi, r_e %hi, s_i %d, m_a %d, aus %hi",
			stealthy, r_rings, e_rings, r_teleport, sustain_strength,
			add_strength, regeneration, ring_exp, r_see_invisible,
			maintain_armor, auto_search);
}

void ring_stats(bool pr) {
	stealthy = 0;
	r_rings = 0;
	e_rings = 0;
	r_teleport = 0;
	sustain_strength = 0;
	add_strength = 0;
	regeneration = 0;
	ring_exp = 0;
	r_see_invisible = 0;
	maintain_armor = 0;
	auto_search = 0;

	object *ring;
	for (short i = 0; i < 2; i++) {
		if (!(ring = ((i == 0) ? rogue.left_ring : rogue.right_ring))) {
			continue;
		}
		r_rings++;
		e_rings++;
		switch (ring->which_kind) {
		case STEALTH:          stealthy++;                  break;
		case R_TELEPORT:       r_teleport = 1;              break;
		case REGENERATION:     regeneration++;              break;
		case SLOW_DIGEST:      e_rings -= 2;                break;
		case ADD_STRENGTH:     add_strength += ring->class; break;
		case SUSTAIN_STRENGTH: sustain_strength = 1;        break;
		case DEXTERITY:        ring_exp += ring->class;     break;
		case R_SEE_INVISIBLE:  r_see_invisible = 1;         break;
		case MAINTAIN_ARMOR:   maintain_armor = 1;          break;
		case SEARCHING:        auto_search += 2;            break;
		case ADORNMENT:        /* Does nothing */           break;
		}
	}
	if (pr) {
		print_stats(STAT_STRENGTH);
		relight();
	}
}
