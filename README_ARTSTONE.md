# ArtStone Marlin Firmware

Custom Marlin firmware for ArtStone automated artwork sales system.

## Overview

This firmware controls a vertical feeder that dispenses individual art pieces drawn on paper to customers. The system uses the E0 motor for precise vertical platform movement with sensor-based safety limits.

## Hardware

- **Board:** BTT SKR V1.4 (LPC1768)
- **Motor:** E0 (repurposed for vertical feeder control)
- **Sensors:** 2x limit switches (filament runout sensors)
- **Application:** Automated artwork sales - dispenses paper-based art pieces

## Quick Start

### Build & Upload

```bash
pio run -e artstone_btt_skr_v1_4 -t upload
```

### M420 - Vertical Feeder Control

```gcode
M420U           # Move feeder UP to sensor 1
M420D           # Move feeder DOWN to sensor 2
M420U S100 F300 # Move up max 100mm at 300mm/min
```

**Parameters:**
- `U` / `D` - Direction (Up/Down)
- `S<mm>` - Max distance (default: 500mm)
- `F<mm/min>` - Feedrate (default: 200mm/min)

## Configuration

### Enable M420 Command

In `Marlin/Configuration_adv.h`:
```cpp
#define VERTICAL_FEEDER_COMMAND
```

### Sensor Configuration

In `Marlin/Configuration.h`:
```cpp
#define FILAMENT_RUNOUT_SENSOR
#if ENABLED(FILAMENT_RUNOUT_SENSOR)
  #define NUM_RUNOUT_SENSORS   2
  #define FIL_RUNOUT_STATE     HIGH  // or LOW, depends on sensor type
  #define FIL_RUNOUT_PULLUP
#endif
```

## Documentation

📖 **Complete documentation:** [docs/VerticalFeederCommand_M420.md](docs/VerticalFeederCommand_M420.md)

Includes:
- Detailed usage guide
- Hardware configuration
- Building instructions
- Troubleshooting
- Implementation details

## Testing

### Check Sensors
```gcode
M119    # Verify sensor states (should be "open")
```

### Test Movement
```gcode
M420U S50    # Test upward movement (50mm max)
M420D S50    # Test downward movement (50mm max)
```

## Project Structure

```
ArtStone-Marlin/
├── Marlin/
│   ├── Configuration.h (sensor setup)
│   ├── Configuration_adv.h (VERTICAL_FEEDER_COMMAND)
│   └── src/gcode/host/M420.cpp (feeder control)
├── ini/features.ini (build rules)
├── docs/VerticalFeederCommand_M420.md (full documentation)
└── README_ARTSTONE.md (this file)
```

## License

GPL v3 - Based on [Marlin Firmware](https://github.com/MarlinFirmware/Marlin)

---

**Branch:** artstone  
**Platform:** BTT SKR V1.4 (LPC1768)  
**Marlin Version:** 2.1.x  
**Custom Feature:** M420 - Vertical feeder control for automated artwork sales

