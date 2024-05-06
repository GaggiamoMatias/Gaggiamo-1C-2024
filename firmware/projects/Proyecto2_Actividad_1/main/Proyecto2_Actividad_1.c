/*! @mainpage Actividad 1 - Proyecto 2: Medidor de distancia por ultrasonido
 *
 * @section genDesc General Description
 *
 * El programa implementa un programa para el uso de un sensor de ultrasonido HC-SR04 para medir distancia a un objeto, mostrando la misma 
 * en un display LCD y controlando LEDs para indicar la proximidad del objeto detectado.
 *  
 * Las principales funcionalidades incluyen la medición de distancia utilizando el sensor de ultrasonido, el control de LEDs para indicar 
 * la magnitud de la distancia medida, la visualización de la distancia medida en el display LCD, y la capacidad de activar y desactivar la 
 * medición (TEC1) y de para mantener o no mantener el resultado de la medición en la pantalla LCD (TEC2).
 * 
 * El programa está estructurado en tres tareas principales:
 * 1. Medición: realiza la lectura continua del sensor de ultrasonido para medir la distancia.
 * 2. Lectura de Teclas: verifica el estado de los interruptores y controla la activación y desactivación de la medición, así como la opción 
 *                       de mantener o no mantener el resultado en la pantalla LCD.
 * 3. Visualización: actualiza periódicamente la pantalla LCD con la distancia medida y controla la iluminación de los LEDs según la distancia 
 *                   medida.
 * 
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 05/04/2024 | Document creation		                         |
 *
 * @author Matias Andres Gaggiamo (matias.gaggiamo@ingenieria.uner.edu.ar)
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"
#include "lcditse0803.h"
#include "hc_sr04.h"
#include "switch.h"
/*==================[macros and definitions]=================================*/
#define CONFIG_PERIOD_MEASURE 1000
#define CONFIG_PERIOD_SWITCH 40
#define CONFIG_PERIOD_SHOW 300

/*==================[internal data definition]===============================*/
uint16_t distance = 0;
bool mostrar = true;
bool medir = true;

/*==================[internal functions declaration]=========================*/
/**
 * @brief Tarea encargada de medir continuamente la distancia utilizando el sensor de ultrasonido HC-SR04.
 *        La tarea se repite con un período definido por CONFIG_PERIOD_MEASURE.
 */
void MeasureTask(void *pvParameter){
    while(true)
    {
        if(medir)
        {
            distance = HcSr04ReadDistanceInCentimeters();
        }
        vTaskDelay(CONFIG_PERIOD_MEASURE / portTICK_PERIOD_MS);
    }
}
/**
 * @brief Tarea encargada de leer el estado de los interruptores y controlar la activación/desactivación de la medición
 *        (interruptor 1) y de mostrar o no mostrar la distancia en el LCD (interruptor 2). 
 *        La tarea se repite con un período definido por CONFIG_PERIOD_SWITCH.
 */
void SwitchTask(void *pvParameter){
    while(true)
    {
        switch(SwitchesRead())
        {
    		case SWITCH_1:
    			medir = !medir;
    		break;
    		case SWITCH_2:
    			mostrar = !mostrar;
    		break;
    	}

        vTaskDelay(CONFIG_PERIOD_SWITCH / portTICK_PERIOD_MS);
    }
}
/**
 * @brief Tarea encargada de actualizar la visualización en el display LCD y controlar el encendido/apagado de los LEDs
 *        según la distancia medida por el sensor HC-SR04:
 *        - distancia es menor a 10 cm, todos los LEDs se apagan.
 *        - distancia está entre 10 y 20 cm, solo el LED_1 se enciende.
 *        - distancia está entre 20 y 30 cm, los LEDs 1 y 2 se encienden.
 *        - distancia es mayor a 30 cm, todos los LEDs se encienden.
 *        Además, la tarea verifica la variable global 'mostrar' y escribe la distancia en el LCD si está activada.
 *        La tarea se repite con un período definido por CONFIG_PERIOD_SHOW.
 */
void ShowTask(void *pvParameter){
    while(true)
    {
        if(distance<10)
        {
            LedOff(LED_1);
            LedOff(LED_2);
            LedOff(LED_3);
        }
        else if(distance<20)
        {
            LedOn(LED_1);
            LedOff(LED_2);
            LedOff(LED_3);
        }
        else if(distance<30)
        {
            LedOn(LED_1);
            LedOn(LED_2);
            LedOff(LED_3);
        }
        else
        {
            LedOn(LED_1);
            LedOn(LED_2);
            LedOn(LED_3);
        }
        if(mostrar)
        {
            LcdItsE0803Write(distance);
        } 

        vTaskDelay(CONFIG_PERIOD_SHOW / portTICK_PERIOD_MS);
    }
}

/*==================[external functions definition]==========================*/
void app_main(void)
{
    // Inicialización de LEDs, módulo LCD, HcSr04 y Switches 
	LedsInit();
    LcdItsE0803Init();
    HcSr04Init(GPIO_3, GPIO_2);
    SwitchesInit();

    // Creación de tareas
    xTaskCreate(&MeasureTask, "Measure", 1024, NULL, 5, NULL);
    xTaskCreate(&SwitchTask, "Switch", 512, NULL, 5, NULL);
	xTaskCreate(&ShowTask, "Show", 512, NULL, 5, NULL);
}
/*==================[end of file]============================================*/