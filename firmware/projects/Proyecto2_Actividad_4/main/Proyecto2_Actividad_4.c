/*! @mainpage Actividad 4 - Proyecto 2: Osciloscopio.
 *
 * @section genDesc General Description
 *
 * El programa consta de una aplicación, basada en el driver analog_io_mcu.h y el driver de transmisión serie uart_mcu.h, que digitaliza una 
 * señal analógica y la transmita a un graficador de puerto serie de la PC, tomando la entrada CH1 del conversor AD y realizando la tranmisión 
 * por la UART conectada al puerto serie de la PC en un formato compatible con un graficador por puerto serie. 
 * 
 * Para probar dicho funcionamiento el programa convierte una señal digital de un ECG en una señal analógica y se la visualiza utilizando la 
 * aplicación.
 * 
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 26/04/2024 | Document creation		                         |
 *
 * @author Matias Andres Gaggiamo (matias.gaggiamo@ingenieria.uner.edu.ar)
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "timer_mcu.h"
#include "uart_mcu.h"
#include "analog_io_mcu.h"
/*==================[macros and definitions]=================================*/
#define CONFIG_PERIOD_ADC 2000
#define CONFIG_PERIOD_DAC 4000
#define BUFFER_SIZE 231
/*==================[internal data definition]===============================*/
TaskHandle_t ADC_task_handle = NULL;
TaskHandle_t DAC_task_handle = NULL;
const char ecg[BUFFER_SIZE] = {
    76, 77, 78, 77, 79, 86, 81, 76, 84, 93, 85, 80,
    89, 95, 89, 85, 93, 98, 94, 88, 98, 105, 96, 91,
    99, 105, 101, 96, 102, 106, 101, 96, 100, 107, 101,
    94, 100, 104, 100, 91, 99, 103, 98, 91, 96, 105, 95,
    88, 95, 100, 94, 85, 93, 99, 92, 84, 91, 96, 87, 80,
    83, 92, 86, 78, 84, 89, 79, 73, 81, 83, 78, 70, 80, 82,
    79, 69, 80, 82, 81, 70, 75, 81, 77, 74, 79, 83, 82, 72,
    80, 87, 79, 76, 85, 95, 87, 81, 88, 93, 88, 84, 87, 94,
    86, 82, 85, 94, 85, 82, 85, 95, 86, 83, 92, 99, 91, 88,
    94, 98, 95, 90, 97, 105, 104, 94, 98, 114, 117, 124, 144,
    180, 210, 236, 253, 227, 171, 99, 49, 34, 29, 43, 69, 89,
    89, 90, 98, 107, 104, 98, 104, 110, 102, 98, 103, 111, 101,
    94, 103, 108, 102, 95, 97, 106, 100, 92, 101, 103, 100, 94, 98,
    103, 96, 90, 98, 103, 97, 90, 99, 104, 95, 90, 99, 104, 100, 93,
    100, 106, 101, 93, 101, 105, 103, 96, 105, 112, 105, 99, 103, 108,
    99, 96, 102, 106, 99, 90, 92, 100, 87, 80, 82, 88, 77, 69, 75, 79,
    74, 67, 71, 78, 72, 67, 73, 81, 77, 71, 75, 84, 79, 77, 77, 76, 76,
};
/*==================[internal functions declaration]=========================*/
/**
 * @brief  Función invocada en la interrupción del timer para la conversión analógica-digital. 
 *         Envía una notificación que es recibida dentro de la tarea correspondiente a leer un señal analógica del canal 1 y enviarla
 *         por el puerto serie de la PC.
 */
void FuncTimerADC(void* param)
{
    vTaskNotifyGiveFromISR(ADC_task_handle, pdFALSE);
}
/**
 * @brief  Función invocada en la interrupción del timer para la conversión digital-analógica. 
 *         Envía una notificación que es recibida dentro de la tarea correspondiente a convertir una señal digital en analógica, la cual
 *         es enviada por el GPIO_0.
 */
void FuncTimerDAC(void* param)
{
    vTaskNotifyGiveFromISR(DAC_task_handle, pdFALSE);
}

/**
 * @brief  Función que digitaliza una señal analógica tomando la entrada CH1 y la transmite por la UART conectada al puerto serie de la PC en 
 *         un formato compatible con un graficador por puerto serie. 
 */
static void ADCTask(void *pvParameter)
{
    uint16_t data = 0;
    while(true)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        AnalogInputReadSingle(CH1, &data);

        UartSendString(UART_PC, (const char*)UartItoa(data, 10));
        UartSendString(UART_PC, "\r");
    }
}
/**
 * @brief  Función que convierte una señal digital de un ECG en una señal analógica.
 */
static void DACTask(void *pvParamter)
{
    uint8_t i = 0;
    while(true)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        AnalogOutputWrite(ecg[i]);
        i++;

        if(i >= BUFFER_SIZE)
        {
            i=0;
        }
    }
}
/*==================[external functions definition]==========================*/
void app_main(void)
{
    // Configuración e inicialización del Timer para la conversión A-D con un período de muestreo de 2 ms.
    timer_config_t timer_ADC = 
    {
        .timer = TIMER_A,
        .period = CONFIG_PERIOD_ADC,
        .func_p = FuncTimerADC,
        .param_p = NULL
    };
    TimerInit(&timer_ADC);

    // Configuración e inicialización del Timer para la conversión A-D con un período de 4 ms.
    timer_config_t timer_DAC = 
    {
        .timer = TIMER_B,
        .period = CONFIG_PERIOD_DAC,
        .func_p = FuncTimerDAC,
        .param_p = NULL
    };
    TimerInit(&timer_DAC);

    // Configuración e inicialización del Transmisor - Receptor asíncrono universal para la transmisión de datos por puertos serie.
    serial_config_t my_uart =
    {
        .port = UART_PC,
        .baud_rate = 115200,
        .func_p = NULL,
        .param_p = NULL
    };
    UartInit(&my_uart);

    // Configuración e inicialización del conversor AD/DA para el manejo de entradas y salidas analógicas.
	analog_input_config_t analog_i =
    {
        .input = CH1,
        .mode = ADC_SINGLE,
        .func_p = NULL,
        .param_p = NULL,
        .sample_frec = NULL
    };
    AnalogInputInit(&analog_i);
    AnalogOutputInit();

    // Creación de tareas para las conversones AD y DA.
    xTaskCreate(&DACTask, "DAC", 512, NULL, 5, &DAC_task_handle);
    xTaskCreate(&ADCTask, "ADC", 512, NULL, 5, &ADC_task_handle);

    // Inicio de Timers.
    TimerStart(timer_ADC.timer);
    TimerStart(timer_DAC.timer);
}
/*==================[end of file]============================================*/