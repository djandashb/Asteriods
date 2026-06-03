#!/usr/bin/env python3
import math
import os
import random
import struct
import wave

SAMPLE_RATE = 44100
DURATION_SECONDS = 1.15
OUTPUT = "player_ship_destroyed.wav"


def clamp(value, low=-1.0, high=1.0):
    return max(low, min(high, value))


def square(phase, duty=0.5):
    return 1.0 if (phase % 1.0) < duty else -1.0


def main():
    rng = random.Random(1979)
    sample_count = int(SAMPLE_RATE * DURATION_SECONDS)
    frames = bytearray()
    phase_low = 0.0
    phase_mid = 0.0
    previous_noise = 0.0

    for index in range(sample_count):
        t = index / SAMPLE_RATE
        p = t / DURATION_SECONDS

        attack = min(1.0, p * 42.0)
        body = math.exp(-p * 3.25)
        tail = math.exp(-p * 5.8)
        crack_gate = 1.0 if p < 0.16 else 0.0

        low_freq = 190.0 - (p * 112.0)
        mid_freq = 480.0 - (p * 260.0)
        wobble = math.sin(t * math.tau * 19.0) * 11.0 * body

        phase_low += max(42.0, low_freq + wobble) / SAMPLE_RATE
        phase_mid += max(95.0, mid_freq - wobble) / SAMPLE_RATE

        raw_noise = rng.uniform(-1.0, 1.0)
        filtered_noise = (raw_noise * 0.64) + (previous_noise * 0.36)
        previous_noise = filtered_noise

        noise_burst = filtered_noise * attack * body * 0.72
        crack = rng.uniform(-1.0, 1.0) * crack_gate * (1.0 - (p / 0.16 if p < 0.16 else 1.0)) * 0.66
        low_rumble = square(phase_low, 0.46) * body * 0.28
        falling_edge = square(phase_mid, 0.31) * tail * 0.18
        spark = math.sin(t * math.tau * (1160.0 - p * 760.0)) * tail * 0.075

        sample = (noise_burst + crack + low_rumble + falling_edge + spark) * 0.78
        sample = clamp(sample)

        left = int(sample * 32767)
        right = int(clamp((sample * 0.94) + (filtered_noise * body * 0.035)) * 32767)
        frames.extend(struct.pack("<hh", left, right))

    with wave.open(os.path.join(os.path.dirname(__file__), OUTPUT), "wb") as wav:
        wav.setnchannels(2)
        wav.setsampwidth(2)
        wav.setframerate(SAMPLE_RATE)
        wav.writeframes(frames)


if __name__ == "__main__":
    main()
