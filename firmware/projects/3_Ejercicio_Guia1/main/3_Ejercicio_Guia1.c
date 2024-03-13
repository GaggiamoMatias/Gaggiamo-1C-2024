/*! @mainpage Ejercio 3 - Guia 1
 *
 * @section genDesc General Description
 *
 * This section describes how the program works.
 *
 * <a href="https://drive.google.com/...">Operation Example</a>
 *
 * @section hardConn Hardware Connection
 *
 * |    Peripheral  |   ESP32   	|
 * |:--------------:|:--------------|
 * | 	PIN_X	 	| 	GPIO_X		|
 *
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 12/09/2023 | Document creation		                         |
 *
 * @author Albano Peñalva (albano.penalva@uner.edu.ar)
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"
#include "switch.h"
/*==================[macros and definitions]=================================*/
#define CONFIG_BLINK_PERIOD 1000
/*==================[internal data definition]===============================*/

enum modo_de_funcionamiento
{
	ON,
	OFF,
	TOGGLE
};

enum numero_led
{
	LED_1,
	LED_2,
	LED_3
};

struct leds
{
	enum numero_led n_led;       // indica el número de led a controlar
	uint8_t n_ciclos;  // indica la cantidad de ciclos de encendido/apagado
	uint8_t periodo;   // indica el tiempo de cada ciclo
	enum modo_de_funcionamiento mode;     //  ON, OFF, TOGGLE
} my_leds; 

/*==================[internal functions declaration]=========================*/
void funcionLeds(struct leds* x)
{
	if(x->mode == ON)
	{
		if(x->n_led == LED_1) LED_ON(LED_1);
		else if(x->n_led == LED_2) LED_ON(LED_2);
		else if(x->n_led == LED_3) LED_ON(LED_3);
	}
	else if(x->mode == OFF)
	{
		if(x->n_led == LED_1) LED_OFF(LED_1);
		else if(x->n_led == LED_2) LED_OFF(LED_2);
		else if(x->n_led == LED_3) LED_OFF(LED_3);
	}
	else if(x->mode == TOGGLE)
	{
		if(i < x->n_ciclos)
	}

}

/*==================[external functions definition]==========================*/
void app_main(void)
{
	struct leds x = my_leds;

	LedsInit();
	SwitchesInit();

	while(1)
	{
		funcionLeds(&x);
		vTaskDelay(CONFIG_BLINK_PERIOD / portTICK_PERIOD_MS);
	}
}
/*==================[end of file]============================================*/