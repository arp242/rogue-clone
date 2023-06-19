CC        ?= cc
PREFIX    ?= /usr/local
MANPREFIX ?= ${PREFIX}/share/man
NAME      ?= rogue-clone
STATIC    ?= -static
CFLAGS    += -std=c99 -g -Wall -pedantic -D_DEFAULT_SOURCE
LDFLAGS   += -lcurses ${STATIC}

.PHONY: all clean install uninstall

SRC = hit.c inventory.c level.c machdep.c main.c message.c monster.c move.c \
      object.c pack.c play.c ring.c room.c save.c score.c throw.c trap.c use.c zap.c
OBJ = ${SRC:.c=.o}

all: rogue-clone

${OBJ}: rogue.h Makefile
.c.o:
	${CC} -c ${CFLAGS} $<
rogue-clone: ${OBJ}
	${CC} -o $@ *.o ${LDFLAGS}

clean:
	rm -f rogue-clone *.o
install:
	install -Dm755 rogue-clone       ${DESTDIR}${PREFIX}/bin/${NAME}
	install -Dm644 doc/rogue-clone.6 ${DESTDIR}${MANPREFIX}/man6/${NAME}.6
uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/${NAME} ${DESTDIR}${MANPREFIX}/man6/${NAME}.6
