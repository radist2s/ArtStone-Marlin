# M420 - Vertical Feeder Control Command

## Overview

M420 is a custom G-code command for Marlin firmware designed for the ArtStone project - an automated artwork sales system. It controls a vertical feeder (paper stack lifter) using the E0 motor with sensor-based limit detection.

This is a customized Marlin firmware for the BTT SKR V1.4 board that includes a vertical feeder control system. The feeder uses the E0 motor for vertical platform movement (Z-axis motion) with sensors detecting safe movement limits. The system is designed to automatically dispense individual art pieces drawn on paper to customers.

## Hardware

- **Board:** BTT SKR V1.4 (LPC1768)
- **Motor:** E0 (repurposed for vertical feeder control)
- **Sensors:** 2x limit switches (filament runout sensors)
- **Application:** Automated artwork sales system - dispenses paper-based art pieces

## Requirements

- **FILAMENT_RUNOUT_SENSOR** must be enabled in Configuration.h
- **NUM_RUNOUT_SENSORS** must be >= 2
- Two sensors properly connected and configured
- E0 motor configured for feeder operation
- PlatformIO installed for building

## Sensor Logic

### Sensor States
- **open** = sensor not triggered (safe to move)
- **TRIGGERED** = sensor triggered (limit reached)

### Operation Algorithm
For **BOTH** commands (M420U and M420D) the logic is identical:
1. Initial state: sensor must be **open** (otherwise error - already at limit)
2. Continue moving: while sensor is **open**
3. Stop: when sensor becomes **TRIGGERED** (limit reached)

## Usage

### M420U - Move UP (lift platform)

Uses sensor 1 to control upward movement of the feeder.

```gcode
M420U
```
**Result:** Moves feeder up until sensor 1 triggers

### M420D - Move DOWN (lower platform)

Uses sensor 2 to control downward movement of the feeder.

```gcode
M420D
```
**Result:** Moves feeder down until sensor 2 triggers

### With Parameters

```gcode
M420U           ; Move up to limit
M420U S100      ; Move up max 100mm
M420U F300      ; Feedrate 300mm/min
M420U S150 F250 ; Combined parameters
M420D S200 F180 ; Move down with limits
M420D F300      ; Move down at 300mm/min
```

## Command Parameters

| Parameter | Description | Default Value |
|-----------|-------------|---------------|
| **U** | Move up (lift) | - |
| **D** | Move down (lower) | - |
| **S** | Max distance in mm | 500 |
| **F** | Feedrate in mm/min | 200 |

## Output Examples

### Successful Operation

Each command outputs **two lines** per status update: machine-readable status code and human-readable message.

```
M420_STATUS:START:UP:0.0:S1:OPEN
echo:M420: Starting UP movement, sensor 1
M420_STATUS:PROGRESS:UP:10.0:S1:OPEN
echo:M420: 10.0mm, sensor 1: ok
M420_STATUS:PROGRESS:UP:20.0:S1:OPEN
echo:M420: 20.0mm, sensor 1: ok
M420_STATUS:PROGRESS:UP:30.0:S1:OPEN
echo:M420: 30.0mm, sensor 1: ok
M420_STATUS:COMPLETE:UP:34.0:S1:TRIGGERED
echo:M420: Complete! Moved 34.0mm, limit reached
ok
```

### Maximum Distance Reached

```
M420_STATUS:START:DOWN:0.0:S2:OPEN
echo:M420: Starting DOWN movement, sensor 2
M420_STATUS:PROGRESS:DOWN:10.0:S2:OPEN
echo:M420: 10.0mm, sensor 2: ok
M420_STATUS:PROGRESS:DOWN:20.0:S2:OPEN
echo:M420: 20.0mm, sensor 2: ok
...
M420_STATUS:PROGRESS:DOWN:500.0:S2:OPEN
echo:M420: 500.0mm, sensor 2: ok
M420_STATUS:MAX_DIST:DOWN:500.0:S2:OPEN
echo:M420: Max distance reached (500.0mm), limit not reached
ok
```

### Errors

```
// Sensor already at limit
M420_STATUS:ERROR:SENSOR_BLOCKED
Error:M420: Sensor already at limit

// Direction not specified
M420_STATUS:ERROR:NO_DIRECTION
Error:M420: Specify U (up) or D (down)

// Sensors not configured
M420_STATUS:ERROR:INSUFFICIENT_SENSORS
Error:M420: Requires 2 sensors
```

## Machine-Readable Status Codes

M420 outputs structured status codes for easy parsing by external systems (displays, controllers, logging systems).

### Output Format

Each operation outputs **two lines** for each status update:

1. **Machine-readable line:** `M420_STATUS:<CODE>:<DIRECTION>:<DISTANCE>:<SENSOR>:<STATE>`
2. **Human-readable line:** `echo: M420: <message>`

### Status Codes

| Code | Description | Example |
|------|-------------|---------|
| `START` | Movement started | `M420_STATUS:START:UP:0.0:S1:OPEN` |
| `PROGRESS` | Movement in progress | `M420_STATUS:PROGRESS:UP:10.0:S1:OPEN` |
| `COMPLETE` | Limit reached successfully | `M420_STATUS:COMPLETE:UP:50.5:S1:TRIGGERED` |
| `MAX_DIST` | Max distance reached | `M420_STATUS:MAX_DIST:UP:500.0:S1:OPEN` |
| `ERROR` | Error occurred | `M420_STATUS:ERROR:SENSOR_BLOCKED` |

### Error Codes

| Code | Meaning |
|------|---------|
| `NO_SENSORS` | Filament sensors not enabled |
| `INSUFFICIENT_SENSORS` | Less than 2 sensors configured |
| `NO_DIRECTION` | No U or D parameter specified |
| `BOTH_DIRECTIONS` | Both U and D specified (invalid) |
| `SENSOR_BLOCKED` | Sensor already at limit (can't start) |
| `USER_INTERRUPT` | User interrupted the operation |

### Example Output

**Successful UP movement:**
```
M420_STATUS:START:UP:0.0:S1:OPEN
echo: M420: Starting UP movement, sensor 1
M420_STATUS:PROGRESS:UP:10.0:S1:OPEN
echo: M420: 10.0mm, sensor 1: ok
M420_STATUS:PROGRESS:UP:20.0:S1:OPEN
echo: M420: 20.0mm, sensor 1: ok
M420_STATUS:COMPLETE:UP:45.5:S1:TRIGGERED
echo: M420: Complete! Moved 45.5mm, limit reached
```

**Error example:**
```
M420_STATUS:ERROR:SENSOR_BLOCKED
Error:M420: Sensor already at limit
```

### Parsing Example (Python)

```python
import re

def parse_m420_status(line):
    """Parse M420 status line"""
    match = re.match(r'M420_STATUS:(\w+):(\w+):([\d.]+):S(\d):(\w+)', line)
    if match:
        return {
            'code': match.group(1),      # START, PROGRESS, COMPLETE, etc.
            'direction': match.group(2),  # UP or DOWN
            'distance': float(match.group(3)),
            'sensor': int(match.group(4)),
            'state': match.group(5)       # OPEN or TRIGGERED
        }
    
    # Check for error
    match = re.match(r'M420_STATUS:ERROR:(\w+)', line)
    if match:
        return {
            'code': 'ERROR',
            'error_type': match.group(1)
        }
    
    return None

# Usage example
status = parse_m420_status("M420_STATUS:PROGRESS:UP:10.0:S1:OPEN")
print(status)
# Output: {'code': 'PROGRESS', 'direction': 'UP', 'distance': 10.0, 'sensor': 1, 'state': 'OPEN'}
```

### Parsing Example (JavaScript/TypeScript)

```typescript
interface M420Status {
  code: 'START' | 'PROGRESS' | 'COMPLETE' | 'MAX_DIST' | 'ERROR';
  direction?: 'UP' | 'DOWN';
  distance?: number;
  sensor?: number;
  state?: 'OPEN' | 'TRIGGERED';
  errorType?: string;
}

function parseM420Status(line: string): M420Status | null {
  // Parse regular status
  const statusMatch = line.match(/M420_STATUS:(\w+):(\w+):([\d.]+):S(\d):(\w+)/);
  if (statusMatch) {
    return {
      code: statusMatch[1] as M420Status['code'],
      direction: statusMatch[2] as 'UP' | 'DOWN',
      distance: parseFloat(statusMatch[3]),
      sensor: parseInt(statusMatch[4]),
      state: statusMatch[5] as 'OPEN' | 'TRIGGERED'
    };
  }
  
  // Parse error
  const errorMatch = line.match(/M420_STATUS:ERROR:(\w+)/);
  if (errorMatch) {
    return {
      code: 'ERROR',
      errorType: errorMatch[1]
    };
  }
  
  return null;
}

// Usage
const status = parseM420Status("M420_STATUS:PROGRESS:UP:10.0:S1:OPEN");
console.log(status);
// Output: { code: 'PROGRESS', direction: 'UP', distance: 10, sensor: 1, state: 'OPEN' }
```

## Building

### Prerequisites

- PlatformIO installed
- BTT SKR V1.4 board

### Compile

```bash
pio run -e artstone_btt_skr_v1_4
```

### Upload

1. Press RESET button on BTT SKR V1.4
2. Copy `firmware.bin` to the board's SD card or USB drive
3. Reboot the printer

Or use direct upload (if board is mounted):

```bash
pio run -e artstone_btt_skr_v1_4 -t upload
```

## Configuration

### Enable the Command

The vertical feeder command is enabled by default in `Configuration_adv.h`:

```cpp
#define VERTICAL_FEEDER_COMMAND
```

To disable the command, comment out this line:

```cpp
//#define VERTICAL_FEEDER_COMMAND
```

### Sensor Configuration

Ensure in `Configuration.h`:

```cpp
#define FILAMENT_RUNOUT_SENSOR
#if ENABLED(FILAMENT_RUNOUT_SENSOR)
  #define NUM_RUNOUT_SENSORS   2          // Minimum 2 sensors
  #define FIL_RUNOUT_STATE     HIGH       // Pin state (LOW or HIGH)
  #define FIL_RUNOUT_PULLUP               // Use pullup
#endif
```

**Note:** Adjust `FIL_RUNOUT_STATE` based on your sensor type (normally open or normally closed).

## Implementation

### File Structure

The command consists of the following components:

1. **Main command file:**
   - `Marlin/src/gcode/host/M420.cpp` - command implementation with sensor control

2. **Header declaration:**
   - `Marlin/src/gcode/gcode.h` - M420() method declaration

3. **Dispatcher registration:**
   - `Marlin/src/gcode/gcode.cpp` - added to switch statement

4. **Configuration:**
   - `Marlin/Configuration_adv.h` - VERTICAL_FEEDER_COMMAND macro
   - `ini/features.ini` - build rule

### Project Structure

```
ArtStone-Marlin/
├── Marlin/
│   ├── Configuration.h
│   ├── Configuration_adv.h (VERTICAL_FEEDER_COMMAND)
│   └── src/
│       └── gcode/
│           └── host/
│               └── M420.cpp (feeder control)
├── ini/
│   └── features.ini (build rules)
├── docs/
│   └── VerticalFeederCommand_M420.md (this file)
└── VERTICAL_FEEDER_SUMMARY.md (quick reference)
```

## Implementation Details

### Sensor Reading Function

The `read_feeder_sensor()` function reads sensor pin states:

```cpp
inline bool read_feeder_sensor(const uint8_t sensor_num) {
  #if NUM_RUNOUT_SENSORS >= 2
    if (sensor_num == 1) {
      return READ(FIL_RUNOUT1_PIN) != FIL_RUNOUT1_STATE;
    }
    else if (sensor_num == 2) {
      return READ(FIL_RUNOUT2_PIN) != FIL_RUNOUT2_STATE;
    }
  #endif
  return false;
}
```

- Returns `true` if sensor TRIGGERED (limit reached)
- Returns `false` if sensor open (safe to move)
- Inverts pin value based on FIL_RUNOUT_STATE

### Movement Loop

The command operates in a loop with 2mm steps:

1. Sets destination for the next step
2. Executes movement via `prepare_line_to_destination()`
3. Synchronizes planner via `planner.synchronize()`
4. Updates current position
5. Reads sensor state
6. Outputs status every ~10mm (5 steps)
7. Checks stop conditions:
   - Sensor became TRIGGERED → success
   - Reached maximum distance → warning
   - User interrupted → error

### Safety Features

- **No temperature check:** Operation doesn't require heated extruder
- **Tool switching:** Automatically switches to T0
- **Maximum distance:** Default 500mm, configurable via S
- **Initial check:** Sensor must be in open state

## Use Cases

### Scenario 1: Load Paper Stack

```gcode
M420U           ; Lift feeder platform to sensor 1
```

### Scenario 2: Lower Platform

```gcode
M420D S150      ; Lower platform up to 150mm
```

### Scenario 3: Fast Movement

```gcode
M420U S100 F400 ; Fast lift: max 100mm at 400mm/min
```

## Testing

### Step 1: Check Configuration

```gcode
M119    ; Check sensor states
```

Expected output:
```
Reporting endstop status
...
filament: open        <- Sensor 1 should be open
filament 2: open      <- Sensor 2 should be open
```

### Step 2: Test M420U

```gcode
M420U S50 F200    ; Test with 50mm limit
```

Expected result:
- Feeder starts moving up
- Status output every ~10mm
- Movement stops when sensor 1 triggers

### Step 3: Test M420D

```gcode
M420D S50 F200    ; Test with 50mm limit
```

Expected result:
- Feeder starts moving down
- Status output every ~10mm
- Movement stops when sensor 2 triggers

### Step 4: Test Different Parameters

```gcode
M420U S100        ; Change maximum distance
M420U F300        ; Change feedrate
M420U S150 F250   ; Combined parameters
```

## Troubleshooting

### Error: "Sensor already at limit"

**Cause:** Sensor is already triggered at the start of operation.

**Solution:**
1. Check sensor state via M119
2. Ensure platform is not at limit position
3. Check sensor wiring

### Error: "Sensors not enabled"

Enable filament runout sensor in `Configuration.h`:
```cpp
#define FILAMENT_RUNOUT_SENSOR
```

### Error: "Requires 2 sensors"

Ensure that NUM_RUNOUT_SENSORS >= 2:
```cpp
#define NUM_RUNOUT_SENSORS 2
```

### Sensors Not Responding

1. Check wiring
2. Verify pin configuration
3. Test with `M119` command
4. Check `FIL_RUNOUT_STATE` setting (HIGH vs LOW)
5. Ensure pins are correctly configured
6. Verify sensors are physically connected

## Application

This command is designed for art installation systems where a vertical feeder lifts and lowers a stack of paper sheets. The E0 motor provides precise vertical movement control, while sensors ensure safe operation within defined limits.

The feeder is used in the ArtStone project for automated artwork sales system. It handles physical art pieces drawn on paper, enabling automated dispensing of individual artwork sheets to customers.

## License

This code is distributed under GPL v3 license, same as the main Marlin firmware.

Based on [Marlin Firmware](https://github.com/MarlinFirmware/Marlin).

## Credits

- Marlin Firmware Team
- ArtStone Project Team

## Support

For issues and questions:
1. Check this documentation
2. Review [VERTICAL_FEEDER_SUMMARY.md](../VERTICAL_FEEDER_SUMMARY.md) for quick reference
3. Consult [Marlin documentation](https://marlinfw.org/docs/)

---

**Branch:** artstone  
**Platform:** BTT SKR V1.4 (LPC1768)  
**Marlin Version:** 2.1.x  
**Custom Features:** Vertical feeder control (M420)
