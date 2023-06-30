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
uint8_t num_char = 0; // Variavel que incrementa a string // = 0

// Função para enviar dados para os registradores de deslocamento
void shiftOut(uint8_t valor) {
	uint8_t i;

	for (i = 0; i < 8; i++) {
		if (valor & (1 << i))
		PORTB |= (1 << DATA_PIN);
		else                   // Manda um bit pra uma saida do registrador de deslocamento
		PORTB &= ~(1 << DATA_PIN);
		
		_delay_us(100);
		PORTB |= (1 << CLOCK_PIN);
		_delay_us(100);          // Muda para outra saida do registrador de deslocamento para outra
		PORTB &= ~(1 << CLOCK_PIN);
		_delay_us(100);
	}
	PORTB &= ~(1 << DATA_PIN);
}

void display_Num(uint8_t numero) {
	// Tabela para exibir os dígitos de 0 a 9
	const uint8_t Tabela[] = { // Valores investidos do display de  7 segmentos anodo comum, pois o registrador de deslocamento envia do mais significativo pro menos
		0b00000010, // 0
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
	
	shiftOut(Tabela[numero]); // Carrega os numeros para função do registrador de deslocamento para poder enviar-los
	
	_delay_us(100);
	PORTB |= (1 << LATCH_PIN);
	_delay_us(100);       // Escreve de fato nos display de 7 segmentos
	PORTB &= ~(1 << LATCH_PIN);
	_delay_us(100);
}

void Tempo_Display(uint16_t tempo) {// Divide em cada tempo e salva com seus respectivos digitos
	uint8_t dezenas = tempo / 100;    // 0,01    
	uint8_t unidade = (tempo / 10) % 10; 
	uint8_t decimos = tempo % 10;
	
	display_Num(dezenas);
	display_Num(unidade);
	display_Num(decimos);
}

// Função para configurar a USART
void Usart(uint16_t baud_rate) {
	uint16_t valor_ubbr = F_CPU / 16 / baud_rate - 1;	// Calculo do valor para o registrador UBRR
	UBRR0H = (uint8_t)(valor_ubbr >> 8); // Valor mais significativo do UBRR
	UBRR0L = (uint8_t)valor_ubbr; // Valor menos significativo do UBRR
	UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0);	// Habilita a recepção, transmissão, a interrupção de recepção e transmissão da USART
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00) | (1 << USBS0); // Coloca para receber ou enviar ate 8 bits e 1 bit de parada
}

// Interrupção para o temporizador TC1
ISR(TIMER1_COMPA_vect) {
	Tempo_Display(decimos);
	decimos++;	// Incrementa o decimos
	if (decimos >= 600) {	// Se o decimos chegar a 600ms(60s)
		contagem_ativa = 0;	// a contagem_ativa faz com que o comando C reconheça que a contagem parou
		//num_char = 0;  //
		UCSR0B |= (1 << UDRIE0); // Habilita a interrupção por registrador de dados vazio (bit UDRE0) > UDRE0 ativo, Um lógico neste bit indica que o transmissor está
		//pronto para receber um novo caractere para transmissão
		TIMSK1 &= ~(1 << OCIE1A );	// Desabilita a interrupção do contador
	}
	
}

// Interrupção para a recepção da Usart
ISR(USART_RX_vect) {
	uint8_t comando = UDR0;	// Verifica se tem dados recebidos no reg UDR0
	if (comando == 'I' || comando == 'i'){	// Recebe I ou i para iniciar a contagem
		TIMSK1 |= (1 << OCIE1A);   // Ativa a interrupção do TC1 usando o registrador OCR1A como TOP.
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
			num_char = 0;
		}
	}
}

// Interrupção acionada quando o UDR0 está vazio
ISR(USART_UDRE_vect) {
	if (num_char <= 6) {
		UDR0 = carac_maximo[num_char];
		num_char++;
		} else {
		UCSR0B &= ~(1 << UDRIE0); // Desabilita a interrupção por registrador de dados vazio
		num_char = 0;  //
	}
}

int main(void) {
	// Configuração dos registradores de deslocamento
	DDRB |= (1 << DATA_PIN) | (1 << LATCH_PIN) | (1 << CLOCK_PIN) ;
	Tempo_Display(0);// Configura para quando iniciar o display inciar em 00,0

	Usart(9600); //Configuração da USART para 9600 bps

	// Configuração do temporizador TC1
	TCCR1B |= (1 << WGM12) | (1 << CS12) | (1 << CS10);// Modo CTC, prescaler de 1024
	OCR1A = 780; // Configura o valor do reg OCR1A para que cada pulso ocarra em 100 ms
	sei();	// Habilita todas as interrupções

	while (1) ;	// Loop principal
	return 0;
}
