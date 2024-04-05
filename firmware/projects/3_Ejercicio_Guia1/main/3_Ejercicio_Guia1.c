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

struct leds
{
	uint8_t n_led;       // indica el número de led a controlar
	uint8_t n_ciclos;  // indica la cantidad de ciclos de encendido/apagado
	uint32_t periodo;   // indica el tiempo de cada ciclo
	enum modo_de_funcionamiento mode;     //  ON, OFF, TOGGLE
} my_leds; 

/*==================[internal functions declaration]=========================*/
void funcionLeds(struct leds* x)
{
	if(x->mode == ON)
	{
		if(x->n_led == 1) LedOn(LED_1);
		else if(x->n_led == 2) LedOn(LED_2);
		else if(x->n_led == 3) LedOn(LED_3);
	}
	else if(x->mode == OFF)
	{
		if(x->n_led == 1) LedOff(LED_1);
		else if(x->n_led == 2) LedOff(LED_2);
		else if(x->n_led == 3) LedOff(LED_3);
	}
	else if(x->mode == TOGGLE)
	{
		uint8_t i = 0;
		while(i<x->n_ciclos)
		{
			if(x->n_led == 1) LedToggle(LED_1);
			else if(x->n_led == 2) LedToggle(LED_2);
			else if(x->n_led == 3) LedToggle(LED_3);
			i++;
			uint8_t j = 0;
			while(j < x->periodo/100)
			{
				j++;
				vTaskDelay(100 / portTICK_PERIOD_MS);
			}
		}
	}
}

/*==================[external functions definition]==========================*/
void app_main(void)
{
	struct leds x = my_leds;

	LedsInit();

	x.mode = TOGGLE;
	x.n_ciclos = 100;
	x.periodo = 500;
	x.n_led = 3;

    funcionLeds(&x);

	
	
}
/*==================[end of file]============================================*/