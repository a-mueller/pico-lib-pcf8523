/*
  This is a library written for the PCF8523
  Copyright (c) 2023 Abraham Mueller

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

#include "pico_pcf8523.hpp"


PCF8523::PCF8523() {}

void PCF8523::Init(i2c_inst_t *i2cInstance) {
  i2c = i2cInstance;
  Reset();
  DebugControl(0x00);
  DebugControl(0x01);
  DebugControl(0x02);
  DebugControl(0x0F);
  DebugControl(0x10);
  DebugControl(0x12);
}


/* Sends the reset command, see datasheet in what state we are in afterwards */
void PCF8523::Reset() {
  uint8_t buf[] = {0x00, 0x58};
  i2c_write_blocking(i2c, i2c_address, buf, 2, false);
}

/* Print the current value of a register */
void PCF8523::DebugControl(uint8_t register_address) {
  uint8_t current_value;
  if (ReadControl(&current_value, register_address)) {
    printf("0x%02x register current value: 0x%02x\n", register_address, current_value);
  } else {
    printf("No answer from register: 0x%02x\n", register_address);
  }
}

/* Updates the current time to the RTC */
bool PCF8523::Write(pcf8523_time_t *time) {
  // buf[0] is the register to write to
    // buf[1] is the value that will be written to the register
    uint8_t buf[2];

    //Write values for the current time in the array
    //index 0 -> second: bits 4-6 are responsible for the ten's digit and bits 0-3 for the unit's digit
    //index 1 -> minute: bits 4-6 are responsible for the ten's digit and bits 0-3 for the unit's digit
    //index 2 -> hour: bits 4-5 are responsible for the ten's digit and bits 0-3 for the unit's digit
    //index 3 -> day of the month: bits 4-5 are responsible for the ten's digit and bits 0-3 for the unit's digit
    //index 4 -> day of the week: where Sunday = 0x00, Monday = 0x01, Tuesday... ...Saturday = 0x06
    //index 5 -> month: bit 4 is responsible for the ten's digit and bits 0-3 for the unit's digit
    //index 6 -> year: bits 4-7 are responsible for the ten's digit and bits 0-3 for the unit's digit

    //NOTE: if the value in the year register is a multiple for 4, it will be considered a leap year and hence will include the 29th of February

    uint8_t current_val[7];
    TimeToRaw(time, current_val);

    for (int i = 3; i < 10; ++i) {
        buf[0] = i;
        buf[1] = current_val[i - 3];
        i2c_write_blocking(i2c, i2c_address, buf, 2, false);
    }
   return true; 
}

bool PCF8523::ReadRaw(uint8_t *buffer) {
  // For this particular device, we send the device the register we want to read
  // first, then subsequently read from the device. The register is auto incrementing
  // so we don't need to keep sending the register we want, just the first.

  // Start reading acceleration registers from register 0x3B for 6 bytes
  uint8_t val = 0x03;
  i2c_write_blocking(i2c, i2c_address, &val, 1, true); // true to keep master control of bus
  int result = i2c_read_blocking(i2c, i2c_address, buffer, 7, false);
  return (result == 7);
}


/* Reads the current time from the RTC */
bool PCF8523::Read(pcf8523_time_t *time) {
  uint8_t raw_time[7];
  if (!ReadRaw(raw_time)) {
    return false;
  }

  RawToTime(raw_time, time);

  return true;
}

/* Convert the raw bytes from the RTC into numbers we can understand */
void PCF8523::RawToTime(uint8_t *raw_time, pcf8523_time_t *time) {
    time -> second = (10 * (int8_t) ((raw_time[0] & 0x70) >> 4)) + ((int8_t) (raw_time[0] & 0x0F));
    time -> minute = (10 * (int8_t) ((raw_time[1] & 0x70) >> 4)) + ((int8_t) (raw_time[1] & 0x0F));
    time -> hour = (10 * (int8_t) ((raw_time[2] & 0x30) >> 4)) + ((int8_t) (raw_time[2] & 0x0F));
    time -> day = (10 * (int8_t) ((raw_time[3] & 0x30) >> 4)) + ((int8_t) (raw_time[3] & 0x0F));
    time -> dotw = (int8_t) (raw_time[4] & 0x07);
    time -> month = (10 * (int8_t) ((raw_time[5] & 0x10) >> 4)) + ((int8_t) (raw_time[5] & 0x0F));
    time -> year = (10 * (int8_t) ((raw_time[6] & 0xF0) >> 4)) + ((int8_t) (raw_time[6] & 0x0F));
}

/* Convert the time from somewhere into raw bytes the RTC can understand */
void PCF8523::TimeToRaw(pcf8523_time_t *time, uint8_t *raw) {
    raw[0] = ((time -> second / 10) << 4) | ((time -> second % 10) & 0x0F);
    raw[1] = ((time -> minute / 10) << 4) | ((time -> minute % 10) & 0x0F);
    raw[2] = ((time -> hour / 10) << 4) | ((time -> hour %10) & 0x0F);
    raw[3] = ((time -> day / 10) << 4) | ((time -> day % 10) & 0x0F);
    raw[4] = time -> dotw & 0x07;
    raw[5] = ((time -> month / 10) << 4) | ((time -> month % 10) & 0x0F);
    raw[6] = ((time -> year / 10) << 4) | ((time -> year % 10) & 0x0F);
}

/* Disables the interrupt pin (SQW on the adafruit board) that gets triggered by alarms and timers */
bool PCF8523::DisableInterrupt() {
  if (!EnableClockout()) {
    return false;
  }
  uint8_t current_value;
  if (ReadControl(&current_value, 0x00)) {
    current_value &= 0xF9;
    // disable alarm interrupts
    if (!WriteControl(&current_value, 0x00)) {
      return false;
    }

    // read register control2 and disable all three watchdog & countdown interrupts
    if (!ReadControl(&current_value, 0x01)) {
      return false;
    }

    current_value |= 0xF8; // binary 11111000 (disable all watchdog & countdown interrupts)
    return WriteControl(&current_value, 0x01);

  } else {
    return false;
  }
}

/* Enables the interrupt pin (SQW on the adafruit board) that gets triggered by alarms and timers */
bool PCF8523::EnableInterrupt() {
  if (!DisableClockout()) {
    return false;
  }

  uint8_t current_value;
  if (ReadControl(&current_value, 0x00)) {
    current_value |= 0x02;
    if (!WriteControl(&current_value, 0x00)) {
      return false;
    }

    // read & update the control2 register
    if (!ReadControl(&current_value, 0x01)) {
      return false;
    }

    current_value |= 0x07; // binary 00000111 (enable all watchdog & countdown interrupts)
    return WriteControl(&current_value, 0x01);

  } else {
    return false;
  }
}

/*
 Clears the AF bit in control register 2 which disables the interrupt and alarm state, but keeps the alarm for next time it hits the condition 
*/
bool PCF8523::AcknowledgeAlarm() {
  uint8_t current_value;
  if (ReadControl(&current_value, 0x01)) {
    current_value &= 0xF7; // binary 11110111
    return WriteControl(&current_value, 0x01);
  } else {
    return false;
  }
}

/*
  Reads the current value of the control register into the buffer
*/
bool PCF8523::ReadControl(uint8_t *buffer, uint8_t register_address) {
  i2c_write_blocking(i2c, i2c_address, &register_address, 1, true); // true to keep master control of bus
  int result = i2c_read_blocking(i2c, i2c_address, buffer, 1, false);
  return (result == 1);
}

/*
  Updates the control register with buffer
*/
bool PCF8523::WriteControl(uint8_t *buffer, uint8_t register_address) {
  uint8_t buf[] = {register_address, *buffer};
  int result = i2c_write_blocking(i2c, i2c_address, buf, 2, false);
  return (result == 2);
}

/*
  Checks whether the alarm is active and hasn't been acknowledged yet
*/
bool PCF8523::CheckAlarm() {
      // Check bit 3 of control register 2 for alarm flags
    uint8_t status[1];
    uint8_t val = 0x01;
    i2c_write_blocking(i2c, i2c_address, &val, 1, true); // true to keep master control of bus
    i2c_read_blocking(i2c, i2c_address, status, 1, false);

    if ((status[0] & 0x08) == 0x08) {
        printf("ALARM RINGING");
        return true;
    } else {
        printf("Alarm not triggered yet");
        return false;
    }
}

/*
  Updates the alarm condition. set all fields to -1 to disable it completely
*/
bool PCF8523::SetAlarm(pcf8523_alarm_t *alarm) {
    uint8_t buf[2]; // first byte is the register address, second byte the value

    // Minute_alarm register
    buf[0] = 0x0A; 
    if (alarm -> minute == -1) {
      // disable
      buf[1] = 0x80; // 10000000 in bits
    } else {
      buf[1] = (alarm -> minute / 10) << 4; // tens digits
      buf[1] &= 0x7F; // 01111111 in bits (set first bit to 0)
      buf[1] |= ((alarm -> minute % 10) & 0x0F); // single digits
    }
    int result = i2c_write_blocking(i2c, i2c_address, buf, 2, false);
    if (result != 2) {
      return false;
    }

    // Hour_alarm register
    buf[0] = 0x0B; 
    if (alarm -> hour == -1) {
      // disable
      buf[1] = 0x80; // 10000000 in bits
    } else {
      // we don't allow to set the clock to am/pm mode, if this is allowed, we would need to check in which mode we are and create this field differently for am/pm mode
      buf[1] = (alarm -> hour / 10) << 4; // tens digits
      buf[1] &= 0x7F; // 01111111 in bits (set first bit to 0)
      buf[1] |= ((alarm -> hour % 10) & 0x0F); // single digits
    }
    result = i2c_write_blocking(i2c, i2c_address, buf, 2, false);
    if (result != 2) {
      return false;
    }

    // Day_alarm register
    buf[0] = 0x0C; 
    if (alarm -> day == -1) {
      // disable
      buf[1] = 0x80; // 10000000 in bits
    } else {
      // we don't allow to set the clock to am/pm mode, if this is allowed, we would need to check in which mode we are and create this field differently for am/pm mode
      buf[1] = (alarm -> day / 10) << 4; // tens digits
      buf[1] &= 0x7F; // 01111111 in bits (set first bit to 0)
      buf[1] |= ((alarm -> day % 10) & 0x0F); // single digits
    }
    result = i2c_write_blocking(i2c, i2c_address, buf, 2, false);
    if (result != 2) {
      return false;
    }

    // Weekday_alarm
    buf[0] = 0x0D; 
    if (alarm -> dotw == -1) {
      // disable
      buf[1] = 0x80; // 10000000 in bits
    } else {
      buf[1] = alarm -> dotw; // single digits
      buf[1] &= 0x07; // 00000111 in bits (set all bits to 0 except the last 3)
    }
    result = i2c_write_blocking(i2c, i2c_address, buf, 2, false);
    if (result != 2) {
      return false;
    }

    return true;
}

bool PCF8523::SetCountdownTimerASeconds(uint8_t seconds) {
    // first enable the timer function
    if (!EnableCountdownTimerA()) {
      printf("Enable timer a failed\n");
      return false;
    }
    // set the clock to 1hz

    uint8_t oneHertzClock = 0x02; //00000010 binary 1HZ clock
    if (!WriteControl(&oneHertzClock, 0x10)) {
      printf("Setting frequency failed\n");
      return false;
    }

    uint8_t buffer = seconds;
    bool success =  WriteControl(&buffer, 0x11);
    DebugControl(0x0F);
    DebugControl(0x10);
    DebugControl(0x11);

    return success;
}

/* Enables the timerA as countdown timer*/
bool PCF8523::EnableCountdownTimerA() {
    // first enable the timer function
    uint8_t current_value;
    if (ReadControl(&current_value, 0x0F)) {
      current_value |= 0x02; //binary 00000010 (set bit 2 to 1)
      current_value &= 0xFB; //binary 11111011 (set bit 3 to 0)
      return WriteControl(&current_value, 0x0F);
    } else {
      return false;
    }
}

/* Disables timerA */
bool PCF8523::DisableTimerA() {
    uint8_t current_value;
    if (ReadControl(&current_value, 0x0F)) {
        current_value &= 0xF9; //binary 11111001 (set bit 3 and 2 to 0)
        return WriteControl(&current_value, 0x0F);
    } else {
      return false;
    }
}

/* Checks if countdown timer A was triggered */
bool PCF8523::CheckCountdownTimerA() {
    uint8_t current_value;
    if (ReadControl(&current_value, 0x01)) {
        return (current_value & 0x40) == 0x40; // binary 01000000 (bit 6)
    } else {
      return false;
    }
}

/* If an timer is triggered, this resets it until it hits the time again */
bool PCF8523::AcknowledgeTimerA() {
    uint8_t current_value;
    if (ReadControl(&current_value, 0x01)) {
        current_value &= 0xBF; // binary 10111111 (clear bit 6)
        return WriteControl(&current_value, 0x01);
    } else {
      return false;
    }
}

/* Enables the clockout instead the INT1 interrupt */
bool PCF8523::EnableClockout() {
  uint8_t current_value;
  if (ReadControl(&current_value, 0x0F)) {
    current_value &= 0xC7; //binary 11000111 (set clockout bits to 000 (default))
    return WriteControl(&current_value, 0x0F);
  } else {
    return false;
  }
}

/* Disables the clockout so we can use the INT1 interrupt */
bool PCF8523::DisableClockout() {
  uint8_t current_value;
  if (ReadControl(&current_value, 0x0F)) {
    current_value |= 0x38; //binary 00111000 (set clockout bits to 111 (disable))
    return WriteControl(&current_value, 0x0F);
  } else {
    return false;
  }
}


