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

#ifndef PICO_PCF8523_H
# define PICO_PCF8523_H

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"


struct pcf8523_time_t {
  int8_t second;
  int8_t minute;
  int8_t hour;
  int8_t day;
  int8_t month;
  // last two digits of the year eg 23 for 2023
  // this will fall over in 2100
  int8_t year;
  //0..6, 0 is Sunday
  int8_t dotw;
};


/*
 Alarm information, setting a field to -1 will ignore it, otherwise it will be checked.
 For example if you would want an alarm every tuesday at 8:00, you would set it like this
  pcf8523_alarm_t alarm;
  alarm.minute = 0;
  alarm.hour = 8;
  alarm.day = -1;
  alarm.dotw = 2;
*/
struct pcf8523_alarm_t {
  int8_t minute;
  int8_t hour;
  int8_t day;
  int8_t dotw;
};

class PCF8523 {
  public:
    PCF8523();
    void Init(i2c_inst_t *i2cInstance);
    void Reset();
    /* Updates the current time to the RTC */
    bool Write(pcf8523_time_t *time);
    /* Reads the current time from the RTC */
    bool Read(pcf8523_time_t *time);
    /* Enables the interrupt pin (SQW on the adafruit board) that gets triggered by alarms and timers */
    bool EnableInterrupt();
    /* Disables the interrupt pin (SQW on the adafruit board) that gets triggered by alarms and timers */
    bool DisableInterrupt();
    /* Sets up an alarm */
    bool SetAlarm(pcf8523_alarm_t *alarm);
    /* If an alarm is active, this resets it until it hits the time again */
    bool AcknowledgeAlarm();
    /* Checks if an alarm was triggered (and not yet acknowledged) */
    bool CheckAlarm();
    /* Sets a countdown timer that triggers the interrupt in 0 - 255 seconds*/
    bool SetCountdownTimerASeconds(uint8_t seconds);
    /* Checks if countdown timer A was triggered (and not yet acknowledged) */
    bool CheckCountdownTimerA();
    /* Disables any setup TimerA */
    bool DisableTimerA();
    /* If an timer is triggered, this resets it until it hits the time again */
    bool AcknowledgeTimerA();
  private:
    //This stores the requested i2c port
    i2c_inst_t *i2c;
    int i2c_address = 0x68;
    bool ReadRaw(uint8_t *buffer);
    void RawToTime(uint8_t *raw, pcf8523_time_t *time);
    void TimeToRaw(pcf8523_time_t *time, uint8_t *raw);
    bool DisableClockout();
    bool EnableClockout();
    void DebugControl(uint8_t register_address);
    bool ReadControl(uint8_t *buffer, uint8_t register_address);
    bool WriteControl(uint8_t *buffer, uint8_t register_address);
    bool EnableCountdownTimerA();
};

#endif
