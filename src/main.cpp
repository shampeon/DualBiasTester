// Ver15: 2025-11-15 Updated to latest libraries, added SSD1306 128x64 display, converted to cpp, changed font and display spacing

// Ver14: Implemented option for tubes that don't use screen grids to disable 5.5% grid calculation, added "Debug" tube to check direct measurement.
// Ver13: Removed unneeded commented-out code sections

// Ver12: Rewrote bias calculation/display section to format values into padded text so flicker-inducing line blanking can be removed

// Next two are needed for the SH1106 compatible OLED library
#include <Arduino.h>
#include <U8x8lib.h>

// Of the oroginal code, basically all that's left is the menu. This uses a different display library, an I2C ADC board, and displays two channels at the time time rather than one.
// Please note that this code is rough and not even properly tidied up, so apologies for the mess.
// This uses the Adafruit ADS1015 precision ADC board; rather than the internal Arduino ADC. The OLED display also runs on the I2C bus. Pushbuttons go to digital input 5,6,7

/**
 * This work is licensed under a Creative Commons Attribution 4.0 International License. https://creativecommons.org/licenses/by/4.0/
 * Please give credit to John Wagner - mrjohnwagner@gmail.com
 *
 * ArduinoBiasMeter
 a bias plug that is read in MILLIVOLTS. At no point should more than 5v be
 *  read on this meter.
 *
 *  Version 1.0 11/28/2017
 */

// include the display library code:
#include <Wire.h>

// ADC Library
#include <Adafruit_ADS1X15.h>

// Display Initialization

// SH1106 OLED
//  U8X8_SH1106_128X64_NONAME_HW_I2C lcd(A5,A4,U8X8_PIN_NONE);
//
// SSD1306/1309 OLED
U8X8_SSD1306_128X64_NONAME_HW_I2C lcd(A5, A4, U8X8_PIN_NONE);

Adafruit_ADS1115 ads;

// Note Changes these to ints, adjust math in the doBias section to avoid "fuzziness" of floats.

float VoltageA = 0.0;
float CurrentA = 0.0;
float ScreenCurrentA = 0.0;
float VoltageB = 0.0;
float CurrentB = 0.0;
float ScreenCurrentB = 0.0;
float PowerA = 0.0;
float PowerB = 0.0;

float CurrentAPrevious = 0.0;
float CurrentBPrevious = 0.0;
float VoltageAPrevious = 0.0;
float VoltageBPrevious = 0.0;

bool FirstBiasRun = false; // ensure results are displayed when started up with static inputs

int guiClick = 7; // Selector button
// Prepping variables for Left/Right Digital buttons
int guiLClick = 5;
int guiRClick = 6;

// presently the GUI has two modes
#define GUI_MENU_SELECT_MODE 0
#define GUI_BIAS_MODE 1
// the GUI starts in menu mode
int guiMode = GUI_MENU_SELECT_MODE;
boolean movedMenu = false;
int curMenuItem = 0;

// declare functions
int processTubeMenu();
void doBias();
boolean buttonClicked();
void displayTubeTypes();
boolean debouncedButtonPressed(int inputPin);

// tube record
struct TUBE
{
  String name;
  int maxDissipation;
  bool screengrid;
};

// currently supported tubes - Tube name, max wattage, and TRUE/FALSE to subtract 5.5% for screen current
TUBE tubes[] = {{"6L6GC", 30, true}, {"EL34", 25, true}, {"6V6", 12, true}, {"6L6/G/GB", 19, true}, {"6L6WGB/WGC", 26, true}, {"5881", 23, true}, {"5881WXT", 26, true}, {"6550", 42, true}, {"EL84", 12, true}, {"RAW No Ig2", 100, false}};
#define arr_len(x) (sizeof(x) / sizeof(*x))

// voltage record for averaging
struct VOLTAGE_AVG
{
  double pinVal;  // average value read from pin
  int count;      // number of times pin value read
  int pin;        // which pin
  double voltage; // calculated voltage
};

void zero(VOLTAGE_AVG *_this)
{
  _this->pinVal = 0;
  _this->count = 0;
}

char LeftArrow = 127;
char RightArrow = 126;

void setup()
{
  // set the joystick pin to pullup
  pinMode(guiClick, INPUT_PULLUP);
  pinMode(guiRClick, INPUT_PULLUP);
  pinMode(guiLClick, INPUT_PULLUP);

  // setup the LCD

  // Config for SH1106. Uncomment only one!

  lcd.begin();
  // lcd.setFont(u8x8_font_5x7_f);
  lcd.setFont(u8x8_font_artossans8_r);
  // lcd.setFlipMode(1); //rotate display 180 degrees

  // uncomment and and edit as desired for boot-up screen.
  lcd.print("  Bias Tester\n Kiel Lydestad\n Ian Evans\n     2025");

  delay(750);

  lcd.clear();

  // Configure ADC Gain
  ads.setGain(GAIN_SIXTEEN);
  ads.begin();
}
void loop()
{

  if (guiMode == GUI_MENU_SELECT_MODE)
  {
    if (processTubeMenu() == 1)
    {

      // zero(&plate);
      // zero(&cathode);
      guiMode = GUI_BIAS_MODE;

      lcd.clear();
      lcd.print(tubes[curMenuItem].name);
      lcd.print(":");
      lcd.setCursor(0, 1);
      lcd.print(round(floor(tubes[curMenuItem].maxDissipation * .5)));
      lcd.print("|");
      lcd.print(round(floor(tubes[curMenuItem].maxDissipation * .6)));
      lcd.print("|");
      lcd.print(round(floor(tubes[curMenuItem].maxDissipation * .7)));
      lcd.print("|");
      lcd.print(tubes[curMenuItem].maxDissipation);

      // Screen header writes

      lcd.setCursor(0, 2);
      lcd.setInverseFont(1);
      lcd.print(" A:");
      lcd.setInverseFont(0);

      lcd.setCursor(8, 2);
      lcd.setInverseFont(1);
      lcd.print(" B:");
      lcd.setInverseFont(0);

      lcd.setCursor(0, 7);
      lcd.setInverseFont(1);
      lcd.print(" Delta:");
      lcd.setInverseFont(0);

      FirstBiasRun = true; // Makes sure that initial bias display shows data

      // lcd.print("w");
    }
  }
  else if (guiMode == GUI_BIAS_MODE)
  {
    lcd.setCursor(0, 1);

    doBias();

    if (buttonClicked()) // Centre Button clicked
    {
      lcd.clear();
      guiMode = GUI_MENU_SELECT_MODE;
    }
  }

  // delay(100);
}

void doBias()
{
  int16_t adc0; // we read from the ADC, we have a sixteen bit integer as a result
  int16_t adc1;
  int16_t adc2;
  int16_t adc3;
  float aWattPerc = 0;
  float bWattPerc = 0;

  char DisplayChar[6];
  float CurrentDiff = 0;

  // Read voltages from all four channels
  adc0 = ads.readADC_SingleEnded(0); // Channel A Voltage
  adc1 = ads.readADC_SingleEnded(1); // Channel A Current
  adc2 = ads.readADC_SingleEnded(2); // Channel B Voltage
  adc3 = ads.readADC_SingleEnded(3); // Channel B Current

  // Multiply digital output reading by step value per bit
  VoltageA = (adc0 * 0.078125);
  CurrentA = (adc1 * 0.0078125);
  VoltageB = (adc2 * 0.078125);
  CurrentB = (adc3 * 0.0078125);

  // Subtract 5.5% of reading to approximate screen current

  if (tubes[curMenuItem].screengrid == false)
  {
  }
  else
  {
    ScreenCurrentA = (CurrentA * 0.055);
    ScreenCurrentB = (CurrentB * 0.055);
    CurrentA = (CurrentA - ScreenCurrentA);
    CurrentB = (CurrentB - ScreenCurrentB);
  }

  // Tube A readings

  // Clear line and write data

  // Volts
  if (VoltageA != VoltageAPrevious || FirstBiasRun)
  {
    lcd.setCursor(0, 3);
    dtostrf(VoltageA, -6, 1, DisplayChar);
    lcd.print(DisplayChar);
    lcd.setCursor(6, 3);
    lcd.print("V");
  }
  // mA
  if (CurrentA != CurrentAPrevious || FirstBiasRun)
  {
    lcd.setCursor(0, 4);
    dtostrf(CurrentA, -5, 1, DisplayChar);
    lcd.print(DisplayChar);
    lcd.setCursor(5, 4);
    lcd.print("mA");
  }

  // Watts
  if (VoltageA != VoltageAPrevious || CurrentA != CurrentAPrevious || FirstBiasRun)
  {
    lcd.setCursor(0, 5);
    dtostrf(abs((CurrentA * VoltageA) / 1000), -6, 1, DisplayChar);
    lcd.print(DisplayChar);

    lcd.setCursor(6, 5);
    lcd.print("W");

    //% Rating
    lcd.setCursor(0, 6);
    aWattPerc = ((CurrentA * VoltageA) / 1000);
    dtostrf(abs((aWattPerc / tubes[curMenuItem].maxDissipation) * 100), -6, 1, DisplayChar);
    lcd.print(DisplayChar);
    lcd.setCursor(6, 6);
    lcd.print("%");
  }

  // Tube B readings

  // Volts
  if (VoltageB != VoltageBPrevious || FirstBiasRun)
  {
    lcd.setCursor(8, 3);
    dtostrf(VoltageB, -6, 1, DisplayChar);
    lcd.print(DisplayChar);
    lcd.setCursor(15, 3);
    lcd.print("V");
  }

  // mA
  if (CurrentB != CurrentBPrevious || FirstBiasRun)
  {
    lcd.setCursor(8, 4);
    dtostrf(CurrentB, -5, 1, DisplayChar);
    lcd.print(DisplayChar);

    lcd.setCursor(14, 4);
    lcd.print("mA");
  }

  // Watts
  if (VoltageB != VoltageBPrevious || CurrentB != CurrentBPrevious || FirstBiasRun)
  {

    lcd.setCursor(8, 5);
    dtostrf(abs((CurrentB * VoltageB) / 1000), -6, 1, DisplayChar);
    lcd.print(DisplayChar);
    lcd.setCursor(15, 5);
    lcd.print("W");

    //% Dissipation
    lcd.setCursor(8, 6);
    bWattPerc = ((CurrentB * VoltageB) / 1000);
    dtostrf(abs((bWattPerc / tubes[curMenuItem].maxDissipation) * 100), -6, 1, DisplayChar);
    lcd.print(DisplayChar);
    lcd.setCursor(15, 6);
    lcd.print("%");
  }

  // Tube A/B difference calculation

  if (CurrentAPrevious != CurrentA || CurrentBPrevious != CurrentB || VoltageAPrevious != VoltageA || VoltageBPrevious != VoltageB || FirstBiasRun)
  {
    lcd.setCursor(8, 7);
    // lcd.print((CurrentA - CurrentB),1);

    CurrentDiff = CurrentA - CurrentB;

    CurrentDiff = abs(CurrentDiff);

    dtostrf(abs(CurrentDiff), -5, 1, DisplayChar);
    lcd.print(DisplayChar);

    lcd.setCursor(14, 7);
    lcd.print("mA");
  }

  // Setting previous data values to evaluate if data has changed
  CurrentAPrevious = CurrentA;
  CurrentBPrevious = CurrentB;
  VoltageAPrevious = VoltageA;
  VoltageBPrevious = VoltageB;
  FirstBiasRun = false; // Flag for first-time run to ensure data has written
}

int processTubeMenu()
{
  displayTubeTypes(); // disply the types of tubes

  if (debouncedButtonPressed(guiLClick) == true && movedMenu == false)
  {
    lcd.clearLine(1);
    lcd.clearLine(2);
    movedMenu = true;
    curMenuItem--;
    if (curMenuItem < 0)
    {
      curMenuItem = arr_len(tubes) - 1;
    }
  }
  else if (debouncedButtonPressed(guiRClick) == true && movedMenu == false)
  {
    lcd.clearLine(1);
    lcd.clearLine(2);
    movedMenu = true;
    curMenuItem++;
    if ((unsigned)curMenuItem >= arr_len(tubes))
    {
      curMenuItem = 0;
    }
  }

  else if (debouncedButtonPressed(guiRClick) == false && debouncedButtonPressed(guiLClick) == false)
  {
    movedMenu = false;
  }

  if (buttonClicked()) // joystick clicked
  {
    return 1;
  }

  return 0;
}

void displayTubeTypes()
{
  lcd.setCursor(0, 0);
  lcd.print("Select Tube");
  lcd.setCursor(0, 1);
  lcd.print("<");
  // lcd.print(" ");
  lcd.print(tubes[curMenuItem].name);
  lcd.print(" ");
  lcd.print(tubes[curMenuItem].maxDissipation);
  lcd.print("W");
  lcd.print(">");
  // lcd.print("   "); //trailing clear removed 7-17-2019
}

// debounce the button

boolean buttonClicked()
{
  if (digitalRead(guiClick) == LOW)
  {
    delay(30);
    if (digitalRead(guiClick) == LOW)
    {
      return true;
    }
  }
  return false;
}

boolean debouncedButtonPressed(int inputPin)
{
  if (digitalRead(inputPin) == LOW)
  {
    delay(30);
    if (digitalRead(inputPin) == LOW)
    {
      return true;
    }
  }
  return false;
}