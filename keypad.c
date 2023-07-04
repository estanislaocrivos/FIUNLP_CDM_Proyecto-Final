#include <avr/io.h>
#define F_CPU 16000000UL
#include <util/delay.h>
#include "keypad.h"

// Librería para el teclado de membrana 4x4

// Alumno: Estanislao Crivos Gandini 68449/0

void keypad_scan_setup(void)
{
	/* Inicializo puertos */
	DDRD = 0xF0; // PD7-4 OUTPUTS // PD3-0 INPUTS
	PORTD = 0b11101111;; // OUTPUT en BAJO // INPUTS en PULL-UP
}

int keypad_scan(void)
{
	// Funci�n que escanea el keypad 4x4 y devuelve el d�gito ingresado

	uint8_t i;
	
	uint8_t entrada;
	
	uint8_t salida[4] = {0x7F, 0xBF, 0xDF, 0xEF}; // Pongo una salida (fila) en cero a la vez
	
	for (i=0; i<4; i++)
	{
		PORTD = salida[i]; // Salidas en bajo una a la vez y entradas en alto
		asm("nop"); // Espera 1 ciclo
		entrada = PIND & 0x0F; 
		
		if (entrada != 0x0F) // Compruebo si se ingres� un digito
		{
			switch(entrada)
			{
				// Verifico qu� tecla que se presiono dependiendo del indice 'i' de la iteraci�n y la fila que se puso en bajo
				
				case 0x07:  switch(i) 
				{
					case 0: return 1; 
					case 1: return 4;
					case 2: return 7; 
					case 3: return 14; // Asterisco
				}
				
				case 0x0B:  switch(i) 
				{
					case 0: return 2; 
					case 1: return 5; 
					case 2: return 8; 
					case 3: return 0; 
				}
				
				case 0x0D:  switch(i) 
				{
					case 0: return 3;
					case 1: return 6; 
					case 2: return 9; 
					//case 3: return 15; // Numeral
				}
			}
		}
	}
	return 0xFF; // Si no se presion� nada
} 