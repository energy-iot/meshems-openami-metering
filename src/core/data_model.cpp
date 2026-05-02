/**
 * @file data_model.cpp
 * @brief Implementation of the Modbus data model
 * 
 * This file defines the data structures that represent the four standard Modbus
 * data types: coils, discrete inputs, holding registers, and input registers.
 * These arrays store the values that can be read from or written to by Modbus
 * master/client devices.
 */

 #include <Arduino.h>
 #include <core/data_model.h>

  PowerData readings[MODBUS_NUM_METERS]; // Array to hold readings for each meter
  
  /**
   * Coils (read-write digital outputs)
   * 
   * This array stores the state of all coils in the system.
   * Coils are binary (ON/OFF) values that can be read and written by Modbus masters.
   * They typically represent control outputs such as relays or digital outputs.
   */
  bool coils[MODBUS_NUM_COILS];
  
  /**
   * Discrete Inputs (read-only digital inputs)
   * 
   * This array stores the state of all discrete inputs in the system.
   * Discrete inputs are binary (ON/OFF) values that can only be read by Modbus masters.
   * They typically represent the state of digital inputs like switches or sensors.
   */
  bool discreteInputs[MODBUS_NUM_DISCRETE_INPUTS];
  
  /**
   * Holding Registers (read-write 16-bit registers)
   * 
   * This array stores the values of all holding registers in the system.
   * Holding registers are 16-bit values that can be read and written by Modbus masters.
   * They typically store configuration parameters or setpoints for the device.
   */
  uint16_t holdingRegisters[MODBUS_NUM_HOLDING_REGISTERS];
 
 /**
  * Input Registers (read-only 16-bit registers)
  * 
  * This array stores the values of all input registers in the system.
  * Input registers are 16-bit values that can only be read by Modbus masters.
  * They typically store measured values from sensors or status information.
  */
 uint16_t inputRegisters[MODBUS_NUM_INPUT_REGISTERS];

// One circular-buffer history entry per meter; default member values in the
// struct definition (data_model.h) initialise each element correctly.
CurrentHistory currentHistory[MODBUS_NUM_METERS];

// Append a current sample to the per-meter circular buffer.
// meterIdx must be in [0, MODBUS_NUM_METERS).
void addCurrentReading(int meterIdx, float value) {
    if (meterIdx < 0 || meterIdx >= MODBUS_NUM_METERS) return;

    CurrentHistory& h = currentHistory[meterIdx];

    int overwriteIdx = h.currentIndex;
    bool wasFull = (h.count >= CURRENT_HISTORY_SIZE);

    h.values[overwriteIdx] = value;
    h.currentIndex = (h.currentIndex + 1) % CURRENT_HISTORY_SIZE;
    if (h.count < CURRENT_HISTORY_SIZE) h.count++;

    // Incremental min/max — O(1) in the common case.
    if (h.count == 1) {
        h.minValue = value;
        h.maxValue = value;
    } else {
        if (value < h.minValue) {
            h.minValue = value;
        } else if (value > h.maxValue) {
            h.maxValue = value;
        } else if (wasFull) {
            // The slot we just overwrote may have held the old min or max.
            // Rescan only when a boundary value could have been lost; amortised O(1).
            h.minValue = h.values[0];
            h.maxValue = h.values[0];
            for (int i = 1; i < h.count; i++) {
                if (h.values[i] < h.minValue) h.minValue = h.values[i];
                if (h.values[i] > h.maxValue) h.maxValue = h.values[i];
            }
        }

        // Guarantee a non-zero range so the display never divides by zero.
        if (h.maxValue <= h.minValue) h.maxValue = h.minValue + 1.0f;
    }
}