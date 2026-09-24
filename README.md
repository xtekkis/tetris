# Tetris

A terminal-based Tetris game built with C++ and curses: PDCurses on Windows, ncurses in Docker.

## Preview

### Title Screen
![Title Screen](Images/titlescreen.png)

### Gameplay
![Gameplay](Images/gameplay.png)

## Features

- All 7 classic Tetris pieces, each in its own color
- Pieces dealt in shuffled sets of 7, so you never wait long for the one you need
- Ghost piece showing where the current piece will land
- Hard drop with the space bar
- Hold a piece to use later
- Short pause when a piece lands, so you can still slide it into place
- Pause and resume
- Next piece preview
- Score, lines and level tracking
- Choose a starting level from 1 to 10 on the title screen
- Completed rows flash before they disappear
- Multi-line clear scoring bonus
- Best score saved between games
- Game over detection with the option to play again

## Scoring

| Action | Points |
|---|---|
| 1 line | 100 x level |
| 2 lines | 300 x level |
| 3 lines | 500 x level |
| 4 lines | 800 x level |
| Soft drop (S) | 1 per row |
| Hard drop (Space) | 2 per row |

## Controls

| Key | Action |
|---|---|
| A or ← | Move left |
| D or → | Move right |
| S or ↓ | Move down |
| W or ↑ | Rotate |
| Space | Hard drop |
| C | Hold piece |
| P | Pause / resume |
| R | Play again (after game over) |
| Enter | Start the game (title screen) |
| ESC | Quit |

Controls work with Caps Lock on.

On the title screen, A and D choose the starting level, from 1 to 10. Higher levels drop pieces faster and score more per line.

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

Your best score is saved in `highscore.txt`, in the folder the game runs from.

### Option 2: Docker

You don't need a compiler, only [Docker Desktop](https://www.docker.com/products/docker-desktop/) (it must be running).

```bash
docker compose run --rm tetris
```

That one command builds the image the first time, applies all the security options below, and keeps your best score between runs. The settings live in [compose.yaml](compose.yaml).

After changing the code, rebuild with `docker compose build`.

Your terminal window must be at least **54 columns x 23 rows**. The game tells you and exits if it is smaller.

If you resize the window while playing, the board re-centres itself. Make it too small and the game waits, showing the size it needs, until you make it bigger again.

#### Keeping your best score

Compose already does this for you. If you prefer plain `docker run`, the container is deleted when you quit and the best score goes with it, so give Docker a storage volume for the game's folder:

```bash
docker run -it --rm --network none --cap-drop ALL --security-opt no-new-privileges --read-only -v tetris-scores:/home/player tetris
```

`-v tetris-scores:/home/player` stores `highscore.txt` in a Docker volume named `tetris-scores`, which survives between runs. Without it the game still plays, it just cannot save.

The volume is managed by Docker rather than being a folder you can browse: `docker volume ls` lists it and `docker volume rm tetris-scores` deletes it. To keep the file somewhere visible instead, mount a folder: `-v "$(pwd)/scores:/home/player"`.

#### What the run options do

These are what compose applies for you, and what to type if you run `docker run` by hand. The first two are needed to play. The rest lock the container down: the game only needs a keyboard and a screen, so everything else is switched off.

| Option | What it does |
|---|---|
| `-it` | Connects your keyboard and terminal, so the game can read keys and draw |
| `--rm` | Deletes the container when you quit |
| `--network none` | No network at all, so the game cannot connect anywhere |
| `--cap-drop ALL` | Drops every Linux privilege, none of which the game uses |
| `--security-opt no-new-privileges` | Stops any program inside from gaining more privileges |
| `--read-only` | Makes the whole filesystem read-only, so nothing inside can be changed |

The game runs fine with all of them. `docker run -it --rm tetris` also works if you want the short version, and `docker build -t tetris .` builds the image without compose.

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