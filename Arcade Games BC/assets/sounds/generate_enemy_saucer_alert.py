#!/usr/bin/env python3
import math
import random
import struct
import wave

SAMPLE_RATE = 44100
DURATION_SECONDS = 3.2
OUTPUT = "enemy_saucer_alert.wav"


def clamp(value, low=-1.0, high=1.0):
    return max(low, min(high, value))


def square(phase, duty=0.5):
    return 1.0 if (phase % 1.0) < duty else -1.0


def main():
    rng = random.Random(1979)
    sample_count = int(SAMPLE_RATE * DURATION_SECONDS)
    phase_a = 0.0
    phase_b = 0.0
    frames = bytearray()

    for index in range(sample_count):
        t = index / SAMPLE_RATE
        segment = int(t * 6.0) % 2
        segment_progress = (t * 6.0) % 1.0

        base_a = 242.0 if segment == 0 else 332.0
        base_b = 306.0 if segment == 0 else 434.0

        glide = math.sin(segment_progress * math.pi) * 18.0
        wobble = math.sin(t * math.tau * 6.7) * 8.0
        drift = math.sin(t * math.tau * 0.62) * 6.0

        freq_a = base_a + glide + wobble + drift
        freq_b = base_b + (glide * 0.75) - wobble + (drift * 0.6)
        phase_a += freq_a / SAMPLE_RATE
        phase_b += freq_b / SAMPLE_RATE

        tone_a = square(phase_a, 0.47)
        tone_b = square(phase_b, 0.36) * 0.42
        ring_mod = 0.72 + (0.28 * square(t * 10.4, 0.5))
        tremolo = 0.68 + (0.32 * math.sin(t * math.tau * 13.0))
        noise = rng.uniform(-0.055, 0.055)

        envelope = 0.86 + (0.14 * math.sin(t * math.tau * 1.5))
        sample = ((tone_a + tone_b) * 0.33 * ring_mod * tremolo * envelope) + noise

        # Small high-pass style transient bite, keeping the sound arcade-sharp.
        sample += 0.055 * math.sin(t * math.tau * (freq_b * 2.0))
        sample = clamp(sample * 0.72)

        left = int(clamp(sample) * 32767)
        right = int(clamp(sample * 0.96) * 32767)
        frames.extend(struct.pack("<hh", left, right))

    with wave.open(OUTPUT, "wb") as wav:
        wav.setnchannels(2)
        wav.setsampwidth(2)
        wav.setframerate(SAMPLE_RATE)
        wav.writeframes(frames)


if __name__ == "__main__":
    main()
