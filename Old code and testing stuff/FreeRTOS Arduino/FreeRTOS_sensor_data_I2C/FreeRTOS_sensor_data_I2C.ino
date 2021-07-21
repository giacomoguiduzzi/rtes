#include <FreeRTOS.h>
#include <semphr.h>
#include <task.h>
#include <Wire.h>
#include <MAX44009.h>
#include <Adafruit_BME280.h>
// #include "MemoryFree.h"
// #include <SoftwareSerial.h>

// #ifdef configUSE_TIME_SLICING
// #undef configUSE_TIME_SLICING
// #define configUSE_TIME_SLICING 0 // Enables time slicing, it's 1 in the header file
// #endif

#define SEALEVELPRESSURE_HPA (1013.25)//< Average sea level pressure is 1013.25 hPa
#define ESP8266_ADDR 0x33
#define SENSORS_DELAY pdMS_TO_TICKS(sensors_delay)
#define CHECK_DATA_DELAY pdMS_TO_TICKS(250)
#define DELAY_FLAG 0x64 // "d" letter

Adafruit_BME280 bme280;
MAX44009 light_sensor;

typedef struct {
  float temperature;
  float humidity;
  float pressure;
  float altitude;
  float brightness;
} sensors_data_t;

typedef enum
{
  FAST = 500,
  MEDIUM = 1000,
  SLOW = 2500,
  VERY_SLOW = 5000,
  TAKE_A_BREAK = 10000
} sensors_delay_t;

uint16_t sensors_delay = FAST;

uint8_t written_data[5];

sensors_data_t sensors_data;
uint8_t *sent_data;

/* void TaskGetTemp(void *pvParameters);
void TaskGetHum(void *pvParameters);
void TaskGetPress(void *pvParameters);
void TaskGetAlt(void *pvParameters);
void TaskGetBright(void *pvParameters);
void TaskSendData(void *pvParameters);
void TaskGetDelay(void *pvParameters); */

volatile TaskHandle_t xTaskSendDataHandle;

SemaphoreHandle_t I2C_bus_mutex, sem_send_data, serial_mutex;
// bool I2C_bus_mutex, sem_send_data, serial_mutex;

// the setup function runs once when you press reset or power the board
void setup() {
  // debug purposes, can't use the hardware serial since it is connected to the USB too and this would print garbage
  Serial.begin(115200);
  // wait for Serial to connect
  while(!Serial);

  Serial.println("Setting up data structures.");
  // Init sensors_data struct
  sensors_data.temperature = 0.0;
  sensors_data.humidity = 0.0;
  sensors_data.pressure = 0.0;
  sensors_data.altitude = 0.0;
  sensors_data.brightness = 0.0;

  sent_data = (uint8_t *)&sensors_data;

  xTaskSendDataHandle = NULL;

  Wire.begin();

  bool status;
  do{
    status = bme280.begin(0x76);
    if(!status){
      Serial.println("Could not find a valid BME280 sensor, check wiring!");
      vTaskDelay(pdMS_TO_TICKS(500));
    }
  }while(!status);

  Serial.println("Set up Serial lines for communication.");

  //vTaskDelay(pdMS_TO_TICKS(3));
  delay(3000);

  /* do {
    uart1_mutex = xSemaphoreCreateMutex();

    if (uart1_mutex == NULL)
      Serial.println("Error while creating uart1_mutex.");
  } while (uart1_mutex == NULL);*/

  /* do {
    uart2_mutex = xSemaphoreCreateMutex();

    if (uart2_mutex == NULL)
      Serial.println("Error while creating uart2_mutex.");
  } while (uart2_mutex == NULL);*/

  /* do {
    I2C_bus_mutex = xSemaphoreCreateMutex();

    if (I2C_bus_mutex == NULL)
      Serial.println("Error while creating uart1_mutex.");
  } while (I2C_bus_mutex == NULL);

  do {
    sem_send_data = xSemaphoreCreateMutex();

    if (sem_send_data == NULL)
      Serial.println("Error while creating uart1_mutex.");
  } while (sem_send_data == NULL); */

  do {
    serial_mutex = xSemaphoreCreateMutex();

    if (serial_mutex == NULL)
      Serial.println("Error while creating uart1_mutex.");
  } while (serial_mutex == NULL);

  // Set the semaphore to 0 so that the TaskSendData function blocks first thing
  // xSemaphoreTake(sem_send_data, portMAX_DELAY);

  // I2C_bus_mutex = sem_send_data = serial_mutex = false;

  Serial.println("Set up FreeRTOS mutexes / semaphores.");

  // Now set up tasks to run independently.
  
  // xTaskCreate(TaskGetTemp, (const portCHAR *)"getTemperature", 128, NULL, 2, NULL);
  // xTaskCreate(TaskGetHum, (const portCHAR *)"getHumidity", 128, NULL, 2, NULL);
  // xTaskCreate(TaskGetPress, (const portCHAR *)"getPressure", 128, NULL, 2, NULL);
  // xTaskCreate(TaskGetAlt, (const portCHAR *)"getAltitude", 128, NULL, 2, NULL);
  // xTaskCreate(TaskGetBright, (const portCHAR *)"getBrightness", 128, NULL, 2, NULL);
  // xTaskCreate(TaskSendData, (const portCHAR *)"sendData", 128, NULL, 3, NULL);
  // xTaskCreate(TaskGetDelay, (const portCHAR *)"getNewDelay", 128, NULL, 3, NULL);
  // xTaskCreate(TaskSendData, "sendData", 128, NULL, 3, NULL);
  // Serial.println("Created sendData task.");
  // xTaskCreate(TaskGetDelay, "getNewDelay", 128, NULL, 3, NULL);

  xSemaphoreTake(serial_mutex, portMAX_DELAY);
  vTaskDelay(pdMS_TO_TICKS(3000));
  
  // BaseType_t xReturned;

  Serial.println("About to create tasks.");

  vTaskDelay(pdMS_TO_TICKS(3000));

  //             func name | human readable name | stack size | priority
  // xTaskCreate(TaskGetTemp, "getTemperature", 64, NULL, 2, NULL);
  // xTaskCreate(TaskGetHum, "getHumidity", 64, NULL, 2, NULL);
  /* xReturned = xTaskCreate(TaskGetTemp, "getTemperature", 64, NULL, 2, NULL);
  if(xReturned != pdPASS)
    Serial.println("Error during creation of TaskGetTemp.");
    
  xReturned = xTaskCreate(TaskGetHum, "getHumidity", 64, NULL, 2, NULL);
  if(xReturned != pdPASS)
    Serial.println("Error during creation of TaskGetHum."); */
  
  /*xReturned = xTaskCreate(TaskGetPress, "getPressure", 64, NULL, 2, NULL);
  if(xReturned != pdPASS)
    Serial.println("Error during creation of TaskGetPress."); */
    
  /*xReturned = xTaskCreate(TaskGetAlt, "getAltitude", 64, NULL, 2, NULL);
  if(xReturned != pdPASS)
    Serial.println("Error during creation of TaskGetAlt.");

  xReturned = xTaskCreate(TaskGetBright, "getBrightness", 64, NULL, 2, NULL);
  if(xReturned != pdPASS)
    Serial.println("Error during creation of TaskGetBright.");*/
  
  /* xReturned = xTaskCreate(TaskGetDelay, "getNewDelay", 384, NULL, 3, NULL);
  if(xReturned != pdPASS)
    Serial.println("Error during creation of TaskGetDelay.");

  xReturned = xTaskCreate(TaskSendData, "sendData", 384, NULL, 3, &xTaskSendDataHandle);
  if(xReturned != pdPASS)
    Serial.println("Error during creation of TaskSendData"); */

  // xSemaphoreTake(serial_mutex, portMAX_DELAY);

  Serial.println("Set up FreeRTOS tasks. Scheduler starting.");

  xSemaphoreGive(serial_mutex);
  
  // Now the task scheduler, which takes over control of scheduling individual tasks, is automatically started.
  vTaskStartScheduler();
}

/* int availableMemory() { 
  int size = 2048; 
  byte *buf; 
  while ((buf = (byte *) malloc(--size)) == NULL); 
  free(buf); 
  return size; 
} */

void loop() {
  xSemaphoreTake(serial_mutex, portMAX_DELAY);
  Serial.println("looping");
  // Serial.println(freeMemory());
  // Serial.println(availableMemory());
  xSemaphoreGive(serial_mutex);
  delay(sensors_delay);
} // Empty. Things are done in Tasks.

/*--------------------------------------------------*/
/*---------------------- Tasks ---------------------*/
/*--------------------------------------------------*/

void TaskGetTemp(void *pvParameters)  // This is a task. pvParameters is necessary, FreeRTOS gets angry otherwise
{  
  (void) pvParameters; // Avoids the warning on unused parameter
  const uint8_t idx = 0;
  TickType_t start_tick, end_tick;
  int delay_;

  xSemaphoreTake(serial_mutex, portMAX_DELAY);
  Serial.println("TaskGetTemp initialized.");
  xSemaphoreGive(serial_mutex);

  for (;;) // A Task shall never return or exit.
  {
    start_tick = xTaskGetTickCount();
    // xSemaphoreTake(uart1_mutex, portMAX_DELAY);
    xSemaphoreTake(I2C_bus_mutex, portMAX_DELAY);
    while(I2C_bus_mutex)
      vTaskDelay(portTICK_PERIOD_MS);
    // I2C_bus_mutex = true;
    // read sensor
    sensors_data.temperature = bme280.readTemperature();
    xSemaphoreGive(I2C_bus_mutex);
    xSemaphoreTake(serial_mutex, portMAX_DELAY);
    Serial.print(sensors_data.temperature);
    Serial.println("°C");
    xSemaphoreGive(serial_mutex);
    // I2C_bus_mutex = false;

    written_data[idx] = 1;

    // TODO: This brings the task to be preempted, verify if this delay thing is correct
    if (sum(written_data) >= 5)
      xTaskNotifyGive(xTaskSendDataHandle);
      // xSemaphoreGive(sem_send_data);

    end_tick = xTaskGetTickCount();
    delay_ = SENSORS_DELAY - (end_tick - start_tick);

    if (delay_ > 0)
    vTaskDelay((uint32_t) delay_);
  }
}

void TaskGetHum(void *pvParameters)
{
  (void) pvParameters; // Avoids the warning on unused parameter
  const uint8_t idx = 1;
  TickType_t start_tick, end_tick;
  int delay_;

  xSemaphoreTake(serial_mutex, portMAX_DELAY);
  Serial.println("TaskGetHum initialized.");
  xSemaphoreGive(serial_mutex);

  for (;;) // A Task shall never return or exit.
  {
    start_tick = xTaskGetTickCount();
    // xSemaphoreTake(uart1_mutex, portMAX_DELAY);
    xSemaphoreTake(I2C_bus_mutex, portMAX_DELAY);
    while(I2C_bus_mutex)
      vTaskDelay(portTICK_PERIOD_MS);
    // I2C_bus_mutex = true;
    // read sensor
    sensors_data.humidity = bme280.readHumidity();
    xSemaphoreGive(I2C_bus_mutex);

    xSemaphoreTake(serial_mutex, portMAX_DELAY);
    // Serial.print("Read new humidity: ");
    // Serial.print(sensors_data.humidity);
    Serial.print(sensors_data.humidity);
    Serial.println("%");
    xSemaphoreGive(serial_mutex);
    // I2C_bus_mutex = false;

    written_data[idx] = 1;

    // TODO: This brings the task to be preempted, verify if this delay thing is correct
    if (sum(written_data) >= 5)
      xTaskNotifyGive(xTaskSendDataHandle);
      // xSemaphoreGive(sem_send_data);

    end_tick = xTaskGetTickCount();
    delay_ = SENSORS_DELAY - (end_tick - start_tick);

    if (delay_ > 0)
    vTaskDelay((uint32_t) delay_);
  }
}

void TaskGetPress(void *pvParameters)
{
  (void) pvParameters; // Avoids the warning on unused parameter
  const uint8_t idx = 2;
  TickType_t start_tick, end_tick;
  int delay_;

  for (;;) // A Task shall never return or exit.
  {
    start_tick = xTaskGetTickCount();
    // xSemaphoreTake(uart1_mutex, portMAX_DELAY);
    xSemaphoreTake(I2C_bus_mutex, portMAX_DELAY);
    while(I2C_bus_mutex)
      vTaskDelay(portTICK_PERIOD_MS);
    // I2C_bus_mutex = true;
    // read sensor
    sensors_data.pressure = bme280.readPressure();
    xSemaphoreGive(I2C_bus_mutex);

    xSemaphoreTake(serial_mutex, portMAX_DELAY);
    Serial.print(sensors_data.pressure);
    Serial.println("hPa");
    xSemaphoreGive(serial_mutex);
    // I2C_bus_mutex = false;
    
    written_data[idx] = 1;

    // TODO: This brings the task to be preempted, verify if this delay thing is correct
    if (sum(written_data) >= 5)
      xTaskNotifyGive(xTaskSendDataHandle);
      // xSemaphoreGive(sem_send_data);

    end_tick = xTaskGetTickCount();
    delay_ = SENSORS_DELAY - (end_tick - start_tick);

    if (delay_ > 0)
    vTaskDelay((uint32_t) delay_);
  }
}

void TaskGetAlt(void *pvParameters)
{
  (void) pvParameters; // Avoids the warning on unused parameter
  const uint8_t idx = 3;
  TickType_t start_tick, end_tick;
  int delay_;

  for (;;) // A Task shall never return or exit.
  {
    start_tick = xTaskGetTickCount();
    // xSemaphoreTake(uart1_mutex, portMAX_DELAY);
    xSemaphoreTake(I2C_bus_mutex, portMAX_DELAY);
    while(I2C_bus_mutex)
      vTaskDelay(portTICK_PERIOD_MS);
    // I2C_bus_mutex = true;
    // read sensor
    sensors_data.altitude = bme280.readAltitude(SEALEVELPRESSURE_HPA);
    xSemaphoreGive(I2C_bus_mutex);

    xSemaphoreTake(serial_mutex, portMAX_DELAY);
    Serial.print(sensors_data.altitude);
    Serial.println("m");
    xSemaphoreGive(serial_mutex);
    // I2C_bus_mutex = false;

    written_data[idx] = 1;

    // TODO: This brings the task to be preempted, verify if this delay thing is correct
    if (sum(written_data) >= 5)
      xTaskNotifyGive(xTaskSendDataHandle);
      // xSemaphoreGive(sem_send_data);

    end_tick = xTaskGetTickCount();
    delay_ = SENSORS_DELAY - (end_tick - start_tick);

    if (delay_ > 0)
    vTaskDelay((uint32_t) delay_);
  }
}

void TaskGetBright(void *pvParameters)
{
  (void) pvParameters; // Avoids the warning on unused parameter
  const uint8_t idx = 4;
  TickType_t start_tick, end_tick;
  int delay_;

  for (;;) // A Task shall never return or exit.
  {
    start_tick = xTaskGetTickCount();
    // xSemaphoreTake(uart1_mutex, portMAX_DELAY);
    xSemaphoreTake(I2C_bus_mutex, portMAX_DELAY);
    while(I2C_bus_mutex)
      vTaskDelay(portTICK_PERIOD_MS);
    // I2C_bus_mutex = true;
    // read sensor
    sensors_data.brightness = light_sensor.get_lux();
    xSemaphoreGive(I2C_bus_mutex);

    xSemaphoreTake(serial_mutex, portMAX_DELAY);
    Serial.print(sensors_data.brightness);
    Serial.println("lux");
    xSemaphoreGive(serial_mutex);
    // I2C_bus_mutex = false;

    written_data[idx] = 1;

    // TODO: This brings the task to be preempted, verify if this delay thing is correct
    if (sum(written_data) >= 5)
      xTaskNotifyGive(xTaskSendDataHandle);
      // xSemaphoreGive(sem_send_data);

    end_tick = xTaskGetTickCount();
    delay_ = SENSORS_DELAY - (end_tick - start_tick);

    if (delay_ > 0)
    vTaskDelay((uint32_t) delay_);
  }
}

void TaskSendData(void *pvParameters) // No delay for this task as it always waits on the semaphore (it's kind of an aperiodic task)
{
  (void) pvParameters; // Avoids the warning on unused parameter
  
  for (;;)
  {
    // Block here always, waiting for the sensor-reading-tasks to unblock me
    //xSemaphoreTake(sem_send_data, portMAX_DELAY);
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    if(sum(written_data) >= 5){

      // Block communication
      xSemaphoreTake(I2C_bus_mutex, portMAX_DELAY);
      while(I2C_bus_mutex)
        vTaskDelay(portTICK_PERIOD_MS);
      // I2C_bus_mutex = true;
      // Reset writing values for the other tasks
      reset_written_data();
    
      // Send data to ESP8266
      /* for (uint8_t i = 0; i < sizeof(sensors_data_t); i++)
        ESP8266_UART2.write(sent_data[i]); */
      Wire.beginTransmission(ESP8266_ADDR);

      for(uint8_t i = 0; i < sizeof(sensors_data_t); i++)
        Wire.write(sent_data[i]);

      Wire.endTransmission();
      // Release mutex on I2C communication
      xSemaphoreGive(I2C_bus_mutex);
      // I2C_bus_mutex = false;
    }
    // Never // xSemaphoreGive on the sem_send_data so that the task will block on the next for loop iteration
  }
}

void TaskGetDelay(void *pvParameters)
{
  (void) pvParameters; // Avoids the warning on unused parameter
  
  union {
    uint16_t uint16;
    uint8_t uint8[2];
  } new_delay;

  char *answer;
  // TickType_t is typedef'd as uint32_t
  TickType_t start_tick, end_tick;
  int delay_;

  // xSemaphoreTake(serial_mutex, portMAX_DELAY);
  Serial.println("TaskGetDelay: task initialized.");
  // xSemaphoreGive(serial_mutex);
  
  for (;;) // A Task shall never return or exit.
  {
    start_tick = xTaskGetTickCount();
    xSemaphoreTake(I2C_bus_mutex, portMAX_DELAY);
    while(I2C_bus_mutex)
        vTaskDelay(portTICK_PERIOD_MS);
      // I2C_bus_mutex = true;
      
    Wire.requestFrom(ESP8266_ADDR, 16);
    // xSemaphoreTake(serial_mutex, portMAX_DELAY);
    Serial.println("New delay: ");
    // xSemaphoreGive(serial_mutex);
    // Waiting for data availability
    while(!Wire.available());
    // Init array
    new_delay.uint8[0] = new_delay.uint8[1] = 0;
    // Read uint16_t value
    new_delay.uint8[0] = Wire.read();
    new_delay.uint8[1] = Wire.read();

    if(new_delay.uint16 != 0){
      // xSemaphoreTake(serial_mutex, portMAX_DELAY);
      Serial.print(new_delay.uint16);
      Serial.print(" (");
      Serial.print(new_delay.uint8[0], HEX);
      Serial.print(new_delay.uint8[1], HEX);
      Serial.println(")");
      // xSemaphoreGive(serial_mutex);
  
      bool ok = set_new_delay(new_delay.uint16);
  
      // Take UART2 mutex to send set_delay result
      // // xSemaphoreTake(uart2_mutex, portMAX_DELAY);
  
      answer = (char *)malloc(sizeof(char) * 3);
  
      if (ok){
        sprintf(answer, "ok");
        // xSemaphoreTake(serial_mutex, portMAX_DELAY);
        Serial.println("ok");
        // xSemaphoreGive(serial_mutex);
      }
      else{
        sprintf(answer, "no");
        Serial.println("no");
      }
  
      // write flag, this is the answer to the new delay value
      Wire.beginTransmission(ESP8266_ADDR);
      Wire.write(DELAY_FLAG);
      for (uint8_t i = 0; i < 3; i++)
        Wire.write(answer[i]);
  
      Wire.endTransmission();
  
      free(answer);
    }
    else{
      Serial.println("no new delay");
    }

    xSemaphoreGive(I2C_bus_mutex);
    // I2C_bus_mutex = false;

    end_tick = xTaskGetTickCount();

    delay_ = CHECK_DATA_DELAY - (end_tick - start_tick);

    if (delay_ > 0)
      vTaskDelay((uint32_t) delay_);
  }
}

bool set_new_delay(const uint16_t new_delay) {
  bool ok = true;

  switch (new_delay) {
    case 500:
      sensors_delay = FAST;
      break;
    case 1000:
      sensors_delay = MEDIUM;
      break;
    case 2500:
      sensors_delay = SLOW;
      break;
    case 5000:
      sensors_delay = VERY_SLOW;
      break;
    case 10000:
      sensors_delay = TAKE_A_BREAK;
      break;
    default:
      // xSemaphoreTake(serial_mutex, portMAX_DELAY);
      Serial.print("New delay error: ");
      Serial.println(new_delay);
      // xSemaphoreGive(serial_mutex);
      ok = false;
      break;
  }

  return ok;
}

uint8_t sum(uint8_t *written_data) {
  uint8_t sum_ = 0;

  for (uint8_t i = 0; i < 5; i++) {
    sum_ += written_data[i];
  }

  return sum_;
}

void reset_written_data() {
  for (uint8_t i = 0; i < 5; i++) {
    written_data[i] = 0;
  }
}
