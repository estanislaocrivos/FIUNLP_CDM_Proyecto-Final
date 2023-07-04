#include <avr/io.h>
#define F_CPU 16000000UL
#include <util/delay.h>
#include "servomotor.h"

// Librería para el servomotor SG90

// Alumno: Estanislao Crivos Gandini 68449/0

void servo_timer_setup(void) 
{
	// Configuracion del Timer 2 para controlar el servomotor
	
	DDRB |= (1<<PORTB3); // Servo OUTPUT PIN (OC2A)
	
	// Seteo TIMER2 en modo PWM para controlar la posici�n del servo
	
	// Modo FAST PWM con MAX en OCR2A:
	
	TCCR2A |= (1<<COM2A1)|(1<<WGM21)|(1<<WGM20); // SET OC2A al cumplirse el match (non-inverting) 
	TCCR2A &= ~(1<<COM2A0); // Modo FAST PWM bottom/max
	TCCR2B |= (1<<CS22)|(1<<CS21)|(1<<CS20); // Prescaler en 1024 ( T = 1024/16MHz = 64us )
	
	ICR1 = 250; // 250*64us = 16ms (per�odo aproximado PWM del servomotor)
	
	// Modificando el contenido del registro OC02A regulo el ancho de pulso y de esa forma consigo posicionar al servo:
	 
	//OCR2A = 16; // 1ms/64us = 15.625 ticks
	//OCR2A = 24; // 1.5ms/64us = 23.4375 ticks
	//OCR2A = 32; // 2ms/64us = 31.25 ticks
}
