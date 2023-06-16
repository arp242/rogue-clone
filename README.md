The original rogue game, for modern systems.

There are many variants of rogue; this is the "original" as was shipped with
4.2BSD from 1980 and derivates. I used to play it on FreeBSD 4 back in the day
(it was removed in FreeBSD 5); NetBSD and DragonFly BSD still ship it.

[The roguelike archive] has links for other variants, and [Wikipedia] has an
overview of the history.

This is the simplest rogue-like game that I know; there are no races, classes,
attributes, or many of the features you get in Angband, Dungeon Crawl, etc. This
is a feature! Sometimes you just want to play a simple game.

Usage
-----
To compile, simply using `make` should work on most systems; you will need
ncurses. The default is to build a statically linked binary; use `make STATIC=`
to build a dynamically linked one.

Run `./rogue` to play the game; you only need this binary – there are no data
files. You can use `make install` or `make install PREFIX=/usr` to install it
system-wide.

In the game use `?` to get a list of keybinds and `/` to get the meaning of a
character on the screen.

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

I should perhaps add numpad keys as an alternative, but my laptop (and many
laptops) doesn't have numpad, so meh.

Changes
-------
The core game is mostly unchanged, aside from a few small things:

- Scores are stored in `~/.local/share/rogue/score` instead of
  `/var/games/rogue/scores`. It also stores all runs for all users, instead of
  just the top run for every user.

  The original assumed there were many people playing rogue on a single system
  (and used setgid to make that writeable), but this is a rare use case now and
  this avoids having to create system directories.

- The default save file is now `~/.local/share/rogue/save` instead of
  `./rogue.dump` in the current directory, and this will be loaded automatically
  on startup if it exists so you don't need to use load it manually with
  `rogue ~/.local/share/rogue/save`.

Sources
-------
Source files for rogue; most are pretty much the same as far as I could see:

- [DragonFly BSD](https://gitweb.dragonflybsd.org/dragonfly.git/tree/HEAD:/games/rogue)
- [NetBSD](https://github.com/NetBSD/src/tree/trunk/games/rogue)
- [OpenBSD](https://github.com/openbsd/src/tree/b8d5a5fb3cd18b5becb179d749e65fc04a659093/games/rogue) (Removed in 3.3)
- [FreeBSD](https://github.com/freebsd/freebsd-src/tree/e05f78b8316cc4c48131cbc9093827a26f204680/games/rogue)
  (Removed in 5).
- https://www.freshports.org/games/bsdgames/ (the "BSD games" were moved to a
  port in FreeBSD 5; I used this as a starting point).

[The roguelike archive]: https://britzl.github.io/roguearchive/
[Wikipedia]: https://en.wikipedia.org/wiki/Rogue_(video_game)
