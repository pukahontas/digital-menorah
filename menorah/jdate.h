#include <TimeLib.h>

// Uses Hebrew calendar computation code by Edward Reingold and Nachum Dershowitz from https://people.sc.fsu.edu/~jburkardt/cpp_src/calendar_rd/calendar_rd.cpp
#include "calendar_rd.h"

#define NISAN     1
#define IYAR      2
#define SIVAN     3
#define TAMMUZ    4
#define AV        5
#define ELUL      6
#define TISHREI   7
#define CHESHVAN  8
#define KISLEV    9
#define TEVET    10
#define SHEVAT   11
#define ADAR     12

const double HORIZON_ANGLE = 90.833; //The angle of the sun from the zenith when the sun is said to have set or risen, in degrees

bool isNight(double lat, double lng);
bool isNight(double lat, double lng, double &sunrise, double &sunset);
bool isNight(int year, int month, int day, int hour, int minute, int second, double lat, double lng);
bool isNight(int year, int month, int day, int hour, int minute, int second, double lat, double lng, double &sunrise, double &sunset);

bool isNight(double lat, double lng) {
  // Uses the current system time
  return isNight(year(), month(), day(), hour(), minute(), second(), lat, lng);
}

bool isNight(double lat, double lng, double &sunrise, double &sunset) {
  // Uses the current system time
  return isNight(year(), month(), day(), hour(), minute(), second(), lat, lng, sunrise, sunset);
}

bool isNight(int year, int month, int day, int hour, int minute, int second, double lat, double lng) {
  double a, b; // Dummy variables
  return isNight(year, month, day, hour, minute, second, lat, lng, a, b);
}

bool isNight(int year, int month, int day, int hour, int minute, int second, double lat, double lng, double &sunrise, double &sunset) {
  // Calculate sunrise/sunset from tiem equations
  // https://gml.noaa.gov/grad/solcalc/solareqns.PDF
  
  bool isLeap = (year % 4 == 0) && (!(year % 100 == 0) || (year % 400 == 0));
  int dayOfYear = day;
  switch (month) {
    case 12:  dayOfYear += 30;
    case 11:  dayOfYear += 31;
    case 10:  dayOfYear += 30;
    case 9:  dayOfYear += 31;
    case 8:  dayOfYear += 31;
    case 7:  dayOfYear += 30;
    case 6:  dayOfYear += 31;
    case 5:  dayOfYear += 30;
    case 4:  dayOfYear += 31;
    case 3:  dayOfYear += 28 + isLeap;
    case 2:  dayOfYear += 31;
    case 1:  dayOfYear += 0;
    break;
    default: break;
  }
  
  double latRAD = lat*PI/180, lngRAD = lng*PI/180;
  double fracYear = 2*PI/(365 + isLeap)*(dayOfYear - 1 + (hour - 12)/24); // Fraction of the year, in radians
  double eqTime = 229.18*(0.000075 + 0.001868*cos(fracYear) - 0.032077*sin(fracYear) - 0.014615*cos(2*fracYear) - 0.040849*sin(2*fracYear)); // Equation of time, in minutes
  double decl = 0.006918 - 0.399912*cos(fracYear) + 0.070257*sin(fracYear) - 0.006758*cos(2*fracYear) + 0.000907*sin(2*fracYear) - 0.002697*cos(3*fracYear) + 0.00148*sin(3*fracYear); // Solar declination angle, in radians
  double trueSolarTime = hour * 60. + minute + second/60. + eqTime + 4*lng; // true solar time in minutes
  double hourAngleHorizon = acos(cos(HORIZON_ANGLE*PI/180)/cos(latRAD)/cos(decl) - tan(latRAD)*tan(decl))*180/PI; // solar hour angle at the horizon, in degrees
  double zenithAngle = acos(sin(latRAD)*sin(decl) - cos(latRAD)*cos(decl)*cos(trueSolarTime/4*PI/180))*180/PI; //local angle from zenith to sun, in degrees

  sunrise = 720 - 4 * (lng + hourAngleHorizon) - eqTime; // Time of sunrise, in minutes past midnight UTC of the local day
  sunset = 720 - 4 * (lng - hourAngleHorizon) - eqTime; // Time of sunset, in minutes past midnight UTC of the local day

  return zenithAngle > HORIZON_ANGLE; // If the sun is past the horizon, it is nighttime
}

HebrewDate currentHebrewDate(double lat, double lng) {
  // Calculate if the local date is different from the GMT date
  // Add the "longitude time" to the current GMT time and see if it's less than 0 (before midnight)
  double offsetDayFrac = hour() / 24. + minute() / 24 / 60 + lng / 360.;  // Local day fraction
  int dayOffset = floor(offsetDayFrac);                                   // Set the local day to be -1 if the local time is a day behind GMT, +1 if it is a day ahead of GMT or 0 if it is that same
  double localDayFrac = offsetDayFrac - dayOffset;                        // Find the day fraction in local time in the range (0..1)

  // Hebrew date assumes it's before sunset, add a day if it's nighttime and after noon (before noon nighttime means pre-dawn)
  return HebrewDate(GregorianDate(month(), day(), year()) + dayOffset) + (isNight(lat, lng) && localDayFrac > .5);
}

int dayOfHoliday(HebrewDate hDate, int month, int day, int holidayLength) {
  int nthDay = hDate - HebrewDate(month, day, hDate.GetYear());
  if (nthDay >= holidayLength)
    nthDay = hDate - HebrewDate(month, day, hDate.GetYear() + 1);
  return nthDay;
}

int dayOfHanukkah (HebrewDate hDate) {
  return dayOfHoliday(hDate, KISLEV, 25, 8);
}

int dayOfPassover(HebrewDate hDate) {
  return dayOfHoliday(hDate, NISAN, 15, 8);
}

String displayHebrewDate (HebrewDate hDate) {
  return String(hDate.GetDay()) + " / " + String(hDate.GetMonth()) + " / " + String(hDate.GetYear());
}