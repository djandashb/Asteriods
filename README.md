# Classic Arcade Game Agent

This project is a local Windows-friendly Python agent for building and testing classic 1970s/1980s arcade-style games.

## What this includes

- `game_agent/cli.py`: an interactive local agent launcher that helps you choose and start a classic game template.
- `games/space_invaders.py`: a playable Space Invaders-style clone.
- `games/asteroids.py`: a playable Asteroids-style clone.
- `games/lunar_lander.py`: a playable Lunar Lander-style clone.
- `games/centipede.py`: a playable Centipede-style clone.

## Requirements

- Python 3.8+ on Windows 10
- `pygame`

## Setup

1. Open a terminal in `h:\AI_Codex_Projects\Windows_Games`
2. Install dependencies:

```powershell
python -m pip install -r requirements.txt
```

## Run the agent

```powershell
python game_agent/cli.py
```

The agent will ask which game you want to launch and guide you locally.

## Run a game directly

```powershell
python games/space_invaders.py
python games/asteroids.py
python games/lunar_lander.py
python games/centipede.py
```

## Next steps

- Use this project as a base to expand the games toward the original arcade feel.
- Add improved sound, enemy patterns, better controls, and scoring to match the original titles.
- Commit and push the folder to your GitHub repository once you're ready.
