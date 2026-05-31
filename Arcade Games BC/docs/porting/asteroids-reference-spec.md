# Asteroids Reference Porting Spec

Source reference: `djandashb/Asteriods.git`, commit `d92318e`.

## Python reference behaviour

- Python reference window: 800 x 600.
- Current SDL3 window: 1024 x 768.
- Frame target: 60 FPS.
- Player starts at screen centre with five lives.
- Reference controls: left/right rotate, up thrust, space fire, `P` pause, `Esc` quit.
- Current SDL3 test controls: left/right rotate, up thrust, `F` fire, `P` pause, `Esc` quit. Space is still under investigation on the MacBook keyboard.
- Movement wraps around screen edges.
- Ship has a short invulnerability window after spawn.
- Bullets expire after roughly one second or when leaving the screen.
- Waves spawn large asteroids away from the player start position.
- Large asteroids split into two medium asteroids.
- Medium asteroids split into two small asteroids.
- Small asteroids are destroyed without splitting.
- Each asteroid hit scores 100 points.
- Clearing a wave scores 500 points and starts the next wave.

## First SDL3 milestone

Implemented now:

- C++20 SDL3 game loop.
- SDL3_image PNG loading.
- Player ship rotation, thrust, drag, firing and wrapping.
- Asteroid spawning, movement, rotation, wrapping and splitting.
- Bullet collision using circle tests.
- Score, lives, wave display and game-over/restart flow.
- Pause state.
- Imported reference sprites using portable lowercase names.

Deferred from the Python version:

- Difficulty selection.
- Shield uses.
- Enemy spaceship.
- Enemy bullets.
- Sound synthesis.
- Pixel-mask collision.
- Atari-style font loading.
- Controller and touch input.

## Asset note

The imported PNGs are useful for private learning and port validation. Before any commercial release, replace them with original or properly licensed artwork and branding.
