The "rogue" game from 4.3BSD-Tahoe, for modern systems.

![screenshot.png](https://raw.githubusercontent.com/arp242/rogue-clone/master/doc/screenshot.png)

There are many versions and variants of the original rogue game; the original
was shipped with 4.2BSD in 1980, and throughout the 80s the original authors
released different versions, and others made their own variants based on
"leaked" source code.

This one is a "clone" added to 4.3BSD-Tahoe in 1987 and was written by Timothy
C. Stoehr; it doesn't re-use any code from the original but was intended as a
"freely distributable open source" re-implementation (for a while it was named
"pdrogue" for "public domain rogue", but got renamed to "rogue" a bit later on).

This is also the "rogue" game that is shipped with NetBSD, DragonFly BSD,
FreeBSD <5, and OpenBSD <3.3, and that you can find in "bsdgames" packages on
some Linux systems.

Since 1987 the meaning of what many people understand as "freely distributable"
and "open source" has changed, and the "don't be evil and don't use for
commercial purposes"-type [license] makes it "non-free", which is why e.g.
Debian ships it as "bsdgames-nonfree". I have no idea how to contact "Timothy C.
Stoehr" to ask for an updated license (a basic internet search doesn't show up
anything), so it is what it is. If you know how to contact him: please let me
know.

There are some differences with the original rogue; the [CHANGES] file lists
some of them, but there are some more (e.g. different monsters). [The roguelike
archive] has links for other variants, and [Wikipedia] has an overview of the
history.

This game is much simpler than more modern roguelikes such as Angband, Dungeon
Crawl, and many others; there are no races, classes, too many attributes, or
many of the other things. This is a feature! Sometimes you just want to play a
simple game. That said, I've never actually finished the game, so while fairly
simple it's not an easy game.

[license]: LICENSE
[CHANGES]: doc/CHANGES
[The roguelike archive]: https://britzl.github.io/roguearchive/
[Wikipedia]: https://en.wikipedia.org/wiki/Rogue_(video_game)

Building
--------
To compile, simply using `make` should work on most systems; you will need some
curses (e.g. ncurses or PDCurses). The default is to build a statically linked
binary; use `make STATIC=` to build a dynamically linked one.

Run `./rogue-clone` to play the game; you only need this binary – there are no
data files. You can use `make install` or `make install PREFIX=/usr` to install
it system-wide.

It should work on all Unix-y systems (including macOS) and Windows (with
PDCurses).

Usage
-----
Use `?` to get a list of keybinds and `/` to get the meaning of a character on
the screen.

The original *A Guide to the Dungeons of Doom* manual is also available as [PDF]
or [text]; most of it still applies, aside from a few details.

[PDF]: doc/a-guide-to-the-dungeons-of-doom.pdf
[text]: doc/a-guide-to-the-dungeons-of-doom.txt

Probably the most confusing for most will be the movement keys; it uses Vim-like
hjkl keys, and with some extra ones for diagonal movement:

	 y  k  u
	  ↖ ↑ ↗
	h ←   → l
	  ↙ ↓ ↘
	 b  j  n

So if you had mastered hjkl in Vim then here's a new challenge. You do get used
to it; I haven't played this in over 10 years, but I can still do these
movements without difficulty from muscle memory (which probably says something
sad about my youth...)

However, to make things a bit easier for the p0sers it also supports arrow keys
and the numpad:

	 7  8  9
	  ↖ ↑ ↗
	4 ←   → 6
	  ↙ ↓ ↘
	 1  2  3

You can set options with `o`; these are *not* saved, but you can use the
`ROGUE_CLONE_OPTS` environment variable, which takes a comma separated list of
options:

| Name            | Default                         | Description                                                      |
| ----            | -------                         | -----------                                                      |
| file            | ~/.local/share/rogue-clone/save | Save file                                                        |
| name=           | $USER                           | Character name; detaults to user name; use `*` for a random name |
| fruit=          | slime-mold                      | Name of the "fruit" food                                         |
| lowhealth=      | 0                               | Print health in standout if HP is lower than this number         |
| noaskquit       | (not set)                       | Don't ask confirmation on SIGQUIT (`^\`, no effect for Q)        |
| noskull, notomb | (not set)                       | Don't display headstone after death                              |
| passgo          | (not set)                       | Run around corners with Shift                                    |
| lock            | (not set)                       | Don't allow changing name or save path in `o` menu               |
| noautosave      | (not set)                       | Don't autosave every 120 moves and on level change               |
| revshift        | (not set)                       | Reverse shift+dir/ctrl+dir                                       |

e.g. `export ROGUE_CLONE_OPTS='name=*,lowhealth=5,passgo'`.

Changes from the original "rogue clone"
---------------------------------------
The core game is unchanged, with most changes being related to the UI and
contemporary usage patterns:

- Scores are stored in `~/.local/share/rogue-clone/score` instead of
  `/var/games/rogue/scores`. It also stores all runs for all users, instead of
  just the top run for every user, and it uses the "name" set in the game
  instead of the user's login name.

  The original assumed there were many people playing rogue on a single system
  (and used setgid to make the score file writeable), but this is a rare use
  case now and this avoids having to create system directories.

- The default save file is now `~/.local/share/rogue-clone/save` instead of
  `./rogue.dump` in the current directory, and this will be loaded automatically
  on startup if it exists so you don't need to use load it manually with `rogue
  ~/.local/share/rogue-clone/save`.

- `!` prints game seed, and you can set the game seed on startup with -S.

  In the original `!` would drop back to shell; this is removed as it doesn't
  work on Windows or WASM and it's pretty useless in modern context anyway where
  you can just open a new xterm or use tmux or whatever.

- Support arrow keys and numpad for movement. Also add `m` and `^M` to run (as
  an alternative to `J`/`^j` etc.)

- To repeat `.` and `s` you now need to use `C<num>s`, instead of `<num>s` –
  this was needed to add numpad support.

- Rename `ROGUEOPTS` to `ROGUE_CLONE_OPTS`; this way it won't conflict with
  other rogue variants.

- Store name in savegame.

- name=* selects a random name from a built-in list.

- Add `lowhealth` option to make health stand out if it's low.

- Add `lock` option to disallow changing the username and savegame location (the
  main use case for this is offering a ssh service to play the game, but I
  haven't gotten around to that yet).

- Automatically open inventory instead of having to press `*`.

- Some more feedback in prompts and when repeating things.

- Also allow Esc and Enter to clear "--press space to continue--" prompts.

- Add `noautosave` option; the default is to autosave every 120 moves and before
  changing levels; you can set this to disable it.

- Add `revshift` option to reverse Shift+dir and Ctrl+dir movements; many more
  modern roguelike games use Shift+dir to mean what's Ctrl+dir.

Sources
-------
Source files for rogue; most are pretty much the same as far as I could see:

- [4.3BSD-Tahoe](https://github.com/weiss/original-bsd/commit/e376515dbfe31966ba0d288786a8073ff710ac2f) (first version)
- [DragonFly BSD](https://gitweb.dragonflybsd.org/dragonfly.git/tree/HEAD:/games/rogue)
- [NetBSD](https://github.com/NetBSD/src/tree/trunk/games/rogue)
- [OpenBSD](https://github.com/openbsd/src/tree/b8d5a5fb3cd18b5becb179d749e65fc04a659093/games/rogue) (Removed in 3.3)
- [FreeBSD](https://github.com/freebsd/freebsd-src/tree/e05f78b8316cc4c48131cbc9093827a26f204680/games/rogue)
  (Removed in 5).
- https://www.freshports.org/games/bsdgames/ (the "BSD games" were moved to a
  port in FreeBSD 5; I used this as a starting point)
