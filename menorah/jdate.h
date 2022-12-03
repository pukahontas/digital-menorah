bool isNight(int year, int month, int day, int hour, int minute, double lat, double lng) {
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
  
  double fracYear = 2*PI/(365 + isLeap)*(dayOfYear - 1 + (hour - 12)/24); // Fraction of the year, in radians
  double eqTime = 229.18*(0.000075 + 0.001868*cos(fracYear) - 0.032077*sin(fracYear) - 0.014615*cos(2*fracYear) - 0.040849*sin(2*fracYear)); // Equation of time, in minutes
  double decl = 0.006918 - 0.399912*cos(fracYear) + 0.070257*sin(fracYear) - 0.006758*cos(2*fracYear) + 0.000907*sin(2*fracYear) - 0.002697*cos(3*fracYear) + 0.00148*sin(3*fracYear); // Solar declination angle, in radians
  double hourAngle = acos(cos(90.833*PI/180)/cos(lat*PI/180)/cos(decl) - tan(lat*PI/180)*tan(decl))/PI*180; // Hour angle, in degrees

  double sunrise = 720 - 4*(lng + hourAngle) - eqTime; // Time of sunrise, in minutes past midnight UTC of the local day
  double sunset = 720 - 4*(lng - hourAngle) - eqTime; // TIme of sunset, in minutes past midnight UTC of the local day

  return minute < sunrise || minute > sunset;
}
