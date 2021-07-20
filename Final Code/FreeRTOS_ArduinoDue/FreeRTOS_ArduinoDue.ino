#include <FreeRTOS.h>
#include <task.h>
#include <Wire.h>
#include <Adafruit_BME280.h>
#include <MAX44009.h>

#define SEALEVELPRESSURE_HPA (1013.25)//< Average sea level pressure is 1013.25 hPa
#define SENSORS_DELAY pdMS_TO_TICKS(sensors_delay) // delay of the tasks that communicate with the sensors
#define CHECK_DELAY pdMS_TO_TICKS(250) // delay of the TaskGetDelay function
#define DELAY_FLAG 0x64 // "d" letter
#define ISR_BUTTON_PIN 45 // pin to manage button pressed signal

Adafruit_BME280 bme; // I2C Define BME280
MAX44009 light_sensor;  // I2C Define MAX44009

// struct to maintain sensor data
typedef struct {
  float temperature;
  float humidity;
  float pressure;
  float altitude;
  float brightness;
} sensors_data_t;

// enum for the sensors communication's delay
typedef enum
{
  MEDIUM = 1000,
  SLOW = 2500,
  VERY_SLOW = 5000,
  TAKE_A_BREAK = 10000
} sensors_delay_t;

// definition of a boolean mutex
typedef enum{
  LOCKED = true,
  UNLOCKED = false
} bool_mutex;

volatile sensors_delay_t sensors_delay = MEDIUM; // current sensors delay
volatile sensors_delay_t old_sensors_delay; // old sensors delay as a backup before editing

// array to check that every task has read a new sensors value before sending the data structure to the webserver
uint8_t written_data[5];

// data struct
sensors_data_t sensors_data;
// pointer to the data struct for transmission
uint8_t *sent_data;

// handle for the TaskSendData function (notifications)
TaskHandle_t xTaskSendDataHandle;

// definition of mutexes, one for the sensors bus and one to avoid overlapping of debug strings
bool_mutex I2C_bus_mutex, serial_mutex;

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
  Serial.println("Pa");

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
  while(*mutex == LOCKED){
    vTaskDelay(pdMS_TO_TICKS(100));
  }
  
  *mutex = LOCKED;
}

static inline void unlock(bool_mutex *mutex){ *mutex = UNLOCKED; }

void setup(){
  Serial.begin(115200);
  while(!Serial)
    delay(100);

  Serial3.begin(115200);
  while(!Serial3)
    delay(100);
  
  Serial.println(F("Setting up data structures."));

  // Data struct init
  sensors_data.temperature = 0.0;
  sensors_data.humidity = 0.0;
  sensors_data.pressure = 0.0;
  sensors_data.altitude = 0.0;
  sensors_data.brightness = 0.0;

  // Setting up pointer to data struct to send it byte by byte via UART
  sent_data = (uint8_t *)&sensors_data;

  xTaskSendDataHandle = NULL;

  // Init sensors (GY-39)
  bool status;
  status = bme.begin(0x76);  
  while(!status){
    Serial.println(F("Could not find a valid BME280 sensor, check wiring!"));
    delay(500);
    status = bme.begin(0x76);
  }

  Serial.println(F("Set up Serial lines for communication."));

  // Init mutexes
  I2C_bus_mutex = serial_mutex = UNLOCKED;

  Serial.println(F("Set up mutexes."));

  // Task creation, 5 tasks to read from sensors, one to check for a new delay, one to send data to webserver
  // function name | verbose task identifier | stack size (words) | task arguments | priority | task handle
  xTaskCreate(TaskReadTemp, (const portCHAR *)"TaskReadTemp", 128, NULL, 2, NULL);
  xTaskCreate(TaskReadHum, (const portCHAR *)"TaskReadHum", 128, NULL, 2, NULL);
  xTaskCreate(TaskReadPress, (const portCHAR *)"TaskReadPress", 128, NULL, 2, NULL);
  xTaskCreate(TaskReadAlt, (const portCHAR *)"TaskReadAlt", 128, NULL, 2, NULL);
  xTaskCreate(TaskReadBright, (const portCHAR *)"TaskReadBright", 128, NULL, 2, NULL);
  xTaskCreate(TaskGetDelay, (const portCHAR *)"TaskGetDelay", 128, NULL, 4, NULL);
  xTaskCreate(TaskSendData, (const portCHAR *)"TaskSendData", 128, NULL, 3, &xTaskSendDataHandle);

  Serial.println(F("Set up FreeRTOS tasks. Scheduler starting."));

  // Setting built-in LED to LOW so it's not always on
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  // ISR setup
  pinMode(ISR_BUTTON_PIN, INPUT_PULLUP);
  // pin to manage, function to execute, condition to call
  attachInterrupt(digitalPinToInterrupt(ISR_BUTTON_PIN), buttonDelay, RISING);

  // Start FreeRTOS Scheduler
  vTaskStartScheduler();

  Serial.println(F("Failed to start FreeRTOS scheduler"));
  while(1);
}

// FreeRTOS default task, prints a separator for the sensors readings, nothing more
void loop(){
  Serial.println("------------------");
  delay(sensors_delay);
}

// function called when the button is pressed. Changes delay and sends it to the webserver
void buttonDelay(){
  // set up data struct for new delay
  union{
    uint16_t uint16;
    uint8_t uint8[2];
  } delay_int;

  // save current delay value
  old_sensors_delay = sensors_delay;

  // set up new delay
  switch(sensors_delay){
    case MEDIUM:
      sensors_delay = SLOW;
      delay_int.uint16 = 2500;
      break;

    case SLOW:
      sensors_delay = VERY_SLOW;
      delay_int.uint16 = 5000;
      break;

    case VERY_SLOW:
      sensors_delay = TAKE_A_BREAK;
      delay_int.uint16 = 10000;
      break;

    case TAKE_A_BREAK:
      sensors_delay = MEDIUM;
      delay_int.uint16 = 1000;
      break;

    default: 
      delay_int.uint16 = 0;
      break;
  };

  // if all good, send new delay to webserver
  if(delay_int.uint16 != 0){
    Serial_print("New delay: ");
    Serial.println(sensors_delay);

    // Serial3 sends on UART3
    Serial3.write('n'); // flag for new delay
    Serial3.write(delay_int.uint8[0]);
    Serial3.write(delay_int.uint8[1]);
  }
}

// pvParameters is necessary, FreeRTOS gets angry otherwise  
void TaskReadTemp(void *pvParameters){
  (void) pvParameters; // Avoids the warning on unused parameter
  const uint8_t idx = 0; // index to write in written_data array
  TickType_t start_tick, end_tick; // structs to take note of ticks for the delay calculation
  int delay_; // integer required because TickType_t is typedef'd as a uint32_t which is an unsigned long, but I need the sign

  Serial_println("TaskReadTemp initialized.");

  for (;;)
  {
    // get start tick
    start_tick = xTaskGetTickCount();
    // lock mutex and read sensor
    lock(&I2C_bus_mutex);
    sensors_data.temperature = bme.readTemperature();
    unlock(&I2C_bus_mutex);
    Serial_print_data("Temperature: ", sensors_data.temperature, "°C");

    // Write that you've done your job
    written_data[idx] = 1;

    // If everyone is done, call the task that sends data
    if (sum(written_data) >= 5)
      xTaskNotifyGive(xTaskSendDataHandle);

    // get end tick
    end_tick = xTaskGetTickCount();
    // calculate if there's some time left
    delay_ = SENSORS_DELAY - (end_tick - start_tick);

    // if this is true I'm done before the deadline, else I'm late!
    if (delay_ > 0)
      vTaskDelay((uint32_t) delay_);
  }
}

void TaskReadHum(void *pvParameters){ 
  (void) pvParameters;
  const uint8_t idx = 1;
  TickType_t start_tick, end_tick;
  int delay_;

  Serial_println("TaskReadHum initialized.");

  for (;;)
  {
    
    start_tick = xTaskGetTickCount();
    lock(&I2C_bus_mutex);
    sensors_data.humidity = bme.readHumidity();
    unlock(&I2C_bus_mutex);
    Serial_print_data("Humidity: ", sensors_data.humidity, "%");

    written_data[idx] = 1;

    if (sum(written_data) >= 5)
      xTaskNotifyGive(xTaskSendDataHandle);

    end_tick = xTaskGetTickCount();
    delay_ = SENSORS_DELAY - (end_tick - start_tick);

    if (delay_ > 0)
      vTaskDelay((uint32_t) delay_);
  }
}

void TaskReadPress(void *pvParameters){
  (void) pvParameters;
  const uint8_t idx = 2;
  TickType_t start_tick, end_tick;
  int delay_;

  Serial_println("TaskReadPress initialized.");

  for (;;)
  {
    
    start_tick = xTaskGetTickCount();
    lock(&I2C_bus_mutex);
    sensors_data.pressure = bme.readPressure();
    unlock(&I2C_bus_mutex);
    Serial_print_data("Pressure: ", sensors_data.pressure, "Pa");

    written_data[idx] = 1;

    if (sum(written_data) >= 5)
      xTaskNotifyGive(xTaskSendDataHandle);

    end_tick = xTaskGetTickCount();
    delay_ = SENSORS_DELAY - (end_tick - start_tick);

    if (delay_ > 0)
      vTaskDelay((uint32_t) delay_);
  }
}

void TaskReadAlt(void *pvParameters){
  (void) pvParameters;
  const uint8_t idx = 3;
  TickType_t start_tick, end_tick;
  int delay_;

  Serial_println("TaskReadAlt initialized.");

  for (;;)
  {
    start_tick = xTaskGetTickCount();
    lock(&I2C_bus_mutex);
    sensors_data.altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);
    unlock(&I2C_bus_mutex);
    Serial_print_data("Altitude: ", sensors_data.altitude, "m");

    written_data[idx] = 1;

    if (sum(written_data) >= 5)
      xTaskNotifyGive(xTaskSendDataHandle);

    end_tick = xTaskGetTickCount();
    delay_ = SENSORS_DELAY - (end_tick - start_tick);

    if (delay_ > 0)
      vTaskDelay((uint32_t) delay_);
  }
}

void TaskReadBright(void *pvParameters){
  (void) pvParameters;
  const uint8_t idx = 4;
  TickType_t start_tick, end_tick;
  int delay_;

  Serial_println("TaskReadBright initialized.");

  for (;;)
  {
    
    start_tick = xTaskGetTickCount();
    lock(&I2C_bus_mutex);
    sensors_data.brightness = light_sensor.get_lux();
    unlock(&I2C_bus_mutex);
    Serial_print_data("Brightness: ", sensors_data.brightness, "lux");

    written_data[idx] = 1;
    
    if (sum(written_data) >= 5)
      xTaskNotifyGive(xTaskSendDataHandle);

    end_tick = xTaskGetTickCount();
    delay_ = SENSORS_DELAY - (end_tick - start_tick);

    if (delay_ > 0)
      vTaskDelay((uint32_t) delay_);
  }
}

void TaskSendData(void *pvParameters){
  (void) pvParameters;

  Serial_println("TaskSendData initialized.");
  
  for (;;)
  {
    // Block here always, waiting for the sensor-reading-tasks to unblock me
    Serial_println("TaskSendData waiting for notification...");
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    Serial_println("TaskSendData has been notified.");

    if(sum(written_data) >= 5){

      // Reset writing values for the other tasks
      reset_written_data();
    
      // Send data to ESP8266
      for(uint8_t i=0; i<sizeof(sensors_data_t); i++)
        Serial3.write(sent_data[i]);

      Serial_println("Sent to ESP8266 new data structure: ");
      Serial_print_data_struct();
    }
    // Never delay on the sem_send_data so that the task will block on the next for loop iteration
  }
}

void TaskGetDelay(void *pvParameters){
  (void) pvParameters;

  // struct for new delay
  union {
    uint16_t uint16;
    uint8_t uint8[2];
  } new_delay;
  
  TickType_t start_tick, end_tick;
  int delay_;

  Serial_println("TaskGetDelay initialized.");
  
  for (;;) 
  {
    start_tick = xTaskGetTickCount();

    // if ESP8266 sent something
    if(Serial3.available()){
      // take a look at the first byte received
      char peeked = (char)Serial3.peek();

      // if it equals 'n' it's the answer to the delay change by the physical button
      if(peeked == 'n'){
          // ignore 'n' letter
          Serial3.read();

          Serial_print("ESP8266 answered to new delay by button: ");

          // read a byte which is 0 or 1
          uint8_t ok = Serial3.read();
          // if 1, all good. Saving the current delay as the old one to make the change permanent
          if(ok){
            old_sensors_delay = sensors_delay;
            Serial_println("ok!");
          }
          // if 0, revert changes
          else{
            sensors_delay = old_sensors_delay;
            Serial_println("not ok.");
          }
      }
      // if the first byte is not 'n' there's a new delay to set coming from the web page
      else{
        Serial_print("New delay set by client: ");

        // reading 2 bytes since the delay is an uint16_t
        new_delay.uint8[0] = Serial3.read();
        new_delay.uint8[1] = Serial3.read();
  
        Serial.println(new_delay.uint16);

        // setting new delay
        bool ok = set_new_delay(new_delay.uint16);

        // allocating memory for the string answer to the webserver 
        char *answer = (char *)malloc(sizeof(char) * 3);
        
        if(ok)
          sprintf(answer, "ok");
        else
          sprintf(answer, "no");
  
        Serial_print("Answering \"");
        Serial.print(answer);
        Serial_println("\" to ESP8266.");

        // Writing 'd' letter and the answer which is 3 bytes
        Serial3.write(DELAY_FLAG);
        for(uint8_t i=0; i<sizeof(answer); i++)
          Serial3.write(answer[i]);

        // free the array's memory, else it's a leak
        free(answer);
      }
    }

    end_tick = xTaskGetTickCount();

    delay_ = CHECK_DELAY - (end_tick - start_tick);

    if (delay_ > 0)
      vTaskDelay((uint32_t) delay_);
  }
}

// checks if the new delay is a good value and sets it
bool set_new_delay(const uint16_t new_delay) {
  bool ok = true;

  switch (new_delay) {
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

// sums written_data's content to check if every task has done its readings
uint8_t sum(uint8_t *written_data) {
  uint8_t sum_ = 0;

  for (uint8_t i = 0; i < 5; i++) {
    sum_ += written_data[i];
  }

  return sum_;
}

// resets written_data writing 0 in every position
void reset_written_data() {
  for (uint8_t i = 0; i < 5; i++) {
    written_data[i] = 0;
  }
}
