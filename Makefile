CC = gcc
CFLAGS = -Wall -Wextra -O2
LDLIBS = -lncurses

GAMES = snake tetris minesweeper donkeykong pacman paciencia game2048 pong enduro frogger centipede digdug spire superautopets asteroids rhythm chess blackjack checkers hangman typer duckhunt galaga qbert breakout
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

enduro: enduro.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

frogger: frogger.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

centipede: centipede.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

digdug: digdug.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

spire: spire.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

superautopets: superautopets.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

asteroids: asteroids.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS) -lm

rhythm: rhythm.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS) -lm

chess: chess.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

blackjack: blackjack.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

checkers: checkers.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

hangman: hangman.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

typer: typer.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

duckhunt: duckhunt.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

galaga: galaga.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS) -lm

qbert: qbert.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

breakout: breakout.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

clean:
	rm -f $(BINS)

run: all
	./menu

.PHONY: install
PREFIX = /usr
BIN_DIR = $(DESTDIR)$(PREFIX)/bin

install: all
	install -d $(BIN_DIR)
	install -m 755 menu $(BIN_DIR)/shellgame
	for game in $(GAMES); do install -m 755 $$game $(BIN_DIR)/shellgame-$$game; done
