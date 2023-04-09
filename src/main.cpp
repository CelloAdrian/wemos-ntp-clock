#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <TimeLib.h>
#include <WiFiUdp.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define OLED_RESET 0
Adafruit_SSD1306 display(OLED_RESET);

const char *ssid = "your_wifi_ssid";
const char *password = "your_wifi_password";
const char *ntpServerName = "pool.ntp.org";
const int timeZone = 2;

WiFiUDP Udp;
unsigned int localPort = 8888;
byte packetBuffer[48];

time_t getNtpTime();
void digitalClockDisplay();

void setup() {
  Serial.begin(9600);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.print("Connecting to ");
  display.println(ssid);
  display.display();
  delay(1000);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Connecting to ");
    display.println(ssid);
    display.display();
  }
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Connected to WiFi");
  display.display();
  delay(1000);
  Udp.begin(localPort);
  setSyncProvider(getNtpTime);
  setSyncInterval(300);
}

void loop() {
  digitalClockDisplay();
  delay(1000);
}

void digitalClockDisplay() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print(day());
  display.print(".");
  display.print(month());
  display.print(".");
  display.print(year());
  display.println();
  display.print(hour());
  display.print(":");
  display.print(minute());
  display.print(":");
  display.print(second());
  display.display();
}

time_t getNtpTime() {
  IPAddress ntpServerIP;
  while (Udp.parsePacket() > 0)
    ;  // discard any previously received packets
  WiFi.hostByName(ntpServerName, ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= 48) {
      Udp.read(packetBuffer, 48);
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 = (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println("No NTP Response :-(");
  return 0;  // return 0 if unable to get the time
}

void sendNTPpacket(IPAddress &address) {
  memset(packetBuffer, 0, 48);
  packetBuffer[0] = 0b11100011;  // LI, Version, Mode
  packetBuffer[1] = 0;           // Stratum, or type of clock
  packetBuffer[2] = 6;           // Polling Interval
  packetBuffer[3] = 0xEC;        // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  Udp.beginPacket(address, 123);  // NTP requests are to port 123
  Udp.write(packetBuffer, 48);
  Udp.endPacket();
}