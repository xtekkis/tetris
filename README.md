# Tetris

A terminal-based Tetris game built with C++ and PDCurses.

## Preview

### Title Screen
![Title Screen](Images/titlescreen.png)

### Gameplay
![Gameplay](Images/gameplay.png)

## Features

- All 7 classic Tetris pieces, each in its own color
- Ghost piece showing where the current piece will land
- Hard drop with the space bar
- Pause and resume
- Next piece preview
- Score, lines and level tracking
- Multi-line clear scoring bonus
- Game over detection with the option to play again

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
| P | Pause / resume |
| R | Play again (after game over) |
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
docker run -it --rm --network none --cap-drop ALL --security-opt no-new-privileges --read-only tetris
```

Your terminal window must be at least **54 columns x 22 rows**. The game tells you and exits if it is smaller.

#### What the run options do

The first two are needed to play. The rest lock the container down: the game only needs a keyboard and a screen, so everything else is switched off.

| Option | What it does |
|---|---|
| `-it` | Connects your keyboard and terminal, so the game can read keys and draw |
| `--rm` | Deletes the container when you quit |
| `--network none` | No network at all, so the game cannot connect anywhere |
| `--cap-drop ALL` | Drops every Linux privilege, none of which the game uses |
| `--security-opt no-new-privileges` | Stops any program inside from gaining more privileges |
| `--read-only` | Makes the whole filesystem read-only, so nothing inside can be changed |

The game runs fine with all of them. `docker run -it --rm tetris` also works if you want the short version.

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