# PCF8523 RTC library

## Warning

I don't know what I am doing.

## Description

This is not a full library, just the bits I need. It basically can set & read the time and use the alarm as well as a countdown timer (A) to trigger the INT1 interrupt (to wake up my board)

## Quickstart

### Read & Write the time

```
PCF8523 pcf8523;
pcf8523.Init(i2c_default);

pcf8523_time_t time;
time.second = 40;
time.minute = 32;
time.hour = 20;
time.dotw = 1;
time.day = 21;
time.month = 8;
time.year = 23;

pcf8523.Write(&time);

sleep_ms(5000);

if (!pcf8523.Read(&time)) {
    printf("Error reading the current time\n");
} else {
    printf("Current time: %d:%d:%d %d.%d.%d\n", time.hour, time.minute, time.second, time.day, time.month, time.year);
}
```

### Setting the alarm to trigger the interrupt (it is open drain, so make sure to pull up to VCC and wakup on low edge)

```
PCF8523 pcf8523;
pcf8523.Init(i2c_default);
pcf8523.EnableInterrupt(); // by default they are disabled and clockout is enabled

pcf8523_alarm_t alarm;
alarm.minute = 33;
alarm.hour = 20;
alarm.day = -1; // -1 means ignored, if you want a specific day you could set this, -1 hear means 20:33 every day
alarm.dotw = -1; // -1 means ignored, if you set this to 1 for example it would mean 20:33 every monday

if (!pcf8523.SetAlarm(&alarm)) {
    printf("Error setting up alarm\n");
} else {
    printf("Alarm setup\n");
}

while(true) {
    sleep_ms(5000);
    if (pcf8523.CheckAlarm()) {
        printf("Alarm triggered\n");
        pcf8523.AcknowledgeAlarm(); // clears the alarm flag and sets the INT1 has high resistance again (meaning your edge will go back up to high if pulled up)
    }
}
```

### Setting a countdown timer to trigger the interrupt periodically (it is open drain, so make sure to pull up to VCC and wakup on low edge)

```
PCF8523 pcf8523;
pcf8523.Init(i2c_default);
pcf8523.EnableInterrupt(); // by default they are disabled and clockout is enabled

// trigger the timer every 15 seconds, it will auto reset every time it is triggered and trigger again in 15 seconds
if (!pcf8523.SetCountdownTimerASeconds(15)) {
    printf("Setting timer failed\n");
}

while(true) {
    sleep_ms(5000);
    if (pcf8523.CheckCountdownTimerA()) {
        printf("Timer triggered\n");
        sleep_ms(1000);
        pcf8523.AcknowledgeTimerA(); // clears the timer flag and sets the INT1 has high resistance again (meaning your edge will go back up to high if pulled up)
    }
}
```



