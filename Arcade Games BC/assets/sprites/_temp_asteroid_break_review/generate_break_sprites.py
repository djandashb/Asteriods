#!/usr/bin/env python3
import json
import math
import os
import random
import struct
import zlib

ROOT = os.path.dirname(__file__)
FRAME_MS = 56


def write_png(path, width, height, pixels):
    def chunk(kind, data):
        body = kind + data
        return struct.pack(">I", len(data)) + body + struct.pack(">I", zlib.crc32(body) & 0xFFFFFFFF)

    raw = bytearray()
    stride = width * 4
    for y in range(height):
        raw.append(0)
        row = pixels[y * stride:(y + 1) * stride]
        raw.extend(row)

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
    px[index:index + 4] = bytes((out_r, out_g, out_b, out_a))


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


def make_piece(rng, origin_radius, piece_radius, angle):
    distance = rng.uniform(origin_radius * 0.08, origin_radius * 0.34)
    center = (math.cos(angle) * distance, math.sin(angle) * distance)
    sides = rng.randint(4, 6)
    points = []
    for index in range(sides):
        a = angle + (math.tau * index / sides) + rng.uniform(-0.42, 0.42)
        r = piece_radius * rng.uniform(0.55, 1.12)
        points.append((center[0] + math.cos(a) * r, center[1] + math.sin(a) * r))
    speed = rng.uniform(origin_radius * 0.58, origin_radius * 1.45)
    spin = rng.uniform(-2.9, 2.9)
    return {
        "points": points,
        "angle": angle,
        "speed": speed,
        "spin": spin,
        "fill": rng.choice(((119, 132, 144), (98, 112, 125), (143, 151, 158), (82, 96, 112))),
    }


def transform(points, x, y, rotation, scale):
    cos_r = math.cos(rotation)
    sin_r = math.sin(rotation)
    output = []
    for px, py in points:
        sx = px * scale
        sy = py * scale
        output.append((x + sx * cos_r - sy * sin_r, y + sx * sin_r + sy * cos_r))
    return output


def render_sequence(name, radius, canvas, pieces, frames, seed):
    rng = random.Random(seed)
    shard_count = pieces
    shards = []
    for index in range(shard_count):
        angle = (math.tau * index / shard_count) + rng.uniform(-0.28, 0.28)
        piece_radius = radius * rng.uniform(0.10, 0.19)
        shards.append(make_piece(rng, radius, piece_radius, angle))

    sequence_dir = os.path.join(ROOT, name)
    os.makedirs(sequence_dir, exist_ok=True)
    sheet_px = bytearray(canvas * frames * canvas * 4)
    manifest_frames = []

    for frame in range(frames):
        progress = frame / (frames - 1)
        ease = 1.0 - (1.0 - progress) * (1.0 - progress)
        fade = max(0.0, 1.0 - (progress * 0.92))
        px = bytearray(canvas * canvas * 4)

        for shard in shards:
            drift = shard["speed"] * ease
            x = (canvas / 2) + math.cos(shard["angle"]) * drift
            y = (canvas / 2) + math.sin(shard["angle"]) * drift
            rotation = shard["spin"] * progress
            scale = 1.0 - progress * 0.18
            alpha = int(235 * fade)
            fill = (*shard["fill"], alpha)
            outline = (190, 222, 246, int(170 * fade))
            draw_poly(px, canvas, canvas, transform(shard["points"], x, y, rotation, scale), fill, outline)

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
        "notes": "Transparent PNG frames. Shards drift radially outward and fade out.",
    }


def main():
    animations = [
        render_sequence("large_break_radial", radius=62, canvas=192, pieces=18, frames=12, seed=101),
        render_sequence("medium_break_radial", radius=39, canvas=144, pieces=12, frames=10, seed=202),
        render_sequence("small_break_radial", radius=28, canvas=112, pieces=8, frames=8, seed=303),
    ]
    with open(os.path.join(ROOT, "manifest.json"), "w", encoding="utf-8") as handle:
        json.dump({"animations": animations}, handle, indent=2)


if __name__ == "__main__":
    main()
