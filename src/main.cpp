#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <time.h>

#ifndef APSSID
#define APSSID "SSID"
#define APPSK  "PASSWORD"
#endif

ESP8266WiFiMulti WiFiMulti;

// Certificado raiz do GitHub
static const char githubCert[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
... (Certificado raiz do GitHub) ...
-----END CERTIFICATE-----
)EOF";

#define FIRMWARE_VERSION "0.1"

const char * GIT_URL = "https://raw.githubusercontent.com/{username}/{repository}/{branch}/{filepath}";

time_t setClock() {
  configTime(2 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  Serial.print("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo));
  return now;
}

void update_started() {
  Serial.println("CALLBACK: HTTP update process started");
}

void update_finished() {
  Serial.println("CALLBACK: HTTP update process finished");
}

void update_progress(int cur, int total) {
  Serial.printf("CALLBACK: HTTP update process at %d of %d bytes...\n", cur, total);
}

void update_error(int err) {
  Serial.printf("CALLBACK: HTTP update fatal error code %d\n", err);
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();
  Serial.println();

  for (uint8_t t = 4; t > 0; t--) {
    Serial.printf("[SETUP] WAIT %d...\n", t);
    Serial.flush();
    delay(1000);
  }

  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(APSSID, APPSK);

  Serial.print(F("Firmware version "));
  Serial.println(FIRMWARE_VERSION);
}

void loop() {
  if ((WiFiMulti.run() == WL_CONNECTED)) {
    BearSSL::WiFiClientSecure client;
    client.setTrustAnchors(new BearSSL::X509List(githubCert));

    setClock();

    ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW);
    ESPhttpUpdate.onStart(update_started);
    ESPhttpUpdate.onEnd(update_finished);
    ESPhttpUpdate.onProgress(update_progress);
    ESPhttpUpdate.onError(update_error);

    Serial.println(F("Update start now!"));
    t_httpUpdate_return ret = ESPhttpUpdate.update(client, GIT_URL);

    switch (ret) {
      case HTTP_UPDATE_FAILED:
        Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
        Serial.println(F("Retry in 10 secs!"));
        delay(10000);  // Wait 10 secs
        break;

      case HTTP_UPDATE_NO_UPDATES:
        Serial.println("HTTP_UPDATE_NO_UPDATES");
        Serial.println("Your code is up to date!");
        delay(10000);  // Wait 10 secs
        break;

      case HTTP_UPDATE_OK:
        Serial.println("HTTP_UPDATE_OK");
        delay(1000);  // Wait a second and restart
        ESP.restart();
        break;
    }
  }
}

