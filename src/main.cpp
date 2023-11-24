#include "esp_camera.h"
#include "Arduino.h"
#include "soc/soc.h"          // Disable brownour problems
#include "soc/rtc_cntl_reg.h" // Disable brownour problems
#include "driver/rtc_io.h"
#include <HTTPClient.h>
#include <WiFi.h>

#define flash_pin 4
#define SEND_FB_AFTER_CLEARING_BUFFER

// Pin definition for CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27

#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

int pictureNumber = 0;

const char *ssid = "Embinary";
const char *password = "Embinary@0108";
const char *post_url = "http://192.168.1.12:5000/upload"; // Location where images are POSTED
bool internet_connected = false;

bool init_wifi(void);
void click_and_send_image_to_http(void);

void print_elapsed_time(void)
{
  unsigned long current_time = micros();
  Serial.print("Elapsed Time: ");
  Serial.print(current_time/1000);
  Serial.println(" mili sec");
}

void setup()
{
  Serial.begin(115200);
  Serial.println("");
  Serial.println("Initilising Program");
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // disable brownout detector
  pinMode(flash_pin, OUTPUT);

  if (init_wifi())
  {
    internet_connected = true;
    Serial.println("Wifi connected");
  }

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound())
  {
    config.frame_size = FRAMESIZE_UXGA; // FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
    config.jpeg_quality = 10;
    config.fb_count = 2;
  }
  else
  {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // Init Camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK)
  {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
}

void loop()
{
  Serial.println("Going to sleep now");
  click_and_send_image_to_http();
  // delay(2000);
}

bool init_wifi()
{
  int connAttempts = 0;
  Serial.println("\r\nConnecting to: " + String(ssid));
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    if (connAttempts > 10)
      return false;
    connAttempts++;
  }
  return true;
}

void click_and_send_image_to_http(void)
{
  camera_fb_t *fb = NULL;

  digitalWrite(flash_pin, HIGH);
  delay(10);
#ifdef SEND_FB_AFTER_CLEARING_BUFFER
  fb = esp_camera_fb_get();
  // Uncomment the following lines if you're getting old pictures
  esp_camera_fb_return(fb); // dispose the buffered image
  fb = NULL;                // reset to capture errors
  fb = esp_camera_fb_get();
  if (!fb)
  {
    fb = NULL; // reset to capture errors
    fb = esp_camera_fb_get();
    if (!fb)
    {
      Serial.println("Camera capture failed");
      return;
    }
  }
  else
  {
    // Serial.println("Image sent to FTP");
  }
#else
  fb = esp_camera_fb_get();
  if (!fb)
  {
    fb = NULL; // reset to capture errors
    fb = esp_camera_fb_get();
    if (!fb)
    {
      Serial.println("Camera capture failed");
      return;
    }
  }
#endif
  digitalWrite(4, LOW);

  HTTPClient http;
  Serial.print("[HTTP] begin...\n");
  print_elapsed_time();
  // configure traged server and url
  http.begin(post_url); // HTTP
  Serial.print("[HTTP] Starting Post ...\n");
  print_elapsed_time();
  // start connection and send HTTP header
  int httpCode = http.sendRequest("POST", fb->buf, fb->len); // we simply put the whole image in the post body.
  Serial.print("[HTTP] Post Completed ...\n");
  print_elapsed_time();
  // httpCode will be negative on error
  if (httpCode > 0)
  {
    // HTTP header has been send and Server response header has been handled
    Serial.printf("[HTTP] POST... code: %d\n", httpCode);
    print_elapsed_time();
    // file found at server
    if (httpCode == HTTP_CODE_OK)
    {
      String payload = http.getString();
      Serial.println(payload);
    }
  }
  else
  {
    Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
    print_elapsed_time();
  }

  http.end();
  esp_camera_fb_return(fb);
}
