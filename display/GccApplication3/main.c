#define F_CPU 8000000UL // configura a frequencia da CPU
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

// Define os pinos para o registrador de deslocamento
#define DATA_PIN PB0 
#define LATCH_PIN PB1
#define CLOCK_PIN PB2

// Define os pinos dos displays de sete segmentos
#define DISPLAY1 0
#define DISPLAY2 1
#define DISPLAY3 2

// Variáveis globais
volatile uint16_t decimos = 0;
volatile uint8_t contagem_ativa = 0;
volatile uint8_t transmit_buffer[6] = {'M', 'a', 'x', 'i', 'm', 'o'};
volatile uint8_t transmit_index = 6;

// Função para enviar dados para os registradores de deslocamento
void shiftOut(uint8_t val) {
	uint8_t i;

	for (i = 0; i < 8; i++) {
		if (val & (1 << i))
		PORTB |= (1 << DATA_PIN);
		else
		PORTB &= ~(1 << DATA_PIN);
		
		_delay_us(100);
		PORTB |= (1 << CLOCK_PIN);
		_delay_us(100);
		PORTB &= ~(1 << CLOCK_PIN);
		_delay_us(100);
	}
	PORTB &= ~(1 << DATA_PIN);
}
void displayNumber(uint8_t number) {
	// Tabela de segmentos para exibir os dígitos de 0 a 9
	const uint8_t segments[] = { // valores investidos de 7 segmentos anodo comum, pois o registrador de deslocamento manda do mais significativo pro menos
		0b00000011, // 0
		0b10011111, // 1
		0b00100101, // 2
		0b00001101, // 3
		0b10011001, // 4
		0b01001001, // 5
		0b01000001, // 6
		0b00011111, // 7
		0b00000001, // 8
		0b00001001  // 9
	};
	
	shiftOut(segments[number]);
	
	_delay_us(100);
	PORTB |= (1 << LATCH_PIN);
	_delay_us(100);
	PORTB &= ~(1 << LATCH_PIN);
	_delay_us(100);
}

void displayTime(uint16_t segundos) {
	uint8_t dezenas = segundos / 100;
	uint8_t unidade = (segundos / 10) % 10;
	uint8_t decimos = segundos % 10;
	
	displayNumber(dezenas);
	displayNumber(unidade);
	displayNumber(decimos);
}

// Função para configurar a USART
void Usart(uint16_t baud_rate) {
	uint16_t ubrr_value = F_CPU / 16 / baud_rate - 1; // calculo do valor para o registrador UBRR
	UBRR0H = (uint8_t)(ubrr_value >> 8); // valor mais significativo do UBRR 
	UBRR0L = (uint8_t)ubrr_value; // valor menos significativo do UBRR
	UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0) | (1 << UDRIE0); // habilita a recpeção, transmissão, a interrupção de recepção e transmissão da USART
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); // coloca para receber ou enviar ate 8 bits
}

// Função para transmitir dados pela USART
void USART_Transmit(uint8_t data) {
	while (!(UCSR0A & (1 << UDRE0))) // verifica sem tem dados para transmitir
	;
	UDR0 = data;
}

// Interrupção para o temporizador TC1
ISR(TIMER1_COMPA_vect) {
	if (contagem_ativa) {
		displayTime(decimos);
		decimos++;
	}
	

	if (decimos >= 600) {
			contagem_ativa = 0;
			transmit_index = 0;
			UCSR0B |= (1 << UDRIE0);	
			TIMSK1 &= ~(1 << OCIE1A);
	}
}

// Interrupção para a recpecao na Usart
ISR(USART_RX_vect) {
	uint8_t comando = UDR0;

	if (comando == 'I' || comando == 'i') // recebe I para iniciar a contagem
	contagem_ativa = 1;
	else if (comando == 'P' || comando == 'p') // recebe P para parar a contagem
	contagem_ativa = 0;

	else if (comando == 'C' || comando == 'c') { // recebe C para continuar a contagem apenas quando esta parada
		if (!contagem_ativa) {
			decimos = 0;
			contagem_ativa = 1;
		}
	}
}

// Interrupção para a transmissão de dados (USART UDRE)
ISR(USART_UDRE_vect) {
	if (transmit_index < 6) {
		UDR0 = transmit_buffer[transmit_index];
		transmit_index++;
		} else {
		UCSR0B &= ~(1 << UDRIE0); // Desabilita a interrupção de transmissão da USART 
	}
}

int main(void) {
	// Configuração dos registradores de deslocamento
	DDRB |= (1 << DATA_PIN) | (1 << LATCH_PIN) | (1 << CLOCK_PIN) 
	displayTime(0); // configura para quando iniciar o display inciar em 00,0

	// Configuração da USART
	Usart(9600);

	// Configuração do temporizador (TIMER1)
	TCCR1B |= (1 << WGM12) | (1 << CS12) | (1 << CS10); // Modo CTC, prescaler de 1024
	OCR1A = 780; //
	TIMSK1 |= (1 << OCIE1A); // Habilita a interrupção por comparação A
	sei();
	// Loop principal
	while (1) {
		
	}

	return 0;
}
