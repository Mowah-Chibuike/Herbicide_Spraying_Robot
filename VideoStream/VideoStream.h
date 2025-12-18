#ifndef VIDEO_STREAM_H
#define VIDEO_STREAM_H

#include "esp_camera.h"
#include <WiFi.h>
#include "esp_timer.h"
#include "img_converters.h"
#include "Arduino.h"
#include "fb_gfx.h"
#include "soc/soc.h" 
#include "soc/rtc_cntl_reg.h"  
#include "esp_http_server.h"
#include <ESPmDNS.h>
#include <esp_now.h>

typedef enum {
  MSG_CREDS,
  MSG_ACK_OK,
  MSG_ACK_FAIL
} msg_type_t;

typedef struct {
  msg_type_t type;
  char ssid[32];
  char password[64];
} esp_packet_t;

#endif