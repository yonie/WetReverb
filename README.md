# WetReverb

A VST3 reverb plugin inspired by 80s digital reverb units like the Yamaha R1000, Electro-Harmonix Holy Grail, and Roland DEP-5.

## Features

- **5 Reverb Modes**: Room, Plate, Hall, Cathedral, Cosmos
- **Authentic 80s Digital Character**: 
  - 24kHz internal sample rate (lo-fi warmth)
  - 12-bit quantization with TPDF dither
  - HF damping and noise floor (-80dBFS)
- **Stereo Processing** with gentle crosstalk
- **Input/Output Meters**: 12-segment LED-style display

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
2. Downsample to 24kHz
3. Pre-delay (mode-dependent)
4. Parallel comb filters with damping (4-5 combs)
5. Series allpass filters for diffusion
6. Late LPF (mode-dependent cutoff)
7. 12-bit quantization with TPDF dither
8. Noise floor injection
9. Upsample to host rate
10. Reconstruction filter

### Reverb Modes

| Mode | RT60 | Comb Filters | Pre-delay | Character |
|------|------|--------------|-----------|-----------|
| Room | 0.6s | 4 | 0ms | Tight, grainy, dark |
| Plate | 1.6s | 4 | 0ms | Dense, metallic |
| Hall | 2.2s | 5 | 20ms | Smooth, warm |
| Cathedral | 3.0s | 5 | 30ms | Lush, long |
| Cosmos | 4.5s | 5 | 40ms | Ethereal, infinite |

## License

Copyright 2026 Yonie

## Related

- [WetDelay](../WetDelay) - The delay plugin this was based on