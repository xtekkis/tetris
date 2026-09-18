# Tetris

A terminal-based Tetris game built with C++ and PDCurses.

## Preview

### Title Screen
![Title Screen](Images/titlescreen.png)

### Gameplay
![Gameplay](Images/gameplay.png)

## Features

- All 7 classic Tetris pieces
- Ghost piece showing where the current piece will land
- Hard drop with the space bar
- Next piece preview
- Score, lines and level tracking
- Multi-line clear scoring bonus
- Game over detection

## Scoring

| Lines Cleared | Points |
|---|---|
| 1 line | 100 x level |
| 2 lines | 300 x level |
| 3 lines | 500 x level |
| 4 lines | 800 x level |

## Controls

| Key | Action |
|---|---|
| A or ← | Move left |
| D or → | Move right |
| S or ↓ | Move down |
| W or ↑ | Rotate |
| Space | Hard drop |
| ESC | Quit |

Controls work with Caps Lock on.

## Built With

- C++
- PDCurses (Windows build)
- ncurses (Docker / Linux build)
- Docker

## How to Run

Clone the repository first:

```bash
git clone https://github.com/xtekkis/tetris.git
cd tetris
```

### Option 1: Visual Studio (Windows)

1. Open `tetris/tetris.sln` in Visual Studio 2022
2. Select the `x64` platform
3. Build and run with `Ctrl + F5`

### Option 2: Docker

You don't need a compiler, only [Docker Desktop](https://www.docker.com/products/docker-desktop/) (it must be running).

```bash
# Build the image (only needed once, or after changing the code)
docker build -t tetris .

# Play
docker run -it --rm tetris
```

- `-it` connects your keyboard and terminal to the container. The game needs this to read key presses and draw the screen.
- `--rm` deletes the container when you quit the game.

Your terminal window should be at least **60 columns x 24 rows**.

#### How the Docker build works

PDCurses only works on Windows and Docker containers run Linux, so the Docker build uses **ncurses** instead. Both libraries provide the same `curses.h` functions, so `main.cpp` compiles without changes.

The [Dockerfile](Dockerfile) builds the image in two stages:

1. **build**: starts from the `gcc` image, installs the ncurses headers and compiles `main.cpp`.
2. **runtime**: starts from a small `debian-slim` image, installs only the ncurses runtime library and copies in the compiled game. The compiler isn't included in this image, so the final image is much smaller.

The [.dockerignore](.dockerignore) file keeps files the build doesn't need (git history, images, Visual Studio files) out of the build.

Useful commands:

| Command | What it does |
|---|---|
| `docker images` | List the images you have built |
| `docker ps -a` | List containers (running and stopped) |
| `docker rmi tetris` | Delete the image |