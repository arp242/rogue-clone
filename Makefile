CC        ?= cc
PREFIX    ?= /usr/local
MANPREFIX ?= ${PREFIX}/share/man
NAME      ?= rogue
STATIC    ?= -static

# Add -DWINDOWS for Windows
CFLAGS  += -std=c99 -g -Os -Wall -pedantic -D_DEFAULT_SOURCE
LDFLAGS += -lncurses ${STATIC}

SRC = hit.c inventory.c level.c machdep.c main.c message.c monster.c move.c \
      object.c pack.c play.c ring.c room.c save.c score.c throw.c trap.c use.c zap.c
OBJ = ${SRC:.c=.o}

.PHONY: all clean install uninstall

all: rogue

.c.o:
	${CC} -c ${CFLAGS} $<

rogue: ${OBJ}
	${CC} -o $@ *.o ${CFLAGS} ${LDFLAGS}

install:
	install -Dm755 rogue       ${DESTDIR}${PREFIX}/bin/${NAME}
	install -Dm644 doc/rogue.6 ${DESTDIR}${MANPREFIX}/man6/${NAME}.6

clean:
	rm -f rogue *.o

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/${NAME} ${DESTDIR}${MANPREFIX}/man6/${NAME}.6
