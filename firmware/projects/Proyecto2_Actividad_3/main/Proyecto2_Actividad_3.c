/*! @mainpage Actividad 3 - Proyecto 2: Medidor de distancia por ultrasonido c/ interrupciones y puerto serie.
 *
 * @section genDesc General Description
 *
 * El programa implementa un programa para el uso de un sensor de ultrasonido HC-SR04 para medir distancia a un objeto, mostrando la misma 
 * en un display LCD y controlando LEDs para indicar la proximidad del objeto detectado. Además, utiliza interrupciones para el control de 
 * teclas y el control de tiempos. Adicionalmente, envía los datos de las mediciones por el puerto serie para poder observarlos en un terminal
 * de la PC.
 *  
 * Las principales funcionalidades incluyen la medición de distancia utilizando el sensor de ultrasonido, el control de LEDs para indicar 
 * la magnitud de la distancia medida, la visualización de la distancia medida en el display LCD y en un Serial Monitor de la PC, y la capacidad 
 * de activar y desactivar la medición (TEC1 o tecla "O" del teclado) y de para mantener o no mantener el resultado de la medición en la 
 * pantalla LCD (TEC2 o letra "H" del teclado).
 * 
 * El programa está estructurado en tres tareas principales:
 * 1. Medición: realiza la lectura continua del sensor de ultrasonido para medir la distancia.
 * 2. Lectura de Teclas: verifica el estado de los interruptores SWITCH_! y SWITCH_2, así como de los caractéres "O" y "H" del teclado 
 *                       para controlar la activación y desactivación de la medición, así como la opción  de mantener o no mantener el resultado 
 *                       en la pantalla LCD.
 * 3. Visualización: actualiza periódicamente la pantalla LCD con la distancia medida y controla la iluminación de los LEDs según la distancia 
 *                   medida.
 * 
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 19/04/2024 | Document creation		                         |
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
#include "timer_mcu.h"
#include "uart_mcu.h"
/*==================[macros and definitions]=================================*/
#define CONFIG_BLINK_PERIOD_MEASURE 500000
#define CONFIG_BLINK_PERIOD_SHOW 200000
/*==================[internal data definition]===============================*/
TaskHandle_t measure_task_handle = NULL;
TaskHandle_t show_task_handle = NULL;

uint16_t distance = 0;
bool mostrar = true;
bool medir = true;
/*==================[internal functions declaration]=========================*/
/**
 * @brief  Función encargada de cambiar el estado de la variable medir (se quiere volver a medir o dejar de medir)
 */
void cambiar_estado_medicion()
{
	medir = !medir;
}
/**
 * @brief  Función encargada de cambiar el estado de la variable mostrar (se quiere mantener o dejar de mantener el resultado por pantalla)
 */
void mantener_resultado()
{
	mostrar = !mostrar;
}

/**
 * @brief Función invocada en la interrupción del timer A. Envía una notificación que es recibida dentro de la tarea correspondiente a medir.
 */
void FuncTimerA(void* param){ vTaskNotifyGiveFromISR(measure_task_handle, pdFALSE); }   /* Envía una notificación a la tarea asociada al LED_1 */
/**
 * @brief Función invocada en la interrupción del timer B. Envía una notificación que es recibida dentro de la tarea correspondiente a mostrar
 *        por pantalla y cambiar el estado de los LED de acuerdo a la medición.
 */
void FuncTimerB(void* param){ vTaskNotifyGiveFromISR(show_task_handle, pdFALSE); }    /* Envía una notificación a la tarea asociada al LED_2 */

/**
 * @brief Función encargada de leer si se apretan las teclas del teclado: en el caso de la telcla "O" para medir o dejar de medir y en el caso
 *        de la letra h para mantener o dejar de mantener el resultado previo.
 */
void FuncUart(void* param)
{  
    uint8_t tecla;
    UartReadByte(UART_PC, &tecla);

    switch(tecla)
    {
    case 'o':
        cambiar_estado_medicion();
        break;
    case 'h':
        mantener_resultado();
        break;
    default:
        break;
    }
}

/**
 * @brief Tarea encargada de medir continuamente la distancia utilizando el sensor de ultrasonido HC-SR04.
 *        La tarea se repite con un período definido por CONFIG_PERIOD_MEASURE.
 */
static void MeasureTask(void *pvParameter)
{
    while(true)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        if(medir)
        {
            distance = HcSr04ReadDistanceInCentimeters();
            UartSendString(UART_PC, (const char*)UartItoa(distance, 10));
            UartSendString(UART_PC, " cm\r\n");
        }
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
static void ShowTask(void *pvParameter)
{
    while(true)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
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
    }
}

/*==================[external functions definition]==========================*/
void app_main(void)
{
    // Inicialización de LEDs, módulo LCD, módulo HcSr04 y Switches.
	LedsInit();
    LcdItsE0803Init();
    HcSr04Init(GPIO_3, GPIO_2);
    SwitchesInit();

    // Configuración e inicialización del Timer A utilizado para la tarea de medir.
    timer_config_t timer_measure = 
    {
        .timer = TIMER_A,
        .period = CONFIG_BLINK_PERIOD_MEASURE,
        .func_p = FuncTimerA,
        .param_p = NULL
    };
    TimerInit(&timer_measure);
    
    // Configuración e inicialización del Timer B utilizado para la tarea de mostrar por pantalla y cambiar el estado de los LEDs
    // a partir de la medición.
    timer_config_t timer_show = 
    {
        .timer = TIMER_B,
        .period = CONFIG_BLINK_PERIOD_SHOW,
        .func_p = FuncTimerB,
        .param_p = NULL
    };
    TimerInit(&timer_show);

    // Configuración e inicialización del puerto serie.
    serial_config_t my_uart =
    {
        .port = UART_PC,
        .baud_rate = 9600,
        .func_p = FuncUart,
        .param_p = NULL
    };
    UartInit(&my_uart);

    // Inicialización de funciones que activarán una interrupción ante la llamada de uno de los switches, llamando a las funciones 
    // enviadas como parámetro por referencia. En el caso del switch_1, se llamará a la función para medir o dejar de medir. En el caso
    // del switch_2 a la función para mantener o dejar de mantener el resultado.
	SwitchActivInt(SWITCH_1, &cambiar_estado_medicion, NULL);
	SwitchActivInt(SWITCH_2, &mantener_resultado, NULL);

    // Creación de tareas para medir la distancia y mostrar la misma por la pantalla LCD y por el cambio de estado de los LEDs.
    xTaskCreate(&MeasureTask, "Measure", 1024, NULL, 5, &measure_task_handle);
	xTaskCreate(&ShowTask, "Show", 512, NULL, 5, &show_task_handle);

    // Comienzo de las cuentas de ambos timers.
    TimerStart(timer_measure.timer);
    TimerStart(timer_show.timer);
}
/*==================[end of file]============================================*/