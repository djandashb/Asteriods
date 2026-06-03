#!/usr/bin/env python3
import json
import math
import os
import random
import struct
import wave
import zlib

ROOT = os.path.dirname(__file__)
SOUND_DIR = os.path.abspath(os.path.join(ROOT, "..", "..", "sounds", "_temp_enemy_saucer_explosion_review"))
FRAME_MS = 56
SAMPLE_RATE = 44100


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
    output = []
    for px, py in points:
        sx = px * scale
        sy = py * scale
        output.append((x + sx * cos_r - sy * sin_r, y + sx * sin_r + sy * cos_r))
    return output


def make_saucer_shards():
    rng = random.Random(2600)
    shards = []

    # Main saucer hull pieces: long wedge/plate fragments from the original UFO silhouette.
    for index in range(12):
        angle = (math.tau * index / 12) + rng.uniform(-0.24, 0.24)
        width = rng.uniform(11.0, 20.0)
        height = rng.uniform(4.0, 8.0)
        points = [(-width, -height), (width * 0.65, -height * 0.65), (width, height * 0.35), (-width * 0.55, height)]
        shards.append({
            "points": points,
            "offset": (math.cos(angle) * rng.uniform(4.0, 15.0), math.sin(angle) * rng.uniform(3.0, 11.0)),
            "angle": angle,
            "speed": rng.uniform(78.0, 185.0),
            "spin": rng.uniform(-4.0, 4.0),
            "fill": rng.choice(((198, 232, 248), (164, 207, 228), (116, 162, 190), (80, 111, 143))),
        })

    # Smaller bright cockpit shards.
    for index in range(10):
        angle = (math.tau * index / 10) + rng.uniform(-0.4, 0.4)
        radius = rng.uniform(3.0, 7.0)
        points = []
        for side in range(rng.randint(3, 5)):
            a = math.tau * side / 4 + rng.uniform(-0.35, 0.35)
            r = radius * rng.uniform(0.6, 1.15)
            points.append((math.cos(a) * r, math.sin(a) * r))
        shards.append({
            "points": points,
            "offset": (math.cos(angle) * rng.uniform(0.0, 10.0), math.sin(angle) * rng.uniform(0.0, 8.0)),
            "angle": angle,
            "speed": rng.uniform(95.0, 220.0),
            "spin": rng.uniform(-6.0, 6.0),
            "fill": rng.choice(((224, 247, 255), (181, 224, 244), (104, 164, 202))),
        })

    return shards


def draw_flash(px, width, height, progress):
    alpha = int(210 * max(0.0, 1.0 - progress * 3.4))
    if alpha <= 0:
        return
    cx = width / 2
    cy = height / 2
    radius = 18.0 + progress * 46.0
    for spoke in range(10):
        angle = math.tau * spoke / 10
        end = (cx + math.cos(angle) * radius, cy + math.sin(angle) * radius)
        draw_line(px, width, height, (cx, cy), end, (220, 245, 255, alpha))


def render_explosion():
    name = "enemy_saucer_explosion"
    canvas = 160
    frames = 12
    shards = make_saucer_shards()
    sequence_dir = os.path.join(ROOT, name)
    os.makedirs(sequence_dir, exist_ok=True)
    sheet_px = bytearray(canvas * frames * canvas * 4)
    manifest_frames = []

    for frame in range(frames):
        progress = frame / (frames - 1)
        ease = 1.0 - (1.0 - progress) * (1.0 - progress)
        fade = max(0.0, 1.0 - progress * 0.96)
        px = bytearray(canvas * canvas * 4)
        draw_flash(px, canvas, canvas, progress)

        for shard in shards:
            drift = shard["speed"] * ease
            x = (canvas / 2) + shard["offset"][0] + math.cos(shard["angle"]) * drift
            y = (canvas / 2) + shard["offset"][1] + math.sin(shard["angle"]) * drift
            rotation = shard["spin"] * progress
            scale = 1.0 - progress * 0.22
            alpha = int(238 * fade)
            fill = (*shard["fill"], alpha)
            outline = (230, 248, 255, int(185 * fade))
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
    }


def square(phase, duty=0.5):
    return 1.0 if (phase % 1.0) < duty else -1.0


def generate_sound():
    os.makedirs(SOUND_DIR, exist_ok=True)
    rng = random.Random(500)
    duration = 0.82
    sample_count = int(SAMPLE_RATE * duration)
    frames = bytearray()
    phase_a = 0.0
    phase_b = 0.0
    previous = 0.0

    for index in range(sample_count):
        t = index / SAMPLE_RATE
        p = t / duration
        burst = math.exp(-p * 7.2)
        tail = math.exp(-p * 3.0)
        freq_a = 620.0 - (p * 360.0)
        freq_b = 930.0 - (p * 520.0)
        phase_a += freq_a / SAMPLE_RATE
        phase_b += freq_b / SAMPLE_RATE
        metallic = (square(phase_a, 0.43) * 0.34 + square(phase_b, 0.28) * 0.24) * tail
        noise = rng.uniform(-1.0, 1.0) * burst * 0.62
        crack = rng.uniform(-1.0, 1.0) * (1.0 if p < 0.11 else 0.0) * 0.74
        sample = (metallic + noise + crack) * 0.72
        high_pass = sample - previous * 0.62
        previous = sample
        sample = max(-1.0, min(1.0, high_pass))
        value = int(sample * 32767)
        frames.extend(struct.pack("<hh", value, int(value * 0.94)))

    path = os.path.join(SOUND_DIR, "enemy_saucer_destroyed.wav")
    with wave.open(path, "wb") as wav:
        wav.setnchannels(2)
        wav.setsampwidth(2)
        wav.setframerate(SAMPLE_RATE)
        wav.writeframes(frames)
    return path


def main():
    animation = render_explosion()
    sound_path = generate_sound()
    with open(os.path.join(ROOT, "manifest.json"), "w", encoding="utf-8") as handle:
        json.dump({
            "animations": [animation],
            "sounds": [{
                "name": "enemy_saucer_destroyed",
                "path": os.path.relpath(sound_path, ROOT),
                "format": "44100Hz 16-bit stereo PCM WAV",
                "notes": "Short metallic burst plus noisy arcade crack for enemy saucer destruction.",
            }],
        }, handle, indent=2)


if __name__ == "__main__":
    main()
