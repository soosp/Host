/**
 * AVRHostResolver.ino
 *
 * Example sketch for Host library — demonstrates how to get IP address of
 * a host identified with FQDN on Arduino AVR platform.
 */

#include <Arduino.h>
#include <SPI.h>
#include <Ethernet.h>

// Decrease the maximum length of FQDN to save memory
#define HOST_FQDN_LEN 63
#include <Host.h>


byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

Host host("arduino.com");

void setup() {
  char str[Host::MAX_FQDN_SIZE]; // HOST_FQDN_LEN + trailing zero
  IPAddress ip(0,0,0,0);

  Serial.begin(115200);
  delay(10);

  Serial.print(F("Starting Ethernet... "));
  if (Ethernet.begin(mac) == 1) {
    Serial.println(F("success."));
  } else {
    Serial.println(F("failed."));
  }

  Serial.print(F("IP address: "));
  Serial.println(Ethernet.localIP());
  Serial.println();

  delay(500);

  host.getFqdn(str, sizeof(str)); // Get FQDN from host object
  Serial.print(F("Resolving host "));
  Serial.print(str);
  Serial.print(F(": "));
  ip = host.getIP(); // Get IP address of the host
  if (ip != IPAddress(0,0,0,0))
    Serial.println(ip);
  else
    Serial.println(F("failed"));

  delay(500);

  host.fromStr(PSTR("espressif.com"));
  host.getFqdn(str, sizeof(str)); // Read back for later use
  Serial.print(F("Resolving host "));
  Serial.print(str);
  Serial.print(F(": "));
  ip = host.getIP(); // Get IP address of the host
  if (ip != IPAddress(0,0,0,0))
    Serial.println(ip);
  else
    Serial.println(F("failed"));

  host.setFqdn(PSTR("platformio.org"));
  host.toStr(str, sizeof(str)); // Read back for later use
  Serial.print(F("Resolving host "));
  Serial.print(str);
  Serial.print(F(": "));
  ip = host.getIP(); // Get IP address of the host
  if (ip != IPAddress(0,0,0,0))
    Serial.println(ip);
  else
    Serial.println(F("failed"));
  
  host.fromStr(PSTR("192.168.0.1"));
  host.toStr(str, sizeof(str)); // Read back for later use
  Serial.print(F("Check IP of host "));
  Serial.print(str);
  Serial.print(F(": "));
  ip = host.getIP(); // Get IP address of the host
  if (ip != IPAddress(0,0,0,0))
    Serial.println(ip);
  else
    Serial.println(F("failed"));
}

void loop() {
  delay(1000);
}
