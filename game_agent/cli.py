import os
import subprocess
import sys

GAMES = {
    "1": ("Space Invaders", "../games/space_invaders.py"),
    "2": ("Asteroids", "../games/asteroids.py"),
    "3": ("Lunar Lander", "../games/lunar_lander.py"),
    "4": ("Centipede", "../games/centipede.py"),
}


def check_pygame():
    try:
        import pygame  # noqa: F401
        return True
    except ImportError:
        return False


def run_game(script_path):
    script = os.path.normpath(os.path.join(os.path.dirname(__file__), script_path))
    if not os.path.exists(script):
        print(f"Game script not found: {script}")
        return

    print(f"Launching game: {script}")
    subprocess.run([sys.executable, script], check=False)


def main():
    print("Classic Arcade Game Agent")
    print("This local agent helps you launch and test classic arcade templates.")
    print()

    installed = check_pygame()
    if not installed:
        print("pygame is not installed.")
        print("Install it with: python -m pip install -r requirements.txt")
        print()

    for key, (name, _) in GAMES.items():
        print(f"{key}. {name}")

    choice = input("Choose a game to launch (1-4): ").strip()
    if choice not in GAMES:
        print("Invalid selection. Please try again.")
        return

    if not installed:
        answer = input("Install pygame now? [Y/n]: ").strip().lower()
        if answer in ["", "y", "yes"]:
            subprocess.run([sys.executable, "-m", "pip", "install", "-r", os.path.join(os.path.dirname(__file__), "..", "requirements.txt")], check=False)
            installed = check_pygame()

    if installed:
        _, script = GAMES[choice]
        run_game(script)
    else:
        print("Cannot launch game until pygame is installed.")


if __name__ == "__main__":
    main()
