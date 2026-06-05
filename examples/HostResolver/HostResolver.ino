/**
 * HostResolver.ino
 *
 * Example sketch for Host library — demonstrates how to get IP address of
 * a host identified with FQDN.
 */

#include <Arduino.h>
#include <WiFi.h>
#include <Host.h>

// Provide your WiFi credentials here
#define WIFI_SSID "your-ssid"
#define WIFI_PASSWORD "your-password"

Host host("arduino.com"); // Host instance identified with FQDN

void setup() {
  char fqdn[Host::MAX_FQDN_LEN + 1]; // 253 characters by RFC + trailing zero
  IPAddress ip(0,0,0,0);

  Serial.begin(115200);
  delay(10);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Waiting for WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("");
  Serial.println("WiFi connected");

  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  delay(500);

  host.getFqdn(fqdn, sizeof(fqdn)); // Get FQDN from host object
  Serial.printf("Resolving host %s: ", fqdn);
  ip = host.getIP(); // Get IP address of the host
  if (ip != IPAddress(0,0,0,0))
    Serial.println(ip);
  else
    Serial.println("failed");

  delay(500);

  host.setFqdn("espressif.com");
  host.getFqdn(fqdn, sizeof(fqdn)); // Read back for later use
  Serial.printf("Resolving host %s: ", fqdn);
  ip = host.getIP(); // Get IP address of the host
  if (ip != IPAddress(0,0,0,0))
    Serial.println(ip);
  else
    Serial.println("failed");
}

void loop() {
  delay(1000);
}
