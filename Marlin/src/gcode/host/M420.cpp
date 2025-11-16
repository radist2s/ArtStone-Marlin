/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2020 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Based on Sprinter and grbl.
 * Copyright (c) 2011 Camiel Gubbels / Erik van der Zalm
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "../../inc/MarlinConfig.h"

#if ENABLED(VERTICAL_FEEDER_COMMAND)

#include "../gcode.h"
#include "../../core/serial.h"
#include "../../module/motion.h"
#include "../../module/planner.h"
#include "../../module/stepper.h"
#include "../../module/tool_change.h"

#if HAS_FILAMENT_SENSOR

// Read sensor state
// Returns: true = TRIGGERED (limit reached), false = open (safe to move)
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

#endif // HAS_FILAMENT_SENSOR

/**
 * M420: Vertical Feeder Control
 * 
 * Controls vertical feeder for automated artwork sales system.
 * Uses E0 motor for vertical Z-axis motion to dispense art pieces on paper.
 * Sensors detect safe movement limits.
 * 
 * Usage:
 *   M420U        Move UP using sensor 1, stop when TRIGGERED
 *   M420D        Move DOWN using sensor 2, stop when TRIGGERED
 *   M420U S100   Move up max 100mm
 *   M420D F300   Move down at 300mm/min
 * 
 * Parameters:
 *   U            Move up (lift platform)
 *   D            Move down (lower platform)
 *   S<mm>        Max distance (default: 500mm)
 *   F<mm/min>    Feedrate (default: 200mm/min)
 * 
 * Logic:
 *   - Sensor must be 'open' at start (safe to move)
 *   - Continue moving while sensor is 'open'
 *   - Stop when sensor becomes 'TRIGGERED' (limit reached)
 */
void GcodeSuite::M420() {
  #if !HAS_FILAMENT_SENSOR
    SERIAL_ERROR_MSG("M420: Sensors not enabled");
    return;
  #elif NUM_RUNOUT_SENSORS < 2
    SERIAL_ERROR_MSG("M420: Requires 2 sensors");
    return;
  #else

  // Detect command type from string_arg or parameters
  // Support both M420U and M420 U formats
  bool move_up = false;
  bool move_down = false;
  
  // Check if U or D is in string_arg (e.g., M420U or M420D)
  if (parser.string_arg && parser.string_arg[0]) {
    const char first_char = parser.string_arg[0];
    if (first_char == 'U' || first_char == 'u') move_up = true;
    else if (first_char == 'D' || first_char == 'd') move_down = true;
  }
  
  // Also check as parameters (e.g., M420 U or M420 D)
  if (!move_up && !move_down) {
    if (parser.seen('U')) move_up = true;
    if (parser.seen('D')) move_down = true;
  }
  
  if (!move_up && !move_down) {
    SERIAL_ERROR_MSG("M420: Specify U (up) or D (down)");
    return;
  }
  
  if (move_up && move_down) {
    SERIAL_ERROR_MSG("M420: Cannot specify both U and D");
    return;
  }
  
  // Get parameters
  const float max_distance = parser.floatval('S', 500.0f); // Default 500mm
  const feedRate_t feedrate = parser.linearval('F', 200.0f); // Default 200mm/min
  
  // Note: Temperature check is intentionally disabled
  // Feeder operation doesn't require heated extruder
  
  // Switch to T0 (feeder motor) if needed
  #if HAS_MULTI_EXTRUDER
    if (active_extruder != 0) {
      tool_change(0);
      planner.synchronize();
    }
  #endif
  
  // Select sensor and direction
  const uint8_t sensor_num = move_up ? 1 : 2;
  const float step_size = move_up ? 2.0f : -2.0f;
  
  // Check initial sensor state - MUST be open (safe to move)
  bool sensor_state = read_feeder_sensor(sensor_num);
  if (sensor_state) {  // If TRIGGERED (at limit)
    SERIAL_ERROR_MSG("M420: Sensor already at limit");
    return;
  }
  
  // Movement loop
  float moved_distance = 0.0f;
  uint8_t status_counter = 0;
  
  SERIAL_ECHO_START();
  SERIAL_ECHOPGM("M420: Moving feeder ");
  SERIAL_ECHOPGM(move_up ? "UP" : "DOWN");
  SERIAL_ECHOPGM(", sensor ");
  SERIAL_ECHO(sensor_num);
  SERIAL_ECHOLNPGM("...");
  
  while (moved_distance < max_distance) {
    // Set destination
    destination = current_position;
    destination.e += step_size;
    feedrate_mm_s = MMM_TO_MMS(feedrate);
    
    // Execute move
    prepare_line_to_destination();
    planner.synchronize();
    
    // Update position and distance
    current_position = destination;
    moved_distance += ABS(step_size);
    
    // Check sensor
    sensor_state = read_feeder_sensor(sensor_num);
    
    // Status update every ~10mm
    if (++status_counter >= 5) {
      SERIAL_ECHO_START();
      SERIAL_ECHOPGM("M420: ");
      SERIAL_ECHO(moved_distance);
      SERIAL_ECHOPGM("mm, sensor ");
      SERIAL_ECHO(sensor_num);
      SERIAL_ECHOPGM(": ");
      SERIAL_ECHOLN(sensor_state ? "LIMIT" : "ok");
      status_counter = 0;
    }
    
    // Stop if sensor reached limit
    if (sensor_state) {
      SERIAL_ECHO_START();
      SERIAL_ECHOPGM("M420: Complete! Moved ");
      SERIAL_ECHO(moved_distance);
      SERIAL_ECHOLNPGM("mm, limit reached");
      return;
    }
    
    // Check for user interrupt
    #if HAS_RESUME_CONTINUE
      if (wait_for_user) {
        SERIAL_ERROR_MSG("M420: Interrupted by user");
        return;
      }
    #endif
  }
  
  // Reached max distance without reaching limit
  SERIAL_ECHO_START();
  SERIAL_ECHOPGM("M420: Max distance reached (");
  SERIAL_ECHO(max_distance);
  SERIAL_ECHOLNPGM("mm), limit not reached");

  #endif // NUM_RUNOUT_SENSORS >= 2
}

#endif // VERTICAL_FEEDER_COMMAND

