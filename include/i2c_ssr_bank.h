#pragma once

/** 8-bit I2C GPIO expander (PCF8574 family) driving an 8-channel SSR / relay bank. */
void setup_i2c_ssr_bank();
/** Poll USB serial: keys 0-7 toggle channels, 'a' all off, '?' help. */
void loop_i2c_ssr_bank_serial();
/** Blink SSR channel 1 on/off every 500ms for testing. */
void loop_i2c_ssr_bank_blink_test();
