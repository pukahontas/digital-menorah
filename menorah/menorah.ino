//#include <RTClib.h>
#include <TimeLib.h>
#include <Wire.h>
#include "uTimerLib.h"
#include <Adafruit_GPS.h>
#include "jdate.h"

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
#define SHAMASHG A0
#define SHAMASHB A4

// Color struct
struct Color {
  char red;
  char green;
  char blue;
} candleColor, shamashColor; 

// Define the bytes for if the candles and shamash should be turned on
// These will be set during interrupts, so they are volatile
boolean shamashOn = false;
char candlesOn = 0xAA;
volatile char activeCandle = 0;  // Index of active candle from 0 to 7

// Set global variables for holding GPS data
boolean validTime = false;     // Has the GPS received a valid time solution yet?
boolean validLocation = true;  // Has the GPS computed a valid location yet?
double lat = 47.606209;        // Latitude in fractional degrees, default to Seattle
double lng = -122.332069;      // Longitude in fractional degrees, default to Seattle

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

  // Set up the interrupt that lights the candles
  TimerLib.setInterval_us(displayInterrupt, 2000);

  // Light all candles in turn
  for (candlesOn = 255; candlesOn > 0; candlesOn = candlesOn >> 1) {
    delay(500);
  }
  candlesOn = 0xAA;
  delay(1000);
  candlesOn = 0x55;
  delay(1000);
}

int prevDay = -1000;
time_t statusTime = 0;
void loop() {
  // The main interrupt handler, runs fairly frequently to check if the day has changed
  // or other reasons why the displayed candles should change

  // Check GPS for current time and location, and store it in global variables
  readGPS();

  if (!validTime || !validLocation)
    return;

  int nthDay = dayOfHanukkah(lat, lng);
  if (nthDay < 0) {
    if (prevDay == -1000) { // First time getting the day, display the number of days till Hanukkah
      candlesOn = -nthDay;
      delay(4000);
    }
    candlesOn = 0;
  } else if (nthDay <= 7) {
    candlesOn = (1 << nthDay + 1) - 1; // Set the first [nthDay + 1] candles on
  } else {
    candlesOn = 0;
  }

  // On a new day, say a little prayer
  if (nthDay != prevDay) {
    prevDay = nthDay;
    Serial.println("Barukh atah Adonai");
    Serial.println("Eloheinu melekh ha'olam");
    Serial.println("asher kid'shanu b'mitzvotav");
    Serial.println("v'tzivanu l'hadlik ner shel hanukkah");
  }

  // Display status every once in a while
  if (now() - statusTime > 5) {
    displayStatus();
    statusTime = now();
  }
}

void displayInterrupt() {
  // Switch to the next candle (roll back to 0 if we pass 7)
  if (activeCandle == 0 && shamashOn)
    flickerShamash();
  else
    offShamash();

  nextCandle();
  candlesOn >> activeCandle & 1 ? flicker() : off();

  // Check for GPS signals frequently
  GPS.read();
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
    Serial.print("Set time to: "); Serial.print(GPS.day); Serial.print("/"); Serial.print(GPS.month); Serial.print("/"); Serial.print(GPS.year+2000); Serial.print(" ");
    Serial.print(padDigits(GPS.hour)); Serial.print(":"); Serial.print(padDigits(GPS.minute)); Serial.print(":"); Serial.println(s);

    if (GPS.fix) {
      lat = GPS.latitudeDegrees;
      lng = GPS.longitudeDegrees;
      validLocation = true;
      // Assume that once we have a single fix, that the location stays consistent during the duration.
      // We don't need to invalidate the solution if we ever lose the fix.
    }
  }
}

void displayStatus () {
  Serial.println();
  Serial.print("System time: "); Serial.print(padDigits(hour())); Serial.print(":"); Serial.print(padDigits(minute())); Serial.print(":"); Serial.print(padDigits(second())); 
  Serial.print(" "); Serial.print(day()); Serial.print("/"); Serial.print(month()); Serial.print("/"); Serial.print(year()); Serial.println();
  Serial.print("Location: "); Serial.print(abs(lat), 6); Serial.print(lat < 0 ? " S" : " N"); Serial.print(" LAT, "); Serial.print(abs(lng), 6); Serial.print(lat < 0 ? " W" : " E"); Serial.print(" LONG"); Serial.println();    
  Serial.print("GPS time fix: "); Serial.print(validTime ? "YES" : "NO"); Serial.print(", GPS location fix: "); Serial.println(GPS.fix ? "YES" : "NO");
  Serial.print("Seconds since time/date/location fix: "); Serial.print(GPS.secondsSinceTime()); Serial.print(" / "); Serial.print(GPS.secondsSinceDate()); Serial.print(" / "); Serial.println(GPS.secondsSinceFix());
  Serial.print("Current Hebrew date: "); Serial.println(displayHebrewDate(currentHebrewDate(lat, lng)));
  Serial.print("Day of Hanukkah: "); Serial.println(dayOfHanukkah(lat, lng));
}

String padDigits (int value) {
  return String(value < 0 ? "-" : "") + String(abs(value) <= 9 ? "0" : "") + String(abs(value));
}

// Select the current "candle" to change the color of.
void setCandle(int c) {
  digitalWrite(S1, c & 1 ? HIGH : LOW);
  digitalWrite(S2, c & 2 ? HIGH : LOW);
  digitalWrite(S3, c & 4 ? HIGH : LOW);
}

void nextCandle() {
  activeCandle++;
  if (activeCandle >= 8) activeCandle = 0;
  setCandle(activeCandle);
}

void setColor (char r, char g, char b) {
  candleColor.red = r;
  candleColor.green = g;
  candleColor.blue = b;

  if (r)
    analogWrite(RED, min(r, 255));
  else {
    pinMode(RED, OUTPUT);
    digitalWrite(RED, LOW);
  }

  if (g)
    analogWrite(GREEN, min(g, 255));
  else {
    pinMode(GREEN, OUTPUT);
    digitalWrite(GREEN, LOW);
  }

  if (b)
    analogWrite(BLUE, min(b, 255));
  else {
    pinMode(BLUE, OUTPUT);
    digitalWrite(BLUE, LOW);
  }
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
  int red = random(255);
  int yellow = random(255) + 64;
  int blue = random(255);

  setColor(red, yellow, blue);
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
