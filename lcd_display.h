#include <avr/io.h>

// Librería para el display 16x2

// Alumno: Estanislao Crivos Gandini 68449/0

#define EN PORTC5
#define RS PORTC4
#define DATO PORTC	

void LCD_write_string(unsigned char *str);

void LCD_cmd(unsigned char cmd);

void LCD_write(unsigned char word);

void LCD_go_to(unsigned char x, unsigned char y);

void LCD_init(void);
