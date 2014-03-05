/*
 * Roboter.c
 *
 * Created: 15.01.2014 14:23:42
 *  Author: Jan-Tarek Butt
 
 ----------------------------------------------
 
 DDRA =||PA1|PA2|PA3|||||
 DDRB =|PB0||PB2||PB4||PB6||
 DDRC =|PC0|PC1|PC2|PC3|PC4|PC5|PC6|PC7|
 DDRD =|PD0|PD1|PD2||PD4|PD5|PD6||
 
 DC_Motor			=PC2,PC3,PC4,PC5
 Serial				=PD0,PD1
 externe Interrups		=PD2
 PWM				=PD4,PD5
 IR_Sensor			=PA1,PA2,PA3
 Ultraschall			=PC0,PC7
 Polulu				=PB0,PB2,PB4,PB6
 Serial uebertragung		=PC6
 uC_aktiv			=PC1
 Lichtschranke			=PD6
 Schrittmotor			=PB0,PB2,PB4,PB6
 
  ----------------------------------------------
 
 */
#define F_CPU 16000000			// Taktfrequentz fest legen
#define BAUDRATE 57200			// Baudrate fuer die serielleschnittstelle fest legen

#include <avr/io.h>
#include <avr/sleep.h>			// lib for Sleep mode
#include <avr/interrupt.h>		// lib for Interrupts
#include <util/delay.h>			// lib for delays
#include "BFGTmega32.h"			// lib for Serial communication

int I_Wert;				// ISR variable fuer serielle komunikation starten
int rPI;				// ISR variable pruefen ob raspberry PI errechbar ist
int unsigned zaehler= 0;		// ISR Timer0 variable fuer overflow benoetigt fuer Ultraschall Radar

ISR(INT0_vect) // Interrupt service routine loest aus wenn Raspberry errechtbar ist.
{
	cli();				// Deaktiviren aller interrups
	if (PIND & (1<<PD2))		// Wenn PD2 == High dann ...
	{
		rPI = 1;		// rPI == Online
	}else
	{
		rPI = 0;		// rPI == Offline
	}
	sei();				// Aktiviren aller interrups
}

ISR(USART_RXC_vect)	// Interrupt service routine loest aus wenn daten empfangen werden
{
	cli();				// Deaktiviren aller interrups
	while(bit_is_clear(UCSRA,RXC));	// warten auf Receive Complete
	I_Wert = UDR;
	sei();				// Aktiviren aller interrups
}

ISR (TIMER0_OVF_vect) // Interrupt service routine loest aus wenn timer0 Register ueberlaeuft
{
	cli();				// Deaktiviren aller interrups
	zaehler++;
	sei();				// Aktiviren aller interrups
}

void Motor_richtung(int Motor_R, int Motor_L) // Motor richtungs steuerung
{
	if (Motor_R == 1)			// Rechter Motor
	{
		PORTC &= ~(1<<PC3);
		PORTC |= (1<<PC2);
	}
	if (Motor_R == 2)
	{
		PORTC &= ~(1<<PC2);
		PORTC |= (1<<PC3);
	}
	if (Motor_L == 3)			// Linker Motor
	{
		PORTC &= ~(1<<PC5);
		PORTC |= (1<<PC4);
	}
	if (Motor_L == 4)
	{
		PORTC &= ~(1<<PC4);
		PORTC |= (1<<PC5);
	}
	if (Motor_R == 5 || Motor_L == 5)	// Motoren Ausschalten
	{
		PORTC &= ~((1<<PC2)|(1<<PC3)|(1<<PC4)|(1<<PC5));
	}
}

int SensorFront(void) // Messung Abstand Front
{
	ADMUX |= (1 << MUX1);			// PA2 fuer AD wandlung EIN
	ADCSRA |= (1 << ADSC);			// AD-Wandlung starten
	_delay_us(500);
	int IR[4] = {0,ADCW,0,0};		// AD-Wandlung 10-Bit
	ADCSRA |= (1 << ADSC);			// AD-Wandlung starten
	_delay_us(500);
	IR[2] = ADCW;				// AD-Wandlung 10-Bit
	ADCSRA |= (1 << ADSC);			// AD-Wandlung starten
	_delay_us(500);
	IR[3] = ADCW;				// AD-Wandlung 10-Bit
	IR[0] = ((IR[1]+IR[2]+IR[3])/3);	// Mittlung der AD-Wandlung
	ADMUX &= ~(1 << MUX1);			// PC2 fuer AD wandlung AUS
	double d = ((1 - ((IR[0] * 0.0048828125) * (1.0 / 18.0))) / ((IR[0] * 0.0048828125) * (2.0 / 27.0)))*2;	// Distanz Umrechnung in cm
	if (d < 5)				// Wenn d kleiner als 5 dann ...
	d = 400;				// Minimum 5cm, Meldung 400
	else if (d > 50)			// Wenn d größer als 50 dann ...
	d = 300;				// Maximum 50cm, Meldung 300
	return d;				// Rückgabe d
}

int SensorRechts(void) // Messung Abstand Rechts
{
	ADMUX |= ((1 << MUX0)|(1 << MUX1));	// PA3 für AD wandlung EIN
	ADCSRA |= (1 << ADSC);			// AD-Wandlung starten
	_delay_us(500);
	int IR[4] = {0,ADCW,0,0};		// AD-Wandlung 10-Bit
	ADCSRA |= (1 << ADSC);			// AD-Wandlung starten
	_delay_us(500);
	IR[2] = ADCW;				// AD-Wandlung 10-Bit
	ADCSRA |= (1 << ADSC);			// AD-Wandlung starten
	_delay_us(500);
	IR[3] = ADCW;				// AD-Wandlung 10-Bit
	IR[0] = ((IR[1]+IR[2]+IR[3])/3);	// Mittlung der AD-Wandlung
	ADMUX &= ~((1 << MUX0)|(1 << MUX1));	// PC3 für AD wandlung AUS
	double d = ((1 - ((IR[0] * 0.0048828125) * (1.0 / 18.0))) / ((IR[0] * 0.0048828125) * (2.0 / 14.0)))*2;	// Distanz Umrechnung in cm
	if (d < 5)				// Wenn d kleiner als 5 dann ...
	d = 400;				// Minimum 5cm, Meldung 400
	else if (d > 50)			// Wenn d größer als 50 dann ...
	d = 300;				// Maximum 50cm, Meldung 300
	return d;				// Rückgabe d
}

int SensorLinks(void) // Messung Abstand Links
{
	ADMUX |= (1 << MUX0);			// PA1 für AD wandlung EIN
	ADCSRA |= (1 << ADSC);			// AD-Wandlung starten
	_delay_us(500);
	int IR[4] = {0,ADCW,0,0};		// AD-Wandlung 10-Bit
	ADCSRA |= (1 << ADSC);			// AD-Wandlung starten
	_delay_us(500);
	IR[2] = ADCW;				// AD-Wandlung 10-Bit
	ADCSRA |= (1 << ADSC);			// AD-Wandlung starten
	_delay_us(500);
	IR[3] = ADCW;				// AD-Wandlung 10-Bit
	IR[0] = ((IR[1]+IR[2]+IR[3])/3);	// Mittlung der AD-Wandlung
	ADMUX &= ~(1 << MUX0);			// PC4 für AD wandlung AUS
	double d = ((1 - ((IR[0] * 0.0048828125) * (1.0 / 18.0))) / ((IR[0] * 0.0048828125) * (2.0 / 12.0)))*2;	// Distanz Umrechnung in cm
	if (d < 5)				// Wenn d kleiner als 5 dann ...
	d = 400;				// Minimum 5cm, Meldung 400
	else if (d > 50)			// Wenn d größer als 50 dann ...
	d = 300;				// Maximum 50cm, Meldung 300
	return d;				// Rückgabe d
}

int Usensor(void) // Messung Ultraschall Sensor
{
	PORTC |= (1<<PC0);
	_delay_us(10);
	PORTC &= ~(1<<PC0);				// Ultraschall Trigger ausloesen
	while((PINC&(1<<PC7))==0);			// Wartet auf Steigene Flanke
	TCCR0 |= (1<<CS01);				// Timer0 mit Prescale von 8 starten
	while(PINC&(1<<PC7));				// Warten solange Echo High
	TCCR0 = 0;					// Timer0 Stoppen
	double t = (((zaehler<<8)+TCNT0)/2)*0.5;	// zeit in us umrechnen
	unsigned int s = (0.0343*t);			// Distanz in cm berechnen
	zaehler=0;					// Timer 0 Overflow auf Null setzen
	uart_send_string(" UL:");
	uart_send_int(s,1);
	uart_send_string(" ");				// Serielle ausgaben von Ultraschall werten
	_delay_ms(1);
	return 0;
}

int SchrittMotor(void) // Schrittmotor steuerung fuer Ultraschall Radar
{
	PORTB &= ~(1<<PB0);	// Schrittmotor Endstufe ENABLE
	PORTB |= (1<<PB4);
	_delay_ms(5);
	PORTB &= ~(1<<PB4);
	_delay_ms(5);		// Ein step vorwaerst
	PORTB |= (1<<PB0);	// Schrittmotor Endstufe DIESABLE
	int Smotor = 0;
	if (PIND & (1<<PD6))	// Wenn Lichtschranke ausloest dann ...
	{
		Smotor = 1;
	}
	Usensor();		// Ultraschall Messung Ausloesen
	return Smotor;
}

int Sensor_Ausgabe(void) //Ausgabe der Sensoren
{
	int Sensoren[4]={SensorFront(),SensorRechts(),SensorLinks(),SchrittMotor()};	// Array, Sensoren auslesen
	uart_send_string("SF:");
	uart_send_int(Sensoren[0],1);	// Infrarot Front Sensor ausgeben
	uart_send_string(" SR:");
	uart_send_int(Sensoren[1],1);	// Infrarot Rechter Sensor ausgeben
	uart_send_string(" SL:");
	uart_send_int(Sensoren[2],1);	// Infrarot Linker Sensor ausgeben
	uart_send_string(" SM:");
	uart_send_int(Sensoren[3],1);	// Schrittmotor Referrers signal der Lichtschranke ausgeben
	uart_send_string("\n\r");	// Zeielnumbruch
	_delay_ms(1);
	return 0;
}

int main(void)
{
	uart_init();					// Serial Inizialisirung
	DDRC = 0b01111111;
	DDRB = 0b11111111;
	DDRA = 0b11110001;
	DDRD &= ~((1 << PD2)|(1<<PD6));			// INT0 und Lichtschranke input...
	DDRD |= (1<<PD4)|(1<<PD5);			// PWM Ausgaenge festlegen
	ADCSRA = 0b10000111;				// Prescaler 128
	ADMUX |= (1<<REFS0);				// 5V Referrens spannung
	TCCR1A |= (1<<WGM10)|(1<<COM1A1)|(1<<COM1B1);
	TCCR1B |= (1<<WGM12)|(1<<CS12);			// Prescaler 256
	GICR |= (1<<INT0);				// IRS INT0 externer Interrupt aktiviren
	MCUCR |= (1<<ISC00);				// ISR INT0 Reaktion auf jede aenderung
	PORTB |= ((1<<PB0)|(1<<PB6));			// Schrittmotor Endstufe aus schalten
	TIMSK |= (1<<TOIE0);				// Timer0 Overflow Interrupt erlauben
	
    while(1)
    {
		sei();					// Aktiviren aller interrups
		uart_init();				// Serial Inizialisirung
		if (rPI == 1)				// Wenn INT0 == High dann ...
		{
			PORTC |= (1<<PC1);
			Sensor_Ausgabe();		// Pereperie auswerten
			if (I_Wert == 49)		// Wenn eingabe == 49 dann ...
			{
				cli();					// Deaktiviren aller interrups
				I_Wert = 0;
				PORTC |= (1<<PC6);			// anzeigen das daten jetzt uebertragen werden koennen
				int Motor_R = (uart_get_int()-48);	// Richtungs vorgabe der Motoren 1-2 Vor-Rueckwaerts 5 = Motoren Aus
				int Motor_L = (uart_get_int()-48);	// Richtungs vorgabe der Motoren 3-4 Vor-Rueckwaerts 5 = Motoren Aus
				if(Motor_R > 5 || Motor_L > 5)		// Wenn eingabe größer als 5 dann ...
				{
					PORTC &= ~(1<<PC6);		// anzeigen das daten nicht mehr uebertragen werden koennen
					return main();			// Zurueck an denn anfang der main funktion
				}
				if(Motor_R == 5 || Motor_L == 5)	// Wenn eingabe == 5 dann ...
				{
					Motor_richtung(Motor_R,Motor_L);	// uebergabe an Motor_richtung
					PORTC &= ~(1<<PC6);			// anzeigen das daten nicht mehr uebertragen werden koennen
					return main();				// Zurueck an denn anfang der main funktion
				}
				int PWM[4] = {0,(uart_get_int()-48),(uart_get_int()-48),(uart_get_int()-48)};	// Annahmen der Motor geschwindigkein 0-255
				PWM[0] = (PWM[1]*100+PWM[2]*10+PWM[3]);
				int PWM1[4] = {0,(uart_get_int()-48),(uart_get_int()-48),(uart_get_int()-48)};	// Annahmen der Motor geschwindigkein 0-255
				PWM1[0] = (PWM1[1]*100+PWM1[2]*10+PWM1[3]);
				if (PWM[0] <= 255 && PWM1[0] <= 255)		// Wenn PWM kleiner als 255 dann ...
				{
					uart_send_string("MR: ");
					uart_send_int(Motor_R,1);
					uart_send_string(" ");
					uart_send_int(PWM[0],1);
					uart_send_string("\n\r");		// Sende zeielnumbruch
					uart_send_string("ML: ");
					uart_send_int(Motor_L,1);
					uart_send_string(" ");
					uart_send_int(PWM1[0],1);
					uart_send_string("\n\r");		// Sende zeielnumbruch
					if ((uart_get_int()-48) == 1)		// Bestaetigung der uebertragenen werte wenn eingabe == 1 dann ...
					{
						Motor_richtung(Motor_R,Motor_L);
						OCR1B = PWM[0];			// PWM setzen
						OCR1A = PWM1[0];		// PWM setzen
					}
				}
				PORTC &= ~(1<<PC6);		// anzeigen das daten nicht mehr uebertragen werden koennen
			}else
			{
				PORTC &= ~(1<<PC6);		// anzeigen das daten nicht mehr uebertragen werden koennen
			}
		}
		else
		{
			PORTC &= ~(1<<PC1);
			Motor_richtung(5,5);			// uebergabe an Motor_richtung
			set_sleep_mode(SLEEP_MODE_IDLE);	// sleep mode Aktiviren
			sleep_mode();				// uC geht Ideln
		}
    }
}
