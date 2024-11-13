#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>  // Include the Wi-Fi library
#include <ESP8266mDNS.h>
#include <Wire.h>

#include "accel_data.h"
#include "behavior.h"
#include "parameters.h"

const char *ssid = "Wineglass";       // The name of the Wi-Fi network that will be created
const char *password = "nocheating";  // The password required to connect to it, leave blank for an open network

ESP8266WebServer server(80);  // Create an HTTP server on port 80

Adafruit_MPU6050 mpu;

std::vector<DataFrame> accel_data;
Difficulty difficulty = medium_difficulty;
bool present = false;
int tamper_counter = 0;
Behavior behavior;
int GREEN = 14;
int BLUE = 12;
int RED = 16;

void set_behavior(Behavior new_behavior) {
    switch (new_behavior) {
        case Behavior::good:
            analogWrite(GREEN, 0xff);
            digitalWrite(RED, LOW);
            digitalWrite(BLUE, LOW);
            behavior = Behavior::good;
            break;
        case Behavior::okey:
            analogWrite(GREEN, 0x4c);
            digitalWrite(RED, HIGH);
            digitalWrite(BLUE, LOW);
            behavior = Behavior::okey;
            break;
        case Behavior::bad:
            analogWrite(GREEN, 0x00);
            digitalWrite(RED, HIGH);
            digitalWrite(BLUE, LOW);
            behavior = Behavior::bad;
            break;
        case Behavior::horrible:
            analogWrite(GREEN, 0x00);
            digitalWrite(RED, LOW);
            digitalWrite(BLUE, HIGH);
            behavior = Behavior::horrible;
            break;
    }
}

void handle_root() {
    server.send(200, "text/html", R"(<!DOCTYPE html>
<html>
<head>
<style>
h1 {
  font-size: 50px;
}
p {
  font-size: 40px;
}
</style>
</head>
<body>

<h1>Wineglass</h1>

<p> <a href="/start_game">Start game</a></p>
<p> <a href="/set_easy">Set to easy</a></p>
<p> <a href="/set_medium">Set to medium</a></p>
<p> <a href="/set_hard">Set to hard</a></p>
<p> <a href="/present_mode">Present mode</a></p>
<p> <a href="/game_mode">Game mode</a></p>
<p> <a href="/callibrate">Callibrate</a></p>

</body>
</html>
)");
}
void set_easy() {
    difficulty = easy_difficulty;
    handle_root();
}
void set_medium() {
    difficulty = medium_difficulty;
    handle_root();
}
void set_hard() {
    difficulty = hard_difficulty;
    handle_root();
}
void present_mode() {
    present = true;
    handle_root();
}
void game_mode() {
    present = false;
    handle_root();
}

void start_game() {
    set_behavior(Behavior::good);
    handle_root();
}

void request_callibrate() {
    callibrate();
    handle_root();
}

void setup(void) {
    Serial.begin(9600);

    accel_data.reserve(11);
    for (int i = 0; i < 5; i++) {
        accel_data.emplace_back(DataFrame{0.0f, 0.0f});
    }

    delay(10);
    Serial.println('\n');

    WiFi.softAP(ssid, password);  // Start the access point
    Serial.print("Access Point \"");
    Serial.print(ssid);
    Serial.println("\" started");

    Serial.print("IP address:\t");
    Serial.println(WiFi.softAPIP());

    if (MDNS.begin("wineglass")) {  // Start the mDNS responder for esp8266.local
        Serial.println("mDNS responder started");
    } else {
        Serial.println("Error setting up MDNS responder!");
    }

    server.on("/", handle_root);
    server.on("/set_easy", set_easy);
    server.on("/set_medium", set_medium);
    server.on("/set_hard", set_hard);
    server.on("/present_mode", present_mode);
    server.on("/game_mode", game_mode);
    server.on("/start_game", start_game);
    server.on("/callibrate", request_callibrate);

    server.begin();
    pinMode(GREEN, OUTPUT);
    pinMode(BLUE, OUTPUT);
    pinMode(RED, OUTPUT);
    set_behavior(Behavior::horrible);
    while (!Serial) {
        delay(10);  // will pause Zero, Leonardo, etc until serial console opens
    }

    // Try to initialize!
    if (!mpu.begin()) {
        Serial.println("Failed to find MPU6050 chip");
        while (1) {
            delay(10);
        }
    }

    mpu.setAccelerometerRange(MPU6050_RANGE_16_G);
    mpu.setGyroRange(MPU6050_RANGE_250_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    // Serial.println("");
    delay(100);
}

void loop() {
    MDNS.update();
    server.handleClient();
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    fetch_new_dataframe(accel_data, a.acceleration);

    auto new_behavior = get_behavior_from_accel(accel_data, difficulty);
    if (new_behavior > behavior || present) {
        set_behavior(new_behavior);
    }

    if (detect_tamper(accel_data)) {
        tamper_counter++;
        if (tamper_counter >= tamper_counter_limit) {
            set_behavior(Behavior::horrible);
        }
    } else {
        tamper_counter = 0;
    }

    delay(50);
}

// Serial.println(g.gyro.z);Game has been set to easy