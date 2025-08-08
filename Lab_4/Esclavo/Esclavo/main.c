/*
 * Esclavo.c
 *
 * Created: 1/08/2025 08:00:04
 * Author : valen
 */

#define F_CPU 16000000 // Frecuencia del sistema: 16 MHz

#include <avr/io.h>        // Librería base de registros para AVR
#include <avr/interrupt.h> // Librería para manejo de interrupciones
#include <util/delay.h>    // Librería para retardos
#include "I2C.h"           // Librería personalizada para comunicación I2C

// Dirección del esclavo
#define SlaveAddress 0x30

// Variables globales
uint8_t buffer = 0;             // Almacena datos recibidos por I2C
uint8_t contador4bits = 0;      // Contador limitado a 4 bits (0-15)

// Prototipos de funciones
void initPorts(void);
void setup(void);

//******************************************************************

int main(void)
{
	initPorts(); // Configura pines de entrada/salida
	setup();     // Configura interrupciones externas y pull-ups
	
	I2C_Slave_Init(SlaveAddress); // Inicializa el esclavo I2C con la dirección 0x30
	sei(); // Habilita interrupciones globales

	while (1)
	{
		// Si el maestro escribe 'R', se limpia el buffer (respuesta automática se da en ISR)
		if (buffer == 'R')
		{
			buffer = 0;
		}
	}
}

// Interrupción del periférico TWI (I2C)
ISR(TWI0_vect)
{
	uint8_t estado;
	estado = TWSR0 & 0xFC; // Se lee el estado de TWI (enmascarando bits menos significativos)

	switch (estado)
	{
		// El esclavo ha sido seleccionado con una escritura (SLA+W)
		case 0x60: // Dirección + write (propia)
		case 0x70: // Dirección general + write
			TWCR0 |= (1 << TWINT); // Limpia la bandera para continuar
			break;

		// El maestro ha enviado un dato al esclavo
		case 0x80: // Datos recibidos con dirección propia
		case 0x90: // Datos recibidos con dirección general
			buffer = TWDR0;        // Se guarda el dato recibido
			TWCR0 |= (1 << TWINT); // Limpia la bandera para continuar
			break;

		// El maestro solicita datos al esclavo (SLA+R)
		case 0xA8: // Dirección + read (propia)
		case 0xB8: // Se envió el dato y el maestro espera más
			TWDR0 = contador4bits; // Se envía el valor del contador como respuesta
			TWCR0 = (1 << TWEN) | (1 << TWIE) | (1 << TWINT) | (1 << TWEA); // Configura para enviar y seguir escuchando
			break;

		// Cualquier otro estado inesperado
		default:
			TWCR0 |= (1 << TWINT) | (1 << TWSTO); // Limpia bandera y libera bus para evitar bloqueos
			break;
	}
}

// Configura los puertos de entrada/salida
void initPorts(void)
{
	// PC0-PC3 como salidas (para mostrar el valor binario del contador)
	DDRC |= (1 << DDC0) | (1 << DDC1) | (1 << DDC2) | (1 << DDC3);
	PORTC = 0; // Inicializa salidas en 0

	// Configura PD4, PD5, PD6 como salidas (pueden ser LEDs u otras señales)
	DDRD |= (1 << DDD4) | (1 << DDD5) | (1 << DDD6);
	PORTD = 0;

	// Configura PB5 como salida (no se usa directamente aquí)
	DDRB |= (1 << DDB5);
}

// Configura interrupciones externas para manejar los botones
void setup(void)
{
	cli(); // Desactiva interrupciones globales mientras se configura

	// PD2 y PD3 como entradas con resistencia pull-up (botones)
	DDRD &= ~(1 << PIND2) & ~(1 << PIND3); // Entradas
	PORTD |= (1 << PIND2) | (1 << PIND3);  // Pull-up activadas

	// Habilita interrupciones por cambio de pin en PD2 y PD3 (PCINT18 y PCINT19)
	PCICR |= (1 << PCIE2); // Interrupciones en el puerto D
	PCMSK2 |= (1 << PCINT18) | (1 << PCINT19); // Solo PD2 y PD3
	sei(); // Habilita interrupciones globales
}

// Interrupción por cambio en PD2 o PD3 (botones)
ISR(PCINT2_vect)
{
	// Si se presiona PD2 (botón de incremento)
	if (!(PIND & (1 << PIND2)))
	{
		contador4bits++; // Incrementa el contador
		contador4bits &= 0x0F; // Asegura que no pase de 4 bits (0-15)
		_delay_ms(250); // Antirrebote simple

		// Actualiza los pines de salida PC0-PC3
		PORTC = (PORTC & 0xF0) | (contador4bits & 0x0F);
	}
	// Si se presiona PD3 (botón de decremento)
	else if (!(PIND & (1 << PIND3)))
	{
		// Manejo de underflow: si está en 0, pasa a 15
		if (contador4bits == 0)
		{
			contador4bits = 15;
		}
		else
		{
			contador4bits--;
		}
		_delay_ms(250); // Antirrebote

		// Actualiza los pines de salida PC0-PC3
		PORTC = (PORTC & 0xF0) | (contador4bits & 0x0F);
	}
}
