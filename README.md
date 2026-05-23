# RingMod

A ring modulator audio plugin built with [JUCE](https://juce.com/). Multiplies the input signal by a sine-wave carrier to produce classic ring modulation sidebands, with a dry/wet mix control.

![GUI](source/GUI.png)

## Parameters

| Parameter | Range | Default | Description |
|---|---|---|---|
| Carrier Frequency | 20 – 2000 Hz | 440 Hz | Frequency of the internal sine oscillator (log-scaled) |
| Dry/Wet Mix | 0 – 1 | 1.0 | 0 = dry input only, 1 = fully ring-modulated output |

## How It Works

Ring modulation multiplies the input signal by a carrier sine wave:

```
output = dry + mix * (input × carrier − dry)
```

This creates sum and difference sidebands around the carrier frequency, producing metallic, bell-like, or robotic timbres depending on the carrier frequency and input material.

## Building

### Requirements

- CMake 3.22+
- JUCE 8 (source tree)
- A C++17 compiler (MSVC 2022, Clang, or GCC)

### Configure and Build

```bash
cmake -B build -DJUCE_PATH="C:/path/to/JUCE"
cmake --build build --config Release
```

The built plugin will be placed in `build/RingMod_artefacts/Release/`.

Supported formats: **VST3**, **AU** (macOS), **Standalone**.

### macOS / Linux

Replace the JUCE path as appropriate. AU is only built on macOS.

```bash
cmake -B build -DJUCE_PATH="/path/to/JUCE"
cmake --build build --config Release
```

## Project Structure

```
ringmod/
├── CMakeLists.txt
├── README.md
├── LICENSE
├── assets/
│   └── icon.svg
└── source/
    ├── PluginProcessor.h / .cpp   # DSP: oscillator, ring mod, state save/load
    ├── PluginEditor.h / .cpp      # GUI: custom knobs, oscilloscope, LED bar
    └── GUI.png                    # Reference design
```

## License

MIT — see [LICENSE](LICENSE).
