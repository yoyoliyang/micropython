
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

#include <U8g2lib.h>
#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

U8G2_SH1106_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

u8g2_uint_t offset;      // current offset for the scrolling text
u8g2_uint_t width;      // pixel width of the scrolling text (must be lesser than 128 unless U8G2_16BIT is defined


ESP8266WiFiMulti WiFiMulti;

// esp8266设备网络名
const char* host_name = "esp8266.D1Mini";
const char* wifi_status;

void wifi_init() {
  // static ip setting
  // 根据自己的需求修改下方的固定ip地址
  IPAddress local_IP(192, 168, 1, 132);
  IPAddress gateway(192, 168, 1, 1);
  IPAddress subnet(255, 255, 255, 0);
  IPAddress primaryDNS(192, 168, 1, 1);

  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS)) {
    String wifi_err_msg = "STA Failed to configure";
    Serial.println(wifi_err_msg);
    wifi_status = wifi_err_msg.c_str();
  }
  WiFi.hostname(host_name);

  WiFi.mode(WIFI_STA);
  // 修改自己的SSID和PSWD
  WiFiMulti.addAP("SSID", "PSWD");
  // wait for WiFi connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
    u8g2.setFont(u8g2_font_profont11_tf);
    u8g2.firstPage();
    do {
      u8g2.setCursor(0, 20);
      u8g2.print(F("Hello,world!"));
      u8g2.setCursor(0, 40);
      u8g2.print(F("connecting wifi..."));
    } while ( u8g2.nextPage() );
    delay(1000);
  }
  Serial.println("wifi connected");
  wifi_status = "wifi connected";
}

void text_scroll(const char* text) {
  u8g2_uint_t x;

  u8g2.firstPage();
  do {
    // draw the scrolling text at current offset
    x = offset;
    u8g2.setFont(u8g2_font_profont11_tf);    // set the target font
    do {                // repeated drawing of the scrolling text...
      u8g2.drawUTF8(x, 30, text);     // draw the scolling text
      x += width;           // add the pixel width of the scrolling text
    } while ( x < u8g2.getDisplayWidth() );   // draw again until the complete display is filled
    u8g2.setFont(u8g2_font_profont11_tf);    // draw the current pixel width
    u8g2.setCursor(0, 8);
    u8g2.print((WiFi.localIP().toString()).c_str());
    u8g2.setCursor(0, 16);
    u8g2.print(wifi_status);

  } while ( u8g2.nextPage() );

  offset -= 1;            // scroll by one pixel
  if ( (u8g2_uint_t)offset < (u8g2_uint_t) - width )
    offset = 0;             // start over again


}
void setup(void) {

  Serial.begin(115200);

  //u8g2初始化
  u8g2.begin();
  u8g2.setFont(u8g2_font_profont11_tf);;  // set the target font to calculate the pixel width

  wifi_init();
  Serial.println(WiFi.localIP());
  //u8x8.drawString(0, 3, (WiFi.localIP().toString()).c_str());
}

void loop() {

  WiFiClient client;

  HTTPClient http;

  Serial.print("[HTTP] begin...\n");
  // configure traged server and url

  // 修改此处的用户名和密码以及个人域名
  http.begin(client, "http://user:pswd@ddns.oray.com/ph/update?hostname=yourdomain.vicp.net");

  Serial.print("[HTTP] GET...\n");
  // start connection and send HTTP header
  int httpCode = http.GET();
  String ddns_result;
  // httpCode will be negative on error
  if (httpCode > 0) {
    // HTTP header has been send and Server response header has been handled
    Serial.printf("[HTTP] GET... code: %d\n", httpCode);

    // file found at server
    if (httpCode == HTTP_CODE_OK) {
      ddns_result = http.getString() + " ";
      Serial.printf("%s\n", ddns_result.c_str());
    }
  } else {
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
  const char* result = ddns_result.c_str();
  width = u8g2.getUTF8Width(result);    // calculate the pixel width of the text  u8g2.setFontMode(0);    // enable transparent mode, which is faster

  for (int n; n < 6000; n++) {
    text_scroll(result);
    delay(10);              // do some small delay
    n++;
  }
}
