#include "wifi_lib.h"

void Initialize_WiFi(GPIO_TypeDef * led_port, uint16_t led) {

	wifi_error_led_port = led_port;
	error_led = led;
	Socket = -1;

	if (WIFI_Init() == WIFI_STATUS_OK) {

		if (WIFI_GetMAC_Address(MAC_Addr) == WIFI_STATUS_OK) {

			if (WIFI_Connect(SSID, PASSWORD, WIFI_ECN_WPA2_PSK) == WIFI_STATUS_OK) {
				if (WIFI_GetIP_Address(IP_Addr) == WIFI_STATUS_OK)
					State = WS_IDLE;
				else {
					HAL_GPIO_TogglePin(wifi_error_led_port, error_led);
					State = WS_ERROR;
					return;
				}
			}
			else {
				HAL_GPIO_TogglePin(wifi_error_led_port, error_led);
				State = WS_ERROR;
				return;
			}
		}
		else {
			HAL_GPIO_TogglePin(wifi_error_led_port, error_led);
			State = WS_ERROR;
			return;
		}
	}
	else {
		HAL_GPIO_TogglePin(wifi_error_led_port, error_led);
		State = WS_ERROR;
		return;
	}
}

void WebServerProcess(void)
{
	WIFI_Status_t ret;

  switch(State)
  {
  case WS_IDLE:
    Socket = 0;
    WIFI_StartServer(Socket, WIFI_TCP_PROTOCOL, "", PORT);

    if(Socket != -1)
      State = WS_CONNECTED;
    else
      State = WS_ERROR;
    break;

  case WS_CONNECTED:
    WIFI_ReceiveData(Socket, resp, 1200, &respLen, 1000);

    if( respLen > 0)
    {
    	// Put up data on the page
      if(strstr((char *)resp, "GET")) /* GET: put web page */
      {
    	  ret = SendWebPage(sensor_data);

    	  if(ret == WIFI_STATUS_ERROR){
    		  printf("There was an error during web page's sending operation.\n\r");
    		  break;
    	  }
      }

      // Get refresh rate of data and set it
      else if(strstr((char *)resp, "POST"))/* POST: received info */
      {
          if(strstr((char *)resp, "radio"))
          {
            if(strstr((char *)resp, "radio=500"))
            	delay = FAST;

            else if(strstr((char *)resp, "radio=1000"))
            	delay = MEDIUM;

            else if(strstr((char *)resp, "radio=2500"))
            	delay = SLOW;

            else if(strstr((char * )resp, "radio=5000"))
            	delay = VERY_SLOW;

            else if(strstr((char *)resp, "radio=10000"))
            	delay = TAKE_A_BREAK;

            if(SendWebPage(sensor_data) != WIFI_STATUS_OK)
            {
              State = WS_ERROR;
          }
        }
      }
    }
    if(WIFI_StopServer(Socket) == WIFI_STATUS_OK)
    {
      WIFI_StartServer(Socket, WIFI_TCP_PROTOCOL, "", PORT);
    }
    else
    {
      State = WS_ERROR;
    }
    break;
  case WS_ERROR:
  default:
    break;
  }
}

WIFI_Status_t SendWebPage(sensor_data_t sensor_data)
{
  uint8_t  temp[50], hum[50], press[50], magneto_dir[50];
  int dataInt1, dataFrac, dataInt2;
  uint16_t SentDataLength;
  WIFI_Status_t ret;

  /* construct web page content */
  strcpy((char *)http, (char *)"HTTP/1.0 200 OK\r\nContent-Type: text/html\r\nPragma: no-cache\r\n\r\n");
  strcat((char *)http, (char *)"<html>\r\n<body>\r\n");
  strcat((char *)http, (char *)"<title>STM32 Web Server</title>\r\n");
  strcat((char *)http, (char *)"<h2>InventekSys : Web Server using Es-Wifi with STM32</h2>\r\n");
  strcat((char *)http, (char *)"<br /><hr>\r\n");

  // Add temperature
  strcat((char *)http, (char *)"<p><form method=\"POST\"><strong>Temperature: <input type=\"text\" size=2 value=\"");
  dataInt1 = sensor_data.temperature;
  dataFrac = sensor_data.temperature - dataInt1;
  dataInt2 = trunc(dataFrac * 100);
  sprintf((char *)temp, "%d.%02d", dataInt1, dataInt2);
  strcat((char *)http, (char *)temp);
  strcat((char *)http, (char *)"\">°C");

  // Add humidity
  strcat((char *)http, (char *)"Humidity: <input type=\"text\" size=2 value=\"");
  dataInt1 = sensor_data.humidity;
  dataFrac = sensor_data.humidity - dataInt1;
  dataInt2 = trunc(dataFrac * 100);
  sprintf((char *)hum, "%d.%02d", dataInt1, dataInt2);
  strcat((char *)http, (char *)hum);
  strcat((char *)http, (char *)"\">\%");

  // Add pressure
  strcat((char *)http, (char *)"Pressure: <input type=\"text\" size=2 value=\"");
  dataInt1 = sensor_data.pressure;
  dataFrac = sensor_data.pressure - dataInt1;
  dataInt2 = trunc(dataFrac * 100);
  sprintf((char *)press, "%d.%02d", dataInt1, dataInt2);
  strcat((char *)http, (char *)press);
  strcat((char *)http, (char *)"\"> hPa");

  // Add north direction
  strcat((char *)http, (char *)"North direction: <input type=\"text\" size=2 value=\"");
  dataInt1 = sensor_data.north_direction;
  dataFrac = sensor_data.north_direction - dataInt1;
  dataInt2 = trunc(dataFrac * 100);
  sprintf((char *)magneto_dir, "%d.%02d", dataInt1, dataInt2);
  strcat((char *)http, (char *)magneto_dir);
  strcat((char *)http, (char *)"\">°");

  // Add delay edit
  strcat((char *)http, (char *)"<p>Delay:<br><input type=\"radio\" name=\"radio\" value=\"500\" >0.5 seconds");
  strcat((char *)http, (char *)"<br><input type=\"radio\" name=\"radio\" value=\"1000\" checked>1 second");
  strcat((char *)http, (char *)"<br><input type=\"radio\" name=\"radio\" value=\"2500\" checked>2.5 seconds");
  strcat((char *)http, (char *)"<br><input type=\"radio\" name=\"radio\" value=\"5000\" checked>5 seconds");
  strcat((char *)http, (char *)"<br><input type=\"radio\" name=\"radio\" value=\"10000\" checked>10 seconds");

  strcat((char *)http, (char *)"</strong><p><input type=\"submit\"></form></span>");
  strcat((char *)http, (char *)"</body>\r\n</html>\r\n");

  ret = WIFI_SendData(0, (uint8_t *)http, strlen((char *)http), &SentDataLength, 1000);

  if((ret == WIFI_STATUS_OK) && (SentDataLength != strlen((char *)http)))
  {
    ret = WIFI_STATUS_ERROR;
  }

  return ret;
}
