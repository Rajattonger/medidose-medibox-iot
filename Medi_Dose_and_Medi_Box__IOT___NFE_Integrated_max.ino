#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include "MAX30100_PulseOximeter.h"

// WiFi Credentials
const char *ssid = "*********";
const char *password = "************";

// Web server on port 80
WebServer server(80);

// Temperature sensor pin
const int PinoNTC = 34;
int vout = 0;

// Pulse Oximeter configuration
PulseOximeter pox;
uint32_t tsLastReport = 0;

// Calibration variables for the NTC thermistor
double Vs = 3.3; 
double R1 = 10000; 
double Beta = 3950; 
double To = 298.15; 
double Ro = 10000;
double adcMax = 4095.0;
double offset = -3.95;  // Calibration Offset
#define REPORTING_PERIOD_MS 1000
void onBeatDetected()
{
    Serial.println("Beat detected!");
}

// Root page handler for the web server
void handleRoot() {
  char msg[4000]; // Adjusted size for added content

  // Temperature reading
  double Vout, Rt = 0;
  double T, Tc, Tccalibrated, Tf, adc = 0;

  adc = analogRead(PinoNTC);
  Vout = adc * Vs / adcMax;
  Rt = R1 * Vout / (Vs - Vout);
  T = 1 / (1 / To + log(Rt / Ro) / Beta);
  Tc = T - 273.15;  // Celsius
  Tc_calibrated = Tc + offset;  // Apply calibration offset
  Tf = Tc_calibrated * 9 / 5 + 32;  // Fahrenheit

  // Pulse oximeter update
  pox.update();
  int heartRate = pox.getHeartRate();
  int spO2 = pox.getSpO2();

  // Temperature Status
  const char* tempStatus = (Tf > 98) 
                           ? "HIGH (Use medicine: Paracetamol)" 
                           : "NORMAL";

  // Heart Rate Status
  const char* heartRateStatus = (heartRate < 30) 
                                ? "LOW (Use medicine: Consult a doctor)"
                                : (heartRate > 120) 
                                ? "HIGH (Use medicine: Beta Blockers - consult doctor)" 
                                : "NORMAL";

  snprintf(msg, 4000,
           "<html>\
<head>\
  <meta http-equiv='refresh' content='4'/>\  
  <meta name='viewport' content='width=device-width, initial-scale=1'/>\  
  <meta charset='UTF-8'/>\  
  <title>Health</title>\  
  <link href='https://cdnjs.cloudflare.com/ajax/libs/font-awesome/5.15.4/css/all.min.css' rel='stylesheet'/>\  
  <style>\
    body { font-family: Arial, sans-serif; margin: 0; padding: 0; background: linear-gradient(to bottom, #2193b0, #6dd5ed); color: #fff; text-align: center; }\
    h1 { margin: 20px; font-size: 2.5rem; }\
    .container { padding: 20px; background: rgba(0, 0, 0, 0.5); margin: 20px auto; border-radius: 10px; width: 90%%; max-width: 500px; box-shadow: 0px 0px 15px rgba(0, 0, 0, 0.3); }\
    .status { font-size: 1.5rem; margin: 10px 0; padding: 10px; border-radius: 5px; background: #fff; color: #333; }\
    .icon { font-size: 2.5rem; margin-right: 10px; }\
    .temp-icon { color: red; }\
    .spo2-icon { color: green; }\
    .heart-rate-icon { color: orange; }\
    .data { font-size: 1.8rem; margin: 10px 0; }\
    .alert { font-size: 1.5rem; color: #ff4d4d; margin-top: 20px; }\
    footer { margin-top: 20px; font-size: 1rem; color: #ddd; }\
  </style>\
</head>\
<body>\
  <h1>Health Monitoring System</h1>\
  <div class='container'>\
    <p class='data'>\
      <i class='fas fa-thermometer-half icon temp-icon'></i>\
      <strong>Temperature:</strong> %.2f °C (%.2f °F)\
    </p>\
    <p class='data'>\
      <i class='fas fa-heartbeat icon heart-rate-icon'></i>\
      <strong>Heart Rate:</strong> %d bpm\
    </p>\
    <p class='data'>\
      <i class='fas fa-tint icon spo2-icon'></i>\
      <strong>SpO2:</strong> %d %%\
    </p>\
    <div class='alert'>\
      <p>\
        <i class='fas fa-exclamation-triangle icon temp-icon'></i>\
        <strong>Temperature Status:</strong> %s\
      </p>\
      <p>\
        <i class='fas fa-exclamation-triangle icon heart-rate-icon'></i>\
        <strong>Heart Rate Status:</strong> %s\
      </p>\
    </div>\
  </div>\
  <footer>Designed with <i class='fas fa-heart' style='color: red;'></i> for Safety</footer>\
</body>\
</html>",
           Tc_calibrated,
           Tf,
           heartRate,
           spO2,
           tempStatus,
           heartRateStatus
          );
  server.send(200, "text/html", msg);
}


void setup() {
  Serial.begin(9600);
  
  // Pulse oximeter setup
 
  // WiFi setup
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.printn(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Web server setup
  server.on("/", handleRoot);
  server.begin();
  Serial.println("HTTP server started");
  
   Serial.print("Initializing pulse oximeter...");
  if (!pox.begin()) {
      Serial.println("FAILED");
      for (;;);
  } else {
      Serial.println("SUCCESS");
  }
  pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);
  pox.setOnBeatDetectedCallback(onBeatDetected);

}

void loop() {
  pox.update();
  if (millis() - tsLastReport > REPORTING_PERIOD_MS) {
    Serial.print("Heart Rate: ");
    Serial.print(pox.getHeartRate());
    Serial.print(" bpm | ");
    Serial.print("SpO2: ");
    Serial.print(pox.getSpO2());
    Serial.println(" %");
    //server.handleClient();
    tsLastReport = millis();
  }
  server.handleClient();
}
