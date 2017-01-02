#include <RTClib.h>
#include <TimeLib.h>

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

// Start of the first night of hanukkah
// Format is (YYYY, MM, DD, hh, mm, ss)
const DateTime HANUKKAH_START = DateTime(2016, 12, 24, 16, 22, 00);

void setup() {
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

  //Set system time to time sketch was compiled
  DateTime t = DateTime(F(__DATE__), F(__TIME__));
  setTime(t.hour(), t.minute(), t.second(), t.day(), t.month(), t.year());
  setCandle(0);
}

int v = 0;
int prevDay = -1;
void loop() {
  int days = (now() - HANUKKAH_START.unixtime()) / 86400;
  if (days != prevDay) {
    prevDay = days;
    Serial.println("Barukh atah Adonai");
    Serial.println("Eloheinu melekh ha'olam");
    Serial.println("asher kid'shanu b'mitzvotav");
    Serial.println("v'tzivanu l'hadlik ner shel hanukkah");
  }

  if (days < 0 || days > 7) {
    off();
  } else {
    if (++v > 7)
      v = 0;

    off();
    setCandle(v);
    if (v <= days)
      flicker();

  }
  digitalWrite(SHAMASHR, random(100) < 40 ? LOW : HIGH);
  digitalWrite(SHAMASHG, random(100) < 20 ? LOW : HIGH);
}

void setCandle (int c) {
  digitalWrite(S1, c & 1 ? HIGH : LOW);
  digitalWrite(S2, c & 2 ? HIGH : LOW);
  digitalWrite(S3, c & 4 ? HIGH : LOW);
}

void flicker () {
  digitalWrite(RED, random(100) < 60 ? HIGH : LOW);
  digitalWrite(GREEN, random(100) < 40 ? HIGH : LOW);
  //digitalWrite(BLUE, random(100) < 10 ? HIGH : LOW);
}

void off () {
  digitalWrite(RED, LOW);
  digitalWrite(GREEN, LOW);
  digitalWrite(BLUE, LOW);
}
