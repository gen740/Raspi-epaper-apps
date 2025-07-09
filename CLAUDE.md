# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a cross-platform e-paper display application system designed for Raspberry Pi devices. The project consists of:

- **Server component** (`apps/raspi/image_server/`): gRPC server running on Raspberry Pi that controls a 7.3" e-paper display
- **Client component** (`apps/mac/sender_app/`): Qt-based macOS application for image processing and transmission
- **E-paper driver** (`epaper/`): Hardware interface for the Waveshare 7.3" e-paper display using GPIO

## Build System

This project uses CMake with Nix for dependency management. Platform-specific components are built conditionally:

### Development Environment Setup
```bash
# Enter Nix development shell
nix develop

# Configure and build
cmake -B build -G Ninja
cmake --build build
```

### Platform-Specific Builds
- **Linux/Raspberry Pi**: Includes e-paper driver and image server
- **macOS**: Includes only the Qt client application

### Running Applications
```bash
# Run the image server on Raspberry Pi
./build/apps/raspi/image_server/image_server

# Run the client app on macOS
./build/apps/mac/sender_app/sender_app
```

## Architecture

### Communication Protocol
- Uses Cap'n Proto for client-server communication
- Protocol defined in `apps/capnproto/image_service.capnp`
- Server expects 800x480 pixel image data (192,000 bytes for 4-bit color)
- Client converts and dithers images before transmission

### Key Components

#### E-paper Driver (`epaper/`)
- `EPD7IN3E` class handles hardware communication via GPIO
- Supports 7 colors: Black, White, Yellow, Red, Blue, Green
- Uses libgpiod for GPIO control on Raspberry Pi

#### Image Server (`apps/raspi/image_server2/`)
- Cap'n Proto service listening on port 50051
- Accepts image data and displays on e-paper screen
- Automatic slideshow from `/home/gen/images/` directory
- Thread-safe display operations with mutex protection

#### Client Application (`apps/mac/sender_app2/`)
- Qt6-based GUI with drag-and-drop image support
- Real-time image preview and processing
- Dithering and color palette conversion
- Network transmission to Raspberry Pi server

#### Common Utilities (`apps/common/`)
- Image loading using STB library
- Color palette definitions and conversions
- Utility functions shared between components

### Dependencies
- **Qt6**: GUI framework (client only)
- **Cap'n Proto**: Network communication
- **libgpiod**: GPIO control (Raspberry Pi only)
- **STB Image**: Image file loading
- **BCM2835**: Hardware abstraction (Raspberry Pi only)

## Development Notes

### Platform Detection
The build system automatically detects the target platform:
- `IS_LINUX=1` for Linux builds (includes e-paper functionality)
- `IS_LINUX=0` for other platforms (client-only builds)

### Image Processing Pipeline
1. Load image using STB library
2. Convert to target resolution (800x480)
3. Apply dithering for e-paper display
4. Convert to 4-bit color format
5. Transmit via Cap'n Proto to server

### GPIO Configuration
E-paper display uses these GPIO pins on Raspberry Pi:
- RST_PIN: 17
- DC_PIN: 25  
- PWR_PIN: 18
- BUSY_PIN: 24