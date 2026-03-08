#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>

uint8_t segments[] = 
{
    0b00111111, 
    0b00000110, 
    0b01011011, 
    0b01001111,
    0b01100110, 
    0b01101101, 
    0b01111101, 
    0b00000111,
    0b01111111, 
    0b01101111
};

volatile uint16_t mainbuf[4];
volatile uint8_t addbuff[15];
volatile uint8_t flag1 = 0;
volatile uint8_t flag2 = 0;
volatile uint8_t cnt = 0;
volatile uint16_t ra = 0;
volatile char c = 0;

ISR(USART_RX_vect) 
{
    c = UDR0;
    UDR0 = c; 
}

ISR(USART_TX_vect) 
{
    if(c != 13) 
        flag1 = 0x01;
    else 
        flag2 = 0x01;
}

ISR(ADC_vect) 
{
    ra = ADC;
}

ISR(TIMER1_COMPB_vect) 
{
    ADCSRA |= (1 << ADSC); 
}

void InitPorts(void) 
{
    DDRB |= (1 << PB0 | 1 << PB1 | 1 << PB3 | 1 << PB5);
}

void InitSPI(void) 
{
    SPSR |= (1 << SPI2X);
    SPCR = (1 << SPE | 1 << MSTR);
}

void InitUSART(void) 
{
    UBRR0H = 0;
    UBRR0L = 51; 
    UCSR0B = (1 << RXEN0 | 1 << TXEN0 | 1 << RXCIE0 | 1 << TXCIE0);
    UCSR0C = (1 << UCSZ01 | 1 << UCSZ00);
}

void InitADC(void) 
{
    ADMUX = (1 << REFS0);
    ADCSRA = (1 << ADEN | 1 << ADIE | 1 << ADPS2 | 1 << ADPS1);
}

void InitTimer1(void) 
{
    TCCR1A = 0;
    TCCR1B = (1 << CS11 | 1 << CS10 | 1 << WGM12);
    OCR1A = 1562;
    OCR1B = 1562; 
    TIMSK1 |= (1 << OCIE1B);
}

void SendData(uint8_t data) 
{
    SPDR = data;
    while(!(SPSR & (1 << SPIF)));
}

void DisplayData(void)
 {
    PORTB &= ~(1 << PB1);
    SendData(segments[mainbuf[3]]);
    SendData(segments[mainbuf[2]]);
    SendData(segments[mainbuf[1]]); 
    SendData(segments[mainbuf[0]]); 
    PORTB |= (1 << PB1);
}

void comp(void) 
{
    if(addbuff[0] == 'C' || addbuff[0] == 67) 
    { 
        mainbuf[0] = addbuff[4] - 48;  
        mainbuf[1] = addbuff[5] - 48; 
        mainbuf[2] = addbuff[6] - 48; 
        mainbuf[3] = addbuff[7] - 48; 
    }
    
    if(addbuff[0] == 'A' || addbuff[0] == 65) 
    {
        if(addbuff[1] == '1')      
            ADMUX = (1 << REFS0) | 0; 
        else 
            if(addbuff[1] == '2') 
                ADMUX = (1 << REFS0) | 1;
        ADCSRA |= (1 << ADSC); 
        while(ADCSRA & (1 << ADSC));         
        uint16_t temp = ra;
        mainbuf[0] = (temp / 1000) % 10;
        mainbuf[1] = (temp / 100) % 10;
        mainbuf[2] = (temp / 10) % 10;
        mainbuf[3] = temp % 10;
    }
}

int main(void) 
{
    InitPorts();
    InitSPI();
    InitTimer1();
    InitADC();
    InitUSART();
    sei();
    PORTB &= ~(1 << PB0);
    while(1)
    {
        if(flag1 == 0x01) 
        {
            flag1 = 0;
            if (cnt < 14) 
                addbuff[cnt++] = c;
        }
        if(flag2 == 0x01) 
        {
            cnt = 0;
            comp();
            flag2 = 0;
            DisplayData();
            for(uint8_t i = 0; i < 15; i++) 
                addbuff[i] = 0;
        }
    }
}