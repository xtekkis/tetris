# ---------- Stage 1: build ----------
# A full image with the C++ compiler and the ncurses headers.
# ncurses is the Linux equivalent of PDCurses and provides the same curses.h API,
# so main.cpp compiles unchanged.
# The -bookworm tag matters: it must match the runtime image below, otherwise the
# compiled game needs a newer C library than the runtime image has and will not start.
FROM gcc:14-bookworm AS build

RUN apt-get update \
    && apt-get install -y --no-install-recommends libncurses-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY tetris/*.cpp tetris/*.h ./
RUN g++ -std=c++17 -O2 -Wall -o tetris *.cpp -lncurses

# ---------- Stage 2: runtime ----------
# A small image that only contains the compiled game and the ncurses runtime library.
# The compiler from stage 1 is left behind, which keeps the final image small.
FROM debian:bookworm-slim

RUN apt-get update \
    && apt-get install -y --no-install-recommends libncurses6 \
    && rm -rf /var/lib/apt/lists/*

# Remove the setuid and setgid bits from programs like su, passwd and mount.
# The game never switches users, so nothing here needs extra privileges.
RUN find / -xdev -perm /6000 -type f -exec chmod a-s {} +

# Run as a non-root user
RUN useradd --create-home player
USER player
WORKDIR /home/player

COPY --from=build /src/tetris /usr/local/bin/tetris

# ncurses waits 1 second after ESC by default to see if more keys follow;
# shorten that so ESC quits immediately.
ENV ESCDELAY=25
ENV TERM=xterm-256color

ENTRYPOINT ["tetris"]
