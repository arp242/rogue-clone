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
 * @(#)play.c 8.1 (Berkeley) 5/31/93
 * $FreeBSD: src/games/rogue/play.c,v 1.3 1999/11/30 03:49:26 billf Exp $
 *
 * This source herein may be modified and/or distributed by anybody who
 * so desires, with the following restrictions:
 *    1.)  No portion of this notice shall be removed.
 *    2.)  Credit shall not be taken for the creation of this source.
 *    3.)  This code is not to be traded, sold, or used for personal
 *         gain or profit.
 */

#include "rogue.h"

bool interrupted = 0;

static const char unknown_command[] = "unknown command";

void play_level(void) {
	int count;
	for (;;) {
		interrupted = 0;
		if (hit_message[0]) {
			message(hit_message, 1);
			hit_message[0] = 0;
		}
		if (trap_door) {
			trap_door = 0;
			return;
		}
		move(rogue.row, rogue.col);
		refresh();

		int ch = rgetchar();
clear_messages:
		check_message();
		count = 0;
switch_start:
		switch (ch) {
		// ^R and & handled in rgetchar(), they work in any location.
		case ' ':                   break;
		case 'f': fight(0);         break;
		case 'F': fight(1);         break;
		case 'e': eat();            break;
		case 'q': quaff();          break;
		case 'r': read_scroll();    break;
		case 'm': move_onto();      break;
		case ',': kick_into_pack(); break;
		case 'd': drop();           break;
		case 'P': put_on_ring();    break;
		case 'R': remove_ring();    break;
		case '=': inv_rings();      break;
		case '^': id_trap();        break;
		case '/': id_type();        break;
		case '?': show_help();      break;
		case 'o': edit_opts();      break;
		case 'I': single_inv(0);    break;
		case 'T': take_off();       break;
		case 'W': wear();           break;
		case 'w': wield();          break;
		case 'c': call_it();        break;
		case 'z': zapp();           break;
		case 't': throw();          break;
		case 'Q': quit(0);          break;
		case 'S': save_game();      break;
		case '.': rest(count > 0 ? count : 1);            break;
		case 's': search(count > 0 ? count : 1, 0);       break;
		case 'i': inventory(&rogue.pack, ALL_OBJECTS, 0); break;
		case '!': messagef(0, "seed: %ld", game_seed);    break;
		case '>': if (drop_check()) { return; }           break;
		case '<': if (check_up())   { return; }           break;
		case 'v': message("rogue-clone: Version III. (Tim Stoehr was here), tektronix!zeus!tims", 0); break;
		case ')':
		case ']':        inv_armor_weapon(ch == ')'); break;
		case 'A' - 0x40: show_average_hp();           break;

		case KEY_UP:    one_move_rogue('k', 1); break;
		case KEY_RIGHT: one_move_rogue('l', 1); break;
		case KEY_DOWN:  one_move_rogue('j', 1); break;
		case KEY_LEFT:  one_move_rogue('h', 1); break;
		case '1': case '2': case '3': case '4': case '6': case '7': case '8': case '9':
			ch = "bjnh lyku"[ch - '1']; // fallthrough
		case 'h': case 'j': case 'k': case 'l': case 'y': case 'u': case 'n': case 'b':
			one_move_rogue(ch, 1); break;

		case 'M':
		case 'M' - 0x40: {
			bool ctrl = (ch & 0x40) == 0x40;
			message("direction? ", 0);
			ch = rgetchar();
			switch (ch) {
			case KEY_UP:    ch = 'k'; break;
			case KEY_RIGHT: ch = 'l'; break;
			case KEY_DOWN:  ch = 'j'; break;
			case KEY_LEFT:  ch = 'h'; break;
			}
			if (ch >= '1' && ch <= '9' && ch != '5')
				ch = "bjnh lyku"[ch - '1'];
			ch ^= (ctrl ? 0x20 : 0x60);
			// fallthrough
		}
		case 'H': case 'H' - 0x40:
		case 'J': case 'J' - 0x40:
		case 'K': case 'K' - 0x40:
		case 'L': case 'L' - 0x40:
		case 'B': case 'B' - 0x40:
		case 'Y': case 'Y' - 0x40:
		case 'U': case 'U' - 0x40:
		case 'N': case 'N' - 0x40:
			multiple_move_rogue(ch); break;

		case 'P' - 0x40:
			do {
				remessage(count++);
				ch = rgetchar();
			} while (ch == '\020');
			goto clear_messages;
			break;

		case 'C':
		 	move(rogue.row, rogue.col);
		 	refresh();
		 	check_message();
		 	message("repeat:", 1);
		 	for (ch = rgetchar(); is_digit(ch); ch = rgetchar()) {
		 		if (count < 100)
		 			count = (10 * count) + (ch - '0');
		 		check_message();
		 		messagef(1, "repeat: %d", count);
		 	}
		 	check_message();
		 	if (ch != CANCEL)
		 		goto switch_start;
			break;

		case 'W' - 0x40: wizardize(); break;
		case 'I' - 0x40: if (wizard) inventory(&level_objects, ALL_OBJECTS, 0); else message(unknown_command, 0); break;
		case 'S' - 0x40: if (wizard) draw_magic_map();                          else message(unknown_command, 0); break;
		case 'T' - 0x40: if (wizard) show_traps();                              else message(unknown_command, 0); break;
		case 'O' - 0x40: if (wizard) show_objects();                            else message(unknown_command, 0); break;
		case 'E' - 0x40: if (wizard) c_object_for_wizard();                     else message(unknown_command, 0); break;
		case 'V' - 0x40: if (wizard) show_monsters();                           else message(unknown_command, 0); break;
		default:
			message(unknown_command, 0);
			break;
		}
	}
}
