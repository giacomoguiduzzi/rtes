#include <MAX44009.h>
#include <Adafruit_BME280.h>
#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <SoftwareSerial.h>

#ifdef configUSE_TIME_SLICING
#undef configUSE_TIME_SLICING
#define configUSE_TIME_SLICING 0 // Enables time slicing, it's 1 in the header file
#endif

#define SEALEVELPRESSURE_HPA (1013.25)//< Average sea level pressure is 1013.25 hPa

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

uint8_t *written_data;

#define SENSORS_DELAY pdMS_TO_TICKS(sensors_delay)
#define CHECK_DATA_DELAY pdMS_TO_TICKS(250)
#define DELAY_FLAG 0x64 // "d" letter

sensors_data_t *sensors_data;
uint8_t *sent_data;

// HardwareSerial GY39_UART1 = HardwareSerial(); 
SoftwareSerial GY39_UART1 = SoftwareSerial(10, 11); // To read from sensor chip
SoftwareSerial ESP8266_UART2 = SoftwareSerial(12, 13); // (RXPin, TXPin)

// define two tasks for Blink & AnalogRead
void TaskGetTemp(void *pvParameters);
void TaskGetHum(void *pvParameters);
void TaskGetPress(void *pvParameters);
void TaskGetAlt(void *pvParameters);
void TaskGetBright(void *pvParameters);
void TaskSendData(void *pvParameters);
void TaskGetDelay(void *pvParameters);

// SemaphoreHandle_t uart1_mutex, uart2_mutex, sem_send_data;
SemaphoreHandle_t uart_mutex, sem_send_data; // only 1 mutex for UART since only 1 SoftwareSerial UART can be used at a time

// the setup function runs once when you press reset or power the board
void setup() {
  // debug purposes, can't use the hardware serial since it is connected to the USB too and this would print garbage
  Serial.begin(115200);

  Serial.println("Setting up stuff.");
  // Init sensors_data struct
  sensors_data = (sensors_data_t *)malloc(sizeof(sensors_data));
  sensors_data->temperature = 0.0;
  sensors_data->humidity = 0.0;
  sensors_data->pressure = 0.0;
  sensors_data->altitude = 0.0;
  sensors_data->brightness = 0.0;

  sent_data = (uint8_t *)sensors_data;

  written_data = (uint8_t *)malloc(sizeof(uint8_t) * 5);

  Serial.println("Set up data structures.");

  bool status;
  do{
    status = bme280.begin(0x76);
    if(!status){
      Serial.println("Could not find a valid BME280 sensor, check wiring!");
      delay(500);
    }
  }while(!status);

  //ESP8266_UART2.begin(38400, SERIAL_8E1);
  ESP8266_UART2.begin(38400); // SERIAL_8E1 not supported

  Serial.println("Set up Serial lines for communication.");

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

  do {
    uart_mutex = xSemaphoreCreateMutex();

    if (uart_mutex == NULL)
      Serial.println("Error while creating uart1_mutex.");
  } while (uart_mutex == NULL);

  do {
    sem_send_data = xSemaphoreCreateMutex();

    if (sem_send_data == NULL)
      Serial.println("Error while creating uart1_mutex.");
  } while (sem_send_data == NULL);

  // Set the semaphore to 0 so that the TaskSendData function blocks first thing
  xSemaphoreTake(sem_send_data, portMAX_DELAY);

  Serial.println("Set up FreeRTOS mutexes / semaphores.");

  // Now set up tasks to run independently.
  
  // xTaskCreate(TaskGetTemp, (const portCHAR *)"getTemperature", 128, NULL, 2, NULL);
  // xTaskCreate(TaskGetHum, (const portCHAR *)"getHumidity", 128, NULL, 2, NULL);
  // xTaskCreate(TaskGetPress, (const portCHAR *)"getPressure", 128, NULL, 2, NULL);
  // xTaskCreate(TaskGetAlt, (const portCHAR *)"getAltitude", 128, NULL, 2, NULL);
  // xTaskCreate(TaskGetBright, (const portCHAR *)"getBrightness", 128, NULL, 2, NULL);
  // xTaskCreate(TaskSendData, (const portCHAR *)"sendData", 128, NULL, 3, NULL);
  // xTaskCreate(TaskGetDelay, (const portCHAR *)"getNewDelay", 128, NULL, 3, NULL);
  //             func name | human readable name | stack size | priority
  xTaskCreate(TaskGetTemp, "getTemperature", 128, NULL, 2, NULL);
  xTaskCreate(TaskGetHum, "getHumidity", 128, NULL, 2, NULL);
  xTaskCreate(TaskGetPress, "getPressure", 128, NULL, 2, NULL);
  xTaskCreate(TaskGetAlt, "getAltitude", 128, NULL, 2, NULL);
  xTaskCreate(TaskGetBright, "getBrightness", 128, NULL, 2, NULL);
  xTaskCreate(TaskGetDelay, "getNewDelay", 128, NULL, 3, NULL);

  Serial.println("Set up FreeRTOS tasks. Scheduler starting.");

  // Now the task scheduler, which takes over control of scheduling individual tasks, is automatically started.
}

void loop() {} // Empty. Things are done in Tasks.

/*--------------------------------------------------*/
/*---------------------- Tasks ---------------------*/
/*--------------------------------------------------*/

void TaskGetTemp(void *pvParameters)  // This is a task. pvParameters is necessary, FreeRTOS gets angry otherwise
{  
  (void) pvParameters; // Avoids the warning on unused parameter
  const uint8_t idx = 0;
  TickType_t start_tick, end_tick;
  uint32_t delay_;

  Serial.println("TaskGetTemp initialized.");

  for (;;) // A Task shall never return or exit.
  {
    start_tick = xTaskGetTickCount();
    // xSemaphoreTake(uart1_mutex, portMAX_DELAY);
    xSemaphoreTake(uart_mutex, portMAX_DELAY);
    // read sensor
    sensors_data->temperature = bme280.readTemperature();
    xSemaphoreGive(uart_mutex);

    written_data[idx] = 1;

    // TODO: This brings the task to be preempted, verify if this delay thing is correct
    if (sum(written_data) >= 5)
      xSemaphoreGive(sem_send_data);

    end_tick = xTaskGetTickCount();
    delay_ = SENSORS_DELAY - (end_tick - start_tick);

    if (delay_ > 0)
    vTaskDelay(delay_);
  }
}

void TaskGetHum(void *pvParameters)
{
  (void) pvParameters; // Avoids the warning on unused parameter
  const uint8_t idx = 1;
  TickType_t start_tick, end_tick;
  uint32_t delay_;

  for (;;) // A Task shall never return or exit.
  {
    start_tick = xTaskGetTickCount();
    // xSemaphoreTake(uart1_mutex, portMAX_DELAY);
    xSemaphoreTake(uart_mutex, portMAX_DELAY);
    // read sensor
    sensors_data->humidity = bme280.readHumidity();
    xSemaphoreGive(uart_mutex);

    written_data[idx] = 1;

    // TODO: This brings the task to be preempted, verify if this delay thing is correct
    if (sum(written_data) >= 5)
      xSemaphoreGive(sem_send_data);

    end_tick = xTaskGetTickCount();
    delay_ = SENSORS_DELAY - (end_tick - start_tick);

    if (delay_ > 0)
    vTaskDelay(delay_);
  }
}

void TaskGetPress(void *pvParameters)
{
  (void) pvParameters; // Avoids the warning on unused parameter
  const uint8_t idx = 2;
  TickType_t start_tick, end_tick;
  uint32_t delay_;

  for (;;) // A Task shall never return or exit.
  {
    start_tick = xTaskGetTickCount();
    // xSemaphoreTake(uart1_mutex, portMAX_DELAY);
    xSemaphoreTake(uart_mutex, portMAX_DELAY);
    // read sensor
    sensors_data->pressure = bme280.readPressure();
    xSemaphoreGive(uart_mutex);

    written_data[idx] = 1;

    // TODO: This brings the task to be preempted, verify if this delay thing is correct
    if (sum(written_data) >= 5)
      xSemaphoreGive(sem_send_data);

    end_tick = xTaskGetTickCount();
    delay_ = SENSORS_DELAY - (end_tick - start_tick);

    if (delay_ > 0)
    vTaskDelay(delay_);
  }
}

void TaskGetAlt(void *pvParameters)
{
  (void) pvParameters; // Avoids the warning on unused parameter
  const uint8_t idx = 3;
  TickType_t start_tick, end_tick;
  uint32_t delay_;

  for (;;) // A Task shall never return or exit.
  {
    start_tick = xTaskGetTickCount();
    // xSemaphoreTake(uart1_mutex, portMAX_DELAY);
    xSemaphoreTake(uart_mutex, portMAX_DELAY);
    // read sensor
    sensors_data->altitude = bme280.readAltitude(SEALEVELPRESSURE_HPA);
    xSemaphoreGive(uart_mutex);

    written_data[idx] = 1;

    // TODO: This brings the task to be preempted, verify if this delay thing is correct
    if (sum(written_data) >= 5)
      xSemaphoreGive(sem_send_data);

    end_tick = xTaskGetTickCount();
    delay_ = SENSORS_DELAY - (end_tick - start_tick);

    if (delay_ > 0)
    vTaskDelay(delay_);
  }
}

void TaskGetBright(void *pvParameters)
{
  (void) pvParameters; // Avoids the warning on unused parameter
  const uint8_t idx = 4;
  TickType_t start_tick, end_tick;
  uint32_t delay_;

  for (;;) // A Task shall never return or exit.
  {
    start_tick = xTaskGetTickCount();
    // xSemaphoreTake(uart1_mutex, portMAX_DELAY);
    xSemaphoreTake(uart_mutex, portMAX_DELAY);
    // read sensor
    sensors_data->brightness = light_sensor.get_lux();
    xSemaphoreGive(uart_mutex);

    written_data[idx] = 1;

    // TODO: This brings the task to be preempted, verify if this delay thing is correct
    if (sum(written_data) >= 5)
      xSemaphoreGive(sem_send_data);

    end_tick = xTaskGetTickCount();
    delay_ = SENSORS_DELAY - (end_tick - start_tick);

    if (delay_ > 0)
    vTaskDelay(delay_);
  }
}

void TaskSendData(void *pvParameters) // No delay for this task as it always waits on the semaphore (it's kind of an aperiodic task)
{
  (void) pvParameters; // Avoids the warning on unused parameter
  
  for (;;)
  {
    // Block here always, waiting for the sensor-reading-tasks to unblock me
    xSemaphoreTake(sem_send_data, portMAX_DELAY);

    // start_tick = xTaskGetTickCount();

    // Block communication
    xSemaphoreTake(uart_mutex, portMAX_DELAY);
    // Avoid other tasks to write on the data structure
    // xSemaphoreTake(uart1_mutex, portMAX_DELAY);
    // Avoid TaskGetDelay to send on the same channel while transmitting data
    // xSemaphoreTake(uart2_mutex, portMAX_DELAY);
    // Reset writing values for the other tasks
    reset_written_data();

    // Send data to ESP8266
    for (uint8_t i = 0; i < sizeof(sensors_data_t); i++)
      ESP8266_UART2.write(sent_data[i]);

    // Release mutex on UART communication
    xSemaphoreGive(uart_mutex);
    // Release mutex on data struct
    // xSemaphoreGive(uart1_mutex);
    // Release mutex on UART2 to send back delay response (eventually)
    // xSemaphoreGive(uart2_mutex);
    // Never xSemaphoreGive on the sem_send_data so that the task will block on the next for loop iteration
    // TODO: Set this delay to the sum of the WCETS of the sensors read tasks
    // end_tick = xTaskGetTickCount();
    // vTaskDelay(SENSORS_DELAY - (end_tick - start_tick));
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
  uint32_t delay_;

  for (;;) // A Task shall never return or exit.
  {
    start_tick = xTaskGetTickCount();
    if (ESP8266_UART2.available()) {
      xSemaphoreTake(uart_mutex, portMAX_DELAY); // Maybe not necessary?
      // Init array
      new_delay.uint8[0] = new_delay.uint8[1] = 0;
      // Read uint16_t value
      new_delay.uint8[0] = ESP8266_UART2.read();
      new_delay.uint8[1] = ESP8266_UART2.read();

      bool ok = set_new_delay(new_delay.uint16);

      // Take UART2 mutex to send set_delay result
      // xSemaphoreTake(uart2_mutex, portMAX_DELAY);

      answer = (char *)malloc(sizeof(char) * 3);

      if (ok)
        sprintf(answer, "ok");

      else
        sprintf(answer, "no");

      // write flag, this is the answer to the new delay value
      ESP8266_UART2.write(DELAY_FLAG);
      for (uint8_t i = 0; i < 3; i++)
        ESP8266_UART2.write(answer[i]);

      // xSemaphoreGive(uart2_mutex);
      xSemaphoreGive(uart_mutex);

      free(answer);
    }

    end_tick = xTaskGetTickCount();

    delay_ = CHECK_DATA_DELAY - (end_tick - start_tick);

    if (delay_ > 0)
      vTaskDelay(delay_);
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
      Serial.print("There was an error while setting new delay: ");
      Serial.println(new_delay);
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
