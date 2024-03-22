#include "interrupt.h"
#include <Adafruit_GPS.h>
#include <FlashStorage.h>
#include "jdate.h"
#include "color.h"

// Pinout constants
// 3-to-8 Demux select pins
#define S1 A1
#define S2 A2
#define S3 A3

// Color enable pins (high is on)
const int RED = 9;
const int GREEN = 6;
const int BLUE = 5;

// Shamash color pins (directly controlled, low is on)
const int SHAMASHR = 12;
const int SHAMASHG = 11;
const int SHAMASHB = 10;

// LED for showing fix
#define FIX_LED 13

// Define the bytes for if the candles and shamash should be turned on
// These will be set during interrupts, so they are volatile
bool shamashOn = true;
char candlesOn = 0xAA;
Color* candleColors[9] = {};  // Array holding the candle colors.

volatile char activeCandle = 0;  // Index of active candle from 0 to 7

// Set global variables for holding GPS data
bool validTime = false;     // Has the GPS received a valid time solution yet?
bool validLocation = true;  // Has the GPS computed a valid location yet?
double lat, lng;            // Latitude in fractional degrees

// Set up flash storage for persistent latitude and longitude
FlashStorage(flashLat, float);
FlashStorage(flashLong, float);

// what's the name of the hardware serial port?
#define GPSSerial Serial1

// Connect to the GPS on the hardware port
Adafruit_GPS GPS(&GPSSerial);

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

  pinMode(FIX_LED, OUTPUT);
  digitalWrite(FIX_LED, LOW);

  // Pullup pin 8 to disable LoRa radio module.
  // Only needed if using the LoRa Feather M0
  pinMode(8, OUTPUT);
  digitalWrite(8, HIGH);

  getStoredLocation();

  /*** Setup GPS Module ***/
  // 9600 NMEA is the default baud rate for Adafruit MTK GPS's- some use 4800
  GPS.begin(9600);
  // Turn on RMC (recommended minimum) and GGA (fix data) including altitude
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_100_MILLIHERTZ);  // 10 second update time

  // Set up the interrupt that lights the candles
  createInterrupt(displayInterrupt, 48e6 / 0xFFFF);  // Create interrupt at (48 MHz / 2^16) = 732.4 Hz

  fillColor(Color::WHITE());

  shamashOn = false;
  candlesOn = 0b10000001;
  delay(1000);
  candlesOn = 0b01000010;
  delay(1000);
  candlesOn = 0b00100100;
  delay(1000);
  candlesOn = 0b00011000;
  delay(1000);
  candlesOn = 0;
  shamashOn = true;

  delay(5000);
}

int prevDay = -1000;
time_t statusTime = 0;

void loop() {
  // Check GPS for current time and location, and store it in global variables
  readGPS();

  if (!validTime || !validLocation)
    return;

  HebrewDate hebrewDate = currentHebrewDate(lat, lng);
  int nthDay = dayOfHanukkah(hebrewDate);
  int nthDayPassover = dayOfPassover(hebrewDate);
  int isYomKippur = dayOfHoliday(hebrewDate, TISHREI, 10, 1);
  int isRoshHashana = dayOfHoliday(hebrewDate, TISHREI, 1, 2);
  int isPurim = dayOfHoliday(hebrewDate, LastMonthOfHebrewYear(hebrewDate.GetYear()), 14, 1);
  int isSukkot = dayOfHoliday(hebrewDate, TISHREI, 15, 7);

  // On a new day, say a little prayer and set the colors correctly
  if (nthDay != prevDay) {
    prevDay = nthDay;
    Serial.println("Barukh atah Adonai");
    Serial.println("Eloheinu melekh ha'olam");
    Serial.println("asher kid'shanu b'mitzvotav");
    Serial.println("v'tzivanu l'hadlik ner shel hanukkah");

    if (nthDay >= 0) {
      candlesOn = (1 << nthDay + 1) - 1;  // Set the first [nthDay + 1] candles on
      shamashOn = true;
      fillColor(new Flame());  // if it's Hanukkah set all the candles to flame color
    } else if (nthDayPassover >= 0) {
      candlesOn = (1 << nthDayPassover + 1) - 1;  // Set the first [nthDay + 1] candles on
      shamashOn = false;
      fillColor(Color::BLUE(), Color::WHITE());
      Serial.print("It's day ");
      Serial.print(nthDayPassover + 1);
      Serial.println(" of Passover");
    } else if (isYomKippur >= 0) {
      candlesOn = 0xFF;
      shamashOn = false;
      fillColor(Color::WHITE(), Color::RED());
      Serial.println("Have a solemn Yom Kippur (Tishrei 10)");
    } else if (isRoshHashana >= 0) {
      candlesOn = 0xFF;
      shamashOn = false;
      fillColor(Color::RED(), Color::HSV(30, 255, 255));  // Orange and red
      Serial.println("It's Rosh Hashana. Happy new year! (Tishrei 1)");
    } else if (isPurim >= 0) {
      candlesOn = 0xFF;
      shamashOn = false;
      fillColor(Color::MAGENTA(), Color::YELLOW());
      Serial.println("It's Purim, drink up! (Adar 14)");
    } else if (isSukkot >= 0) {
      candlesOn = (1 << isSukkot + 1) - 1;
      shamashOn = false;
      fillColor(Color::GREEN(), Color::CYAN());
      Serial.println("It's Sukkot (Tishrei 15-21)");
    } else if (nthDay < 0) {  // No current holiday, display the number of days until Hanukkah
      candlesOn = -nthDay;
      shamashOn = -nthDay > 0xFF;
      for (int i = 0; i < 8; i++)
        setColor(i, new Rainbow(i * 45));  // Otherwise set them to rainbow ride
      Serial.print(-nthDay);
      Serial.println(" days until Hanukkah");
    } else {
      candlesOn = 0;
      Serial.println("Something went wrong.");
    }
  }

  // Display status every once in a while
  if (now() - statusTime > 5) {
    displayStatus(hebrewDate);
    statusTime = now();
  }
}

void displayInterrupt() {
  shamashOn ? onShamash() : offShamash();

  off();
  // Switch to the next candle (roll back to 0 if we pass 7)
  activeCandle++;
  activeCandle &= 7;
  digitalWrite(S1, activeCandle & 1 ? HIGH : LOW);
  digitalWrite(S2, activeCandle & 2 ? HIGH : LOW);
  digitalWrite(S3, activeCandle & 4 ? HIGH : LOW);
  on();

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
      digitalWrite(FIX_LED, LOW);
      return;
    }

    // GPS time appears to be valid. Set the system time to GPS time.
    validTime = true;
    float s = GPS.seconds + GPS.milliseconds / 1000. + GPS.secondsSinceTime();  // Offset the time based on how long it's been since the message was received.
    setTime(GPS.hour, GPS.minute, s, GPS.day, GPS.month, GPS.year + 2000);
    Serial.print("Set time to: ");
    Serial.print(GPS.day);
    Serial.print("/");
    Serial.print(GPS.month);
    Serial.print("/");
    Serial.print(GPS.year + 2000);
    Serial.print(" ");
    Serial.print(padDigits(GPS.hour, 2, 0));
    Serial.print(":");
    Serial.print(padDigits(GPS.minute, 2, 0));
    Serial.print(":");
    Serial.println(padDigits(s, 2, 2));

    if (GPS.fix) {
      lat = GPS.latitudeDegrees;
      lng = GPS.longitudeDegrees;
      validLocation = true;
      digitalWrite(FIX_LED, HIGH);
      storeLocation();
      // Assume that once we have a single fix, that the location stays consistent during the duration.
      // We don't need to invalidate the solution if we ever lose the fix.
    }
  }
}

void displayStatus(HebrewDate hebrewDate) {
  Serial.println();
  Serial.print("System time: ");
  Serial.print(padDigits(hour(), 2, 0));
  Serial.print(":");
  Serial.print(padDigits(minute(), 2, 0));
  Serial.print(":");
  Serial.print(padDigits(second(), 2, 0));
  Serial.print(" ");
  Serial.print(day());
  Serial.print("/");
  Serial.print(month());
  Serial.print("/");
  Serial.print(year());
  Serial.println();
  Serial.print("Location: ");
  Serial.print(abs(lat), 6);
  Serial.print(lat < 0 ? " S" : " N");
  Serial.print(" LAT, ");
  Serial.print(abs(lng), 6);
  Serial.print(lat < 0 ? " W" : " E");
  Serial.print(" LONG");
  Serial.println();
  Serial.print("GPS time fix: ");
  Serial.print(validTime ? "YES" : "NO");
  Serial.print(", GPS location fix: ");
  Serial.println(GPS.fix ? "YES" : "NO");
  Serial.print("Seconds since time/date/location fix: ");
  Serial.print(GPS.secondsSinceTime());
  Serial.print(" / ");
  Serial.print(GPS.secondsSinceDate());
  Serial.print(" / ");
  Serial.println(GPS.secondsSinceFix());
  double sunrise, sunset;
  isNight(lat, lng, sunrise, sunset);
  Serial.print("Sunset at ");
  Serial.print(padDigits(sunset / 60., 2, 0));
  Serial.print(":");
  Serial.print(padDigits((int)sunset % 60, 2, 0));
  Serial.print(":");
  Serial.print(padDigits((sunset - floor(sunset)) * 60, 2, 0));
  Serial.print(", Sunrise at ");
  Serial.print(padDigits(sunrise / 60., 2, 0));
  Serial.print(":");
  Serial.print(padDigits((int)sunrise % 60, 2, 0));
  Serial.print(":");
  Serial.println(padDigits((sunrise - floor(sunrise)) * 60, 2, 0));
  Serial.print("Current Hebrew date: ");
  Serial.println(displayHebrewDate(hebrewDate));
  Serial.print("Day of Hanukkah: ");
  Serial.println(dayOfHanukkah(hebrewDate));
}

String padDigits(float value, int padIntegral, int decimalDigits) {
  int integral = (int)abs(value); // Integral part of the value
  int frac = (int)((abs(value) - integral) * pow(10, decimalDigits)); // Fractional part, rounded to decimalDigits places
  String whole = String(integral);
  while (whole.length() < padIntegral)
    whole = " " + whole;

  return String(value < 0 ? "-" : "") + whole + (frac > 0 ? String(".") + String(frac) : "");
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

  digitalWrite(SHAMASHR, HIGH);
  digitalWrite(SHAMASHG, HIGH);
  digitalWrite(SHAMASHB, HIGH);
}

void on() {
  if (candlesOn >> activeCandle & 1) {
    Color* color = candleColors[activeCandle];
    int r = color->red();
    int g = color->green();
    int b = color->blue();

    analogWrite(RED, r);
    analogWrite(GREEN, g);
    analogWrite(BLUE, b);
  }
}

void onShamash() {
  Color* color = candleColors[8];
  int r = color->redShamash();
  int g = color->greenShamash();
  int b = color->blueShamash();

  if (r > 0) {
    analogWrite(SHAMASHR, 255 - r);
  } else {
    pinMode(SHAMASHR, OUTPUT);
    digitalWrite(SHAMASHR, HIGH);
  }
  if (g > 0) {
    analogWrite(SHAMASHG, 255 - g);
  } else {
    pinMode(SHAMASHG, OUTPUT);
    digitalWrite(SHAMASHG, HIGH);
  }
  if (b > 0) {
    analogWrite(SHAMASHB, 255 - b);
  } else {
    pinMode(SHAMASHB, OUTPUT);
    digitalWrite(SHAMASHB, HIGH);
  }
}

void setColor(int candle, Color* color) {
  candleColors[candle] = color;
}

// Set the candles with color
void fillColor(Color* a, Color* b, Color* c, Color* d, Color* e, Color* f, Color* g, Color* h, Color* i) {
  setColor(0, a);
  setColor(1, b);
  setColor(2, c);
  setColor(3, d);
  setColor(4, e);
  setColor(5, f);
  setColor(6, g);
  setColor(7, h);
  setColor(8, i);
}

// Add overloaded methods to fill with repeating colors
void fillColor(Color* a) {
  fillColor(a, a, a, a, a, a, a, a, a);
}
void fillColor(Color* a, Color* b) {
  fillColor(a, b, a, b, a, b, a, b, a);
}
void fillColor(Color* a, Color* b, Color* c) {
  fillColor(a, b, c, a, b, c, a, b, c);
}
void fillColor(Color* a, Color* b, Color* c, Color* d) {
  fillColor(a, b, c, d, a, b, c, d, a);
}
void fillColor(Color* a, Color* b, Color* c, Color* d, Color* e) {
  fillColor(a, b, c, d, e, a, b, c, d);
}
void fillColor(Color* a, Color* b, Color* c, Color* d, Color* e, Color* f) {
  fillColor(a, b, c, d, e, f, a, b, c);
}
void fillColor(Color* a, Color* b, Color* c, Color* d, Color* e, Color* f, Color* g) {
  fillColor(a, b, c, d, e, f, g, a, b);
}
void fillColor(Color* a, Color* b, Color* c, Color* d, Color* e, Color* f, Color* g, Color* h) {
  fillColor(a, b, c, d, e, f, g, h, a);
}

// Check EEPROM to see if the last known location is stored.
void getStoredLocation() {
  lat = flashLat.read();
  lng = flashLong.read();
  Serial.print("Read location from flash memory: ");
  Serial.print(lat);
  Serial.print(" lat, ");
  Serial.print(lng);
  Serial.println(" long.");
}

void storeLocation() {
  float readLat = flashLat.read();
  float readLong = flashLong.read();

  // To prevent excessive writes from wearing out flash memory, only write if the GPS has a fix
  // and if the location changes by more than 1 degree
  if (GPS.fix) {
    if (abs(readLat - lat) > 1 || abs(readLong - lng) > 1) {
      flashLat.write(lat);
      flashLong.write(lng);
      Serial.print("Wrote location to flash memory: ");
      Serial.print(lat);
      Serial.print(" lat, ");
      Serial.print(lng);
      Serial.println(" long.");
    }
  }
}