/*! @mainpage Proyecto Integrador - Electrónica Programable - "Sistema de Control de Acceso y Monitoreo de Seguridad Ambiental"
 *
 * @section genDesc General Description
 * 
 * Esta aplicación implementa un sistema de monitoreo ambiental y control de acceso para su posible utilización en laboratorios o industrias con
 * riesgos de accidentes. El sistema mide la temperatura, la humedad y la concentración de gas butano, y controla una alarma en caso de que estos
 * valores superen ciertos umbrales predefinidos. Además, el sistema gestiona el acceso mediante tarjetas RFID, permitiendo o denegando el acceso
 * según el UID de la tarjeta y las condiciones óptimas en la habitación.
 *
 * @section hardConn Hardware Connection
 *
 * |    Peripheral  |   ESP32   	|
 * |:--------------:|:--------------|
 * | 	PIN_X	 	| 	GPIO_X		|
 *
 * |   	MRFC522		|   ESP-EDU		|
 * |:--------------:|:--------------|
 * | 	SDO/MISO 	|	GPIO_22		|
 * | 	3V3		 	| 	3V3			|
 * | 	SCK		 	| 	GPIO_20		|
 * | 	SDI/MOSI 	| 	GPIO_21		|
 * | 	RESET	 	| 	GPIO_18		|
 * | 	CS		 	| 	GPIO_9		|
 * | 	GND		 	| 	GND			|
 *
 * |   	MQ2			|   ESP-EDU		|
 * |:--------------:|:--------------|
 * | 	A0		 	|	GPIO_0		|
 * | 	5V		 	| 	5V			|
 * | 	GND		 	| 	GND			|
 *
 * |   	Si7007		|   ESP-EDU		|
 * |:--------------:|:--------------|
 * | 	CS 			|	GPIO_23		|
 * | 	3V3		 	| 	3V3			|
 * | 	PWM1		| 	GPIO_1		|
 * | 	PWM2 	 	| 	GPIO_2		|
 * | 	GND		 	| 	GND			|
 *
 * |   Servo_sg90	|   ESP-EDU		|
 * |:--------------:|:--------------|
 * | 	CONTROL 	|	GPIO_3		|
 * | 	3V3		 	| 	3V3			|
 * | 	GND		 	| 	GND			|
 * 
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 17/05/2024 | Document creation		                         |
 *
 * @author Matias Andres Gaggiamo (matias.gaggiamo@ingenieria.uner.edu.ar)
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"
#include "buzzer.h"
#include "timer_mcu.h"
#include "uart_mcu.h"
#include "dht11.h"
#include "mq2.h"
#include "analog_io_mcu.h"
#include "rfid_utils.h"
#include "mfrc522.h"
#include "servo_sg90.h"
#include "Si7007.h"
/*==================[macros and definitions]=================================*/
/**
 * @brief Período de medición en microsegundos.
 */
#define CONFIG_MEASURE_PERIOD 2000000
/**
 * @brief Período de chequeo de la alarma en milisegundos.
 */
#define CONFIG_ALARM_PERIOD 1000
/**
 * @brief Período de chequeo del acceso en milisegundos.
 */
#define CONFIG_ACCESS_PERIOD 1000

/**
 * @brief Tiempo de operación de la puerta en milisegundos.
 */
#define OPER_DOOR_PERIOD 3000
/**
 * @brief Número de UIDs válidos para acceso.
 */
#define NUM_UIDS 3
/*==================[internal data definition]===============================*/
/**
 * @brief Handle para la tarea de medición.
 */
TaskHandle_t measure_task_handle = NULL;
/**
 * @brief Handle para la tarea de alarma.
 */
TaskHandle_t alarm_task_handle = NULL;
/**
 * @brief Handle para la tarea de acceso.
 */
TaskHandle_t access_task_handle = NULL;

/**
 * @brief Indica si la puerta puede ser abierta o no.
 */
bool permisoPuerta = false;
/**
 * @brief Indica si se debe abrir la puerta o no.
 */
bool abrirPuerta = false;
/**
 * @brief Indica si la alarma debe ser activada o no.
 */
bool activarAlarma = false;

/**
 * @brief Umbral de concentración de gas butano.
 */
uint16_t umbral_gas = 25000; 
/**
 * @brief Umbral de temperatura.
 */
uint16_t umbral_temp = 100;
/**
 * @brief Umbral de humedad.
 */
uint16_t umbral_hum = 100;

/**
 * @brief Resistencia R0 en aire en condiciones normales del sensor MQ2.
 */
uint16_t RO_ = 0;
/**
 * @brief Valor de gas butano medido.
 */
float valor_gas = 0;
/**
 * @brief Valor de gas butano convertido a entero.
 */
uint16_t valor_gas_convertido = 0; // valor de gas butano convertido a entero.

/**
 * @brief Valor de temperatura medido.
 */
float temp = 0;
/**
 * @brief Valor de temperatura convertido a entero.
 */
uint16_t temp_converted = 0;
/**
 * @brief Valor de humedad medido.
 */
float hum = 0;
/**
 * @brief Valor de humedad convertido a entero.
 */
uint16_t hum_converted = 0;

/**
 * @brief Último ID de usuario leído.
 */

unsigned int last_user_ID;
/**
 * @brief Instancia del lector RFID.
 */
MFRC522Ptr_t mfrcInstance;

/**
 * @brief UIDs válidos para acceso.
 */
uint8_t valid_uids[NUM_UIDS][4] = 
{
    {0xd3, 0x1c, 0xe9, 0xfc},
    {0Xf6, 0x84, 0x19, 0x9e},
    {0x83, 0x33, 0xbd, 0xb}
};
/*==================[internal functions declaration]=========================*/
/**
 * @brief Inicializa los componentes del sistema.
 */
void inicializar();
/**
 * @brief Valida un UID de tarjeta RFID.
 * 
 * @param uid Puntero al array que contiene el UID de la tarjeta.
 * @param size Tamaño del UID.
 * @return true Si el UID es válido.
 * @return false Si el UID no es válido.
 */
bool validateUID(uint8_t *uid, uint8_t size);
/**
 * @brief Maneja la lectura de una tarjeta/tag:
 * - Muestra el UID de la tarjeta leída a través del puerto UART.
 * - Convierte el UID en un identificador de usuario.
 * - Valida el UID contra una lista de UIDs válidos.
 * - Controla el acceso permitiendo o denegando la apertura de la puerta.
 */
void handleRFIDCard();

/**
 * @brief  Función invocada en la interrupción del timer para la medición de concentración de gas, temperatura y humedad. 
 *         Envía una notificación que es recibida dentro de la tarea correspondiente a medir las condiciones ambientales.
 */
void FuncTimerA(void* param)
{
    vTaskNotifyGiveFromISR(measure_task_handle, pdFALSE);
}

/**
 * @brief Tarea de medición de temperatura, humedad y concentración de gas.
 * 
 * Esta tarea se ejecuta periódicamente con el control de un temporizador:
 * - Mide la temperatura, humedad y concentración de gas butano utilizando el sensor MQ2 y el sensor Si7007.
 * - Convierte las lecturas obtenidas a un formato adecuado para su transmisión y las envía por UART.
 * - Verifica si alguna de las mediciones supera los umbrales definidos, activando la alarma y bloqueando el acceso si es necesario.
 * 
 * @param pvParameter Parámetro de la tarea (no utilizado).
 */
static void measureTask(void *pvParameter)
{
    while(true)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		temp = Si7007MeasureTemperature();
		hum = Si7007MeasureHumidity();
		valor_gas = MQGetPercentage(MQRead()/RO_);

    	valor_gas_convertido = (uint16_t)(valor_gas*1000000);
		temp_converted = (uint16_t)(temp);
		hum_converted = (uint16_t)(hum);

		UartSendString(UART_PC, "\nTemperatura: ");
		UartSendString(UART_PC, (const char*)UartItoa(temp_converted, 10));
        UartSendString(UART_PC, " °C\r\n");

		UartSendString(UART_PC, "Humedad: ");
		UartSendString(UART_PC, (const char*)UartItoa(hum_converted, 10));
        UartSendString(UART_PC, "%\r\n");

		UartSendString(UART_PC, "Concentracion de gas: ");
		UartSendString(UART_PC, (const char*)UartItoa(valor_gas_convertido, 10));
        UartSendString(UART_PC, " ppm.\r\n");

		if(temp_converted >= umbral_temp || hum_converted >= umbral_hum || valor_gas_convertido >= umbral_gas)
		{
			activarAlarma = true;
			permisoPuerta = false;
		}
		else
		{
			activarAlarma = false;
			permisoPuerta = true;
		}
    }
}
/**
 * @brief Tarea de manejo de la alarma.
 * 
 * - Si la alarma está activada, alterna el estado de varios LEDs y el buzzer para indicar la condición de alarma.
 * - Si la alarma no está activada, apaga todos los LEDs y el buzzer.
 * 
 * @param pvParameter Parámetro de la tarea (no utilizado).
 */
static void alarmTask(void *pvParameter)
{
	bool encender_buzzer = false;
	while(true)
	{
		if(activarAlarma)
		{
			LedToggle(LED_1);
			LedToggle(LED_2);
			LedToggle(LED_3);
		

			if(encender_buzzer)
			{
				//GPIOOn(GPIO_8);
				//BuzzerPlayTone(4000,750);
				encender_buzzer = false;
			}
			else
			{
				//GPIOOff(GPIO_8);
				//BuzzerOff();
				encender_buzzer = true;
			}
		}
		else
		{
			LedsOffAll();
			//GPIOOff(GPIO_8);
			//BuzzerOff();
			encender_buzzer = false;
		}
		vTaskDelay(CONFIG_ALARM_PERIOD/portTICK_PERIOD_MS);
	}
}
/**
 * @brief Tarea de control de acceso mediante RFID.
 * 
 * - Inicializa el control de acceso y envía un mensaje por UART.
 * - Verifica si hay una nueva tarjeta RFID presente y, si es así, lee su UID.
 * - Maneja la tarjeta RFID utilizando la función `handleRFIDCard`.
 * - Si la tarjeta es válida y se permite el acceso, mueve el servo para abrir la puerta y enciende un LED.
 * 
 * @param pvParameter Parámetro de la tarea (no utilizado).
 */
static void accessTask(void *pvParameter)
{
	UartSendString(UART_PC,"Inicializando control de acceso.\r\n");

	while(1)
	{
		if (PICC_IsNewCardPresent(mfrcInstance)) 
		{
			if (PICC_ReadCardSerial(mfrcInstance)) 
			{
				handleRFIDCard();
				if(abrirPuerta)
				{
					ServoMove(SERVO_0, -90);
					LedOn(LED_1);

					vTaskDelay(OPER_DOOR_PERIOD / portTICK_PERIOD_MS);

					ServoMove(SERVO_0, 0);
					LedOff(LED_1);

					abrirPuerta = false;
				}
			}
		}
		vTaskDelay(CONFIG_ACCESS_PERIOD / portTICK_PERIOD_MS);
	}
}
/*==================[external functions definition]==========================*/
void app_main(void)
{
	inicializar();

	timer_config_t timer_measure = 
    {
        .timer = TIMER_A,
        .period = CONFIG_MEASURE_PERIOD,
        .func_p = FuncTimerA,
        .param_p = NULL
    };
    TimerInit(&timer_measure);

	xTaskCreate(&measureTask, "MEASURE", 4096, NULL, 5, &measure_task_handle);
	xTaskCreate(&alarmTask, "ALARM", 1024, NULL, 5, &alarm_task_handle);
	xTaskCreate(&accessTask, "ACCESS", 1024, NULL, 5, &access_task_handle);

	TimerStart(timer_measure.timer);
}

void inicializar()
{
	printf("INICIALIZO\n");

	MQInit(GPIO_0);
	LedsInit();
	ServoInit(SERVO_0, GPIO_3);

	UartSendString(UART_PC, "Calibrando...\n");
	RO_ = MQCalibration();
	UartSendString(UART_PC, "Calibracion finalizada!\n");

	serial_config_t my_uart =
    {
        .port = UART_PC,
        .baud_rate = 115200,
        .func_p = NULL,
        .param_p = NULL
    };
    UartInit(&my_uart);

	Si7007_config sensorTempHum =
	{
		.select = GPIO_23,
		.PWM_1 = GPIO_1,
		.PWM_2 = GPIO_2
	};
	Si7007Init(&sensorTempHum);

	setupRFID(&mfrcInstance);
}

void handleRFIDCard() 
{
	//	show card UID
	UartSendString(UART_PC,"\nCard uid bytes: ");
	for (uint8_t i = 0; i < mfrcInstance->uid.size; i++) 
	{
		UartSendString(UART_PC," 0X");
		UartSendString(UART_PC, (char*)UartItoa(mfrcInstance->uid.uidByte[i], 16));
		UartSendString(UART_PC," ");
	}
	UartSendString(UART_PC,"\n\r");

	// Convert the uid bytes to an integer, byte[0] is the MSB
	last_user_ID =
		(int)mfrcInstance->uid.uidByte[3] |
		(int)mfrcInstance->uid.uidByte[2] << 8 |
		(int)mfrcInstance->uid.uidByte[1] << 16 |
		(int)mfrcInstance->uid.uidByte[0] << 24;

	UartSendString(UART_PC,"Card Read user ID: ");
	UartSendString(UART_PC, (char*)UartItoa(last_user_ID, 10));
	UartSendString(UART_PC,"\n\r");

	// Validate the UID
    if (validateUID(mfrcInstance->uid.uidByte, mfrcInstance->uid.size) && permisoPuerta)
    {
        UartSendString(UART_PC, "\nAccess Granted\n\r");
        abrirPuerta = true; // Permitir acceso
    }
    else
    {
        UartSendString(UART_PC, "\nAccess Denied\n\r");
        abrirPuerta = false; // Denegar acceso
    }
}

bool validateUID(uint8_t *uid, uint8_t size)
{
	for (int i = 0; i < NUM_UIDS; i++) 
	{
        if (memcmp(uid, valid_uids[i], size) == 0) 
		{
            return true;
        }
    }
    return false;
}
/*==================[end of file]============================================*/