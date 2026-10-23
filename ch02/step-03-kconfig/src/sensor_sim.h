#ifndef SENSOR_SIM_H_
#define SENSOR_SIM_H_

#include <stdint.h>

/* Returns one fake distance reading in millimetres, 300..5000. */
int32_t sensor_sim_read(void);

#endif /* SENSOR_SIM_H_ */
