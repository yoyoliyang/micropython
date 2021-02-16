
/**

esp8266 花生壳免费二级域名解析脚本
实现的功能：
		wifi 固定ip
		ddns循环解析
		webserver （异步）

*/

#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <ESP8266HTTPClient.h>

#include <WiFiClient.h>
#include <ESPAsyncWebServer.h>


ESP8266WiFiMulti WiFiMulti;

AsyncWebServer server(80);
// buildin led with gpio 2
const int led = D4;
// 每次ddns更新所等待的时间
const int ddns_update_time = 60000;
// ddns状态，反馈到webserver中，公用户查看
String ddns_status = "Not yet initialized";
// esp8266设备网络名
const char* host_name = "esp8266.D1mini";

// 根路径回调函数
void handleRoot(AsyncWebServerRequest *request) {
  Serial.println("User requested.");
  request->send(200, "text/plain", ddns_status);
}

// 内建led灯闪烁, 作为错误提示
void _blink() {
  digitalWrite(led, HIGH);
  delay(100);
  digitalWrite(led, LOW);
  delay(100);
}

void setup() {

  Serial.begin(115200);
  // Serial.setDebugOutput(true);

  Serial.println();
  Serial.println();
  Serial.println();

  // static ip setting
  // 根据自己的需求修改下方的固定ip地址
  IPAddress local_IP(192, 168, 1, 131);
  IPAddress gateway(192, 168, 1, 1);
  IPAddress subnet(255, 255, 255, 0);
  IPAddress primaryDNS(192, 168, 1, 1);

  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS)) {
    Serial.println("STA Failed to configure");
    while (1) {
      _blink();
    }
  }
  WiFi.hostname(host_name);

  WiFi.mode(WIFI_STA);
  // 修改自己的SSID和PSWD
  WiFiMulti.addAP("SSID", "PSWD");
  // wait for WiFi connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  pinMode(led, OUTPUT);
  Serial.println("wifi connected");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {

  WiFiClient client;

  HTTPClient http;

  Serial.print("[HTTP] begin...\n");
  // configure traged server and url

  // 修改此处的用户名和密码以及个人域名
  http.begin(client, "http://username:password@ddns.oray.com/ph/update?hostname=yourdomain.vicp.net");

  /*
    // or
    http.begin(client, "http://jigsaw.w3.org/HTTP/Basic/");
    http.setAuthorization("guest", "guest");

    // or
    http.begin(client, "http://jigsaw.w3.org/HTTP/Basic/");
    http.setAuthorization("Z3Vlc3Q6Z3Vlc3Q=");
  */


  Serial.print("[HTTP] GET...\n");
  // start connection and send HTTP header
  int httpCode = http.GET();

  // httpCode will be negative on error
  if (httpCode > 0) {
    // HTTP header has been send and Server response header has been handled
    Serial.printf("[HTTP] GET... code: %d\n", httpCode);

    // file found at server
    if (httpCode == HTTP_CODE_OK) {
      ddns_status = http.getString();
      Serial.println(ddns_status);
    }
  } else {
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();

  delay(ddns_update_time);

}
