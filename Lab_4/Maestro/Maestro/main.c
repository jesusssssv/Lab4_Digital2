/*
 * Maestro.c
 *
 * Created: 1/08/2025 07:12:48
 * Author : valen
 */

#define F_CPU 16000000 // Frecuencia del CPU, necesaria para las funciones de _delay
#include <avr/io.h>    // Librer�a principal de registros del microcontrolador AVR
#include <util/delay.h> // Librer�a para funciones de retardo
#include "LCD_8bits.h"  // Librer�a para el manejo de LCD en modo 8 bits
#include "I2C.h"        // Librer�a personalizada para protocolo I2C

// Direcciones de esclavos I2C
#define slave_1 0x30 // Direcci�n del esclavo 1 (Contador)
#define slave_2 0x40 // Direcci�n del esclavo 2 (ADC)

// Variables
uint8_t direccion;
uint8_t temp;
uint8_t bufferI2C;
uint8_t bufferI2C_2;
uint8_t valorI2C = 0;    // Valor recibido del esclavo 1 (contador)
uint8_t valorI2C_2 = 0;  // Valor recibido del esclavo 2 (ADC)

int main(void)
{
	// Inicializaci�n de LCD e I2C
	initLCD8(); // Inicializa el LCD en modo 8 bits
	I2C_Master_Init(100000, 1); // Inicializa el I2C a 100kHz, como maestro

	// Mensaje de bienvenida en la pantalla
	LCD8_Clear();
	LCD8_Set_Cursor(0, 0);
	LCD8_Write_String("Sistema I2C");
	LCD8_Set_Cursor(0, 1);
	LCD8_Write_String("Iniciando...");
	_delay_ms(2000); // Espera 2 segundos

	while (1)
	{
		// ========== ACTUALIZACI�N DEL DISPLAY ==========
		LCD8_Clear(); // Limpia la pantalla LCD

		// Mostrar valor del contador (esclavo 1)
		LCD8_Set_Cursor(0, 0);
		LCD8_Write_String("Contador: ");
		LCD8_Set_Cursor(4, 1);
		LCD8_Variable_U(valorI2C); // Muestra valor num�rico sin signo

		// Mostrar valor del ADC (esclavo 2)
		LCD8_Set_Cursor(11, 0);
		LCD8_Write_String("ADC: ");
		LCD8_Set_Cursor(12, 1);
		LCD8_Variable_U(valorI2C_2); // Muestra valor num�rico sin signo

		_delay_ms(600); // Espera para que se actualice el display correctamente

		// ========== COMUNICACI�N I2C - CONTADOR ==========
		I2C_Master_Start(); // Inicio de comunicaci�n I2C
		// Direcci�n de esclavo 1 + escritura (bit 0 = 0)
		bufferI2C = slave_1 << 1 & 0b11111110;
		temp = I2C_Master_Write(bufferI2C); // Enviar direcci�n del esclavo
		if(temp != 1){
			I2C_Master_Stop(); // Si no responde, se detiene
		} else {
			I2C_Master_Write('R'); // Se env�a comando 'R' para pedir dato
			I2C_Master_Stop(); // Se finaliza la comunicaci�n
		}

		_delay_ms(10); // Espera breve

		I2C_Master_Start(); // Nueva comunicaci�n para lectura
		bufferI2C = slave_1 << 1 | 0b00000001; // Direcci�n + lectura (bit 0 = 1)
		temp = I2C_Master_Write(bufferI2C);
		if (temp != 1){
			I2C_Master_Stop(); // Si no responde, se detiene
		} else{
			TWCR0 = (1<<TWINT)|(1<<TWEN); // Habilitar lectura sin ACK
			while (!(TWCR0 & (1<<TWINT))); // Espera hasta que se reciba el dato
			valorI2C = TWDR0; // Guarda el dato recibido
			I2C_Master_Stop(); // Finaliza la comunicaci�n
		}
		_delay_ms(10); // Espera breve

		// ========== COMUNICACI�N I2C - ADC ==========
		I2C_Master_Start(); // Inicia comunicaci�n
		bufferI2C_2 = slave_2 << 1 & 0b11111110; // Direcci�n de esclavo 2 + escritura
		temp = I2C_Master_Write(bufferI2C_2);
		if(temp != 1){
			I2C_Master_Stop(); // Detiene si hay error
		} else {
			I2C_Master_Write('L'); // Se env�a comando 'L' para pedir lectura de ADC
			I2C_Master_Stop();
		}

		_delay_ms(10); // Espera breve

		I2C_Master_Start(); // Nueva comunicaci�n para leer
		bufferI2C_2 = slave_2 << 1 | 0b00000001; // Direcci�n + lectura
		temp = I2C_Master_Write(bufferI2C_2);
		if (temp != 1){
			I2C_Master_Stop();
		} else{
			TWCR0 = (1<<TWINT)|(1<<TWEN); // Preparar lectura
			while (!(TWCR0 & (1<<TWINT))); // Espera dato
			valorI2C_2 = TWDR0; // Guardar valor le�do del ADC
			I2C_Master_Stop();
		}
		_delay_ms(10); // Espera entre lecturas

		// Espera antes de iniciar siguiente ciclo
		_delay_ms(100);
	}
}


