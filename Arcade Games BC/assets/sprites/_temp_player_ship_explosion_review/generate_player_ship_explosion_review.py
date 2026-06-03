#!/usr/bin/env python3
import json
import math
import os
import random
import struct
import zlib

ROOT = os.path.dirname(__file__)
FRAME_MS = 70


def write_png(path, width, height, pixels):
    def chunk(kind, data):
        body = kind + data
        return struct.pack(">I", len(data)) + body + struct.pack(">I", zlib.crc32(body) & 0xFFFFFFFF)

    raw = bytearray()
    stride = width * 4
    for y in range(height):
        raw.append(0)
        raw.extend(pixels[y * stride:(y + 1) * stride])

    data = bytearray()
    data.extend(b"\x89PNG\r\n\x1a\n")
    data.extend(chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)))
    data.extend(chunk(b"IDAT", zlib.compress(bytes(raw), 9)))
    data.extend(chunk(b"IEND", b""))
    with open(path, "wb") as handle:
        handle.write(data)


def alpha_blend(px, index, color):
    sr, sg, sb, sa = color
    if sa <= 0:
        return
    dr, dg, db, da = px[index:index + 4]
    out_a = sa + (da * (255 - sa) // 255)
    if out_a == 0:
        px[index:index + 4] = b"\x00\x00\x00\x00"
        return
    out_r = (sr * sa + dr * da * (255 - sa) // 255) // out_a
    out_g = (sg * sa + dg * da * (255 - sa) // 255) // out_a
    out_b = (sb * sa + db * da * (255 - sa) // 255) // out_a
    px[index:index + 4] = bytes((
        max(0, min(255, out_r)),
        max(0, min(255, out_g)),
        max(0, min(255, out_b)),
        max(0, min(255, out_a)),
    ))


def point_in_poly(x, y, points):
    inside = False
    j = len(points) - 1
    for i, current in enumerate(points):
        xi, yi = current
        xj, yj = points[j]
        if ((yi > y) != (yj > y)) and (x < ((xj - xi) * (y - yi) / ((yj - yi) or 0.0001)) + xi):
            inside = not inside
        j = i
    return inside


def draw_line(px, width, height, start, end, color):
    x0, y0 = start
    x1, y1 = end
    steps = max(1, int(max(abs(x1 - x0), abs(y1 - y0))))
    for step in range(steps + 1):
        t = step / steps
        x = int(round(x0 + (x1 - x0) * t))
        y = int(round(y0 + (y1 - y0) * t))
        if 0 <= x < width and 0 <= y < height:
            alpha_blend(px, (y * width + x) * 4, color)


def draw_poly(px, width, height, points, fill, outline):
    min_x = max(0, int(math.floor(min(p[0] for p in points))) - 1)
    max_x = min(width - 1, int(math.ceil(max(p[0] for p in points))) + 1)
    min_y = max(0, int(math.floor(min(p[1] for p in points))) - 1)
    max_y = min(height - 1, int(math.ceil(max(p[1] for p in points))) + 1)
    for y in range(min_y, max_y + 1):
        for x in range(min_x, max_x + 1):
            if point_in_poly(x + 0.5, y + 0.5, points):
                alpha_blend(px, (y * width + x) * 4, fill)

    for start, end in zip(points, points[1:] + points[:1]):
        draw_line(px, width, height, start, end, outline)


def transform(points, x, y, rotation, scale):
    cos_r = math.cos(rotation)
    sin_r = math.sin(rotation)
    transformed = []
    for px, py in points:
        sx = px * scale
        sy = py * scale
        transformed.append((x + sx * cos_r - sy * sin_r, y + sx * sin_r + sy * cos_r))
    return transformed


def make_ship_shards():
    rng = random.Random(1024)
    shards = []

    # Triangular hull pieces, biased to read like the player's arrowhead ship.
    hull_shapes = [
        [(-26, -4), (10, -15), (4, -4), (12, 5)],
        [(-26, 4), (12, -5), (4, 4), (10, 15)],
        [(-5, -16), (24, 0), (-2, -5)],
        [(-5, 16), (24, 0), (-2, 5)],
        [(-20, -3), (-8, -9), (-8, 9), (-20, 3)],
        [(-3, -5), (11, 0), (-3, 5)],
    ]

    for index, shape in enumerate(hull_shapes):
        center_x = sum(p[0] for p in shape) / len(shape)
        center_y = sum(p[1] for p in shape) / len(shape)
        angle = math.atan2(center_y, center_x) + rng.uniform(-0.35, 0.35)
        shards.append({
            "points": [(x - center_x, y - center_y) for x, y in shape],
            "offset": (center_x, center_y),
            "angle": angle,
            "speed": rng.uniform(420.0, 760.0),
            "spin": rng.uniform(-4.2, 4.2),
            "fill": rng.choice(((210, 238, 249), (174, 216, 236), (126, 174, 205), (90, 129, 166))),
        })

    for index in range(11):
        angle = (math.tau * index / 11) + rng.uniform(-0.32, 0.32)
        radius = rng.uniform(3.0, 7.0)
        points = []
        sides = rng.randint(3, 5)
        for side in range(sides):
            a = math.tau * side / sides + rng.uniform(-0.36, 0.36)
            r = radius * rng.uniform(0.65, 1.2)
            points.append((math.cos(a) * r, math.sin(a) * r))
        shards.append({
            "points": points,
            "offset": (math.cos(angle) * rng.uniform(0.0, 16.0), math.sin(angle) * rng.uniform(0.0, 12.0)),
            "angle": angle,
            "speed": rng.uniform(500.0, 920.0),
            "spin": rng.uniform(-6.2, 6.2),
            "fill": rng.choice(((232, 249, 255), (183, 225, 245), (112, 178, 216))),
        })

    return shards


def draw_flash(px, width, height, progress):
    alpha = int(220 * max(0.0, 1.0 - progress * 3.2))
    if alpha <= 0:
        return
    cx = width / 2
    cy = height / 2
    radius = 16.0 + progress * 40.0
    for spoke in range(8):
        angle = math.tau * spoke / 8
        draw_line(px, width, height, (cx, cy), (cx + math.cos(angle) * radius, cy + math.sin(angle) * radius), (225, 248, 255, alpha))


def render_explosion():
    name = "player_ship_explosion"
    canvas = 768
    frames = 12
    shards = make_ship_shards()
    sequence_dir = os.path.join(ROOT, name)
    os.makedirs(sequence_dir, exist_ok=True)
    sheet_px = bytearray(canvas * frames * canvas * 4)
    manifest_frames = []

    for frame in range(frames):
        progress = frame / (frames - 1)
        ease = 1.0 - (1.0 - progress) * (1.0 - progress)
        fade = max(0.0, 1.0 - progress * 0.98)
        px = bytearray(canvas * canvas * 4)
        draw_flash(px, canvas, canvas, progress)

        for shard in shards:
            drift = shard["speed"] * ease
            x = (canvas / 2) + shard["offset"][0] + math.cos(shard["angle"]) * drift
            y = (canvas / 2) + shard["offset"][1] + math.sin(shard["angle"]) * drift
            rotation = shard["spin"] * progress
            scale = 1.0 - progress * 0.2
            alpha = int(238 * fade)
            draw_poly(
                px,
                canvas,
                canvas,
                transform(shard["points"], x, y, rotation, scale),
                (*shard["fill"], alpha),
                (232, 248, 255, int(185 * fade)),
            )

        frame_name = f"frame_{frame:02d}.png"
        write_png(os.path.join(sequence_dir, frame_name), canvas, canvas, px)
        manifest_frames.append(frame_name)

        for y in range(canvas):
            src_start = y * canvas * 4
            src_end = src_start + canvas * 4
            dst_start = (y * canvas * frames + frame * canvas) * 4
            sheet_px[dst_start:dst_start + canvas * 4] = px[src_start:src_end]

    write_png(os.path.join(ROOT, f"{name}_sheet.png"), canvas * frames, canvas, sheet_px)
    return {
        "name": name,
        "frames": manifest_frames,
        "sprite_sheet": f"{name}_sheet.png",
        "canvas": [canvas, canvas],
        "frame_count": frames,
        "frame_ms": FRAME_MS,
        "duration_seconds": round((frames * FRAME_MS) / 1000, 3),
        "notes": "Transparent PNG frames. Player ship hull shards drift outward and fade.",
    }


def main():
    animation = render_explosion()
    with open(os.path.join(ROOT, "manifest.json"), "w", encoding="utf-8") as handle:
        json.dump({"animations": [animation]}, handle, indent=2)


if __name__ == "__main__":
    main()
