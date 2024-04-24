/******************************************************************************
; Universidad del Valle de Guatemala
; 1E2023: Programacion de Microcontroladores
; main.c
; Autor: Jacob Tabush
; Proyecto: Poslaboratorio 6
; Hardware: ATMEGA328P
; Creado: 24/04/2024
; Ultima modificacion: 24/04/2024
*******************************************************************************/

#define F_CPU 16000000
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdbool.h>

void initADC(void);
void initUART9600(void);
void writeUART(char senddata);
void writeString(char* senddata);
char changesomebits(char oldvalue, char bitstochange, char newvalue);

uint16_t ADCResult = 0;
uint8_t receivedata = 0;
uint8_t mode = 0; // 0 deplegar menu, 1 esperando opcion menu, 2 enviar pot, 3 desplegar esperar ascii, 4 esperando ascii para mandar
char menu[] = "\n MENU\n 1. Leer potenciometro \n 2. Enviar ASCII \n";
char asciitime[] = "Envia el caracter ASCII que quieras desplegar:";
char valorpot[]= "Valor ADC:";

void setup(void)
{
	cli();
	
	initADC();
	
	DDRD = 0b11100000; // Ponemos a D5-7 como salidas
	DDRB = 0b00011111; // Ponemos a B0-4 como salidas
	
	initUART9600();
	
	sei();
}

int main(void)
{
	setup();
	PORTD = 0x80;
	
	while (1)
	{
		ADCSRA |= (1<<ADSC); //iniciamos el ADC
		
		// mostramos el menu
		if (mode == 0) {writeString(menu);
		mode++;}
		
		//desplegamos valor del pot
		else if (mode == 2) {
		writeUART(10); //newline
		writeString(valorpot); 
		writeUART(ADCResult/100 + 0x30); // desplegar cienes
		writeUART((ADCResult % 100)/10 + 0x30); // desplegar decenas
		writeUART((ADCResult % 100) % 10 + 0x30); //desplegar unidades
		writeUART(10); //newline
		mode = 0;
		}
		
		//Esperamos al car a desplegar
		else if (mode == 3) {
			writeUART(10);
			writeString(asciitime);
			mode++;
		}
	}
}

void initADC(void) //Funcion para inicializar el ADC
{
	ADMUX = 0;
	
	ADMUX |= (1<<REFS0); //conectamos a AVcc
	ADMUX &= ~(1<<REFS1);
	
	ADMUX |= 0b00000110; //Seleccionamos A6
	
	ADMUX |= (1<<ADLAR); // Justificado a la izquierda
	
	ADCSRA |= (1<<ADEN); //Encendemos el ADC
	ADCSRA |= (1<<ADIE); // Encendemos el interrupt
	ADCSRA |= (0b00000111); //Prescaler de 128
}

void initUART9600(void) {
	DDRD = changesomebits(DDRD, 0x03, (1<<DDD1)); // arreglamos d0 y d1
	
	UCSR0A = 0;
	UCSR0A |= (1<<U2X0); // double speed
	
	UCSR0B = 0;
	UCSR0B |= (1<<RXCIE0)|(1<<RXEN0)|(1<<TXEN0); //RX con interrupt y TX encendido
	
	UCSR0C = 0;
	UCSR0C |= (1<< UCSZ01)|(1<<UCSZ00); //8 bits sin paridad con 1 bit de stop
	
	UBRR0 = 207; //Baud rate de 9600 para prescaler de 16MHz
	
}

ISR(ADC_vect){ // recibimos el valor del ADC
	ADCResult = ADCH;
	
	return;
}

void writeUART(char senddata){ // Funcion para mandar un caracter por UART
	while(!(UCSR0A & (1<<UDRE0) )); // Revisamos si el buffer esta lleno
	UDR0 = senddata; //Colocamos el valor a mandar
}

void writeString(char* senddata){ //Funcion para mandar varios datos
	for (uint8_t i = 0; senddata[i] != '\0'; i++) { //Seguimos mandando hasta que nos topamos con un caracter vacio
		writeUART(senddata[i]);
	}
}

ISR(USART_RX_vect) {
	receivedata = UDR0;
	
	while(!(UCSR0A & (1<<UDRE0)));
	
	UDR0 = receivedata;
	
	if (mode == 1) {// si estamos en modo desplegando menu
		if (receivedata == 0x31) {mode = 2;} //pasamos al modo desplegar pot
		if (receivedata == 0x32) {mode = 3;} //pasamos al modo esperar caracter
			}
	else if (mode == 4) { //recibiendo un valor ASCII
		PORTD = receivedata<<5;
		PORTB = (receivedata & 0b11111000)>>3;
		mode = 0; // desplegamos el menu de nuevo
	}
	
}

char changesomebits(char oldvalue, char bitstochange, char newvalue) //Funcion para solo cambiar algunos bits en un registro
{	char result = (newvalue & bitstochange) | (oldvalue & ~bitstochange);
	return result;
}