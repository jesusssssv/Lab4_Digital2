/*
 * I2C.c
 *
 * Created: 1/08/2025 07:12:48
 * Author : valen
 */

#include "I2C.h"  // Inclusión del archivo de cabecera con las declaraciones de funciones y definiciones

//***************************************************************
// Función para inicializar I2C en modo Maestro
//***************************************************************
void I2C_Master_Init(unsigned long SCL_Clock, uint8_t Prescaler) {

    DDRC &= ~((1 << DDC4) | (1 << DDC5));  // Configura los pines PC4 (SDA) y PC5 (SCL) como entradas (modo I2C)

    // Configura el prescaler para el bit rate
    switch (Prescaler) {
        case 1:
            TWSR0 &= ~((1 << TWPS1) | (1 << TWPS0)); // Prescaler = 1
            break;
        case 4:
            TWSR0 &= ~(1 << TWPS1);  // Prescaler = 4
            TWSR0 |= (1 << TWPS0);
            break;
        case 16:
            TWSR0 &= ~(1 << TWPS0);  // Prescaler = 16
            TWSR0 |= (1 << TWPS1);
            break;
        case 64:
            TWSR0 |= (1 << TWPS1) | (1 << TWPS0);  // Prescaler = 64
            break;
		default:
			TWSR0 &= ~((1 << TWPS1) | (1 << TWPS0)); // Si se pasa otro valor, usa 1 como predeterminado
			Prescaler = 1;
			break;
    }

    // Configura el Bit Rate Register (TWBR0) según la frecuencia de CPU y prescaler
    // TWBR0 debe ser mayor a 10 para funcionamiento estable
	TWBR0 = (((16000000)/SCL_Clock) - 16) / (2 * Prescaler); 

	TWCR0 |= (1 << TWEN);  // Habilita la interfaz TWI (I2C)
}

//************************************************************************
// Función que inicia la comunicación I2C (Start condition)
//************************************************************************
void I2C_Master_Start(void){
    TWCR0 = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN); // Genera condición START y habilita TWI
    while (!(TWCR0 & (1 << TWINT))); // Espera hasta que la condición START se haya transmitido
}

//************************************************************************
// Función que detiene la comunicación I2C (Stop condition)
//************************************************************************
void I2C_Master_Stop(void){
    TWCR0 = (1 << TWEN) | (1 << TWINT) | (1 << TWSTO); // Genera condición STOP
}

//************************************************************************
// Función para transmitir un dato del maestro al esclavo
// Retorna 1 si hay éxito, o el código de error (estado) si no
//************************************************************************
uint8_t I2C_Master_Write(uint8_t dato){
    uint8_t estado;

    TWDR0 = dato;  // Carga el dato en el registro de datos TWI
    TWCR0 = (1 << TWEN) | (1 << TWINT); // Inicia la transmisión del dato

    while (!(TWCR0 & (1 << TWINT))); // Espera a que se complete la transmisión

    estado = TWSR0 & 0xF8; // Extrae el código de estado del TWSR

    // Verifica si el dato fue transmitido correctamente y se recibió ACK
    if (estado == 0x18 || estado == 0x28 || estado == 0x40) {
        return 1; // Éxito
    } else {
        return estado; // Error, devuelve código de estado
    }
}

//************************************************************************
// Función para recibir un dato desde el esclavo
// Si ack = 1, envía ACK tras la lectura, si es 0, no envía ACK
// Retorna 1 si éxito, o código de estado si hay error
//************************************************************************
uint8_t I2C_Master_Read(uint8_t *buffer, uint8_t ack){
    uint8_t estado;
    
    if (ack) {
        TWCR0 |= (1 << TWEA);  // Habilita envío de ACK tras recibir dato
    } else {
        TWCR0 &= ~(1 << TWEA); // Deshabilita ACK (se envía NACK)
    }

    TWCR0 |= (1 << TWINT); // Inicia recepción del dato
    while (!(TWCR0 & (1 << TWINT))); // Espera a que se reciba el dato

    estado = TWSR0 & 0xF8; // Extrae código de estado

    // Si el estado indica que se recibió el dato correctamente (con o sin ACK)
    if (estado == 0x58 || estado == 0x50) {
        *buffer = TWDR0;  // Guarda el dato recibido en el puntero proporcionado
        return 1;         // Éxito
    } else {
        return estado;    // Error
    }
}

//*****************************************************************************
// Función para inicializar I2C en modo Esclavo con una dirección específica
//*****************************************************************************
void I2C_Slave_Init(uint8_t address) {
    DDRC &= ~((1 << DDC4) | (1 << DDC5));  // Configura los pines SDA y SCL como entradas

    TWAR0 = address << 1;  // Asigna la dirección del esclavo (7 bits alineados a la izquierda)
    // TWAR = (address << 1 | 0x01);  // Alternativa: permite dirección general

    // Habilita: interfaz TWI, reconocimiento automático de direcciones (ACK), interrupción de TWI
    TWCR0 = (1 << TWEA) | (1 << TWEN) | (1 << TWIE);
}
