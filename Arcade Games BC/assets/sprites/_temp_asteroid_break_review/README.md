# Asteroid Break Review Sprites

Temporary review assets for asteroid breakup animation.

Open `preview.html` in a browser to review the frame sequences.

Current review playback timing: `56ms` per frame.

Generated assets:

- `large_break_radial/`: 12 transparent PNG frames at `192x192`
- `large_break_radial_sheet.png`: 12-frame horizontal sprite sheet
- `medium_break_radial/`: 10 transparent PNG frames at `144x144`
- `medium_break_radial_sheet.png`: 10-frame horizontal sprite sheet
- `small_break_radial/`: 8 transparent PNG frames at `112x112`
- `small_break_radial_sheet.png`: 8-frame horizontal sprite sheet
- `manifest.json`: frame counts, canvas sizes, and duration notes

Regenerate after tweaks:

```zsh
python3 assets/sprites/_temp_asteroid_break_review/generate_break_sprites.py
```

Useful values to adjust in `generate_break_sprites.py`:

- `pieces`: number of shards
- `frames`: animation length
- `duration_seconds`: intended playback time in the manifest
- `speed`: radial spread range inside `make_piece`
- `fade`: disappearance timing inside `render_sequence`
