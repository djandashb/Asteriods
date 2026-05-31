# Retro Arcade Collection - MacBook Pro M2 Project

## Primary development machine

- MacBook Pro M2
- VS Code
- OpenAI Codex
- C++20 / C++23
- SDL3
- CMake
- Git
- Xcode for macOS/iOS build targets

## Target platforms

- macOS first
- Windows 10/11 second
- iPhone/iPad later
- Steam / itch.io first for commercial testing
- Apple App Store only when the games are polished

## Strategy

Do not start by paying for the Apple Developer Program.

First build:
1. A reusable C++ SDL3 arcade engine.
2. An Asteroids-style game based on the existing Python/Pygame version.
3. Additional classic-inspired arcade games.
4. A polished retro arcade collection.

Only pay for Apple Developer Program when ready for TestFlight/App Store publishing.

## Core rule

Build one shared C++ game core. Keep platform-specific code thin.

## Current build

The first SDL3/C++ Asteroids-style milestone now lives in:

- `src/Game.h`
- `src/Game.cpp`
- `src/main.cpp`
- `assets/sprites`

It ports the existing Python/Pygame Asteroids behaviour into a clean C++ structure rather than translating the Python line by line.

Current window size: `1024x768`.

## macOS setup

Install the local toolchain pieces:

```zsh
brew install cmake sdl3 sdl3_image
```

Build:

```zsh
cmake -S . -B build
cmake --build build
```

Run:

```zsh
./build/retro_arcade
```

Controls:

- `Return`: start/restart
- `F`: fire
- Arrow keys or `WASD`: move
- `P`: pause/resume
- `Esc`: quit
