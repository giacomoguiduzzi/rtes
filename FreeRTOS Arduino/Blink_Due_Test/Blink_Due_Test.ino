#include <FreeRTOS.h>
// #include <semphr.h>
#include <task.h>
#include <Wire.h>
#include <MAX44009.h>
#include <Adafruit_BME280.h>

// SemaphoreHandle_t led_mutex;
Adafruit_BME280 bme280;
MAX44009 light_sensor;

void blink1(void *pvParameters)
{
  Serial.println("Blink1 task started");
  
  for( ;; ) {
    //xSemaphoreTake(led_mutex, portMAX_DELAY);
    digitalWrite(LED_BUILTIN, HIGH);
    vTaskDelay(1000);
    //xSemaphoreGive(led_mutex);
    //xSemaphoreTake(led_mutex, portMAX_DELAY);
    digitalWrite(LED_BUILTIN, LOW);
    vTaskDelay(1000);
    //xSemaphoreGive(led_mutex);
  }
}

void blink2(void *pvParameters)
{
  Serial.println("Blink2 task started");
  pinMode(LED_BUILTIN, OUTPUT);
  
  for( ;; ) {
    // xSemaphoreTake(led_mutex, portMAX_DELAY);
    vTaskDelay(1000);
    digitalWrite(LED_BUILTIN, LOW);
    vTaskDelay(1000);
    // xSemaphoreGive(led_mutex);
    // xSemaphoreTake(led_mutex, portMAX_DELAY);
    digitalWrite(LED_BUILTIN, HIGH);
    
    // xSemaphoreGive(led_mutex);
  }
}

void setup() 
{
  Serial.begin(115200);

  /* do {
    led_mutex = xSemaphoreCreateMutex();

    if (led_mutex == NULL)
      Serial.println("Error while creating mutex.");
  } while (led_mutex == NULL);

  Serial.println("Created mutex.");
  delay(3000); */

  Wire.begin();

  bool status;
  do{
    status = bme280.begin(0x76);
    if(!status){
      Serial.println("Could not find a valid BME280 sensor, check wiring!");
      // vTaskDelay(pdMS_TO_TICKS(500));
      delay(500);
    }
  }while(!status);

  Serial.println("Initialized GY-39 sensor chip");

  pinMode(LED_BUILTIN, OUTPUT);

  xTaskCreate(blink1, (const portCHAR *)"blink1", 128, NULL, 2, NULL);
  xTaskCreate(blink2, (const portCHAR *)"blink2", 128, NULL, 2, NULL);
  vTaskStartScheduler();

  Serial.println("Failed to start FreeRTOS scheduler");
  while(1);
}

void loop()
{
}
