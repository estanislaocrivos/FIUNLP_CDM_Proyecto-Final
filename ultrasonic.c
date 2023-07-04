#include <avr/io.h>
#define F_CPU 16000000UL
#include <util/delay.h>
#include <avr/interrupt.h>
#include "ultrasonic.h"

// Librería para el sensor de ultrasonido HC-SR04

// Alumno: Estanislao Crivos Gandini 68449/0 

void sensor_timer_setup(void)
{
	// Configuraci�n del timer utilizado por el sensor de ultrasonido

	TCCR1A = 0; // Normal Mode (count up)
	TIMSK1 |= (1<<TOIE1); // Habilito interrup. por overflow
	sei(); // Habilito interrupciones
}