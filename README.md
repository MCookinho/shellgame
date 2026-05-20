# Shell Games

**Shell Games** is a collection of 25 classic and original arcade games that run directly in your terminal. Built with C and ncurses, with a unified menu, language support (EN/PT), and multiple themes.

---

## Games

| # | Game | Description |
|---|------|-------------|
| 1 | **2048** | Slide and merge tiles to reach 2048 |
| 2 | **Asteroids** | Blast asteroids in deep space |
| 3 | **Blackjack** | Beat the dealer in classic 21 |
| 4 | **Breakout** | Break all the bricks with paddle and ball |
| 5 | **Centipede** | Shoot the creeping centipede through mushrooms |
| 6 | **Checkers** | Classic draughts for two players |
| 7 | **Chess** | Classic chess for two players |
| 8 | **Dig Dug** | Dig tunnels and pump enemies until they pop |
| 9 | **Donkey Kong** | Climb platforms dodging barrels |
| 10 | **Duck Hunt** | Aim and shoot ducks in this classic gallery |
| 11 | **Enduro** | Race through traffic on the open road. Survive 4 days |
| 12 | **Frogger** | Cross a busy highway to reach home |
| 13 | **Galaga** | Classic arcade shooter — defeat alien waves |
| 14 | **Hangman** | Guess the word before the man is hanged |
| 15 | **Minesweeper** | Find all mines without exploding |
| 16 | **Pac-Man** | Eat dots and avoid ghosts in the maze |
| 17 | **Pong** | Table tennis for 1 or 2 players |
| 18 | **Q\*Bert** | Hop on cubes to change their color |
| 19 | **Rhythm Spire** | 4-lane rhythm game — feel the beat |
| 20 | **Snake** | Classic snake — eat food and grow |
| 21 | **Solitaire** | Classic Klondike solitaire card game |
| 22 | **Spire Ascent** | A deck-building roguelike — climb the Spire |
| 23 | **Super Auto Pets** | Auto-battler — collect pets and fight |
| 24 | **Tetris** | Stack falling blocks to clear lines |
| 25 | **The Typer** | Type words to survive the zombie apocalypse |
|   | **Settings** | Change language (EN/PT) and theme (Dark/Light/Retro) |

---

## Requirements

- **C compiler** (GCC, Clang, or MSVC)
- **ncurses library** (or PDCurses on Windows)

### Installing ncurses

**Linux (Debian/Ubuntu)**
```bash
sudo apt install libncurses-dev build-essential
```

**Linux (Fedora/RHEL)**
```bash
sudo dnf install ncurses-devel gcc
```

**macOS**
```bash
# ncurses is pre-installed. Just install Xcode Command Line Tools:
xcode-select --install
```

**Windows**
```bash
# Option 1: Windows Subsystem for Linux (WSL)
# Install Ubuntu WSL, then follow Linux instructions above

# Option 2: MSYS2
pacman -S mingw-w64-x86_64-ncurses mingw-w64-x86_64-gcc

# Option 3: Use PDCurses (see build instructions below)
```

---

## Build & Run

```bash
# Clone the repository
git clone https://github.com/MCookinho/shellgame.git
cd shellgame

# Build everything
make

# Build and launch the menu
make run

# Or launch a game directly
./snake
./tetris
```

### Install system-wide
```bash
sudo make install
```
Then launch with `shellgame` or `shellgame-<game>`.

### AUR (Arch Linux)
```bash
yay -S shellgame
```

---

## Controls

| Game | Movement | Action |
|------|----------|--------|
| **2048** | WASD / Arrow keys | — |
| **Asteroids** | WASD / Arrow keys | **Space**: shoot |
| **Blackjack** | Arrow keys to navigate | **Enter**: confirm |
| **Breakout** | ← → / A D move paddle | — |
| **Centipede** | ← → move | **Space**: shoot |
| **Checkers** | Arrow keys + **Enter** to select/move | — |
| **Chess** | Arrow keys + **Enter** to select/move | — |
| **Dig Dug** | WASD / Arrow keys | **Space**: pump enemy |
| **Donkey Kong** | WASD / Arrow keys | **Space**: jump |
| **Duck Hunt** | Arrow keys aim | **Space**: shoot |
| **Enduro** | ← → move lanes | — |
| **Frogger** | WASD / Arrow keys | — |
| **Galaga** | ← → move | **Space**: shoot |
| **Hangman** | Type letters | — |
| **Minesweeper** | Arrow keys move cursor | **Enter**: reveal, **F**: flag |
| **Pac-Man** | WASD / Arrow keys | — |
| **Pong** | W/S (P1), ↑/↓ (P2) | **Space**: start |
| **Q\*Bert** | WASD / Arrow keys (diagonal) | — |
| **Rhythm Spire** | D F J K keys | — |
| **Snake** | WASD / Arrow keys | — |
| **Solitaire** | Arrow keys + **Space** to pick/place | — |
| **Spire Ascent** | 1-9: play card | **Enter/Space**: end turn |
| **Super Auto Pets** | Arrow keys navigate | **Enter**: confirm |
| **Tetris** | ← → move, ↑ rotate, ↓ soft drop | **Space**: hard drop |
| **The Typer** | Type the word on the zombie | — |

**Q** quits any game. **Enter** confirms in menus.

---

## Configuration

Settings are persisted to `~/.config/shellgames/config`.

### Languages
- **English** (default)
- **Português**

### Themes
- **Dark** — classic terminal look (default)
- **Light** — white background for bright terminals
- **Retro** — green-on-black amber monochrome

---

## Project Structure

```
├── menu.c             # Unified launcher menu
├── config.h           # Shared configuration (lang, theme, colors)
├── game2048.c         # 2048
├── asteroids.c        # Asteroids
├── blackjack.c        # Blackjack
├── breakout.c         # Breakout
├── centipede.c        # Centipede
├── checkers.c         # Checkers
├── chess.c            # Chess
├── digdug.c           # Dig Dug
├── donkeykong.c       # Donkey Kong
├── duckhunt.c         # Duck Hunt
├── enduro.c           # Enduro
├── frogger.c          # Frogger
├── galaga.c           # Galaga
├── hangman.c          # Hangman
├── minesweeper.c      # Minesweeper
├── pacman.c           # Pac-Man
├── pong.c             # Pong
├── qbert.c            # Q*Bert
├── rhythm.c           # Rhythm Spire
├── snake.c            # Snake
├── paciencia.c        # Klondike Solitaire
├── spire.c            # Spire Ascent
├── superautopets.c    # Super Auto Pets
├── tetris.c           # Tetris
├── typer.c            # The Typer
├── Makefile           # Build system
└── README.md          # This file
```

Each game is a standalone binary. The menu launches via `system()`.

All games compile with zero warnings at `-Wall -Wextra -O2`.

---

## Building from Source (one-liner)

```bash
gcc -Wall -Wextra -O2 -lncurses -o menu menu.c
```

---

## License

This project is open source. Feel free to play, modify, and share.
