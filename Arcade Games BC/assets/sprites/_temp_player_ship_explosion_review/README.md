# Player Ship Explosion Review

Temporary review assets for player ship death animation.

Open `preview.html` in a browser to review the animation.

Generated assets:

- `player_ship_explosion/`: 12 transparent PNG frames at `768x768`
- `player_ship_explosion_sheet.png`: 12-frame horizontal sprite sheet
- `manifest.json`: frame count, timing, and notes

Regenerate after tweaks:

```zsh
python3 assets/sprites/_temp_player_ship_explosion_review/generate_player_ship_explosion_review.py
```

Useful values to adjust in `generate_player_ship_explosion_review.py`:

- `FRAME_MS`: playback timing
- `hull_shapes`: main player-ship shard shapes
- shard `speed` ranges
- flash spoke count and radius in `draw_flash`
