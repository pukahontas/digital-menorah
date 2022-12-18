#include <TimeLib.h>

const double HORIZON_ANGLE = 90.833; //The angle of the sun from the zenith when the sun is said to have set or risen, in degreestgggggggggggggggggggggggggggggggggggggggggg

bool isNight(double lat, double lng);
bool isNight(int year, int month, int day, int hour, int minute, int second, double lat, double lng);

bool isNight(double lat, double lng) {
  // Uses the current system time
  return isNight(year(), month(), day(), hour(), minute(), second(), lat, lng);
}

bool isNight(int year, int month, int day, int hour, int minute, int second, double lat, double lng) {
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

  double sunrise = 720 - 4 * (lng + hourAngleHorizon) - eqTime; // Time of sunrise, in minutes past midnight UTC of the local day
  double sunset = 720 - 4 * (lng - hourAngleHorizon) - eqTime; // TIme of sunset, in minutes past midnight UTC of the local day

  Serial.print(day); Serial.print("/"); Serial.print(month); Serial.print("/"); Serial.print(year);Serial.print(" ");
  Serial.print(hour); Serial.print(":"); Serial.print(minute); Serial.print(":"); Serial.print(second);Serial.print(", ");
  Serial.print(lat); Serial.print(" lat "); Serial.print(lng); Serial.print(" long");
  Serial.print(", sunset at "); Serial.print((int)(sunset / 60)); Serial.print(":"); Serial.print(((int)sunset) % 60);
  Serial.print(", sunrise at "); Serial.print((int)(sunrise / 60)); Serial.print(":"); Serial.println(((int)sunrise) % 60);
  Serial.print("Solar zenith angle: "); Serial.println(zenithAngle);

  return zenithAngle > HORIZON_ANGLE; // If the sun is past the horizon, it is nighttime
}
