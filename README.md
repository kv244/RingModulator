# RingMod

[![Compile](https://github.com/kv244/RingModulator/actions/workflows/compile.yml/badge.svg)](https://github.com/kv244/RingModulator/actions/workflows/compile.yml)
[![Lint](https://github.com/kv244/RingModulator/actions/workflows/lint.yml/badge.svg)](https://github.com/kv244/RingModulator/actions/workflows/lint.yml)

**Version 1.1.0**

A ring modulator audio plugin built with [JUCE](https://juce.com/). Multiplies the input signal by a selectable carrier waveform to produce classic ring modulation sidebands, with a dry/wet mix control.

![GUI](source/GUI.png)

GUI prototype on Figma Community: https://www.figma.com/community/file/1657604575511164578

## Parameters

| Parameter | Range | Default | Description |
|---|---|---|---|
| Carrier Frequency | 20 – 2000 Hz | 440 Hz | Frequency of the carrier oscillator (log-scaled) |
| Dry/Wet Mix | 0 – 1 | 1.0 | 0 = dry input only, 1 = fully ring-modulated output |
| Waveform | Sine / Saw / Square / Triangle | Sine | Shape of the carrier oscillator |

## View in Renoise
<img width="1198" height="202" alt="image" src="https://github.com/user-attachments/assets/94902cca-59f5-4e0d-8fa1-0b67ae85162c" />


## How It Works

Ring modulation multiplies the input signal by a carrier waveform:

```
output = dry + mix * (input × carrier − dry)
```

This creates sum and difference sidebands around the carrier frequency, producing metallic, bell-like, or robotic timbres depending on the carrier frequency, waveform, and input material.

### Waveform character

| Waveform | Character |
|---|---|
| Sine | Clean sidebands — classic ring mod sound |
| Saw | Dense harmonic stack; brighter and more aggressive |
| Square | Odd harmonics only; hollow, nasal timbre |
| Triangle | Softer than square; gentle odd-harmonic colour |

> **Note:** Saw and Square carriers contain harmonics above Nyquist that will alias at high carrier frequencies. This is an intentional DSP characteristic and adds to the character of those modes.

## Building

### Requirements

- CMake 3.22+
- A C++17 compiler (MSVC 2022, Clang, or GCC)
- Git (JUCE 8.0.13 is fetched automatically if no local copy is provided)

### Configure and Build

```bash
# JUCE is downloaded automatically via FetchContent
cmake -B build
cmake --build build --config Release
```

To use a local JUCE tree instead (faster, avoids the download):

```bash
cmake -B build -DJUCE_PATH="/path/to/JUCE"
cmake --build build --config Release
```

The built plugin will be placed in `build/RingMod_artefacts/Release/`.

Supported formats: **VST3**, **Standalone**.

### Installing into a DAW

Copy `build/RingMod_artefacts/Release/VST3/RingMod.vst3` to:

| OS | Default VST3 folder |
|---|---|
| Windows | `C:\Program Files\Common Files\VST3\` |
| macOS | `/Library/Audio/Plug-Ins/VST3/` |

**Auto-install after build** (Windows, requires admin shell):

```bash
cmake -B build -DJUCE_PATH="..." -DCOPY_PLUGIN=ON
cmake --build build --config Release
```

**MSI installer** — download `RingMod-v1.1.0-win64.msi` from [GitHub Releases](https://github.com/kv244/RingModulator/releases) for a one-click install to the system VST3 folder.

#### Ableton Live
Preferences → Plug-Ins → enable "Use VST3 Plug-In Custom Folder" or point it at the Common Files path above. Re-scan plug-ins after copying.

#### Renoise
Preferences → Plug-Ins/Devices → add the VST3 folder and click Re-scan. The plugin supports both mono and stereo FX tracks.

A self-contained example project is included at [`examples/sample_ringMod.xrns`](examples/sample_ringMod.xrns) — open it directly in Renoise 3.5+ to hear the plugin in use (square wave bass, 342 Hz carrier, 29% wet mix).

## Project Structure

```
ringmod/
├── CMakeLists.txt
├── README.md
├── LICENSE
├── assets/
│   └── icon.svg
├── installer/
│   └── RingMod.wxs        # WiX v4 MSI installer definition
├── .github/workflows/
│   ├── compile.yml        # Build + MSI check on every push to master
│   ├── lint.yml           # cppcheck static analysis
│   └── release.yml        # Build + publish GitHub Release on v* tags
└── source/
    ├── PluginProcessor.h / .cpp   # DSP: phase accumulator, ring mod, state save/load
    ├── PluginEditor.h / .cpp      # GUI: custom knobs, oscilloscope, waveform selector
    └── GUI.png                    # Reference screenshot
```

## Changelog

### 1.1.0
- Added **Waveform** parameter: Sine, Saw, Square, Triangle carrier shapes
- Replaced `juce::dsp::Oscillator` with a direct phase accumulator for allocation-free waveform switching on the audio thread
- Waveform switches only at phase zero-crossing to avoid mid-cycle discontinuities
- Added `isBusesLayoutSupported` — plugin now accepts mono and stereo FX tracks (Ableton Live, Renoise)
- Added `processBlockBypassed` for click-free DAW bypass
- Added WiX v4 MSI installer; releases published automatically to GitHub Releases on version tags

### 1.0.0
- Initial release: sine carrier ring modulator, dry/wet mix, oscilloscope, custom knob UI

## License

MIT — see [LICENSE](LICENSE).
