# Agent Usage Guide

## Recommended working pattern

Use the Main Project Orchestrator first.

Ask it to break the work into tasks.

Then assign each task to the correct specialist agent.

## Example prompt to start

Use the Main Project Orchestrator.

Create a MacBook Pro M2-first C++ SDL3 project plan for porting my Python/Pygame Asteroids game into a clean cross-platform C++ arcade engine.

Target macOS first, Windows second, iOS later.

Do not translate Python line-by-line. Use the Python version as the gameplay reference.

## Example prompt for porting

Use the Python to C++ Porting Analyst.

Review my Python/Pygame Asteroids project and extract the gameplay rules, object behaviours, speeds, scoring, collision logic and assets into a C++ SDL3 porting specification.

## Example prompt for coding

Use the C++ SDL3 Game Developer.

Create the initial C++ SDL3 project structure with CMake, a game loop, a window, input handling and placeholder player rendering.

## Example prompt for testing

Use the Code Tester and QA Agent.

Review the latest C++ build and create a test checklist comparing it against the Python reference game.
