# ---------- Stage 1: build ----------
# A full image with the C++ compiler and the ncurses headers.
# ncurses is the Linux equivalent of PDCurses and provides the same curses.h API,
# so main.cpp compiles unchanged.
FROM gcc:14 AS build

RUN apt-get update \
    && apt-get install -y --no-install-recommends libncurses-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY tetris/main.cpp .
RUN g++ -std=c++17 -O2 -Wall -o tetris main.cpp -lncurses

# ---------- Stage 2: runtime ----------
# A small image that only contains the compiled game and the ncurses runtime library.
# The compiler from stage 1 is left behind, which keeps the final image small.
FROM debian:bookworm-slim

RUN apt-get update \
    && apt-get install -y --no-install-recommends libncurses6 \
    && rm -rf /var/lib/apt/lists/*

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
