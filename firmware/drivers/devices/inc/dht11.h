#ifndef DHT11_H
#define DHT11_H
/** \addtogroup Drivers_Programable Drivers Programable
 ** @{ */
/** \addtogroup Drivers_Devices Drivers devices
 ** @{ */
/** \addtogroup DHT11 dht11
 ** @{ */

/** \brief DHT11 driver for the ESP-EDU Board.
 * 
 * @author Matias Andres Gaggiamo
 *
 * @section changelog
 *
 * |   Date	    | Description                                    						|
 * |:----------:|:----------------------------------------------------------------------|
 * | 17/05/2024 | Document creation		                         						|
 * 
 **/

/*==================[inclusions]=============================================*/
#include "gpio_mcu.h"
#include <stdbool.h>
/*==================[macros]=================================================*/

/*==================[typedef]================================================*/

/*==================[external data declaration]==============================*/

/*==================[external functions declaration]=========================*/

void dht11Init( gpio_t gpio , io_t io);
bool dht11Read( float *phum, float *ptemp );

/** @} doxygen end group definition */
/** @} doxygen end group definition */
/** @} doxygen end group definition */

#endif /* #ifndef DHT11_H */

/*==================[end of file]============================================*/

