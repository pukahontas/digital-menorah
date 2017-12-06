#include <RTClib.h>
#include <TimeLib.h>
#include <Wire.h>

RTC_DS1307 rtc;

// Pinout constants
// 3-to-8 Demux select pins
#define S1 10
#define S2 11
#define S3 12
// Color enable pins (high is on)
#define RED A1
#define GREEN  A3
#define BLUE A5
// Shamash color pins (directly controlled)
#define SHAMASHR A2
#define SHAMASHG A4
#define SHAMASHB 13

// Boolean, is the RTC available for accurate timekeeping
int RTCenabled = false;

// Start of the first night of hanukkah
// Format is (YYYY, MM, DD, hh, mm, ss)
#define HANUKKAH_START_LENGTH 10
const DateTime HANUKKAH_START[] = {
   DateTime(2017, 12, 12, 16, 17, 00), // 2017
   DateTime(2018, 12, 2, 16, 19, 00),  // 2018
   DateTime(2019, 12, 22, 16, 21, 00), // 2019
   DateTime(2020, 12, 10, 16, 17, 00), // 2020
   DateTime(2021, 11, 28, 16, 20, 00), // 2021
   DateTime(2022, 12, 18, 16, 19, 00), // 2022
   DateTime(2023, 12, 8, 16, 17, 00),  // 2023
   DateTime(2024, 12, 25, 16, 23, 00), // 2024
   DateTime(2025, 12, 14, 16, 18, 00), // 2025
   DateTime(2026, 12, 4, 16, 18, 00)   // 2026
};
int hanukkahStartIndex = 0; 

void setup() {
  // Setup pins
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);

  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(BLUE, OUTPUT);

  pinMode (SHAMASHR, OUTPUT);
  pinMode (SHAMASHG, OUTPUT);
  pinMode (SHAMASHB, OUTPUT);

  digitalWrite(SHAMASHR, HIGH);
  digitalWrite(SHAMASHG, HIGH);
  digitalWrite(SHAMASHB, HIGH);

  
  if (rtc.begin()) {
    RTCenabled = true;
    
    if (!rtc.isrunning()) {
      // Set the RTC to the date & time this sketch was compiled
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
  } else {
    //Set system time to time sketch was compiled
    DateTime t = DateTime(F(__DATE__), F(__TIME__));
    setTime(t.hour(), t.minute(), t.second(), t.day(), t.month(), t.year());
  }

  // Set up to start at the most recent year
  for (hanukkahStartIndex = 0; hanukkahStartIndex < HANUKKAH_START_LENGTH; hanukkahStartIndex++) {
    if (daysBetween(HANUKKAH_START[hanukkahStartIndex], getTime()) < 8)
      break;       
  }
  
  setCandle(0);
}

int v = 0;
int prevDay = -1;

void loop() {
  // Loop through Hannukah start times to see which one is the most recently started
  int days = daysBetween(HANUKKAH_START[hanukkahStartIndex], getTime());

  if (days < 0)
    off();
  else if (days > 7) {
    off();    
    // After the eight day, switch to the next year
    hanukkahStartIndex++;
  } else {
    // On a new day, say a little prayer
    if (days != prevDay) {
      prevDay = days;
      Serial.println("Barukh atah Adonai");
      Serial.println("Eloheinu melekh ha'olam");
      Serial.println("asher kid'shanu b'mitzvotav");
      Serial.println("v'tzivanu l'hadlik ner shel hanukkah");
    }

    // Switch to the next candle (roll back to 0 if we pass 7)
    if (++v > 7)
      v = 0;

    off();
    setCandle(v);
    if (v <= days)
      flicker();

  }

  // Flicker the shamash
  digitalWrite(SHAMASHR, random(100) < 40 ? LOW : HIGH);
  digitalWrite(SHAMASHG, random(100) < 20 ? LOW : HIGH);
}

// Select the current "candle" to change the color of.
void setCandle (int c) {
  digitalWrite(S1, c & 1 ? HIGH : LOW);
  digitalWrite(S2, c & 2 ? HIGH : LOW);
  digitalWrite(S3, c & 4 ? HIGH : LOW);
}

// Flicker the current candle
// (i.e, probablistically set the candle to off or on [in a reddish-yellow color])
void flicker () {
  digitalWrite(RED, random(100) < 60 ? HIGH : LOW);
  digitalWrite(GREEN, random(100) < 40 ? HIGH : LOW);
  //digitalWrite(BLUE, random(100) < 10 ? HIGH : LOW);
}

// Turn current candle off
void off () {
  digitalWrite(RED, LOW);
  digitalWrite(GREEN, LOW);
  digitalWrite(BLUE, LOW);
}

// Get the current time
// If the RTC is available, use its current time
// Otherwise, use an estimate from the system clock.
DateTime getTime () {
  if (RTCenabled)
    return rtc.now();
  else
    return now();
}

// Calculate the days from the first date to the other
// If the second date happens before the first, the result will be negative.
int daysBetween (DateTime a, DateTime b) {
  // Declare helper variables to convert to signed longs
  long aL = a.unixtime();
  long bL = b.unixtime();
  return (bL - aL) / 86400 - (aL > bL);
}
