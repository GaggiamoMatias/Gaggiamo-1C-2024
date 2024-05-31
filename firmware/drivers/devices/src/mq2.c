/**
 * @file led.c
 * @author Albano Peñalva (albano.penalva@uner.edu.ar)
 * @brief 
 * @version 0.1
 * @date 2023-10-20
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include "mq2.h"
#include "analog_io_mcu.h"
#include "delay_mcu.h"
#include "math.h"

analog_input_config_t my_conversor; //!< conversor AD

float           LPGCurve[3]  =  {2.3,0.21,-0.47};   //!< arreglo caracteristico de la curva del gas licuado de petroleo. formato {x,y,pendiente}


void MQInit(uint8_t channel)
{
	my_conversor.input = channel;
	my_conversor.mode = ADC_SINGLE;
	my_conversor.func_p = NULL;
    my_conversor.param_p = NULL;
    my_conversor.sample_frec = NULL;
	AnalogInputInit(&my_conversor);
}

uint16_t MQResistanceCalculation(uint16_t raw_adc)
{
	uint16_t RL_VALUE = 1;
	uint16_t valores_bits = 4095;
	return ((RL_VALUE*(valores_bits-raw_adc))/raw_adc);
}

uint16_t MQCalibration()
{
	  int i;
	  uint16_t val=0;
	  uint16_t value=0;

	  for (i=0;i<CALIBRATION_SAMPLE_TIMES;i++) 
	  {
		AnalogInputReadSingle(my_conversor.input, &value);
	    val += MQResistanceCalculation(value);
	    DelayMs(CALIBRATION_SAMPLE_INTERVAL);
	  }
	  val = val/CALIBRATION_SAMPLE_TIMES;

	  val = val/RO_CLEAN_AIR_FACTOR;

	  if(val == 0)
		  val = 1;

	  return val;
}

uint16_t MQRead()
{
	  int i;
	  uint16_t rs=0;
	  uint16_t value=0;

	  for (i=0;i<READ_SAMPLE_TIMES;i++) {
		AnalogInputReadSingle(my_conversor.input, &value);
	    rs += MQResistanceCalculation(value);
	    DelayMs(READ_SAMPLE_INTERVAL);
	  }

	  rs = rs/READ_SAMPLE_TIMES;

	  return rs;
}

float  MQGetPercentage(uint16_t rs_ro_ratio)
{
	return (pow(10,( ((log((float)rs_ro_ratio)-LPGCurve[1])/LPGCurve[2]) + LPGCurve[0])));
}

