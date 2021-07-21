#include <FreeRTOS.h>
#include <task.h>
#include <Wire.h>
#include <Adafruit_BME280.h>
#include <MAX44009.h>

#define SEALEVELPRESSURE_HPA (1013.25)//< Average sea level pressure is 1013.25 hPa
// #define ESP8266_ADDR 0x33
#define DUE_ADDR 0x33
#define SENSORS_DELAY pdMS_TO_TICKS(sensors_delay)
#define CHECK_DATA_DELAY pdMS_TO_TICKS(2000)
#define DELAY_FLAG 0x64 // "d" letter

Adafruit_BME280 bme; //  I2C Define BME280
MAX44009 light_sensor;    //   I2C Define MAX44009

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

typedef enum{
  LOCKED = true,
  UNLOCKED = false
} bool_mutex;

uint16_t sensors_delay = MEDIUM;

// uint8_t written_data[5];

sensors_data_t sensors_data;
uint8_t *sent_data;

// TaskHandle_t xTaskSendDataHandle;

bool_mutex I2C_bus_mutex, serial_mutex;

typedef enum {DELAY_OK, DELAY_NOT_OK, NONE} delay_status;
delay_status delay_ok;

void Serial_println(const char *string){
  lock(&serial_mutex);
  Serial.println(string);
  unlock(&serial_mutex);
}

void Serial_print(const char *string){
  lock(&serial_mutex);
  Serial.print(string);
  unlock(&serial_mutex);
}

void Serial_print_data(const char *name, float data, const char *unit){
  lock(&serial_mutex);
  Serial.print(name);
  Serial.print(data);
  Serial.println(unit);
  unlock(&serial_mutex);
}

void Serial_print_data_struct(){
  lock(&serial_mutex);
  
  Serial.print("Temperature: ");
  Serial.print(sensors_data.temperature);
  Serial.println("°C");

  Serial.print("Humidity: ");
  Serial.print(sensors_data.humidity);
  Serial.println("%");

  Serial.print("Pressure: ");
  Serial.print(sensors_data.pressure);
  Serial.println("hPa");

  Serial.print("Altitude: ");
  Serial.print(sensors_data.altitude);
  Serial.println("m");

  Serial.print("Brightness: ");
  Serial.print(sensors_data.brightness);
  Serial.println("lux");
  
  unlock(&serial_mutex);
}

static inline void lock(bool_mutex *mutex){
  /* while(__atomic_test_and_set((void *)mutex, __ATOMIC_SEQ_CST))
    vTaskDelay(pdMS_TO_TICKS(1)); // enable preemption on this task waiting for the mutex */
  // Serial.println("Sono dentro lock()");
  while(*mutex == LOCKED){
    vTaskDelay(pdMS_TO_TICKS(100));
  }
    //vTaskDelay(pdMS_TO_TICKS(1));
  
  *mutex = LOCKED;
  // Serial.println("Sto uscendo da lock()");
}

static inline void unlock(bool_mutex *mutex){ *mutex = UNLOCKED; }

void setup() {
  Serial.begin(9600);
  while(!Serial);
  
  Serial.println(F("Setting up data structures."));

  sensors_data.temperature = 0.0;
  sensors_data.humidity = 0.0;
  sensors_data.pressure = 0.0;
  sensors_data.altitude = 0.0;
  sensors_data.brightness = 0.0;

  sent_data = (uint8_t *)&sensors_data;

  delay_ok = DELAY_OK;

  // xTaskSendDataHandle = NULL;

  bool status;
  status = bme.begin(0x76);  
  while(!status) {
    Serial.println(F("Could not find a valid BME280 sensor, check wiring!"));
    delay(500);
    status = bme.begin(0x76);
  }

  Wire.begin(0x33);
  Wire.onReceive(getNewDelay);
  Wire.onRequest(dataRequired);

  Serial.println(F("Set up Serial lines for communication."));

  I2C_bus_mutex = serial_mutex = UNLOCKED;

  Serial.println(F("Set up mutexes."));

  xTaskCreate(TaskReadTemp, (const portCHAR *)"TaskReadTemp", 128, NULL, 2, NULL);
  xTaskCreate(TaskReadHum, (const portCHAR *)"TaskReadHum", 128, NULL, 2, NULL);
  xTaskCreate(TaskReadPress, (const portCHAR *)"TaskReadPress", 128, NULL, 2, NULL);
  xTaskCreate(TaskReadAlt, (const portCHAR *)"TaskReadAlt", 128, NULL, 2, NULL);
  xTaskCreate(TaskReadBright, (const portCHAR *)"TaskReadBright", 128, NULL, 2, NULL);
  // xTaskCreate(TaskGetDelay, (const portCHAR *)"TaskGetDelay", 128, NULL, 3, NULL);
  // xTaskCreate(TaskSendData, (const portCHAR *)"TaskSendData", 128, NULL, 3, &xTaskSendDataHandle);
  // xTaskCreate(TaskReadData, (const portCHAR *)"TaskReadData", 128, NULL, 2, NULL);

  Serial.println(F("Set up FreeRTOS tasks. Scheduler starting."));

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  
  vTaskStartScheduler();

  Serial.println(F("Failed to start FreeRTOS scheduler"));
  while(1);
}

void loop() {
  // Serial_println("\nlooping!\n");
  // delay(500);
}


void TaskReadTemp(void *pvParameters){  // This is a task. pvParameters is necessary, FreeRTOS gets angry otherwise  
  (void) pvParameters; // Avoids the warning on unused parameter
  const uint8_t idx = 0;
  TickType_t start_tick, end_tick;
  int delay_;

  Serial_println("TaskReadTemp initialized.");

  for (;;)
  {
    start_tick = xTaskGetTickCount();
    lock(&I2C_bus_mutex);
    // read sensor
    sensors_data.temperature = bme.readTemperature();
    unlock(&I2C_bus_mutex);
    Serial_print_data("Temperature: ", sensors_data.temperature, "°C");
    // I2C_bus_mutex = false;

    /* written_data[idx] = 1;

    // TODO: This brings the task to be preempted, verify if this delay thing is correct
    if (sum(written_data) >= 5)
      xTaskNotifyGive(xTaskSendDataHandle);*/
      // xSemaphoreGive(sem_send_data);

    end_tick = xTaskGetTickCount();
    delay_ = SENSORS_DELAY - (end_tick - start_tick);

    if (delay_ > 0)
      vTaskDelay((uint32_t) delay_);
  }
}

/* void TaskReadTemp(void *pvParameters){
  (void) pvParameters;

  for(;;){
    lock(&I2C_bus_mutex);
    Serial.print("Temperature = ");
    Serial.print(bme.readTemperature());
    Serial.println(" °C");

    unlock(&I2C_bus_mutex);

    vTaskDelay(pdMS_TO_TICKS(500));
  }
}*/ 

void TaskReadHum(void *pvParameters){  // This is a task. pvParameters is necessary, FreeRTOS gets angry otherwise  
  (void) pvParameters; // Avoids the warning on unused parameter
  const uint8_t idx = 1;
  TickType_t start_tick, end_tick;
  int delay_;

  Serial_println("TaskReadHum initialized.");

  for (;;)
  {
    
    start_tick = xTaskGetTickCount();
    lock(&I2C_bus_mutex);
    // read sensor
    sensors_data.humidity = bme.readHumidity();
    unlock(&I2C_bus_mutex);
    Serial_print_data("Humidity: ", sensors_data.humidity, "%");
    // I2C_bus_mutex = false;

    /* written_data[idx] = 1;

    // TODO: This brings the task to be preempted, verify if this delay thing is correct
    if (sum(written_data) >= 5)
      xTaskNotifyGive(xTaskSendDataHandle); */
      // xSemaphoreGive(sem_send_data);

    end_tick = xTaskGetTickCount();
    delay_ = SENSORS_DELAY - (end_tick - start_tick);

    if (delay_ > 0)
      vTaskDelay((uint32_t) delay_);
  }
}

void TaskReadPress(void *pvParameters){  // This is a task. pvParameters is necessary, FreeRTOS gets angry otherwise  
  (void) pvParameters; // Avoids the warning on unused parameter
  const uint8_t idx = 2;
  TickType_t start_tick, end_tick;
  int delay_;

  Serial_println("TaskReadPress initialized.");

  for (;;)
  {
    
    start_tick = xTaskGetTickCount();
    lock(&I2C_bus_mutex);
    // read sensor
    sensors_data.pressure = bme.readPressure();
    unlock(&I2C_bus_mutex);
    Serial_print_data("Pressure: ", sensors_data.pressure, "hPa");
    // I2C_bus_mutex = false;

    /* written_data[idx] = 1;

    // TODO: This brings the task to be preempted, verify if this delay thing is correct
    if (sum(written_data) >= 5)
      xTaskNotifyGive(xTaskSendDataHandle); */
      // xSemaphoreGive(sem_send_data);

    end_tick = xTaskGetTickCount();
    delay_ = SENSORS_DELAY - (end_tick - start_tick);

    if (delay_ > 0)
      vTaskDelay((uint32_t) delay_);
  }
}

void TaskReadAlt(void *pvParameters){  // This is a task. pvParameters is necessary, FreeRTOS gets angry otherwise  
  (void) pvParameters; // Avoids the warning on unused parameter
  const uint8_t idx = 3;
  TickType_t start_tick, end_tick;
  int delay_;

  Serial_println("TaskReadAlt initialized.");

  for (;;)
  {
    
    start_tick = xTaskGetTickCount();
    lock(&I2C_bus_mutex);
    // read sensor
    sensors_data.altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);
    unlock(&I2C_bus_mutex);
    Serial_print_data("Altitude: ", sensors_data.altitude, "m");
    // I2C_bus_mutex = false;

   /* written_data[idx] = 1;

    // TODO: This brings the task to be preempted, verify if this delay thing is correct
    if (sum(written_data) >= 5)
      xTaskNotifyGive(xTaskSendDataHandle); */
      // xSemaphoreGive(sem_send_data);

    end_tick = xTaskGetTickCount();
    delay_ = SENSORS_DELAY - (end_tick - start_tick);

    if (delay_ > 0)
      vTaskDelay((uint32_t) delay_);
  }
}

void TaskReadBright(void *pvParameters){  // This is a task. pvParameters is necessary, FreeRTOS gets angry otherwise  
  (void) pvParameters; // Avoids the warning on unused parameter
  const uint8_t idx = 4;
  TickType_t start_tick, end_tick;
  int delay_;

  Serial_println("TaskReadBright initialized.");

  for (;;)
  {
    
    start_tick = xTaskGetTickCount();
    lock(&I2C_bus_mutex);
    // read sensor
    sensors_data.brightness = light_sensor.get_lux();
    unlock(&I2C_bus_mutex);
    Serial_print_data("Brightness: ", sensors_data.brightness, "lux");
    // I2C_bus_mutex = false;

    /* written_data[idx] = 1;

    // TODO: This brings the task to be preempted, verify if this delay thing is correct
    if (sum(written_data) >= 5)
      xTaskNotifyGive(xTaskSendDataHandle); */
      // xSemaphoreGive(sem_send_data);

    end_tick = xTaskGetTickCount();
    delay_ = SENSORS_DELAY - (end_tick - start_tick);

    if (delay_ > 0)
      vTaskDelay((uint32_t) delay_);
  }
}

/* void TaskSendData(void *pvParameters) // No delay for this task as it always waits on the semaphore (it's kind of an aperiodic task)
{
  (void) pvParameters; // Avoids the warning on unused parameter

  Serial_println("TaskSendData initialized.");
  
  for (;;)
  {
    // Block here always, waiting for the sensor-reading-tasks to unblock me
    //xSemaphoreTake(sem_send_data, portMAX_DELAY);
    Serial_println("TaskSendData waiting for notification...");
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    Serial_println("TaskSendData has been notified.");

    if(sum(written_data) >= 5){

      // Block communication
      lock(&I2C_bus_mutex);
      // I2C_bus_mutex = true;
      // Reset writing values for the other tasks
      reset_written_data();
    
      // Send data to ESP8266
      Wire.beginTransmission(ESP8266_ADDR);

      for(uint8_t i = 0; i < sizeof(sensors_data_t); i++)
        Wire.write(sent_data[i]);

      Wire.endTransmission();
      // Release mutex on I2C communication
      unlock(&I2C_bus_mutex);
      // I2C_bus_mutex = false;

      Serial_println("Sent to ESP8266 new data structure: ");
      Serial_print_data_struct();
    }
    // Never xSemaphoreGive on the sem_send_data so that the task will block on the next for loop iteration
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

  Serial_println("TaskGetDelay initialized.");
  
  for (;;) // A Task shall never return or exit.
  {
    start_tick = xTaskGetTickCount();
    lock(&I2C_bus_mutex);
      
    Wire.requestFrom(ESP8266_ADDR, 16);
    // lock(&serial_mutex);
    Serial_print("Checking for new delay: ");
    // unlock(&serial_mutex);
    // Waiting for data availability
    for(uint8_t i = 0; i < 3; i++){
      if(Wire.available())
        break;

      vTaskDelay(pdMS_TO_TICKS(100));
    }

    // while(!Wire.available());
    
    if(Wire.available()){
      // Init array
      new_delay.uint8[0] = new_delay.uint8[1] = 0;
      // Read uint16_t value
      new_delay.uint8[0] = Wire.read();
      new_delay.uint8[1] = Wire.read();
  
      if(new_delay.uint16 != sensors_delay){
        lock(&serial_mutex);
        Serial.print(new_delay.uint16);
        Serial.print(" (");
        Serial.print(new_delay.uint8[0], HEX);
        Serial.print(new_delay.uint8[1], HEX);
        Serial.println(")");
        unlock(&serial_mutex);
    
        bool ok = set_new_delay(new_delay.uint16);
    
        // Take UART2 mutex to send set_delay result
        // // xSemaphoreTake(uart2_mutex, portMAX_DELAY);
    
        answer = (char *)malloc(sizeof(char) * 3);
    
        if (ok){
          sprintf(answer, "ok");
          // lock(&serial_mutex);
          Serial_println("ok");
          // unlock(&serial_mutex);
        }
        else{
          sprintf(answer, "no");
          Serial_println("no");
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
        Serial_println("received delay is equal to current, not changing.");
      }
    }
    else
      Serial_println("ESP8266 didn't answer a new delay request.");
      
    unlock(&I2C_bus_mutex);
    // I2C_bus_mutex = false;

    end_tick = xTaskGetTickCount();

    delay_ = CHECK_DATA_DELAY - (end_tick - start_tick);

    if (delay_ > 0)
      vTaskDelay((uint32_t) delay_);
  }
}

void TaskReadData(void *pvParameters){
  (void) pvParameters;

  for(;;){
    lock(&I2C_bus_mutex);
    // Output data to serial monitor
    Serial.print("Ambient Light luminance :");
    Serial.print(light_sensor.get_lux());
    Serial.println(" lux");
  
    Serial.print("Temperature = ");
    Serial.print(bme.readTemperature());
    Serial.println(" *C");
  
    // Convert temperature to Fahrenheit
    // Serial.print("Temperature = ");
    // Serial.print(1.8 * bme.readTemperature() + 32);
    // Serial.println(" *F");
    
    Serial.print("Pressure = ");
    Serial.print(bme.readPressure() / 100.0F);
    Serial.println(" hPa");
  
    Serial.print("Approx. Altitude = ");
    Serial.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
    Serial.println(" m");
  
    Serial.print("Humidity = ");
    Serial.print(bme.readHumidity());
    Serial.println(" %");
    Serial.println();

    unlock(&I2C_bus_mutex);

    vTaskDelay(pdMS_TO_TICKS(500));
  }
} */

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
      lock(&serial_mutex);
      Serial.print("New delay error: ");
      Serial.println(new_delay);
      unlock(&serial_mutex);
      ok = false;
      break;
  }

  return ok;
}

/* uint8_t sum(uint8_t *written_data) {
  uint8_t sum_ = 0;

  for (uint8_t i = 0; i < 5; i++) {
    sum_ += written_data[i];
  }

  return sum_;
} */

/* void reset_written_data() {
  for (uint8_t i = 0; i < 5; i++) {
    written_data[i] = 0;
  }
} */

void dataRequired(){
  // Requesting delay confirmation
  if(delay_ok == DELAY_OK || delay_ok == DELAY_NOT_OK){
    char *answer = (char *)malloc(sizeof(char) * 3);
    
    if(delay_ok == DELAY_OK)
      sprintf(answer, "ok");
    else
      sprintf(answer, "no");

    lock(&I2C_bus_mutex);
    
    for(uint8_t i=0; i<sizeof(answer); i++)
      Wire.write(answer[i]);

    unlock(&I2C_bus_mutex);

    delay_ok = NONE;
  }
  
  // Requesting new sensors data
  else if(delay_ok == NONE)
    sendSensorData();
}

void sendSensorData() {
  uint8_t struct_size = sizeof(sensors_data_t);

  lock(&I2C_bus_mutex);
  
  for(uint8_t i=0; i<struct_size; i++)
    Wire.write(sent_data[i]);

  unlock(&I2C_bus_mutex);

  Serial_println("Sent new data to webserver.");
}

void getNewDelay(int numBytes) {

  union{
    uint16_t uint16;
    uint8_t uint8[2];  
  } new_delay;

  for(uint8_t i=0; i<numBytes; i++)
    new_delay.uint8[i] = Wire.read();

  bool ok = set_new_delay(new_delay.uint16);

  if(ok){
    lock(&serial_mutex);
    Serial.print("Got new delay from webserver:");
    Serial.println(new_delay.uint16);
    delay_ok = DELAY_OK;
    unlock(&serial_mutex);
  }
  else
    delay_ok = DELAY_NOT_OK;
}
