/*
 * Esclavo 2.c
 *
 * Created: 1/08/2025 11:17:06
 * Author : valen
 */ 

#define F_CPU 16000000 // Frecuencia del sistema: 16 MHz

#include <avr/io.h>         // Librer�a base para registros del AVR
#include <avr/interrupt.h>  // Librer�a para manejo de interrupciones
#include <util/delay.h>     // Librer�a para retardos

#include "ADC.h"            // Librer�a personalizada para manejar el ADC
#include "I2C.h"            // Librer�a personalizada para manejar el I2C

// Direcci�n I2C del esclavo
#define SlaveAddress 0x40

// Variables globales
uint8_t buffer = 0;         // Almacena el dato recibido por I2C (comando del maestro)
uint16_t valueADC = 0;      // Valor de 10 bits del ADC (pero se usar� solo 8 bits al enviar)

//******************************************************************

int main(void)
{
	ADC_init();                  // Inicializa el m�dulo ADC
	//UART_init();              // UART comentado (no se usa en este programa)
	I2C_Slave_Init(SlaveAddress); // Inicializa esclavo I2C con direcci�n 0x40
	
	sei(); // Habilita interrupciones globales

	while (1) 
	{
		// Lee el valor del canal ADC 6, y lo reduce de 10 bits a 8 bits (divisi�n por 4)
		valueADC = ADC_read(6) >> 2;

		// El buffer act�a como bandera para saber si el maestro pidi� el dato ('L')
		if (buffer == 'L')
		{
			buffer = 0; // Se limpia el buffer despu�s de atender la petici�n
		}

		// Se puede agregar un _delay_ms si se desea limitar la frecuencia de lectura
	}
}

// Rutina de interrupci�n del perif�rico I2C (TWI)
ISR(TWI_vect)
{
	uint8_t estado;
	estado = TWSR & 0xFC; // M�scara para obtener c�digo de estado TWI (se ignoran bits de control)

	switch (estado)
	{
		// El maestro inici� comunicaci�n con esclavo (SLA+W)
		case 0x60: // Direcci�n propia + escritura
		case 0x70: // Direcci�n general + escritura
			TWCR |= (1 << TWINT); // Limpia bandera para seguir escuchando
			break;

		// El maestro envi� un dato (comando)
		case 0x80: // Direcci�n propia
		case 0x90: // Direcci�n general
			buffer = TWDR;       // Guarda el dato recibido (espera 'L')
			TWCR |= (1 << TWINT); // Limpia bandera
			break;

		// El maestro solicita datos (SLA+R)
		case 0xA8: // Direcci�n propia + lectura
		case 0xB8: // Maestro ya recibi� un byte y quiere otro
			TWDR = valueADC;      // Se carga el dato del ADC en el registro de transmisi�n
			TWCR = (1 << TWEN) | (1 << TWIE) | (1 << TWINT) | (1 << TWEA); // Se prepara para enviar y seguir escuchando
			break;

		// Cualquier otro estado inesperado
		default:
			TWCR |= (1 << TWINT) | (1 << TWSTO); // Limpia bandera y genera condici�n de parada para liberar bus
			break;
	}
}
