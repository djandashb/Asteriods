# Enemy Saucer Explosion Review

Temporary review assets for enemy saucer destruction.

Open `preview.html` in a browser to review the animation and sound.

Generated assets:

- `enemy_saucer_explosion/`: 12 transparent PNG frames at `160x160`
- `enemy_saucer_explosion_sheet.png`: 12-frame horizontal sprite sheet
- `../../sounds/_temp_enemy_saucer_explosion_review/enemy_saucer_destroyed.wav`: review WAV
- `manifest.json`: frame count, timing, and sound notes

Regenerate after tweaks:

```zsh
python3 assets/sprites/_temp_enemy_saucer_explosion_review/generate_saucer_explosion_review.py
```

Useful values to adjust in `generate_saucer_explosion_review.py`:

- `FRAME_MS`: playback timing
- shard counts in `make_saucer_shards`
- shard `speed` ranges
- sound `duration`, oscillator frequencies, and noise balance in `generate_sound`
