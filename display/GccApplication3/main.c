#define F_CPU 8000000UL // Configura a frequencia da CPU
#include <avr/io.h> // Importa as bibliotecas utilizadas para o funcionamento, como a do avr e a de delay
#include <avr/interrupt.h>
#include <util/delay.h>

// Define os pinos para o registrador de deslocamento
#define DATA_PIN PB0 
#define LATCH_PIN PB1
#define CLOCK_PIN PB2

// Variaveis globais
uint16_t decimos = 0; // Variavel para contar o tempo de 100ms
uint8_t contagem_ativa = 0; 
char carac_maximo[6] = {"Maximo"}; // String que vai ser enviado quando o o display contar 60 seg
uint8_t num_char; // Variavel que incrementa a string

// shiftOut(uint8_t valor);
//display_Num(uint8_t numero);
//Tempo_Display(uint16_t tempo);
//Usart(uint16_t baud_rate);
//ISR(TIMER1_COMPA_vect);
//ISR(USART_RX_vect);
//ISR(USART_UDRE_vect);



// Fun��o para enviar dados para os registradores de deslocamento
void shiftOut(uint8_t valor) {
	uint8_t i;

	for (i = 0; i < 8; i++) {
		if (valor & (1 << i))
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

void display_Num(uint8_t numero) {
	 // Tabela para exibir os d�gitos de 0 a 9
	const uint8_t Tabela[] = { // Valores investidos do display de  7 segmentos anodo comum, pois o registrador de deslocamento envia do mais significativo pro menos
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
	
	shiftOut(Tabela[numero]); // Carrega os numeros para fun��o do registrador de deslocamento para poder enviar-los
	
	_delay_us(100);
	PORTB |= (1 << LATCH_PIN);
	_delay_us(100);
	PORTB &= ~(1 << LATCH_PIN);
	_delay_us(100);
}

void Tempo_Display(uint16_t tempo) {// Divide em cada tempo e salva com seus respectivos digitos
	uint8_t decimos = tempo / 100;
	uint8_t unidade = (tempo / 10) % 10;
	uint8_t dezenas = tempo % 10;
	
	display_Num(decimos);
	display_Num(unidade);
	display_Num(dezenas);
}

// Fun��o para configurar a USART
void Usart(uint16_t baud_rate) {
	uint16_t valor_ubbr = F_CPU / 16 / baud_rate - 1;	// Calculo do valor para o registrador UBRR
	UBRR0H = (uint8_t)(valor_ubbr >> 8); // Valor mais significativo do UBRR 
	UBRR0L = (uint8_t)valor_ubbr; // Valor menos significativo do UBRR
	UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0);	// Habilita a recep��o, transmiss�o, a interrup��o de recep��o e transmiss�o da USART
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00) | (1 << USBS0); // Coloca para receber ou enviar ate 8 bits e 1 bit de parada
}

// Interrup��o para o temporizador TC1
ISR(TIMER1_COMPA_vect) {
		Tempo_Display(decimos); 
		decimos++;	// Incrementa o decimos
		if (decimos >= 600) {	// Se o decimos chegar a 600ms(60s) 
			contagem_ativa = 0;	// a contagem_ativa faz com que o comando C reconhe�a que a contagem parou
			num_char = 0; 
			UCSR0B |= (1 << UDRIE0); // Habilita a interrupcao de transmissao
			TIMSK1 &= ~(1 << OCIE1A );	// Desabilita a interrup��o do contador
		}
	
}

// Interrup��o para a recpecao na Usart
ISR(USART_RX_vect) {
	uint8_t comando = UDR0;	// Verifica se tem dados recebidos no reg UDR0 
	if (comando == 'I' || comando == 'i'){	// Recebe I ou i para iniciar a contagem
	TIMSK1 |= (1 << OCIE1A); 
	contagem_ativa = 1;
	}
	else if (comando == 'P' || comando == 'p'){	// Recebe P ou p para parar a contagem
	TIMSK1 &= ~(1 << OCIE1A); 
	contagem_ativa = 0;
	}
	else if (comando == 'C' || comando == 'c') { // Recebe C ou c para resetar a contagem apenas quando estiver parada
		if (!contagem_ativa) {
			TIMSK1 |= (1 << OCIE1A); 
			decimos = 0;
		}
	}
}

// Interrup��o para a transmiss�o de dados
ISR(USART_UDRE_vect) {
	if (num_char <= 6) {
		UDR0 = carac_maximo[num_char];
		num_char++;
		} else {
		UCSR0B &= ~(1 << UDRIE0);// Desabilita a interrup��o de transmiss�o da USART 
	}
}

int main(void) {
	// Configura��o dos registradores de deslocamento
	DDRB |= (1 << DATA_PIN) | (1 << LATCH_PIN) | (1 << CLOCK_PIN) ;
	Tempo_Display(0);// Configura para quando iniciar o display inciar em 00,0

	Usart(9600); //Configura��o da USART para 9600 bps

	// Configura��o do temporizador TC1
	TCCR1B |= (1 << WGM12) | (1 << CS12) | (1 << CS10);// Modo CTC, prescaler de 1024
	OCR1A = 780; // Configura o valor do reg OCR1A para que cada pulso ocarra em 100 ms
	sei();	// Habilita todas as interrup��es

	while (1) ;	// Loop principal
	return 0;
}
