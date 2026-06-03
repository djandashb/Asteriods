#!/usr/bin/env python3
import math
import os
import random
import struct
import wave

SAMPLE_RATE = 44100
OUTPUT = "wave_clear_fanfare.wav"
PEAK_VOLUME = 0.35

NOTES = [
    (0.00, 523.25, 0.120, 0.080),
    (0.13, 659.25, 0.120, 0.080),
    (0.26, 783.99, 0.120, 0.090),
    (0.39, 1046.50, 0.160, 0.120),
    (0.58, 783.99, 0.120, 0.100),
    (0.72, 1046.50, 0.350, 0.250),
]


def cents_to_ratio(cents):
    return 2.0 ** (cents / 1200.0)


def square(phase):
    return 1.0 if math.sin(phase) >= 0.0 else -1.0


def triangle(phase):
    return (2.0 / math.pi) * math.asin(math.sin(phase))


def envelope(t, duration, release):
    attack = 0.003
    decay = 0.025
    sustain = 0.65
    if t < 0.0:
        return 0.0
    if t < attack:
        return t / attack
    if t < attack + decay:
        amount = (t - attack) / decay
        return 1.0 - ((1.0 - sustain) * amount)
    if t < duration:
        return sustain
    if t < duration + release:
        amount = (t - duration) / release
        return sustain * (1.0 - amount)
    return 0.0


def synth_sample(t):
    value = 0.0
    for start, frequency, duration, release in NOTES:
        local_t = t - start
        amp = envelope(local_t, duration, release)
        if amp <= 0.0:
            continue

        vibrato_cents = 0.0
        if frequency > 1000.0 and start > 0.70:
            vibrato_cents = math.sin(2.0 * math.pi * 6.0 * local_t) * 8.0

        freq = frequency * cents_to_ratio(vibrato_cents)
        phase = 2.0 * math.pi * freq * local_t
        tri_phase = 2.0 * math.pi * (freq * cents_to_ratio(-4.0)) * local_t
        tone = (square(phase) * 0.85) + (triangle(tri_phase) * 0.15)

        if 0.0 <= local_t < 0.008:
            tone += random.uniform(-1.0, 1.0) * 0.12 * (1.0 - (local_t / 0.008))

        value += tone * amp

    return max(-1.0, min(1.0, value * PEAK_VOLUME))


def main():
    random.seed(7)
    duration = 1.25
    sample_count = int(SAMPLE_RATE * duration)
    path = os.path.join(os.path.dirname(__file__), OUTPUT)
    with wave.open(path, "wb") as wav:
        wav.setnchannels(2)
        wav.setsampwidth(2)
        wav.setframerate(SAMPLE_RATE)
        for i in range(sample_count):
            t = i / SAMPLE_RATE
            sample = int(synth_sample(t) * 32767)
            packed = struct.pack("<hh", sample, sample)
            wav.writeframesraw(packed)


if __name__ == "__main__":
    main()
