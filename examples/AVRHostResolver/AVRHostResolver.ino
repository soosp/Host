/**
 * AVRHostResolver.ino
 *
 * Example sketch for Host library — demonstrates how to get IP address of
 * a host identified with FQDN on Arduino AVR platform.
 */

#include <Arduino.h>
#include <SPI.h>
#include <Ethernet.h>
#include <Host.h>


byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

Host host("arduino.com");

void setup() {
  char fqdn[Host::MAX_FQDN_LEN + 1]; // 253 characters by RFC + trailing zero
  IPAddress ip(0,0,0,0);

  Serial.begin(115200);
  delay(10);

  Serial.print("Starting Ethernet... ");
  if (Ethernet.begin(mac) == 1) {
    Serial.println("success.");
  } else {
    Serial.println("failed.");
  }

  Serial.print("IP address: ");
  Serial.println(Ethernet.localIP());
  Serial.println();

  delay(500);

  host.getFqdn(fqdn, sizeof(fqdn)); // Get FQDN from host object
  Serial.print("Resolving host ");
  Serial.print(fqdn);
  Serial.print(": ");
  ip = host.getIP(); // Get IP address of the host
  if (ip != IPAddress(0,0,0,0))
    Serial.println(ip);
  else
    Serial.println("failed");

  delay(500);

  host.setFqdn("espressif.com");
  host.getFqdn(fqdn, sizeof(fqdn)); // Read back for later use
  Serial.print("Resolving host ");
  Serial.print(fqdn);
  Serial.print(": ");
  ip = host.getIP(); // Get IP address of the host
  if (ip != IPAddress(0,0,0,0))
    Serial.println(ip);
  else
    Serial.println("failed");
}

void loop() {
  delay(1000);
}
