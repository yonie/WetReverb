# WetReverb

A VST3 reverb plugin inspired by 80s digital reverb units like the Yamaha R1000, Electro-Harmonix Holy Grail, and Roland DEP-5.

## Features

- **5 Reverb Modes**: Room, Plate, Hall, Cathedral, Cosmos
- **Authentic 80s Digital Character**: 
  - 24kHz internal sample rate (lo-fi warmth)
  - 12-bit quantization with TPDF dither
  - HF damping and noise floor (-80dBFS)
  - Stereo crosstalk for authentic analog bleed
- **Schroeder-style Algorithm**: Parallel comb filters + series allpass diffusion
- **Input/Output Meters**: 9-segment LED-style display (green/yellow/red zones)
- **Mode Switching**: Smooth transitions with buffer clearing on preset change

## Building

### Prerequisites

- CMake 3.15+
- C++17 compiler
- VST3 SDK (symlinked from WetDelay/vst3sdk)

### Build Commands

```bash
./build.sh    # Build the plugin
./install.sh  # Install to ~/.vst3/
```

## Technical Details

### Audio Processing Chain

1. Anti-alias filter (10kHz LPF at host rate)
2. Downsample to 24kHz (linear interpolation)
3. Pre-delay (mode-dependent)
4. Parallel comb filters with internal damping (4-5 combs)
5. Series allpass filters for diffusion (2 allpasses)
6. Output low-pass filter (6kHz)
7. High-pass filter (80Hz)
8. 12-bit quantization with TPDF dither
9. Noise floor injection (-80dBFS)
10. Upsample to host rate
11. Reconstruction filter (10kHz LPF)

### Reverb Modes

| Mode | RT60 | Comb Filters | Pre-delay | Feedback | Character |
|------|------|--------------|-----------|----------|-----------|
| Room | 0.6s | 4 | 0ms | 0.62 | Tight, grainy, dark |
| Plate | 1.6s | 4 | 0ms | 0.78 | Dense, metallic |
| Hall | 2.2s | 5 | 20ms | 0.84 | Smooth, warm |
| Cathedral | 3.0s | 5 | 30ms | 0.88 | Lush, long |
| Cosmos | 4.5s | 5 | 40ms | 0.91 | Ethereal, infinite |

## License

Copyright 2026 Yonie

## Related

- [WetDelay](https://github.com/yonie/WetDelay) - The delay plugin this was based on