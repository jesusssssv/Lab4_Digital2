/*
 * I2C.c
 *
 * Created: 1/08/2025 07:12:48
 * Author : valen
 */

#include "I2C.h"  // Inclusi�n del archivo de cabecera con las declaraciones de funciones y definiciones

//***************************************************************
// Funci�n para inicializar I2C en modo Maestro
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

    // Configura el Bit Rate Register (TWBR0) seg�n la frecuencia de CPU y prescaler
    // TWBR0 debe ser mayor a 10 para funcionamiento estable
	TWBR0 = (((16000000)/SCL_Clock) - 16) / (2 * Prescaler); 

	TWCR0 |= (1 << TWEN);  // Habilita la interfaz TWI (I2C)
}

//************************************************************************
// Funci�n que inicia la comunicaci�n I2C (Start condition)
//************************************************************************
void I2C_Master_Start(void){
    TWCR0 = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN); // Genera condici�n START y habilita TWI
    while (!(TWCR0 & (1 << TWINT))); // Espera hasta que la condici�n START se haya transmitido
}

//************************************************************************
// Funci�n que detiene la comunicaci�n I2C (Stop condition)
//************************************************************************
void I2C_Master_Stop(void){
    TWCR0 = (1 << TWEN) | (1 << TWINT) | (1 << TWSTO); // Genera condici�n STOP
}

//************************************************************************
// Funci�n para transmitir un dato del maestro al esclavo
// Retorna 1 si hay �xito, o el c�digo de error (estado) si no
//************************************************************************
uint8_t I2C_Master_Write(uint8_t dato){
    uint8_t estado;

    TWDR0 = dato;  // Carga el dato en el registro de datos TWI
    TWCR0 = (1 << TWEN) | (1 << TWINT); // Inicia la transmisi�n del dato

    while (!(TWCR0 & (1 << TWINT))); // Espera a que se complete la transmisi�n

    estado = TWSR0 & 0xF8; // Extrae el c�digo de estado del TWSR

    // Verifica si el dato fue transmitido correctamente y se recibi� ACK
    if (estado == 0x18 || estado == 0x28 || estado == 0x40) {
        return 1; // �xito
    } else {
        return estado; // Error, devuelve c�digo de estado
    }
}

//************************************************************************
// Funci�n para recibir un dato desde el esclavo
// Si ack = 1, env�a ACK tras la lectura, si es 0, no env�a ACK
// Retorna 1 si �xito, o c�digo de estado si hay error
//************************************************************************
uint8_t I2C_Master_Read(uint8_t *buffer, uint8_t ack){
    uint8_t estado;
    
    if (ack) {
        TWCR0 |= (1 << TWEA);  // Habilita env�o de ACK tras recibir dato
    } else {
        TWCR0 &= ~(1 << TWEA); // Deshabilita ACK (se env�a NACK)
    }

    TWCR0 |= (1 << TWINT); // Inicia recepci�n del dato
    while (!(TWCR0 & (1 << TWINT))); // Espera a que se reciba el dato

    estado = TWSR0 & 0xF8; // Extrae c�digo de estado

    // Si el estado indica que se recibi� el dato correctamente (con o sin ACK)
    if (estado == 0x58 || estado == 0x50) {
        *buffer = TWDR0;  // Guarda el dato recibido en el puntero proporcionado
        return 1;         // �xito
    } else {
        return estado;    // Error
    }
}

//*****************************************************************************
// Funci�n para inicializar I2C en modo Esclavo con una direcci�n espec�fica
//*****************************************************************************
void I2C_Slave_Init(uint8_t address) {
    DDRC &= ~((1 << DDC4) | (1 << DDC5));  // Configura los pines SDA y SCL como entradas

    TWAR0 = address << 1;  // Asigna la direcci�n del esclavo (7 bits alineados a la izquierda)
    // TWAR = (address << 1 | 0x01);  // Alternativa: permite direcci�n general

    // Habilita: interfaz TWI, reconocimiento autom�tico de direcciones (ACK), interrupci�n de TWI
    TWCR0 = (1 << TWEA) | (1 << TWEN) | (1 << TWIE);
}
