#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <NewSoftSerial.h>

#define configUSE_TIME_SLICING 0

typedef struct {
  float temperature;
  float pressure;
  float humidity;
  float altitude;
  float brigthness;
} sensor_data_t;

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
#define CHECK_DATA_DELAY 250
#define DELAY_FLAG 0x64 // "d" letter

sensor_data_t *sensor_data;
uint8_t *sent_data;

HardwareSerial GY39_UART = HardwareSerial(); // To read from sensor chip
NewSoftSerial data_comm_serial = NewSoftSerial(12, 13); // (RXPin, TXPin)

// define two tasks for Blink & AnalogRead
void TaskGetTemp(void *pvParameters);
void TaskGetHum(void *pvParameters);
void TaskGetPress(void *pvParameters);
void TaskGetAlt(void *pvParameters);
void TaskGetBright(void *pvParameters);
void TaskSendData(void *pvParameters);
void TaskGetDelay(void *pvParameters);

SemaphoreHandle_t sensors_mutex; 

// the setup function runs once when you press reset or power the board
void setup() {

  // Init sensor_data struct
  sensor_data = (sensor_data_t *)malloc(sizeof(sensor_data));
  sensor_data->temperature = 0.0;
  sensor_data->humidity = 0.0;
  sensor_data->pressure = 0.0;
  sensor_data->north_direction = 0.0;

  sent_data = (uint8_t *)sensor_data;

  written_data = (uint8_t *)malloc(sizeof(uint8_t) * 5);

  // debug purposes
  Serial.begin(115200);
  GY39_UART.begin(115200);
  data_comm_serial.begin(38400, SERIAL_8E1);

  do{
    sensors_mutex = xSemaphoreCreateMutex();

    if(sensors_mutex == NULL)
      Serial.println("Error while creating mutex.");
  }while(sensors_mutex==NULL);

  // Now set up tasks to run independently.
  //          func name             human readable name     stack size  priority
  xTaskCreate(TaskGetTemp, (const portCHAR *)"getTemperature", 128, NULL, 2, NULL);
  xTaskCreate(TaskGetHum, (const portCHAR *)"getHumidity", 128, NULL, 2, NULL);
  xTaskCreate(TaskGetPress, (const portCHAR *)"getPressure", 128, NULL, 2, NULL);
  xTaskCreate(TaskGetAlt, (const portCHAR *)"getAltitude", 128, NULL, 2, NULL);
  xTaskCreate(TaskGetBright, (const portCHAR *)"getBrightness", 128, NULL, 2, NULL);
  xTaskCreate(TaskSendData, (const portCHAR *)"sendData", 128, NULL, 3, NULL);
  xTaskCreate(TaskGetDelay, (const portCHAR *)"getNewDelay", 128, NULL, 3, NULL);

  // Now the task scheduler, which takes over control of scheduling individual tasks, is automatically started.
}

void loop(){} // Empty. Things are done in Tasks.

/*--------------------------------------------------*/
/*---------------------- Tasks ---------------------*/
/*--------------------------------------------------*/

void TaskGetTemp(void *pvParameters)  // This is a task.
{
  const uint8_t idx = 0;

  for (;;) // A Task shall never return or exit.
  {
    if(written_data[idx] == 0){
      xSemaphoreTake(sensors_mutex, portMAX_DELAY);
      // read sensor
      // sensors_data->temperature = ???
      xSemaphoneGive(sensors_mutex);
  
      written_data[idx] = 1;
    }
    vTaskDelay(SENSORS_DELAY); 
  }
}

void TaskGetHum(void *pvParameters)  // This is a task.
{
  const uint8_t idx = 1;

  for (;;) // A Task shall never return or exit.
  {
    if(written_data[idx] == 0){
      xSemaphoreTake(sensors_mutex, portMAX_DELAY);
      // read sensor
      // sensors_data->humidity = ???
      xSemaphoneGive(sensors_mutex);
  
      written_data[idx] = 1;
    }
    vTaskDelay(SENSORS_DELAY); 
  }
}

void TaskGetPress(void *pvParameters)  // This is a task.
{
  const uint8_t idx = 2;

  for (;;) // A Task shall never return or exit.
  {
    if(written_data[idx] == 0){
      xSemaphoreTake(sensors_mutex, portMAX_DELAY);
      // read sensor
      // sensors_data->pressure = ???
      xSemaphoneGive(sensors_mutex);
  
      written_data[idx] = 1;
    }
    vTaskDelay(SENSORS_DELAY); 
  }
}

void TaskGetAlt(void *pvParameters)  // This is a task.
{
  const uint8_t idx = 3;

  for (;;) // A Task shall never return or exit.
  {
    if(written_data[idx] == 0){
      xSemaphoreTake(sensors_mutex, portMAX_DELAY);
      // read sensor
      // sensors_data->altitude = ???
      xSemaphoneGive(sensors_mutex);
  
      written_data[idx] = 1;
    }
    vTaskDelay(SENSORS_DELAY); 
  }
}

void TaskGetBright(void *pvParameters)  // This is a task.
{
  const uint8_t idx = 4;

  for (;;) // A Task shall never return or exit.
  {
    if(written_data[idx] == 0){
      xSemaphoreTake(sensors_mutex, portMAX_DELAY);
      // read sensor
      // sensors_data->brightness = ???
      xSemaphoneGive(sensors_mutex);
  
      written_data[idx] = 1;
    }
    vTaskDelay(SENSORS_DELAY); 
  }
}

void TaskSendData(void *pvParameters)  // This is a task.
{

  for (;;) // A Task shall never return or exit.
  {
    if(sum(written_data) >= 5){
      xSemaphoreTake(sensors_mutex, portMAX_DELAY);
      reset_written_data();

      for(uint8_t i=0; i<sizeof(sensor_data_t); i++)
        data_comm_serial.write(sent_data[i]);
      
      xSemaphoneGive(sensors_mutex);
    }
    vTaskDelay(SENSORS_DELAY); 
  }
}

void TaskGetDelay(void *pvParameters)  // This is a task.
{
  typedef union{
    uint16_t uint16;
    uint8_t uint8[2];  
  } new_delay;

  new_delay.uint8[0] = new_delay.uint8[1] = 0;

  for (;;) // A Task shall never return or exit.
  {
    if(data_comm_serial.available()){
      new_delay.uint8[0] = data_comm_serial.read();
      new_delay.uint8[1] = data_comm_serial.read();

      uint8_t ok = set_new_delay(new_delay.uint16);

      
  
      if(!ok)
      
    }
    vTaskDelay(CHECK_DATA_DELAY); 
  }
}

uint8_t set_new_delay(const uint16_t new_delay){
  uint8_t ok = true;
  
  switch(new_delay){
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

void sum(uint8_t *written_data){
    uint8_t sum_ = 0;

    for(uint8_t i=0; i<5; i++){
      sum_ += written_data[i];  
    }

    return sum_;
}

void reset_written_data(){
  for(uint8_t i=0; i<5; i++){
    written_data[i] = 0;
  }
}
