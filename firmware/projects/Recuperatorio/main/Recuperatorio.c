/*! @mainpage Recuperatorio Examen - Electronica Programable
 *
 * @section genDesc General Description
 *
 * El programa consta de una aplicación de un dispositivo que se utilizará para medir la temperatura de individuos en la entrada de la FI-UNER. 
 * Dicho dispositivo cuenta con una termopila para el sensado de temperatura, con su correspondiente circuito de acondicionamiento. Cuenta además con un sensor de 
 * ultrasonido HC-SR04 para medir la distancia de la persona a la termopila. Estos sensores trabajan midiendo la temperatura de un objeto a la distancia, siendo esta 
 * distancia preestablecida (según el sensor, lentes, etc.) para obtener datos de temperatura correctos. Para la termopila utilizada esa distancia es de 10cm ±2cm.
 * El promedio de la temperatura se enviará por puerto serie a la PC junto con la distancia a la que se tomó la medida, utilizando el siguiente formato: 
 * [temperatura]Cº persona a [distancia] cm
 * Se enciende una alarma por temperatura mayor a 37.5°C.
 *
 * @section hardConn Hardware Connection
 *
 * |    Peripheral  |   ESP32   	|
 * |:--------------:|:--------------|
 * | 	PIN_X	 	| 	GPIO_X		|
 * |:--------------:|:--------------|
 * |    HC-SR04	    |   ESP32   	|
 * |:--------------:|:--------------|
 * | 	ECHO	 	| 	GPIO_3		|
 * | 	TRIGGER	 	| 	GPIO_2		|
 * | 	+5V	 		| 	+5V			|
 * | 	GND	 		| 	GND			|
 * |:--------------:|:--------------|
 * | SENSOR_TEMP A0	| 	GPIO_1		|
 * | 	ALARMA	 	| 	GPIO_22		|
 *
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 18/06/2024 | Document creation		                         |
 *
 * @author Matias A. Gaggiamo (matias.gaggiamo@ingenieria.uner.edu.ar)
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"
#include "hc_sr04.h"
#include "switch.h"
#include "timer_mcu.h"
#include "uart_mcu.h"
#include "analog_io_mcu.h"
/*==================[macros and definitions]=================================*/
#define CONFIG_PERIOD_DISTANCIA 1000000
#define CONFIG_PERIOD_TEMPERATURA 100000
#define CONFIG_PERIOD_ALARMA 1000
/*==================[internal data definition]===============================*/
TaskHandle_t distancia_task_handle = NULL;
TaskHandle_t temperatura_task_handle = NULL;

typedef struct
{
	gpio_t pin;			/*!< GPIO pin number */
	io_t dir;			/*!< GPIO direction '0' IN;  '1' OUT*/
} gpioConf_t;

gpioConf_t SENSOR_TEMP = {GPIO_1, GPIO_INPUT};
gpioConf_t ALARMA = {GPIO_22, GPIO_OUTPUT};

uint16_t distancia = 0;
bool medirTemperatura = false;
bool encenderAlarma = false;
bool reiniciarMedicion = false;

/*==================[internal functions declaration]=========================*/
/**
 * @brief Función invocada en la interrupción del timer A. Envía una notificación que es recibida dentro de la tarea correspondiente a medir distancia.
 */
void FuncTimerA(void* param){ vTaskNotifyGiveFromISR(distancia_task_handle, pdFALSE); } 
/**
 * @brief Función invocada en la interrupción del timer B. Envía una notificación que es recibida dentro de la tarea correspondiente a medir temperatura.
 */
void FuncTimerB(void* param){ vTaskNotifyGiveFromISR(temperatura_task_handle, pdFALSE); } 

/**
 * @brief Tarea encargada de medir cada 1s la distancia entre la persona y la termopila utilizando el sensor de ultrasonido HC-SR04.
 */
static void DistanciaTask(void *pvParameter)
{
    while(true)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        distancia = HcSr04ReadDistanceInCentimeters();

		if(distancia>8 && distancia<12) 
		{
			medirTemperatura = true;
			LedOn(LED_2);
		}
		else if(distancia<8)
		{
			medirTemperatura = false;
			LedOn(LED_1);
		}
		else if(distancia>12)
		{
			medirTemperatura = false;
			LedOn(LED_3);
			if(distancia>140)
			{
				reiniciarMedicion = true;
			}
		}
    }
}

/**
 * @brief Tarea encargada de medir la señal obtenida por el sensor analógico de temperatura, de convertirla a valor de temperatura y de obtener un promedio
 * 	      de 10 valores de la misma, solo en el caso de que este permitido medirla (la distancia debe estar en el rango de distancias para la correcta medición de temperatura.)
 */
static void TemperaturaTask(void *pvParameter)
{
	uint16_t data_mV = 0;
	float data_temp = 0;
	float sumatoria_temp = 0;
	float prom_temp = 0;
	uint16_t prom_temp_convertido = 0;
	uint8_t n_ciclos = 1;

    while(true)
    {
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		if(medirTemperatura)
		{
			AnalogInputReadSingle(CH1, &data_mV);

			// Interpolacion lineal con Y=Temperatura, X=data[mV], X0=0 mV, X1=3300 mV, Y0=20°C y Y1=50°C. 
			data_temp = (30/3300)*data_mV + 20;
			sumatoria_temp = sumatoria_temp + data_temp;
			n_ciclos = n_ciclos + 1;

			if(n_ciclos == 10)
			{
				prom_temp = sumatoria_temp / n_ciclos;
				prom_temp_convertido = (uint16_t)prom_temp;

				UartSendString(UART_PC, (const char*)UartItoa(prom_temp_convertido, 10));
        		UartSendString(UART_PC, "°C persona a ");
				UartSendString(UART_PC, (const char*)UartItoa(distancia, 10));
				UartSendString(UART_PC, " cm");

				sumatoria_temp = 0;
				n_ciclos = 0;
				
				if(prom_temp > 37.5)
				{
					encenderAlarma = true;
				}
				else
				{
					encenderAlarma = false;
				}
			}
		}
		if(reiniciarMedicion)
		{
			sumatoria_temp = 0;
			n_ciclos = 0;
			reiniciarMedicion = false;
		}
    }
}

/**
 * @brief Tarea encargada de activar o desactivar la alarma en el caso que sea correspondiente.
 */
static void AlarmaTask(void *pvParameter)
{
	while(true)
	{
		if(encenderAlarma)
		{
			GPIOOn(ALARMA.pin);
		}
		else
		{
			GPIOOff(ALARMA.pin);
		}
		vTaskDelay(CONFIG_PERIOD_ALARMA / portTICK_PERIOD_MS);
	}
}
/*==================[external functions definition]==========================*/
void app_main(void)
{
	// Inicialización de LEDs, de sensor de ultrasonido HC-SR04, y de GPIOs correspondienetes al sensor de temperatura y la alarma.
	LedsInit();
	HcSr04Init(GPIO_3, GPIO_2);

	GPIOInit(SENSOR_TEMP.pin, SENSOR_TEMP.dir);
	GPIOInit(ALARMA.pin, ALARMA.dir);

	// Configuración e inicialización del Transmisor - Receptor asíncrono universal para la transmisión de datos por puertos serie.
    serial_config_t my_uart =
    {
        .port = UART_PC,
        .baud_rate = 115200,
        .func_p = NULL,
        .param_p = NULL
    };
    UartInit(&my_uart);

	// Configuración e inicialización del conversor AD para el manejo de la entrada analógica de la termopila.
	analog_input_config_t analog_i =
    {
        .input = CH1,
        .mode = ADC_SINGLE,
        .func_p = NULL,
        .param_p = NULL,
        .sample_frec = NULL
    };
    AnalogInputInit(&analog_i);

	// Configuración e inicialización del Timer para la medición de la distancia.
	timer_config_t timer_distancia = 
    {
        .timer = TIMER_A,
        .period = CONFIG_PERIOD_DISTANCIA,
        .func_p = FuncTimerA,
        .param_p = NULL
    };
    TimerInit(&timer_distancia);

	// Configuración e inicialización del Timer para la medición de la temperatura.
	timer_config_t timer_temperatura = 
    {
        .timer = TIMER_B,
        .period = CONFIG_PERIOD_TEMPERATURA,
        .func_p = FuncTimerB,
        .param_p = NULL
    };
    TimerInit(&timer_temperatura);

	// Creación de tareas para medir distancia, medir temperatura y manejar la alarma.
	xTaskCreate(&DistanciaTask, "Distancia", 1024, NULL, 5, &distancia_task_handle);
	xTaskCreate(&TemperaturaTask, "Temperatura", 1024, NULL, 5,  &temperatura_task_handle);
	xTaskCreate(&AlarmaTask, "Alarma", 1024, NULL, 5, NULL);

	TimerStart(timer_distancia.timer);
	TimerStart(timer_temperatura.timer);
}
/*==================[end of file]============================================*/