//#include <RTClib.h>
#include <TimeLib.h>
#include <Wire.h>
#include "uTimerLib.h"
#include <Adafruit_GPS.h>
#include "jdate.h"

// Uses Hebrew calendar computation code by Edward Reingold and Nachum Dershowitz from https://people.sc.fsu.edu/~jburkardt/cpp_src/calendar_rd/calendar_rd.cpp
#include "calendar_rd.h"

// what's the name of the hardware serial port?
#define GPSSerial Serial1

// Connect to the GPS on the hardware port
Adafruit_GPS GPS(&GPSSerial);

// Pinout constants
// 3-to-8 Demux select pins
#define S1 10
#define S2 11
#define S3 12
// Color enable pins (high is on)
#define RED A1
#define GREEN A3
#define BLUE A5
// Shamash color pins (directly controlled)
#define SHAMASHR A2
#define SHAMASHG A4
#define SHAMASHB 13

// Define the bytes for if the candles and shamash should be turned on
// These will be set during interrupts, so they are volatile
volatile boolean shamashOn = false;
volatile char candlesOn = 0x00;
volatile int dayOfHanukkah = 0;

// Set global variables for holding GPS data
volatile boolean validTime = false;     // Has the GPS received a valid time solution yet?
volatile boolean validLocation = true;  // Has the GPS computed a valid location yet?
volatile int nighttime = -1;             // 0 = day, 1 = night, other = unknown
volatile double lat = 47.606209;        // Latitude in fractional degrees, default to Seattle
volatile double lng = -122.332069;      // Longitude in fractional degrees, default to Seattle

void setup() {
  Serial.begin(115200);  //Set serial baud rate

  // Setup pins
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);

  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(BLUE, OUTPUT);

  pinMode(SHAMASHR, OUTPUT);
  pinMode(SHAMASHG, OUTPUT);
  pinMode(SHAMASHB, OUTPUT);

  digitalWrite(SHAMASHR, HIGH);
  digitalWrite(SHAMASHG, HIGH);
  digitalWrite(SHAMASHB, HIGH);

  /*** Setup GPS Module ***/
  // 9600 NMEA is the default baud rate for Adafruit MTK GPS's- some use 4800
  GPS.begin(9600);
  // Turn on RMC (recommended minimum) and GGA (fix data) including altitude
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_100_MILLIHERTZ);  // 10 second update time

  post();
}

void loop() {
  // Switch to the next candle (roll back to 0 if we pass 7)
  for (char i = 1, n = 0; n < 8; i *= 2, n++) {
    setCandle(n);
    if (i & candlesOn) flicker();

    delay(2);
    off();
  }

  if (shamashOn)
    flickerShamash();
  delay(2);
  offShamash();

  GPS.read();
}
volatile int prevDay = -1;
void mainInterrupt() {
  off();
  // The main interrupt handler, runs fairly frequently to check if the day has changed
  // or other reasons why the displayed candles should change

  // Check GPS for current time and location, and store it in global variables
  readGPS();

  if (!validTime || !validLocation)
    return;

  if (dayOfHanukkah < 0)
    candlesOn = -dayOfHanukkah;
  else if (dayOfHanukkah > 7) {
    candlesOn = 0;
  }
  // On a new day, say a little prayer
  if (dayOfHanukkah != prevDay) {
    prevDay = dayOfHanukkah;
    Serial.println("Barukh atah Adonai");
    Serial.println("Eloheinu melekh ha'olam");
    Serial.println("asher kid'shanu b'mitzvotav");
    Serial.println("v'tzivanu l'hadlik ner shel hanukkah");
  }
}

void readGPS() {
  // if a sentence is received, we can check the checksum, parse it...
  if (GPS.newNMEAreceived()) {
    Serial.println(GPS.lastNMEA());
    if (!GPS.parse(GPS.lastNMEA()))
      return;  // we can fail to parse a sentence in which case we should just wait for the next one

    if (GPS.year < 22 || GPS.year >= 80) {  // If the GPS time is outside the range 2022 to 2080, assume we don't have the right time
      validTime = false;
      return;
    }

    // GPS time appears to be valid. Set the system time to GPS time.
    validTime = true;
    float s = GPS.seconds + GPS.milliseconds / 1000. + GPS.secondsSinceTime();  // Offset the time based on how long it's been since the message was received.
    setTime(GPS.hour, GPS.minute, s, GPS.day, GPS.month, GPS.year + 2000);

    if (GPS.fix) {
      lat = GPS.latitudeDegrees;
      lng = GPS.longitudeDegrees;
      Serial.print(lat);
      Serial.print(" latitude ");
      Serial.print(lng);
      Serial.println(" longitude");
      validLocation = true;
      // Assume that once we have a single fix, that the location stays consistent during the duration.
      // We don't need to invalidate the solution if we ever lose the fix.
    }
  }

  boolean getNight = isNight(lat, lng);
  if (nighttime != getNight) {  // Nighttime status has changed, recalculate number of days
    // Calculate if the local date is different from the GMT date
    // Add the "longitude time" to the current GMT time and see if it's less than 0 (before midnight)
    double offsetDayFrac = hour() / 24. + minute() / 24 / 60 + lng / 360.; // Local day fraction
    int dayOffset = floor(offsetDayFrac); // Set the local day to be -1 if the local time is a day behind GMT, +1 if it is a day ahead of GMT or 0 if it is that same 
    double localDayFrac = offsetDayFrac - dayOffset; // Find the day fraction in local time in the range (0..1)

    // Hebrew date assumes it's before sunset, add a day if it's nighttime and after noon (before noon nighttime means pre-dawn)
    HebrewDate currentHebrewDate = HebrewDate(GregorianDate(month(), day(), year()) + dayOffset) + (getNight && localDayFrac > .5);

    dayOfHanukkah = currentHebrewDate - HebrewDate(9, 25, currentHebrewDate.GetYear()) ;

    Serial.println(dayOffset); Serial.println(localDayFrac);Serial.println(getNight);
    Serial.print(currentHebrewDate.GetDay());Serial.print("/");Serial.print(currentHebrewDate.GetMonth());Serial.print("/");Serial.println(currentHebrewDate.GetYear());
    Serial.print(dayOfHanukkah);
    Serial.println(" day of Hanukkah");
  }
  nighttime = getNight;
}

void post() {
  // Light all candles in order
  candlesOn = candlesOn * 2 + 1;

  if (candlesOn == 255)  // The char has fully filled
    TimerLib.setTimeout_s(post2, 1);
  else
    TimerLib.setTimeout_s(post, 1);
}

// Power On Self Test
void post2() {
  // Find number of days until the *next* hanukkah and write it out in binary
  // Shamash is MSB
  if (false) {
    TimerLib.setTimeout_s(post3, 4);  // wait 4 seconds
  } else
    post3();  // Go immediately
}

void post3() {
  TimerLib.setInterval_s(mainInterrupt, 2);  // Start main interrupt function
}

// Select the current "candle" to change the color of.
void setCandle(int c) {
  digitalWrite(S1, c & 1 ? HIGH : LOW);
  digitalWrite(S2, c & 2 ? HIGH : LOW);
  digitalWrite(S3, c & 4 ? HIGH : LOW);
}

// Flicker the current candle
// (i.e, probablistically set the candle to off or on [in a reddish-yellow color])
void flicker() {
  flicker(false);
}
void flickerShamash() {
  flicker(true);
}
void flicker(boolean shamash) {
  int red = random(128);
  int yellow = random(255) + 64;
  int blue = random(40);

  analogWrite(shamash ? SHAMASHR : RED, min(yellow, 255));
  analogWrite(shamash ? SHAMASHG : GREEN, min(yellow, 255));
  analogWrite(shamash ? SHAMASHB : BLUE, blue);
}

// Turn current candle off
void off() {
  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(BLUE, OUTPUT);

  digitalWrite(RED, LOW);
  digitalWrite(GREEN, LOW);
  digitalWrite(BLUE, LOW);
}

void offShamash() {
  pinMode(SHAMASHR, OUTPUT);
  pinMode(SHAMASHG, OUTPUT);
  pinMode(SHAMASHB, OUTPUT);

  digitalWrite(SHAMASHR, LOW);
  digitalWrite(SHAMASHG, LOW);
  digitalWrite(SHAMASHB, LOW);
}
