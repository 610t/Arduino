/*
    Scratch remote sensor protocol

    You need to get ssid , password and host

    http://tiisai.dip.jp/?p=3665
*/

/*
  Copyright 2016,2019-2020 Takeshi MUTOH

  Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
   This program is demonstration that Scrath Remote Sensor Protocol with M5Stack.
*/

#include <WiFi.h>
#include <M5StickC.h>
#include <Wire.h>

/*
  network.h contains network information below:

  const char* ssid     = "SSID";
  const char* password = "PASSWORD";
  const char* host     = "Scratch Host IP";
*/
#include "network.h"

const int Port = 42001;
//WiFiClient client;

void WiFiSetup() {
wifisetup:
  Serial.println("Wifi begin.");
  WiFi.disconnect();
  WiFi.begin(ssid, password);
  Serial.println("End of Wifi begin.");

  int c = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    M5.Lcd.print(".");
    Serial.print(".");
    if (c > 10) goto wifisetup;
    c += 1;
  }
  M5.Lcd.println("");
  Serial.println("End of WiFiSetup()");
}

void setup() {
  // Init M5
  M5.begin();
  delay(100);

  // Init Serial
  Serial.begin(115200);
  delay(10);

  // We start by connecting to a WiFi network
  M5.Lcd.println("Welcome to Scratch Remoto Sensor!!");
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFiSetup();

  M5.Lcd.println("WiFi connected.");

  Serial.println("Wifi OK");

  // Wire setup
  Wire.begin();
  pinMode(5, INPUT);
  digitalWrite(5, HIGH);

  // Accel
  M5.IMU.Init();

  // LED
  pinMode(M5_LED, OUTPUT);
  digitalWrite(M5_LED, LOW);

  // Button
  pinMode(M5_BUTTON_HOME, INPUT);
  pinMode(M5_BUTTON_RST, INPUT);

  delay(1000);
}

String getValue(char name, String msg) {
  msg.replace(String(name) + " ", "");
  Serial.println("{" + String(name) + ":" + String(msg) + "}");
  return msg;
}

void broadcast(WiFiClient client, String msg) {
  char scmd[32] = {0};
  char buf[100] = {0};
  String cmd = "broadcast \"" + msg + "\"";

  cmd.toCharArray(buf, cmd.length() + 1);
  strcpy(scmd + 4, buf);
  //scmd[3] = (uint8_t)strlen(scmd + 4);
  scmd[3] = cmd.length();
  Serial.println(">pre broadcast:" + String(scmd + 4));
  client.setTimeout(100);
  //  if (client.write((const uint8_t*)scmd, 4 + strlen(scmd + 4))) {
  if (client.write((const uint8_t*)scmd, 4 + cmd.length())) {
    Serial.println(">>broadcast:" + msg + " ok");
  } else {
    Serial.println(">>broadcast:" + msg + " err");
  }
}

void sensor_update(WiFiClient client, String varName, String varValue) {
  char scmd[32] = {0};
  char buf[100] = {0};
  String cmd = "sensor-update \"" + varName + "\" " + varValue + " ";

  cmd.toCharArray(buf, cmd.length() + 1);
  sprintf(scmd + 4, buf);
  scmd[3] = (uint8_t)strlen(scmd + 4);
  client.setTimeout(100);
  if (client.write((const uint8_t*)scmd, 4 + strlen(scmd + 4))) {
    //Serial.println("sensor-update ok");
    return;
  } else {
    //Serial.println("sensor-update err");
    return;
  }
}

void loop() {
  uint8_t buffer[128] = {0};
  int r = 0, g = 0, b = 0;
  String s;
  char* str;

  // Reconnect to WiFi
  while (!(WiFi.status() == WL_CONNECTED)) {
    Serial.println("WiFi not connected");
    WiFiSetup();
  }

  // Use WiFiClient class to create TCP connections
  WiFiClient client;

  Serial.println("Before client connect");
  while (!client.connect(host, Port)) {
    Serial.println("connection failed");
  }
  Serial.println("create tcp ok");

  while (!client.connected()) {
    Serial.println("client not connected retry WiFiSetup");
    WiFiSetup();
    Serial.println("Stop connection");
    client.stop();
    Serial.println("Before client.connect");
    //client.setTimeout(100);
    client.connect(host, Port);
    Serial.println("After client.connect");
  }
  Serial.println("Client connected");

  // Read all from server and print them to Serial.
  uint32_t len = 0;
  String msg = "";
  char *c;

  Serial.println("Let us go to read messages.");

  //// Receive msg
  len = 0;
  int av = client.available();
  Serial.println("available:" + String(av));
  //if (av > 0) {
  client.setTimeout(100);
  len = client.readBytes(buffer, sizeof(buffer));
  //}

  Serial.println("Get length:" + String(len));

  while (len > 0) {
    M5.Lcd.setCursor(0, 0);
    Serial.print("Received:[");
    // Skip 4 byte message header and get string.
    for (uint32_t i = 4; i < len; i++) {
      Serial.print((char)buffer[i]);
      msg += (char)buffer[i];
    }
    Serial.print("]\r\n");

    while ((!msg.startsWith("broadcast") && !msg.startsWith("sensor-update")) && msg.length() > 0 ) {
      msg = msg.substring(1);
    }

    if (msg.startsWith("broadcast") == true) {
      // message
      msg.replace("broadcast ", "");
      msg.replace("\"", "");
      Serial.println("{broadcast:" + msg + "}");
      M5.Lcd.println("{broadcast:" + msg + "}");
    } else if (msg.startsWith("sensor-update")) {
      // value
      msg.replace("sensor-update ", "");
      msg.replace("\"", "");
      msg.trim();
      //M5.Lcd.println("{sensor-update " + msg + "}");

      while (msg.length() > 0) {
        msg.trim();
        switch (msg.charAt(0)) {
          case 'r':
            r = int(getValue('r', msg).toFloat());
            break;
          case 'g':
            g = int(getValue('g', msg).toFloat());
            break;
          case 'b':
            b = int(getValue('b', msg).toFloat());
            break;
          case 's':
            s = getValue('s', msg);
            break;
          case 'l':
            int led =  int(getValue('l', msg).toFloat());
            digitalWrite(M5_LED, led);
            break;
        }
        //Serial.println("{{msg:" + msg + "}}");

        // Skip var_value
        while (msg.charAt(0) != ' ' && msg.length() > 0) {
          msg = msg.substring(1);
        }
        //Serial.println("{{msg2:" + msg + "}}");
      }

      //// Output
      // RGB background
      M5.Lcd.fillScreen(uint16_t (((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3) ));

      M5.Lcd.println("{RGB:(" + String(r) + ", " + String(g) + ", " + String(b) + ")}");
      Serial.println("{RGB:(" + String(r) + ", " + String(g) + ", " + String(b) + ")}");
      // msg
      //M5.Lcd.setCursor(0, 100);
      //M5.Lcd.setTextSize(5);
      M5.Lcd.println("{s:" + s + "}");
    } else {
      Serial.println("NOP");
    }

    len = msg.length();
  }

  // broadcast
  broadcast(client, "test");
  // broadcast by button
  if (digitalRead(M5_BUTTON_HOME) == LOW) {
    broadcast(client, "BtnA");
  }
  if (digitalRead(M5_BUTTON_RST) == LOW) {
    broadcast(client, "BtnB");
  }

  // sensor-update
  sensor_update(client, "v", String(random(0, 255)));

  // sensor-update by accel
  float ax = 0;
  float ay = 0;
  float az = 0;

  M5.IMU.getAccelData(&ax, &ay, &az);
  sensor_update(client, "ax", String(-1 * 240 * ax));
  sensor_update(client, "ay", String(-1 * 180 * ay));
  sensor_update(client, "az", String(1000 * az));
  M5.Lcd.println("{ax,ay,az:(" + String(ax) + ", " + String(ay) + ", " + String(az) + ")}");

  // sensor-update by gyro
  int16_t gyroX = 0;
  int16_t gyroY = 0;
  int16_t gyroZ = 0;

  M5.IMU.getGyroAdc(&gyroX, &gyroY, &gyroZ);
  sensor_update(client, "gx", String(gyroX));
  sensor_update(client, "gy", String(gyroY));
  sensor_update(client, "gz", String(gyroZ));
  M5.Lcd.println("{gx,gy,gz:(" + String(gyroX) + ", " + String(gyroY) + ", " + String(gyroZ) + ")}");

  client.stop();

}
