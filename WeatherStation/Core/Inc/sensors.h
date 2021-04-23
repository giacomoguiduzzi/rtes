#ifndef SENSORS
#define SENSORS

typedef struct {
	union {
		float temp;
		uint8_t uint_temp[4];
	} temperature;

	union {
		float pres;
		uint8_t uint_pres[4];
	} pressure;

	union {
		float hum;
		uint8_t uint_hum[4];
	} humidity;

	union{
		float n_dir;
		uint8_t uint_ndir[4];
	} north_direction;

	uint8_t written_data;
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
