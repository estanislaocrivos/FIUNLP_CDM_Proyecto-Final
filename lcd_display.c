#include <avr/io.h>
#define F_CPU 16000000UL
#include <util/delay.h>
#include "lcd_display.h"

// Librería para el display 16x2

// Alumno: Estanislao Crivos Gandini 68449/0

void LCD_init(void)
{
	// Funcion para inicializar el display
	
	DDRC = 0xFF; // Todos los PORTC como OUTPUTS
	
	// Rutina de inicializacion brindada por el fabricante
	
	_delay_ms(20);
	LCD_cmd(0x30);
	_delay_ms(5);
	LCD_cmd(0x30);
	_delay_ms(10);
	LCD_cmd(0x32);
	_delay_ms(5);
	
	LCD_cmd(0x28);
	_delay_ms(5);
	
	LCD_cmd(0x0C);
	_delay_ms(5);
	LCD_cmd(0x01);
	_delay_ms(5);
	LCD_cmd(0x02);
	_delay_ms(5);
	LCD_cmd(0x06);
	_delay_ms(5);
	LCD_cmd(0x80);
	
	_delay_ms(100);
	
	return;
}

void LCD_cmd(unsigned char cmd)
{
	// Funcion para enviarle un comando al display (ver comandos en hoja de datos)
	
	// Separo el numero de 8 bits en parte alta y parte baja:
	
	uint8_t DATO_LOW =  cmd & 0x0F;
	uint8_t DATO_HIGH = (cmd>>4) & 0x0F;
	
	// Cargo parte alta:
	
	DATO = DATO_HIGH;
	
	PORTC &=~ (1<<RS); // Habilito envio de comandos
	PORTC |= (1<<EN);  // Habilito ingreso de datos
	_delay_ms(10);
	PORTC &=~ (1<<EN); // Deshabilito ingreso de datos
	
	// Cargo parte baja:
	
	DATO = DATO_LOW; // Parte baja del numero
	
	PORTC &=~ (1<<RS);
	PORTC |= (1<<EN); // Habilito ingreso de datos
	_delay_ms(10);
	PORTC &=~ (1<<EN); // Deshabilito ingreso de datos
	
	return;
}

void LCD_write(unsigned char word)
{
	// Funcion para escribir un caracter en el display:
	
	// Separo el numero de 8 bits en parte alta y baja:
	
	uint8_t DATO_LOW =  word & 0x0F;
	uint8_t DATO_HIGH = (word>>4) & 0x0F;
	
	// Cargo parte alta:
	
	DATO = DATO_HIGH;
	
	PORTC |= (1<<RS)|(1<<EN); // Habilito envio de datos // Habilito ingreso de datos
	_delay_ms(10);
	PORTC &=~ (1<<EN); // Deshabilito ingreso de datos
	
	// Cargo parte baja:
	
	DATO = DATO_LOW; // Parte baja del numero
	
	PORTC |= (1<<RS)|(1<<EN); // Habilito envio de datos // Habilito ingreso de datos
	_delay_ms(10);
	PORTC &=~ (1<<EN); // Deshabilito envio de datos // Deshabilito ingreso de datos
	
	return;
}

void LCD_write_string(unsigned char *str)
{
	// Funcion para escribir una cadena de caracteres en el display
	
	unsigned char i=0;
	while (str[i] != 0x00)
	{
		LCD_write(str[i++]);
	}
	return;
}

void LCD_go_to(unsigned char x, unsigned char y)
{
	// Funcion para ubicar el cursor en el display

	unsigned char inicio[] = {0x80|0x00, 0x80|0xC0};
	LCD_cmd(inicio[y-1]+x-1);
}
