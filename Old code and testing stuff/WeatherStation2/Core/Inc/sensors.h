#ifndef SENSORS
#define SENSORS

typedef struct {
	float temperature;
	float pressure;
	float humidity;
	float north_direction;
} sensor_data_t;

typedef enum
{
  FAST = 500,
  MEDIUM = 1000,
  SLOW = 2500,
  VERY_SLOW = 5000,
  TAKE_A_BREAK = 10000
} sensors_delay_t;

#endif
