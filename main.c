// Proyecto Final - TP3 - Circuitos Digitales y Microprocesadores
// Cursada Virtual COVID-19 2020/2021
// Facultad de Ingeniería UNLP

// Alumno: Estanislao Crivos Gandini 68449/0

// Implementación de un sistema integral de seguridad y automatización para aberturas 
// utilizando el microcontrolador AVR ATmega328P y el kit Arduino UNO

#define F_CPU 16000000UL

/* Librerías externas: */
#include <avr/io.h>
#include <string.h>
#include <util/delay.h>
#include <avr/interrupt.h>

/* Librerías propias: */
#include "keypad.h"
#include "lcd_display.h"
#include "servomotor.h"
#include "ultrasonic.h"

/* Defino puertos: */

// Sensor:
#define echo_pin    PORTB0
#define trigger_pin PORTB1
// Buzzer:
#define buzzer_pin PORTB4
// Servo:
#define servo_pin   PORTB3
// Relays:
#define relay_1_pin PORTB2 // Utilizado para el estado: cerrado
#define relay_2_pin PORTB5 // Utilizado para el estado: abierto

/* Defino variables globales: */
char string[10];
double ticks = 0;
double distance = 0;
volatile int timer_overflow = 0;
volatile int key_flag = 0;
int j, key_entered, check;
int flecha_der = 0x7E; // Caracteres utilizados para el display
int flecha_izq = 0x7F;
int key_entered_array[4] = {0, 0, 0, 0}; // Arreglo donde se almacena el passcode ingresado
int key_stored_array[4]  = {1, 2, 3, 4}; // Passcode de acceso por defecto 
int key_root_array[4] = {2, 5, 8, 0};	 // Passcode de superusuario
	
/* Defino funciones: */
void startup_setup(void);		// Función que inicializa puertos e interrupciones
void pin_set_interrupt(void);   // Función para setear pines de interrupción
void LCD_welcome_setup(void);	// Función que muestra la pantalla de welcome en stand-by
void standby_mode(void);		// Función que restablece el modo de stand-by
void admit_access(void);		// Función ejecutada al admitir acceso
void deny_access(void);			// Función ejecutada al denegar acceso
void root_menu(void);			// Función que ejecuta el menu de superusuario
double measure_distance(void);	// Función para medir con el sensor de ultrasonido
int compare(int array_1[4],int array_2[4]); // Función para comparar arreglos de 4 elementos


/* ------------------------------------------------------------------------------------------------ */


int main(void) /* Programa principal */
{
	/* SETUP: */
		
	/* Configuro modo bajo consumo: */
	SMCR |= (1<<SM1)|(1<<SE); // Low-power mode
	
	/* Inicializo perifericos, variables y modos de funcionamiento: */
	startup_setup();	  // Inicializo puertos
	keypad_scan_setup();  // Inicializo teclado
	sensor_timer_setup(); // Inicializo timer para el sensor de ultrasonido
	servo_timer_setup();  // Inicializo timer para el servomotor
	LCD_init();			  // Inicializo display
	LCD_welcome_setup();  // Mensaje de bienvenida
		
	OCR2A = 8; // Servomotor en posición 0 grados (traba cerrada)
	
	_delay_ms(1000);
	
	asm("sleep");
	
	/* PROGRAMA PRINCIPAL */

	while (1) 
	{	
		/* La variable key_flag es puesta en alto dentro de la interrupción por INT1 */
		
		while (key_flag==1) // A la espera de interrupción
		{				
			cli(); // Desactivo interrupciones por INT1
			
			key_entered	= keypad_scan(); // Escaneo el keypad
		
			if (key_entered != 0xFF) // Si se ingreso un dígito, ejecutar:
			{
				key_entered_array[j] = key_entered; // Almaceno el dígito ingresado en un array
				j++;
				
				PORTB ^= (1<<buzzer_pin); // Sonido de confirmación 
				_delay_ms(50);
				PORTB ^= (1<<buzzer_pin);
				
				LCD_go_to(j+6,2); // Imprimo número en pantalla
				itoa(key_entered,string,10);
				LCD_write_string(string);
				_delay_ms(500);
				
				if (j==4) // Compruebo si se ingresaron los 4 dígitos
				{
					check = compare(key_entered_array,key_stored_array); // Verifico key
					
					if (check == 0) // Admitir acceso
					{
						admit_access(); 
						standby_mode(); // Restablezco estado de stand-by
					}
					else
					{
						check = compare(key_entered_array,key_root_array); // Compruebo si se ingreso el key de superusuario
						
						if (check==0) // Acceso al menu raíz
						{
							root_menu();
							standby_mode(); // Restablezco estado de stand-by
						}
						else // Denegar acceso
						{
							deny_access();
							standby_mode(); // Restablezco estado de stand-by
						}
					}
					
				} // Fin del if(...) secundario
							
			} // Fin del if(...) principal

		} // Fin del loop while(...)
		 
	} // Fin del loop while(1)
	
} // Fin del MAIN


/* ------------------------------------------------------------------------------------------------ */


ISR(INT1_vect) /* Interrupt Service Routine para interrup. externa por INT1 */
{
	// Codigo a ejecutar al detectar una interrupcion por keypad
	
	PORTB ^= (1<<buzzer_pin); // Sonido de confirmación
	_delay_ms(50);
	PORTB ^= (1<<buzzer_pin);
	
	_delay_ms(25); // Delay antirebote
	
	LCD_cmd(0x01); // Limpio display
	LCD_go_to(1,1);
	LCD_write_string("Enter 4-dig key:");
	
	key_flag = 1; // Flag que le avisa al main que se ingresó el digito '*'
}


/* ------------------------------------------------------------------------------------------------ */


ISR(TIMER1_OVF_vect) /* Interrupt Service Routine para interrup. interna del Timer 1 */
{
	timer_overflow++; // Variable utilizada por la función del sensor
}


/* ------------------------------------------------------------------------------------------------ */


double measure_distance(void) /* Función para medir distancia con el sensor de ultrasonido HCSR04 */
{	
	/* El timer se acciona al recibir la senal de echo. Se incrementa la variable timer_overflow en cada interrupción del timer.
	Al finalizar la senal de echo, el timer deja de contar y se utiliza esta variable para determinar el tiempo que la senal estuvo en alto y 
	así medir la distancia.*/
	
	PORTB |= (1<<trigger_pin);
	_delay_us(10); // Pulso de 10 microsegundos para el trigger del sensor
	PORTB &= ~(1<<trigger_pin);
	
	TCNT1 = 0; // Clear Counter Flag
	TCCR1B |= (1<<ICES1)|(1<<CS10); // Captura en flanco asc. (ECHO) // Prescaler = 0
	TIFR1 |= (1<<ICF1)|(1<<TOV1); // Limpio flags
	
	while ((TIFR1 & (1 << ICF1)) == 0); // Espero flanco asc. de la senal de ECHO

	TCNT1 = 0; // Clear Timer Counter
	
	TCCR1B |= (1<<CS10); // Captura en flanco desc. (ECHO)
	TCCR1B &= ~(1<<ICES1); // Prescaler = 0
	
	TIFR1 |= (1<<ICF1)|(1<<TOV1); // Limpio flags	
	
	timer_overflow = 0; // Reinicio cuenta del timer overflow

	while ((TIFR1 & (1 << ICF1)) == 0); // Espero flanco desc. de la senal de ECHO

	ticks = ICR1 + (65535 * timer_overflow); // Cuento ticks del timer (65535 = 2 a la 16)

	distance = ticks / 932.9; // Distancia medida en centímetros [ 1/932.9 = [(34300cm/s)/2] * (1/16MHz) ]
	
	return distance;
} 


/* ------------------------------------------------------------------------------------------------ */


void admit_access(void) /* Función ejecutada al admitir acceso */
{
	int k; // Variable de iteración
	
	PORTB ^= (1<<buzzer_pin); // Sonido de confirmación
	_delay_ms(50);
	PORTB ^= (1<<buzzer_pin);
	_delay_ms(50);
	PORTB ^= (1<<buzzer_pin);
	_delay_ms(50);
	PORTB ^= (1<<buzzer_pin);
		
	LCD_cmd(0x01); // Limpio display
	
	OCR2A = 38; // Servomotor en posición 180 grados (traba abierta)
	
	PORTB |= (1<<relay_1_pin);
	PORTB &= ~(1<<relay_2_pin); // Estado actual: puerta abriéndose
	
	_delay_ms(500);

	LCD_cmd(0X01); // Confirmo por display
	LCD_go_to(2,1);
	LCD_write_string("ACCESS GRANTED");
	LCD_go_to(1,2);
	LCD_write_string("<<OPENING DOOR>>");
	
	_delay_ms(1000);
	
	// Display muestra un juego visual por 5 segundos:

	for (k=1; k<=16; k++)
	{
		LCD_go_to(k,1);
		LCD_write(flecha_der);
		_delay_ms(150);
	}
	
	for (k=1; k<=16; k++)
	{
		LCD_go_to(17-k,2);
		LCD_write(flecha_izq);
		_delay_ms(150);
	}
	
	distance = measure_distance(); // Senso presencia en el paso de la puerta
	
	while(distance <= 20) // Acá determino el valor límite de distancia medida
	{
		// Mientras hayan obstáculos, sigo midiendo
		LCD_cmd(0x01);
		LCD_go_to(4,1);
		LCD_write_string("ON HOLD...");
		_delay_ms(500);
		distance = measure_distance();
	}	
	
	_delay_ms(1000);
	
	// Cierro puerta:
	
	LCD_cmd(0x01);
	LCD_go_to(1,1);
	LCD_write_string("<<<<<BEWARE>>>>>");
	LCD_go_to(3,2);
	LCD_write_string("CLOSING DOOR");
	
	PORTB ^= (1<<buzzer_pin); // Sonidos de advertencia
	_delay_ms(200);
	PORTB ^= (1<<buzzer_pin);
	_delay_ms(200);
	PORTB ^= (1<<buzzer_pin);
	_delay_ms(200);
	PORTB ^= (1<<buzzer_pin);
	_delay_ms(200);
	PORTB ^= (1<<buzzer_pin);
	_delay_ms(200);
	PORTB ^= (1<<buzzer_pin);
	
	_delay_ms(500);
	
	PORTB |=  (1<<relay_2_pin);
	PORTB &= ~(1<<relay_1_pin); // Estado actual: puerta cerrándose

	_delay_ms(2000);
	
	LCD_cmd(0x01);
	
	OCR2A = 8; // Servomotor en posicion 0 grados (traba puesta)
	
	_delay_ms(1000);
	
	return;
} 


/* ------------------------------------------------------------------------------------------------ */


void deny_access(void) /* Función ejecutada al denegar acceso */
{
	PORTB ^= (1<<buzzer_pin); // Sonido de confirmaci�n
	_delay_ms(500);
	PORTB ^= (1<<buzzer_pin);
	
	LCD_cmd(0X01);
	LCD_go_to(2,1);
	LCD_write_string("ACCESS  DENIED");
	
	_delay_ms(2000);
	
	LCD_cmd(0x01);
	
	return;
}


/* ------------------------------------------------------------------------------------------------ */


void LCD_welcome_setup(void) /* Función que escribe el display de bienvenida */
{
	// Display de bienvenida en stand-by:
	
	LCD_go_to(5,1);
	LCD_write_string("WELCOME!");
	LCD_go_to(1,2);
	LCD_write_string("Press * to enter");
	_delay_ms(50);
}


/* ------------------------------------------------------------------------------------------------ */


int compare(int array_1[4],int array_2[4]) /* Función para comparar igualdad entre arreglos */
{
	// Devuelve 0 si los arreglos son iguales (1 si no lo son)
	
	uint8_t equal = 0;
	uint8_t l = 0;
	
	for (l=0; l<=3; l++)
	{
		if (array_1[l]==array_2[l])
		{
			equal++;
		}
	}
	
	if (equal==4)
	{
		return 0; // Los arrays son =
	}
	else
	{
		return 1; // Los arrays son !=
	}
}


/* ------------------------------------------------------------------------------------------------ */


void root_menu(void) /* Función ejecutada al entrar en el modo administrador */
{
	// Men� de opciones de superusuario. Se ingresa con el key brindado por el fabricante
	
	int w = 0; // Variables de control
	int k = 0;
	
	LCD_cmd(0x01); // Confirmo acceso por display
	LCD_go_to(1,1);
	LCD_write_string("***** ROOT *****");
	LCD_go_to(1,2);
	LCD_write_string("***** MENU *****");
	
	_delay_ms(1500);
	
	LCD_cmd(0x01); // Brindo opciones de superusuario
	LCD_go_to(1,1);
	LCD_write_string("1 Change key");
	LCD_go_to(1,2);
	LCD_write_string("2 Keep door open");
	
	while(key_flag == 1)
	{
		key_entered=keypad_scan(); // Leo el teclado
		
		if (key_entered != 0xFF)
		{
			PORTB ^= (1<<buzzer_pin);
			_delay_ms(50);
			PORTB ^= (1<<buzzer_pin);
			
			_delay_ms(100); // Prevengo reingreso del 1 o 2
			
			switch(key_entered) 
			{
				case 1: // Cambio password
				{
					LCD_cmd(0x01);
					LCD_go_to(1,1);
					LCD_write_string("Enter new key:");
					
					while(key_flag == 1)
					{
						key_entered	= keypad_scan(); // Escaneo el keypad
					
						if (key_entered != 0xFF) // Si se ingreso un digito, ejecutar:
						{
							key_stored_array[w] = key_entered; // Sobreescribo array de contrasena
						
							w++;
							
							PORTB ^= (1<<buzzer_pin); // Sonido de confirmacion
							_delay_ms(50);
							PORTB ^= (1<<buzzer_pin);
						
							LCD_go_to(w+6,2); // Imprimo en pantalla el digito
							itoa(key_entered,string,10); 
							LCD_write_string(string);
							_delay_ms(500);
						
							if (w==4) // Compruebo si se ingresaron los 4 digitos
							{
								key_flag = 0;	
							}
						}
					}
					LCD_cmd(0x01);
					LCD_write_string("**KEY  CHANGED**");
					_delay_ms(1000);
					LCD_cmd(0x01);
					return;
				}
				
				case 2: // Se abre la puerta indefinidamente
				{
					LCD_cmd(0x01);
					
					OCR2A = 38; // Servomotor en posicion 180 grados (traba abierta)
										
					PORTB |= (1<<relay_1_pin);
					PORTB &= ~(1<<relay_2_pin); // Estado actual: puerta abriendose
					
					LCD_cmd(0x01);
					LCD_go_to(1,1);
					LCD_write_string("Press any key to");
					LCD_go_to(1,2);
					LCD_write_string("  restore door  ");
										
					key_entered = keypad_scan(); // Si se ingresa algun digito, restablecer funcionamiento normal
					
					while (key_entered == 0xFF) // Escaneo el teclado hasta que se ingrese algun digito
					{
						key_entered = keypad_scan();
					}
						
					distance = measure_distance(); // Senso presencia en el paso de la puerta
								
					while(distance <= 20)
					{
						LCD_cmd(0x01);
						LCD_go_to(4,1);
						LCD_write_string("ON HOLD...");
						_delay_ms(500);
						distance = measure_distance();
					}
							
					LCD_cmd(0x01);
					LCD_go_to(1,1);
					LCD_write_string("<<<<<BEWARE>>>>>");
					LCD_go_to(3,2);
					LCD_write_string("CLOSING DOOR");
							
					PORTB ^= (1<<buzzer_pin);
					_delay_ms(200);
					PORTB ^= (1<<buzzer_pin);
					_delay_ms(200);
					PORTB ^= (1<<buzzer_pin);
					_delay_ms(200);
					PORTB ^= (1<<buzzer_pin);
					_delay_ms(200);
					PORTB ^= (1<<buzzer_pin);
					_delay_ms(200);
					PORTB ^= (1<<buzzer_pin);
					
					_delay_ms(500);
							
					PORTB |=  (1<<relay_2_pin);
					PORTB &= ~(1<<relay_1_pin); // Estado actual: puerta cerrándose

					_delay_ms(2000);
							
					LCD_cmd(0x01);

					OCR2A = 8; // Servomotor en posicion 0 grados (traba puesta)
							
					_delay_ms(1000);
							
					return;
				}
			}
		}
	}
}


/* ------------------------------------------------------------------------------------------------ */


void standby_mode(void) /* Función que restablece el modo stand-by */ 
{
	// Funci�n que restablece el estado de stand-by luego de un ciclo de trabajo del sistema
	
	key_flag=0; 
	
	j=0;
	
	LCD_welcome_setup();
		
	PORTD = 0b11101111; // Reinicio keypad
	
	sei(); // Habilito interrupciones
	
	asm("sleep"); // Ingreso en modo bajo consumo
}


/* ------------------------------------------------------------------------------------------------ */


void startup_setup(void) /* Configuración de puertos E/S */
{
	/* Inicializaci�n de puertos */
		
	// Relays:
	DDRB |= (1<<relay_1_pin);
	DDRB |= (1<<relay_2_pin);
	PORTB &= ~(1<<relay_1_pin); // Estado actual: puerta cerrada (relay CLOSED activado)
	PORTB |= (1<<relay_2_pin); 
		
	// Buzzer:
	DDRB |= (1<<buzzer_pin);
	PORTB &= ~(1<<buzzer_pin);
		
	// Sensor:
	DDRB |= (1<<trigger_pin); // Trigger como Output
	DDRB &= ~(1<<echo_pin);  // Echo como Input
	PORTB |= (1<<echo_pin); // Pull-Up para recibir el echo del sensor
	
	EICRA = 0;
	EIMSK |=  (1<<INT1); // Seteo interrup. por pin PD2 (INT1)
	sei(); // Habilito interrupciones
}


/* ------------------------------------------------------------------------------------------------ */
