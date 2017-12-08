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

// Array to hold candle status
#define SHAMASH 0
boolean candle[] = {0, 0, 0, 0, 0, 0, 0, 0, 0};

// Start of the first night of hanukkah
// Format is (YYYY, MM, DD, hh, mm, ss)
#define HANUKKAH_START_LENGTH 10
const DateTime HANUKKAH_START[] = {
  DateTime(2017, 12, 3, 16, 17, 00), // December 12, 2017 4:17pm
   DateTime(2017, 12, 12, 16, 17, 00), // December 12, 2017 4:17pm
   DateTime(2018, 12, 2, 16, 19, 00),  // December  2, 2018 4:19pm
   DateTime(2019, 12, 22, 16, 21, 00), // December 22, 2019 4:21pm
   DateTime(2020, 12, 10, 16, 17, 00), // December 10, 2020 4:17pm
   DateTime(2021, 11, 28, 16, 20, 00), // November 28, 2021 4:20pm
   DateTime(2022, 12, 18, 16, 19, 00), // December 18, 2022 4:19pm
   DateTime(2023, 12, 8, 16, 17, 00),  // December  8, 2023 4:17pm
   DateTime(2024, 12, 25, 16, 23, 00), // December 25, 2024 4:23pm
   DateTime(2025, 12, 14, 16, 18, 00), // December 14, 2025 4:18pm
   DateTime(2026, 12, 4, 16, 18, 00)   // December  4, 2026 4:18pm
};
int hanukkahStartIndex = 0; 

// Is the RTC available for accurate timekeeping
boolean RTCenabled = false;

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


  // Start candle engine at 44000 Hz
  engineStart(100000);
  
  if (rtc.begin()) {
    RTCenabled = true;
    
    if (!rtc.isrunning()) {
      // Set the RTC to the date & time this sketch was compiled
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }

    // Sets the RTC as the system time sync provider
    setSyncProvider(getRTCTime);
    
    if(timeStatus()!= timeSet) 
      Serial.println("Unable to sync with the RTC");
    else
      Serial.println("RTC has set the system time");  
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

  // Perform power on self test
  post();
  
  setCandle(0);
}

int v = 0;
int prevDay = -1;

void loop() {
  Serial.println("Run loop");
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

    for (int i = 0; i <= 8; i++) {
      candleSet(i, i <= days + 1);
    }      
  }
  delay(1000);
}

void engineStart (int freq) {
  tcConfigure(freq); //configure the timer to run at <sampleRate>Hertz
  tcStartCounter(); //starts the timer
}

void engine() {
  if (candle[SHAMASH])
    flickerShamash();
  else
    offShamash();
  for (int n = 0; n < 10; n++) {
    for (int i = 1; i <= 8; i++) {
      off();
      setCandle(i - 1);
      if (candle[i])
        flicker();  
    }
  }
  off();
}


// Power On Self Test
void post() {    
  // Light all candles in order
  Serial.println("POST start");
  candleOn(SHAMASH);
  for (int n = 1; n <= 8; n++) { // Number of candles to light
    delay(1000);
    candleOn(n);
  }
  delay(2000);
  
  candleClear();
  // Find number of days until the *next* hannukah and write it out in binary
  // Shamash is MSB
  int days = daysBetween(HANUKKAH_START[hanukkahStartIndex], getTime());
  for (int i = 1; i < 9; i++) {
    candleSet(i, days & 1);
    days = days >> 1;
  }
  candleSet(SHAMASH, days & 1);
  delay(5000);
  Serial.println("POST pass");
}  

/* **** Candle Functions **** */

void candleOn (int c) {
  candle[c] = true;
}

void candleOff (int c) {
  candle[c] = false;
}

void candleToggle (int c) {
  candle[c] = !candle[c];
}

void candleSet (int c, boolean b) {
  candle[c] = b;
}

void candleClear() {
  candleOff(SHAMASH);
  allOff();
}

void allOff() {
  for (int i = 1; i < 9; i++)
    candleOff(i); 
}

/* **** Time Functions **** */

// Get the current time in seconds from UTC epoch
// If the RTC is available, use its current time
// Otherwise, use an estimate from the system clock.
DateTime getTime () {
    return now();
}

time_t getRTCTime () {
  return rtc.now().unixtime();
}

time_t getUnixTime () {
  return getTime().unixtime();
}

// Calculate the days from the first date to the other
// If the second date happens before the first, the result will be negative.
int daysBetween (DateTime a, DateTime b) {
  // Declare helper variables to convert to signed longs
  long aL = a.unixtime();
  long bL = b.unixtime();
  return (bL - aL) / 86400 - (aL > bL);
}

/* **** Candle Engine Functions ****
 Do not use directly
*/

// Select the current "candle" to change the color of.
void setCandle (int c) {
  digitalWrite(S1, c & 1 ? HIGH : LOW);
  digitalWrite(S2, c & 2 ? HIGH : LOW);
  digitalWrite(S3, c & 4 ? HIGH : LOW);
}

// Flicker the current candle
// (i.e, probablistically set the candle to off or on [in a reddish-yellow color])
void flicker () {
  boolean red = random(100) < 40;
  boolean yellow = random(100) < 60;
  digitalWrite(RED, red || yellow ? HIGH : LOW);
  digitalWrite(GREEN, yellow ? HIGH : LOW);
  //digitalWrite(BLUE, random(100) < 10 ? HIGH : LOW);
}

void flickerShamash () {
  // Flicker the shamash
  boolean red = random(100) < 20;
  boolean yellow = random(100) < 40;
  digitalWrite(SHAMASHR, red || yellow ? LOW : HIGH);
  digitalWrite(SHAMASHG, yellow ? LOW : HIGH);
  //digitalWrite(SHAMASHB, random(100) < 10 ? HIGH : LOW);
}

// Turn current candle off
void off () {
  digitalWrite(RED, LOW);
  digitalWrite(GREEN, LOW);
  digitalWrite(BLUE, LOW);
}

void offShamash () {
  digitalWrite(SHAMASHR, LOW);
  digitalWrite(SHAMASHG, LOW);
  digitalWrite(SHAMASHB, LOW);
}

/* **** Interrupt Timer Function ****/
//this function gets called by the interrupt at <sampleRate>Hertz
void TC5_Handler (void) {
  //YOUR CODE HERE 
  engine();
  // END OF YOUR CODE
  TC5->COUNT16.INTFLAG.bit.MC0 = 1; //don't change this, it's part of the timer code
}

/* 
 *  TIMER SPECIFIC FUNCTIONS FOLLOW
 *  you shouldn't change these unless you know what you're doing
 */

//Configures the TC to generate output events at the sample frequency.
//Configures the TC in Frequency Generation mode, with an event output once
//each time the audio sample frequency period expires.
 void tcConfigure(int sampleRate)
{
 // Enable GCLK for TCC2 and TC5 (timer counter input clock)
 GCLK->CLKCTRL.reg = (uint16_t) (GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_ID(GCM_TC4_TC5)) ;
 while (GCLK->STATUS.bit.SYNCBUSY);

 tcReset(); //reset TC5

 // Set Timer counter Mode to 16 bits
 TC5->COUNT16.CTRLA.reg |= TC_CTRLA_MODE_COUNT16;
 // Set TC5 mode as match frequency
 TC5->COUNT16.CTRLA.reg |= TC_CTRLA_WAVEGEN_MFRQ;
 //set prescaler and enable TC5
 TC5->COUNT16.CTRLA.reg |= TC_CTRLA_PRESCALER_DIV1 | TC_CTRLA_ENABLE;
 //set TC5 timer counter based off of the system clock and the user defined sample rate or waveform
 TC5->COUNT16.CC[0].reg = (uint16_t) (SystemCoreClock / sampleRate - 1);
 while (tcIsSyncing());
 
 // Configure interrupt request
 NVIC_DisableIRQ(TC5_IRQn);
 NVIC_ClearPendingIRQ(TC5_IRQn);
 NVIC_SetPriority(TC5_IRQn, 0);
 NVIC_EnableIRQ(TC5_IRQn);

 // Enable the TC5 interrupt request
 TC5->COUNT16.INTENSET.bit.MC0 = 1;
 while (tcIsSyncing()); //wait until TC5 is done syncing 
} 

//Function that is used to check if TC5 is done syncing
//returns true when it is done syncing
bool tcIsSyncing()
{
  return TC5->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY;
}

//This function enables TC5 and waits for it to be ready
void tcStartCounter()
{
  TC5->COUNT16.CTRLA.reg |= TC_CTRLA_ENABLE; //set the CTRLA register
  while (tcIsSyncing()); //wait until snyc'd
}

//Reset TC5 
void tcReset()
{
  TC5->COUNT16.CTRLA.reg = TC_CTRLA_SWRST;
  while (tcIsSyncing());
  while (TC5->COUNT16.CTRLA.bit.SWRST);
}

//disable TC5
void tcDisable()
{
  TC5->COUNT16.CTRLA.reg &= ~TC_CTRLA_ENABLE;
  while (tcIsSyncing());
}
