CC = gcc
CFLAGS = -Wall -Wextra -O2
LDLIBS = -lncurses

GAMES = snake tetris minesweeper donkeykong pacman paciencia game2048 pong spaceinvaders enduro frogger centipede digdug
BINS = $(GAMES) menu

.PHONY: all clean run

all: $(BINS)

menu: menu.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

snake: snake.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

tetris: tetris.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

minesweeper: minesweeper.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

donkeykong: donkeykong.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

pacman: pacman.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

paciencia: paciencia.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

game2048: game2048.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

pong: pong.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

spaceinvaders: spaceinvaders.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

enduro: enduro.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

frogger: frogger.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

centipede: centipede.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

digdug: digdug.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

clean:
	rm -f $(BINS)

run: all
	./menu

.PHONY: install
PREFIX = /usr
INSTALL_DIR = $(DESTDIR)$(PREFIX)/bin

install: all
	install -d $(INSTALL_DIR)
	install -m 755 menu $(INSTALL_DIR)/shellgame
	for game in $(GAMES); do install -m 755 $$game $(INSTALL_DIR)/shellgame-$$game; done
