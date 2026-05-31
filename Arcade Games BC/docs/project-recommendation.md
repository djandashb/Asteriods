# Recommendation for MacBook Pro M2 Development

## Best stack

Use the MacBook Pro M2 as the main development machine.

Recommended stack:

- VS Code
- OpenAI Codex
- C++20 / C++23
- SDL3
- CMake
- GitHub
- Xcode
- iPhone Simulator
- Later: Apple Developer Program

## Why SDL3

SDL3 is a better long-term choice than Pygame for performance and portability.

It allows the same C++ game code to target:

- macOS
- Windows
- iOS
- iPadOS
- Linux

## Development path

1. Freeze the existing Python Asteroids game as the reference version.
2. Document the rules, controls, scoring, speeds and behaviours.
3. Create a new C++ SDL3 project.
4. Port the game behaviour, not the Python code line-by-line.
5. Keep artwork and useful design notes.
6. Add controller support.
7. Add touch controls later for iPhone/iPad.
8. Package first for macOS and Windows.
9. Publish to Apple only when the collection is mature.

## Commercial direction

Avoid exact clones.

Create games inspired by classic arcade titles but with:

- Original names
- Original artwork
- Original sounds
- Different branding
- Enhanced gameplay
- Optional CRT effects
- Modern menus
- Controller support
