#include <TinyGPS++.h>
#include <M5Stack.h>
#include <Wire.h>
#define RX_PIN 16
#define TX_PIN 17
#define RESET_PIN \
  5

TinyGPSPlus gps;
unsigned long startTime = 0;
double globalLatitude = 0.0;
double globalLongitude = 0.0;
String locationRequest = "";
void drawHeartIcon(int x, int y)
{
  M5.Lcd.fillEllipse(x - 10, y - 5, 9, 9, TFT_RED);
  M5.Lcd.fillEllipse(x + 5, y - 5, 9, 9, TFT_RED);
  M5.Lcd.fillTriangle(x - 17, y, x + 12, y, x - 2, y + 15, TFT_RED);
}

void drawCircleWithHole(int x, int y, int outerRadius, int innerRadius, uint16_t outerColor, uint16_t innerColor)
{
  M5.Lcd.fillCircle(x, y, outerRadius, outerColor);
  M5.Lcd.fillEllipse(x, y, innerRadius * 2, innerRadius * 2, innerColor);
  m5.Lcd.fillTriangle(x - 10, y + 12, x + 10, y + 12, x, y + 25, TFT_BLUE);
}

void header(const char *string)
{
  M5.Lcd.setTextSize(1);
  M5.Lcd.setTextColor(WHITE, BLUE);
  M5.Lcd.fillRect(0, 0, 320, 30, BLUE);
  M5.Lcd.setTextDatum(TC_DATUM);
  M5.Lcd.drawString(string, 160, 3, 4);
  M5.Lcd.drawString("Submit", 65, 220, 2);
  M5.Lcd.drawString("Emergency Alert", 160, 220, 2);
  M5.Lcd.drawString("Test SIM CARD", 270, 220, 2);
  M5.Lcd.setCursor(180, 65, 2);
  M5.Lcd.setTextColor(WHITE, BLACK);
  drawHeartIcon(30, 70);
  drawCircleWithHole(30, 120, 15, 3, TFT_BLUE, TFT_WHITE);
}

String _readSerial(uint32_t timeout)
{
  uint64_t timeOld = millis();
  while (!Serial2.available() && !(millis() > timeOld + timeout))
  {
    delay(13);
  }
  String str;
  while (Serial2.available())
  {
    if (Serial2.available() > 0)
    {
      str += (char)Serial2.read();
    }
  }
  Serial.print(str);
  return str;
}

void simcard_test()
{
  Serial2.print(F("AT+CPIN?\r"));
  String simcard_status = _readSerial(3000);
  Serial.print(simcard_status);
  M5.Lcd.setCursor(240, 60, 2);
  if (simcard_status == "")
  {
    M5.Lcd.setTextColor(WHITE, RED);
    M5.Lcd.print("No SIM Card");
  }
  else if (simcard_status.indexOf("READY") != -1)
  {
    M5.Lcd.setTextColor(WHITE, 0x03E0);
    M5.Lcd.print("SIM Card OK");
  }
}

void makeCall(const char *phoneNumber)
{
  Serial2.print("ATD");
  Serial2.print(phoneNumber);
  Serial2.print(";\r");
}

void sendSMS(const char *phoneNumber, const char *message)
{
  String latitudeString = String(globalLatitude, 6);
  String longitudeString = String(globalLongitude, 6);
  Serial2.print("AT+CMGF=1\r");
  delay(500);
  Serial2.print("AT+CMGS=\"");
  Serial2.print(phoneNumber);
  Serial2.print("\"\r");
  delay(500);
  Serial2.print(String(message) + latitudeString + "," + longitudeString);
  delay(500);
  Serial2.print((char)26); // Ctrl+Z pour envoyer le message
  delay(500);
}

void displayInfo()
{
  while (Serial1.available() > 0)
  {
    char c = Serial1.read();
    Serial.print(c); // Affichez la sortie série du module GPS
    if (gps.encode(c))
    {
      M5.Lcd.setCursor(60, 120);
      M5.Lcd.print(F("Loc: "));
      if (gps.location.isValid())
      {
        M5.Lcd.setTextColor(WHITE, DARKGREEN);
        M5.Lcd.print("Lat:");
        M5.Lcd.print(gps.location.lat(), 6);
        M5.Lcd.print(F(", Long:"));
        M5.Lcd.print(gps.location.lng(), 6);
        globalLatitude = gps.location.lat();
        globalLongitude = gps.location.lng();
      }
      else
      {
        M5.Lcd.setTextColor(WHITE, RED);
        M5.Lcd.print(F("INVALID"));
      }
      M5.Lcd.println();
      break;
    }
  }

  M5.update();
}

bool hasNewSMS()
{
  String response = "";
  while (Serial2.available())
  {
    char c = Serial2.read();
    response += c;
  }

  if (response.indexOf("+CMTI:") != -1)
  {
    return true;
  }

  return false;
}

void setupGPS()
{
  Serial1.begin(9600, SERIAL_8N1, 13, 5);
  Serial1.setTimeout(500);
}

void setup()
{
  M5.begin();
  M5.Power.begin();
  header("Welcome to GuardianTrace");
  Serial2.begin(115200, SERIAL_8N1, 16, 17);
  delay(1000);
  setupGPS();
  Serial2.print("AT+CPIN=\"0000\"\r");
  delay(1000);
  simcard_test();
}

void loop()
{
  M5.Lcd.setCursor(80, 60);
  M5.Lcd.setTextColor(WHITE, DARKGREEN);
  M5.Lcd.print("Frequency : 80 BPM");
  displayInfo();
  if (M5.BtnC.wasPressed())
  {
    simcard_test();
  }
  if (M5.BtnB.wasPressed())
  {
    sendSMS("Your Number", "https://www.google.com/maps?q=");
    delay(10000);
    makeCall("Your Number");
  }
  if (hasNewSMS())
  {
    M5.Lcd.setCursor(0, 170);
    M5.Lcd.setTextColor(WHITE, DARKGREEN);
    locationRequest = "Demande de localisation, maintenez le bouton Submit enfonce !";
    M5.Lcd.print(locationRequest);
    delay(5000);
  }
  if (M5.BtnA.isPressed())
  {
    M5.Lcd.setCursor(120, 190);
    M5.Lcd.setTextColor(WHITE, DARKGREEN);
    sendSMS("Your Number", "https://www.google.com/maps?q=");
    locationRequest = "Localisation envoye !";
    M5.Lcd.print(locationRequest);
    delay(2000);
    locationRequest = "";
  }
  locationRequest = "";
  M5.update();
}
