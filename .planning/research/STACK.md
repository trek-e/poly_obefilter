# Technology Stack

**Project:** HydraQuartet VCF-OB (VCV Rack Module)
**Researched:** 2026-01-29
**Confidence:** HIGH

## Recommended Stack

### Core Framework

| Technology | Version | Purpose | Why Recommended |
|------------|---------|---------|-----------------|
| VCV Rack SDK | 2.6.6+ | Module framework and API | Official SDK for VCV Rack 2.x with stable API, includes all headers, build system, and component library. Latest version (2.6.6 released Nov 2025) includes UTF-32/UTF-8 support and MidiParser class. |
| C++11 | std=c++11 | Programming language | Required by VCV Rack SDK compile flags. While individual plugins can use C++17, the SDK builds with C++11 standard. Provides sufficient features for DSP and UI code. |
| Makefile | Standard | Build system | Official VCV Rack build system. Uses `make` to compile, `make dist` to package, `make install` to deploy to user folder. No spaces allowed in paths. |
| helper.py | Bundled with SDK | Scaffolding tool | Python script included in SDK for creating plugin templates and generating modules from SVG panels. Automatically sets up plugin.json, source files, and directory structure. |

### DSP Libraries (Built into SDK)

| Library | Purpose | When to Use |
|---------|---------|-------------|
| rack::dsp | Digital signal processing utilities | Always - provides BiquadFilter, IIRFilter, RCFilter, ExponentialFilter, SlewLimiter, PeakFilter for filter modules |
| rack::simd | SIMD optimization (float_4) | For polyphonic processing - process 4 channels simultaneously with simd::pow(), simd::sin(), simd::trunc() |
| pffft | Fast Fourier Transform | If frequency-domain processing needed (not required for state-variable filters) |
| dsp::approx | Fast math approximations | For performance-critical DSP - dsp::exp2_taylor5() and similar functions |
| dsp::ode | ODE solvers (Euler, RK2, RK4) | For modeling analog circuits - useful for accurate filter emulation |
| dsp::Trigger | Trigger detection (SchmittTrigger, BooleanTrigger) | For CV gate/trigger inputs and button handling |

### Graphics & UI

| Technology | Version | Purpose | Why Recommended |
|------------|---------|---------|-----------------|
| NanoVG | Bundled with SDK | Vector graphics rendering | Built into VCV Rack - renders all UI elements, custom widgets, and dynamic displays. Use nvg* functions for custom drawing in Widget::draw() override. |
| NanoSVG | Bundled with SDK | SVG parsing | Automatically renders SVG panels. Limited gradient support - stick to two-color linear gradients. |
| Component Library | componentlibrary.hpp | UI widgets | Rack's built-in knobs, ports, switches, lights, screws. Use createParam(), createInput(), createOutput() helpers instead of building from scratch. |
| Framebuffer Widget | FramebufferWidget | GPU caching | Wrap expensive custom widgets to cache rendering. Only redraws when dirty flag set - critical for performance. |

### Panel Design Tools

| Tool | Purpose | Notes |
|------|---------|-------|
| Inkscape | SVG panel creation (RECOMMENDED) | Official recommendation. Free, cross-platform. Export with "Path > Object to Path" to convert all text. Set document to millimeters. Height = 128.5mm, width = multiple of 5.08mm (1 HP). |
| Adobe Illustrator | SVG panel creation (alternative) | Commercial option. Export with "Export As..." not "Save As...". Settings: Inline Style, Convert to Outlines, Embed, Layer Name, disable Minify/Responsive. |
| Affinity Designer | SVG panel creation (alternative) | Low-cost alternative (~$50). Export SVG with "Export Text as Curves" and "Use relative coordinates" enabled. |

### Development Tools

| Tool | Purpose | Notes |
|------|---------|-------|
| GDB | Debugging | Included with MinGW64 on Windows, standard on Linux/Mac. Use with `-d` flag to launch Rack in dev mode (uses terminal for logging, disables library to prevent plugin overwrites). |
| VS Code | IDE/Editor | Community standard. Configure tasks.json and launch.json for integrated debugging with GDB. |
| Git | Version control | Tag releases with `git tag vX.Y.Z` and `git push --tags`. Use commit hashes (not branch names) when submitting to VCV Library. |
| GitHub Actions | CI/CD (optional) | Community workflows available for automated multi-platform builds using VCV Rack Plugin Toolchain Docker image v14. |

## Build Environment Requirements

### Windows (MSYS2 MinGW 64-bit)

```bash
# Required packages (run in MinGW 64-bit shell, NOT default MSYS shell)
pacman -Syu git wget make tar unzip zip \
  mingw-w64-x86_64-gcc \
  mingw-w64-x86_64-cmake \
  mingw-w64-x86_64-gdb \
  mingw-w64-x86_64-jq \
  mingw-w64-x86_64-pkgconf \
  autoconf automake libtool \
  python zstd
```

### macOS (Homebrew)

```bash
brew install git wget cmake autoconf automake libtool jq python zstd pkg-config
```

### Linux (Ubuntu 16.04+)

```bash
sudo apt-get install git wget cmake autoconf automake libtool \
  libx11-dev libglu1-mesa-dev \
  libasound2-dev libjack-jackd2-dev \
  libgl1-mesa-dev libglu1-mesa-dev \
  jq python3 zstd pkg-config
```

### Linux (Arch)

```bash
sudo pacman -Syu git wget make tar unzip zip \
  gcc cmake gdb jq python zstd pkgconf \
  autoconf automake libtool \
  mesa glu alsa-lib jack
```

## Installation

### Initial Setup

```bash
# Download VCV Rack SDK from https://vcvrack.com/downloads/
# Extract SDK (available for Windows x64, Mac x64, Mac ARM64, Linux x64)

# Set environment variable
export RACK_DIR=/path/to/Rack-SDK

# Create plugin template
$RACK_DIR/helper.py createplugin YourPluginSlug

# Create module from SVG panel
$RACK_DIR/helper.py createmodule ModuleName res/ModuleName.svg src/ModuleName.cpp
```

### Development Workflow

```bash
# Build plugin
make

# Install to Rack user folder
make install

# Create distributable package
make dist

# Clean build artifacts
make clean
```

## Alternatives Considered

| Category | Recommended | Alternative | Why Not Alternative |
|----------|-------------|-------------|---------------------|
| Build System | Makefile | CMake | Makefile is the official standard. CMake support exists (unofficial Rack-SDK project) but adds complexity and isn't VCV Library compatible. |
| C++ Standard | C++11 | C++17 | SDK compiles with C++11. While individual plugins CAN use C++17 by overriding flags, C++11 is sufficient and ensures broadest compatibility. |
| Panel Design | Inkscape | Programmatic (code-gen) | Code-generated panels lack artistic flexibility. Most successful modules use SVG workflow for professional appearance. |
| DSP Library | rack::dsp | External (JUCE, etc.) | rack::dsp is optimized for VCV Rack, includes SIMD support, and is already linked. External libraries add binary size and linking complexity. |
| Graphics API | NanoVG | Direct OpenGL | NanoVG is the only supported API. Direct OpenGL would break compatibility and require reimplementing all UI infrastructure. |

## What NOT to Use

| Avoid | Why | Use Instead |
|-------|-----|-------------|
| Spaces in file paths | Breaks Makefile-based build system | Use underscores or hyphens in directory names |
| Master branch name | Rack repo uses version branches (v2, etc.) | Use `main` or version-specific branch names |
| Building Rack from source | Unnecessary for plugin development, slow | Download precompiled Rack SDK |
| Complex SVG gradients | Not fully supported by NanoSVG | Stick to simple two-color linear gradients |
| CSS styling in SVG | Not implemented in NanoSVG | Use inline styles, convert text to paths |
| Symbols in Illustrator SVG | Cause rendering issues in Rack | Flatten or expand symbols before export |
| `git add -A` for commits | Risk accidentally committing secrets | Add files explicitly by name |
| Branch names for Library submission | VCV requires commit hashes | Use `git rev-parse HEAD` to get hash |
| v prefix in plugin.json version | Version format is MAJOR.MINOR.REVISION | Use "2.0.1" not "v2.0.1" |

## Stack Patterns by Module Type

**For State-Variable Filter Module (This Project):**
- Use `rack::dsp::BiquadFilter` or `rack::dsp::IIRFilter` as foundation
- Implement `rack::simd::float_4` for 8-voice polyphony (process 2x float_4 vectors per sample)
- Use `configParam()` with attenuverters for CV control
- Set output channels with `setChannels()` based on input polyphony
- Override `process()` method for per-sample DSP
- Use `SchmittTrigger` for any mode switching buttons

**For Oscillator Modules:**
- Use `dsp::MinBLEP` for anti-aliased waveforms
- Use `dsp::PulseGenerator` for gate outputs
- Use `dsp::Resampler` if variable sample rate needed

**For Effect Modules:**
- Use `dsp::RingBuffer` for delay lines
- Use `dsp::RealFFT` for frequency-domain effects
- Use `FramebufferWidget` for spectrum analyzers to cache expensive visualization

## Version Compatibility

| Rack Version | SDK Version | Plugin MAJOR Version | Compiler | SSE Requirements |
|--------------|-------------|----------------------|----------|------------------|
| 2.6.6 | 2.6.6 | 2.x.x | GCC 12+ / Clang 14+ | SSE4.2 + POPCNT (nehalem) |
| 2.6.x | 2.6.x | 2.x.x | GCC 12+ / Clang 14+ | SSE4.2 + POPCNT (nehalem) |
| 2.5.x | 2.5.x | 2.x.x | GCC 11+ / Clang 13+ | SSE4.2 + POPCNT (nehalem) |
| 2.0.x | 2.0.x | 2.x.x | GCC 11+ / Clang 13+ | SSE4.2 + POPCNT (nehalem) |

**Important:** Plugin version MAJOR number must match Rack MAJOR version (e.g., plugin version 2.4.2 means compatible with Rack 2.x).

## Optimization Flags

VCV Rack SDK automatically applies:
```makefile
CXXFLAGS += -std=c++11
CXXFLAGS += -O3 -march=nehalem -funsafe-math-optimizations
```

**-march=nehalem** requires: SSE4.2 and POPCNT instruction sets (2008+ CPUs)

## Plugin Manifest (plugin.json)

### Required Fields
```json
{
  "slug": "YourPluginSlug",
  "name": "Your Plugin Name",
  "version": "2.0.0",
  "license": "GPL-3.0-or-later",
  "author": "Your Name"
}
```

### Recommended Optional Fields
```json
{
  "brand": "Synth-etic Intelligence",
  "description": "Polyphonic multimode filter inspired by Oberheim designs",
  "authorEmail": "support@example.com",
  "sourceUrl": "https://github.com/yourusername/yourplugin",
  "manualUrl": "https://yourplugin.com/manual",
  "changelogUrl": "https://github.com/yourusername/yourplugin/blob/main/CHANGELOG.md"
}
```

### Module Entry
```json
{
  "modules": [
    {
      "slug": "HydraQuartetVCF",
      "name": "HydraQuartet VCF-OB",
      "description": "8-voice polyphonic multimode filter (SEM 12dB, OB-X 24dB)",
      "tags": ["Filter", "Polyphonic"],
      "keywords": ["oberheim", "sem", "obx", "state variable", "multimode"]
    }
  ]
}
```

## Library Submission Process

1. **Prepare Repository:**
   - Host on GitHub with public access
   - Include LICENSE file matching plugin.json license field
   - Tag releases: `git tag v2.0.0 && git push --tags`

2. **Submit to VCV Library:**
   - Create issue at https://github.com/VCVRack/library/issues
   - Title = plugin slug (exact match)
   - Provide source code URL
   - Post commit hash (from `git rev-parse HEAD`)

3. **Push Updates:**
   - Increment version in plugin.json (2.0.0 → 2.0.1)
   - Commit and push
   - Post new commit hash in library issue thread

## Sources

**HIGH Confidence (Official Documentation):**
- [VCV Rack Plugin Development Tutorial](https://vcvrack.com/manual/PluginDevelopmentTutorial) - Core workflow, requirements, C++11 standard
- [VCV Rack Building Guide](https://vcvrack.com/manual/Building) - Build dependencies, compiler setup, environment requirements
- [VCV Rack Plugin API Guide](https://vcvrack.com/manual/PluginGuide) - Advanced features, polyphony, expanders, data persistence
- [VCV Rack Module Panel Guide](https://vcvrack.com/manual/Panel) - SVG specifications, component placement, design guidelines
- [VCV Rack Plugin Manifest](https://vcvrack.com/manual/Manifest) - plugin.json requirements and fields
- [VCV Rack DSP Manual](https://vcvrack.com/manual/DSP) - DSP theory and FFT library
- [VCV Rack Polyphony Manual](https://vcvrack.com/manual/Polyphony) - Polyphonic cable system (16 channels)
- [VCV Rack API Reference - rack::dsp](https://vcvrack.com/docs-v2/namespacerack_1_1dsp) - DSP classes and functions
- [VCV Rack API Reference - rack::simd](https://vcvrack.com/docs-v2/namespacerack_1_1simd) - SIMD optimization
- [VCV Rack API Reference - componentlibrary.hpp](https://vcvrack.com/docs-v2/componentlibrary_8hpp) - UI widget library
- [VCV Rack Changelog v2](https://github.com/VCVRack/Rack/blob/v2/CHANGELOG.md) - Version 2.6.6 current as of Nov 2025

**MEDIUM Confidence (VCV Community - Official Forum):**
- [VCV Community Development Forum](https://community.vcvrack.com/c/development/8) - Best practices, workflows
- [C++ Standard Discussion](https://community.vcvrack.com/t/newest-c-standard-allowed-for-inclusion-in-plugin-library/5970) - C++11 requirement confirmed
- [VS Code Debugging Setup](https://medium.com/@tonetechnician/how-to-setup-your-windows-vs-code-environment-for-vcv-rack-plugin-development-and-debugging-6e76c5a5f115) - IDE configuration
- [GitHub Actions Workflow](https://community.vcvrack.com/t/automated-building-and-releasing-plugins-on-github-with-github-actions/11364) - CI/CD automation

**MEDIUM Confidence (Open Source Examples):**
- [ValleyAudio/ValleyRackFree](https://github.com/ValleyAudio/ValleyRackFree) - GPL-3.0 plugin with filters, reverb, oscillators
- [VCV Rack GitHub Organization](https://github.com/vcvrack) - Official repositories
- [VCV Library](https://library.vcvrack.com/) - Published plugin examples

---
*Stack research for: VCV Rack 2.x polyphonic filter module*
*Researched: 2026-01-29*
