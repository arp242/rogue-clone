CC        ?= cc
PREFIX    ?= /usr/local
MANPREFIX ?= ${PREFIX}/share/man
NAME      ?= rogue

CFLAGS  += -DUNIX -g -Os
LDFLAGS += -lncurses -static

SRC = hit.c init.c inventory.c level.c machdep.c main.c message.c monster.c \
      move.c object.c pack.c play.c random.c ring.c room.c save.c score.c \
      spec_hit.c throw.c trap.c use.c zap.c strlcpy.c
OBJ = ${SRC:.c=.o}

.PHONY: all clean install uninstall

all: rogue

.c.o:
	${CC} -c ${CFLAGS} $<

rogue: ${OBJ}
	${CC} -o $@ ${OBJ} ${CFLAGS} ${LDFLAGS}

install:
	install -Dm755 rogue   ${DESTDIR}${PREFIX}/bin/${NAME}
	install -Dm644 rogue.6 ${DESTDIR}${MANPREFIX}/man6/${NAME}.6

clean:
	rm -f rogue ${OBJ}

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/${NAME} ${DESTDIR}${MANPREFIX}/man6/${NAME}.6
