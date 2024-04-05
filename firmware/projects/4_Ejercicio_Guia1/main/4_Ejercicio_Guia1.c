/*! @mainpage Ejercicio 4, 5 y 6 - Guia 1
 *
 * @section genDesc General Description
 *
 * El programa presenta funcionalidades, tales como la conversion de un numero de binario natural
 * a BCD y el cambio de estado de puertos de E/S, con la finalidad de mostrar por un display LCD un numero de 3 digitos. 
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 15/03/2024 | Document creation		                         |
 * | 05/04/2024 | Document completion		                     |
 *
 * @author Matias Andres Gaggiamo (matias.gaggiamo@ingenieria.uner.edu.ar)
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include "gpio_mcu.h"
/*==================[macros and definitions]=================================*/

/*==================[internal data definition]===============================*/
typedef struct
{
	gpio_t pin;			/*!< GPIO pin number */
	io_t dir;			/*!< GPIO direction '0' IN;  '1' OUT*/
} gpioConf_t;

gpioConf_t puertos_bits [4] = 
{	
	{GPIO_20, GPIO_OUTPUT},
	{GPIO_21, GPIO_OUTPUT},
	{GPIO_22, GPIO_OUTPUT},
	{GPIO_23, GPIO_OUTPUT}
};

gpioConf_t puertos_LCD [3] = 
{	
	{GPIO_9, GPIO_OUTPUT},
	{GPIO_18, GPIO_OUTPUT},
	{GPIO_19, GPIO_OUTPUT}
};
/*==================[internal functions declaration]=========================*/

/*==================[external functions definition]==========================*/
/**
 * @brief Convierte un dato de 32 bits en binario natural a BCD, guardando los valores de
 * cada digito en el arreglo recibido como parametro por referencia.
 * 
 * @param data Dato a convertir en BCD
 * @param digits Cantidad de digitos del dato a convertir en BCD
 * @param bcd_number Arreglo que contiene los digitos BCD del dato convertido.
 * @return 0
 */
int8_t  convertToBcdArray (uint32_t data, uint8_t digits, uint8_t * bcd_number)
{
	uint32_t aux;

	for(uint8_t i = 0; i<digits; i++)
	{
		aux = data % 10;
		data = data/10;
		bcd_number[i]=aux;
	}
	return 0;
}

/**
 * @brief Prende o apaga los puertos de entrada y salida recibidos como parametro por referencia
 * a partir del estado de los bits que conforman el digitoBCD recibido como parametro.
 * 
 * @param digitoBCD Digito en formato BCD sobre el cual se lee bit a bit para cambiar el estado de los puertos E/S.
 * @param puertosGPIO Vector de estructura que modela el puerto de E/S sobre los cuales se modifican su estado a partir del 
 * digito BCD recibido como parametro
 * @return 0
 */
int8_t cambiarEstadoGPIO(uint8_t digitoBCD, gpioConf_t* puertosGPIO)
{
	uint8_t masc = 1;

	for(int i = 0; i<4; i++)
	{
		if(digitoBCD&(masc<<i))
		{
			GPIOOn(puertosGPIO[i].pin);
		}
		else GPIOOff(puertosGPIO[i].pin);
	}

	return 0;
}

/**
 * @brief Muestra por el display el dato recibido como parametro a partir del envio de pulsos a los pines
 * de seleccion de cada BCD 7 segmentos.
 * 
 * @param dato Dato que se quiere mostrar por display.
 * @param digitos Cantidad de digitos del dato a mostrar por display.
 * @param puertos_bits Arreglo de estructuras que modelan los puertos E/S que transportan la informacion de un digito BCD a mostrar.
 * @param puertos_bits Arreglo de estructuras que modelan los puertos E/S que se utilizan para seleccionar cada uno de los modulos BCD 7 segmentos.
 * @return 0
 */
int8_t mostrarPorDisplay(int32_t dato, int8_t digitos, gpioConf_t* puertos_bits, gpioConf_t* puertos_LCD)
{
    uint8_t arr [digitos];
    convertToBcdArray(dato, digitos, arr);

	for(int i = digitos-1; i >= 0; i--)
	{
		cambiarEstadoGPIO(arr[i], puertos_bits);
		GPIOOn(puertos_LCD[i].pin);
		GPIOOff(puertos_LCD[i].pin);
	}

	return 0;
}

void app_main(void)
{
	uint32_t numero = 999;
	uint8_t digitos = 3;

	for(uint8_t i = 0; i<4; i++)
	{
		GPIOInit(puertos_bits[i].pin, puertos_bits[i].dir);
	}
	for(uint8_t i = 0; i<3; i++)
	{
		GPIOInit(puertos_LCD[i].pin, puertos_LCD[i].dir);
	}

	mostrarPorDisplay(numero, digitos, puertos_bits, puertos_LCD);
}
/*==================[end of file]============================================*/