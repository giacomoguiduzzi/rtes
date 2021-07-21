#ifndef WIFI_LIB
#define WIFI_LIB
#include "stm32l4xx_hal.h"
#include "wifi.h"
#include "sensors.h"
#include <math.h>

#define SSID     "Vodafone - Packets Are Coming"
#define PASSWORD "Arouteroficeandfire96!"
#define PORT           80

/* Private typedef------------------------------------------------------------*/
typedef enum
{
  WS_IDLE = 0,
  WS_CONNECTED,
  WS_DISCONNECTED,
  WS_ERROR,
} WebServerState_t;

/* Private variables ---------------------------------------------------------*/
static   uint8_t http[1024];
static   uint8_t resp[1024];
uint16_t respLen;
uint8_t  IP_Addr[4];
uint8_t  MAC_Addr[6];
int32_t Socket;
static   WebServerState_t  State = WS_ERROR;
GPIO_TypeDef *wifi_error_led_port;
uint16_t error_led;

/* Private function prototypes -----------------------------------------------*/
void Initialize_WiFi(GPIO_TypeDef * led_port, uint16_t led);
WIFI_Status_t SendWebPage(sensor_data_t sensor_data);
void WebServerProcess(void);

extern sensors_delay_t delay;
extern sensor_data_t sensor_data;

#endif
