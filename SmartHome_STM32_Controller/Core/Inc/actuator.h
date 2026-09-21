/*
 * actuator.h
 *
 *  Created on: Sep 21, 2026
 *      Author: ducnam
 */

#ifndef INC_ACTUATOR_H_
#define INC_ACTUATOR_H_

#include <stdbool.h>

void Actuator_Init(void);
void Actuator_SetRelay1(bool state);
void Actuator_SetRelay2(bool state);
void Actuator_SetMosfet1(bool state);
void Actuator_SetMosfet2(bool state);
void Actuator_SetBuzzer(bool state);

#endif /* INC_ACTUATOR_H_ */
