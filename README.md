
    .---. .----.
   /  ___/ |    |
   | (___  |    |
   \___ \ |    |
   ____) ||    |
  '____/ '----'

  ~~ Terminal Arcade Collection ~~

# Shell Games

**Shell Games** is a collection of classic arcade games that run directly in your terminal. Built with C and ncurses, it brings 9 retro games to your command line with zero bloat.

---

## Games

| # | Game | Description |
|---|------|-------------|
| 1 | **Snake** | Classic snake — eat food and grow without hitting yourself |
| 2 | **Tetris** | Stack falling blocks and clear lines. Levels, score, next piece preview |
| 3 | **Minesweeper** | Find all mines without exploding. Flag and reveal cells |
| 4 | **Donkey Kong** | Climb platforms dodging barrels — a platformer in the terminal |
| 5 | **Pac-Man** | Eat dots and avoid ghosts in the maze. Each ghost has unique AI |
| 6 | **Solitaire** | Classic Klondike solitaire with drag-and-drop card movement |
| 7 | **2048** | Slide and merge tiles to reach 2048 |
| 8 | **Pong** | Table tennis for 1 or 2 players. CPU opponent in single-player |
| 9 | **Space Invaders** | Defend Earth from alien invasions across 6 escalating levels |
| 10 | **Enduro** | Race through traffic on the open road. Survive 4 days |
| 11 | **Frogger** | Cross a busy highway to reach home! |
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
git clone https://github.com/your-username/shell-games.git
cd shell-games

# Build everything
make

# Build and launch the menu
make run

# Or launch a game directly
./snake
./tetris
./pacman
```

### Windows with PDCurses

If using native Windows (no WSL), compile with PDCurses:

```bash
gcc -Wall -Wextra -O2 -o snake snake.c -lpdcurses
```

---

## Controls

| Game | Movement | Action |
|------|----------|--------|
| **Snake** | WASD / Arrow keys | — |
| **Tetris** | ← → move, ↑ rotate, ↓ soft drop | **Space**: hard drop |
| **Minesweeper** | Arrow keys to move cursor | **Enter**: reveal, **F**: flag |
| **Donkey Kong** | WASD / Arrow keys | **Space**: jump |
| **Pac-Man** | WASD / Arrow keys | — |
| **Solitaire** | Arrow keys + **Space** to pick/place | — |
| **2048** | WASD / Arrow keys | — |
| **Pong** | W/S (P1), ↑/↓ (P2) | **Space**: start |
| **Space Invaders** | ← → move | **Space**: shoot |
| **Enduro** | ← → move lanes | — |
| **Frogger** | WASD / Arrow keys | — |

**Q** quits any game. **Enter** confirms in menus.

---

## Configuration

Settings are persisted to `~/.config/shellgames/config`.

### Languages
- **English** (default)
- **Português**

Select in the Settings menu (option 10).

### Themes
- **Dark** — classic terminal look (default)
- **Light** — white background for bright terminals
- **Retro** — green-on-black amber monochrome

---

## Project Structure

```
├── config.h           # Shared configuration (lang, theme, colors)
├── menu.c             # Unified launcher menu
├── snake.c            # Snake game
├── tetris.c           # Tetris
├── minesweeper.c      # Minesweeper
├── donkeykong.c       # Donkey Kong
├── enduro.c           # Enduro
├── frogger.c          # Frogger
├── pacman.c           # Pac-Man
├── paciencia.c        # Klondike Solitaire
├── game2048.c         # 2048
├── pong.c             # Pong
├── spaceinvaders.c    # Space Invaders (6 levels)
├── Makefile           # Build system
└── README.md          # This file
```

Each game is a standalone binary. The menu launches via `system()`.

---

## Building from Source (one-liner)

```bash
gcc -Wall -Wextra -O2 -lncurses -o menu menu.c
```

All games compile with zero warnings at `-Wall -Wextra -O2`.

---

## License

This project is open source. Feel free to play, modify, and share.
