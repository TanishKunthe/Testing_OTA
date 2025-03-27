#include <WiFi.h>
#include <Update.h>
#include <HTTPClient.h>
#include <time.h>  // Include time library

#define fota_ssid       "E-ARTKEY_4G"
#define fota_password   "Connect@Eartkey"
#define OTA_URL         "https://raw.githubusercontent.com/TanishKunthe/Testing_OTA/main/firmware.bin"
#define VERSION_URL     "https://raw.githubusercontent.com/TanishKunthe/Testing_OTA/main/version.txt"  // Version file URL

#define CURRENT_VERSION "1.0"  // Set the current firmware version
#define GOOGLE_SCRIPT_URL "https://script.google.com/macros/s/AKfycbzja2NSUw-Cgqm7q6Woc6JIYcoK9meJGC6YQArANVMOBjQ25IbUGMsi5wrMaYbdli6Myw/exec"

void setup() {
  delay(2000);
  Serial.begin(115200);
  Serial.println("Testing the FoTa Github Cloning with ACK");

  WiFi.begin(fota_ssid, fota_password);
  Serial.println("Connecting to WiFi...");

  Serial.print("Version : ");   Serial.println(CURRENT_VERSION);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected");

  // Fetch version from server and compare
  String newVersion = fetchVersion();

  if (newVersion != "" && newVersion > CURRENT_VERSION) {
    Serial.println("New Version Available: " + newVersion);
    if (performOTA()) {
      Serial.println("OTA update successful, restarting...");
      logUpdateToGoogleSheet(newVersion);  // Log the update to Google Sheets
      esp_restart();
    }
    else {
      Serial.println("OTA update failed.");
    }
  }
  else {
    Serial.println("No update required. Already running latest version: " + String(CURRENT_VERSION));
  }
}

String fetchVersion() {
  WiFiClientSecure client;
  client.setInsecure();  // Bypass SSL certificate verification
  HTTPClient http;
  http.begin(client, VERSION_URL);
  int httpCode = http.GET();

  Serial.print("Version Fetch HTTP Code: ");
  Serial.println(httpCode);

  if (httpCode == HTTP_CODE_OK) {
    String version = http.getString();
    version.trim();  // Remove any newline characters
    Serial.println("Fetched Version: " + version);
    return version;
  } else {
    Serial.println("Failed to fetch version.");
    return "";
  }
}

bool performOTA() {
  delay(2000);
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.begin(client, OTA_URL);

  int httpCode = http.GET();
  Serial.print("OTA HTTP Code: ");
  Serial.println(httpCode);

  if (httpCode == HTTP_CODE_OK) {
    int contentLength = http.getSize();
    if (contentLength > 0) {
      if (!Update.begin(contentLength)) {
        Update.printError(Serial);
        return false;
      }

      size_t written = Update.writeStream(http.getStream());
      if (written == contentLength) {
        Serial.println("Written : " + String(written) + " successfully.");
      } else {
        Update.printError(Serial);
        return false;
      }

      if (!Update.end()) {
        Update.printError(Serial);
        return false;
      }

      return true;  // OTA successful
    }
  } else {
    Serial.printf("HTTP Error: %s\n", http.errorToString(httpCode).c_str());
  }

  return false;  // OTA failed
}

void logUpdateToGoogleSheet(String version) {
  HTTPClient http;

  // Get current date and time
  time_t now = time(nullptr);
  struct tm* timeinfo = localtime(&now);
  String date = String(timeinfo->tm_year + 1900) + "-" + String(timeinfo->tm_mon + 1) + "-" + String(timeinfo->tm_mday);
  String time = String(timeinfo->tm_hour) + ":" + String(timeinfo->tm_min) + ":" + String(timeinfo->tm_sec);

  // Prepare the POST request
  http.begin(GOOGLE_SCRIPT_URL);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  String jsonPayload = "version=" + version + "&date=" + date + "&time=" + time;

  int httpResponseCode = http.POST(jsonPayload);
  if (httpResponseCode > 0) {
    Serial.printf("Log sent successfully: %s\n", httpResponseCode);
  } else {
    Serial.printf("Error sending log: %s\n", http.errorToString(httpResponseCode).c_str());
  }
  http.end();
}

void loop() {
}
