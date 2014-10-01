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
 
 IR_Sensor		=PA1,PA2,PA3
 Schrittmotor		=PB0,PB2,PB4,PB6
 DC_Motor		=PC2,PC3,PC4,PC5
 Ultraschall		=PC0,PC7
 Serial uebertragung	=PC6
 uC_aktiv		=PC1
 Serial			=PD0,PD1
 PWM			=PD4,PD5
 externe Interrups	=PD2
 Schrittmotor Endstop	=PD6
  ----------------------------------------------
 */
#define F_CPU 16000000		// Define clock speed
#define BAUDRATE 57200		// Baudrate fuer die serielleschnittstelle fest legen
//------------Ultraschall Sensor Konfiguration--------------
#define ZAELERTRIGGER 10	// Ultraschall trigger signal maximum Overfow limmit für Timer0
#define	ZAELERECHO 300		// Ultraschall echo signal maximum Overfow limmit für Timer0

#include <avr/io.h>
#include <avr/sleep.h>		// lib for Sleep mode
#include <avr/interrupt.h>	// lib for Interrupts
#include <util/delay.h>		// lib for delays
#include "BFGTmega32.h"		// lib for Serial communication

int I_Wert;			// ISR variable fuer serielle komunikation starten
int rPI;			// ISR variable pruefen ob raspberry PI errechbar ist
int unsigned zaehler= 0;	// ISR Timer0 variable fuer overflow benoetigt fuer Ultraschall Radar
int ulHistory[2] = {0,0};	// Ultraschall Sensor History
int stepperRound= 0;		// counter for stepper driver
int firstStart = 1;		// Kallibrier variable fuer das Ultraschall Radar


ISR(INT0_vect) // Interrupt service routine loest aus wenn Raspberry errechtbar ist.
{
	cli();			// Deaktiviren aller interrups
	if (PIND & (1<<PD2))	// Wenn PD2 == High dann ...
	{
		rPI = 1;	// rPI == Online
	}else
	{
		rPI = 0;	// rPI == Offline
	}
	sei();			// Aktiviren aller interrups
}

ISR(USART_RXC_vect)	// Interrupt service routine loest aus wenn daten empfangen werden
{
	cli();					// Deaktiviren aller interrups
	while(bit_is_clear(UCSRA,RXC));		// warten auf Receive Complete
	I_Wert = UDR;
	sei();					// Aktiviren aller interrups
}

ISR (TIMER0_OVF_vect) // Interrupt service routine loest aus wenn timer0 Register ueberlaeuft
{
	cli();		// Deaktiviren aller interrups
	zaehler++;
	sei();		// Aktiviren aller interrups
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

int SensorRechts(void) // Messung Abstand Rechts
{
	ADMUX |= (1 << MUX0);			// PA1 fuer AD wandlung EIN
	ADCSRA |= (1 << ADSC);			// AD-Wandlung starten
	_delay_us(500);
	int IR[4] = {0,(ADCL+(256*ADCH)),0,0};	// AD-Wandlung 10-Bit
	ADCSRA |= (1 << ADSC);			// AD-Wandlung starten
	_delay_us(500);
	IR[2] = (ADCL+(256*ADCH));		// AD-Wandlung 10-Bit
	ADCSRA |= (1 << ADSC);			// AD-Wandlung starten
	_delay_us(500);
	IR[3] = (ADCL+(256*ADCH));		// AD-Wandlung 10-Bit
	IR[0] = ((IR[1]+IR[2]+IR[3])/3);	// Mittlung der AD-Wandlung
	ADMUX &= ~(1 << MUX0);			// PC4 fuer AD wandlung AUS
	double d = ((1 - ((IR[0] * 0.0048828125) * (1.0 / 18.0))) / ((IR[0] * 0.0048828125) * (2.0 / 27.0)))*2;	// Distanz Umrechnung in cm fuer SHARP 2Y0A12 F 11
	if (d < 7)				// Wenn d kleiner als 5 dann ...
	d = 400;				// Minimum 5cm, Meldung 400
	else if (d > 50)			// Wenn d groeßer als 50 dann ...
	d = 300;				// Maximum 50cm, Meldung 300
	return d;				// Rueckgabe d
}

int SensorFront(void) // Messung Abstand Front
{
	ADMUX |= (1 << MUX1);			// PA2 fuer AD wandlung EIN
	ADCSRA |= (1 << ADSC);			// AD-Wandlung starten
	_delay_us(500);
	int IR[4] = {0,(ADCL+(256*ADCH)),0,0};	// AD-Wandlung 10-Bit
	ADCSRA |= (1 << ADSC);			// AD-Wandlung starten
	_delay_us(500);
	IR[2] = (ADCL+(256*ADCH));		// AD-Wandlung 10-Bit
	ADCSRA |= (1 << ADSC);			// AD-Wandlung starten
	_delay_us(500);
	IR[3] = (ADCL+(256*ADCH));		// AD-Wandlung 10-Bit
	IR[0] = ((IR[1]+IR[2]+IR[3])/3);	// Mittlung der AD-Wandlung
	ADMUX &= ~(1 << MUX1);			// PC2 fuer AD wandlung AUS
	double d = ((1 - ((IR[0] * 0.0048828125) * (1.0 / 18.0))) / ((IR[0] * 0.0048828125) * (2.0 / 14.0)))*2;	// Distanz Umrechnung in cm fuer SHARP 2D120XF 9Z
	if (d < 5)				// Wenn d kleiner als 5 dann ...
	d = 400;				// Minimum 5cm, Meldung 400
	else if (d > 50)			// Wenn d groeßer als 50 dann ...
	d = 300;				// Maximum 50cm, Meldung 300
	return d;				// Rueckgabe d
}

int SensorLinks(void) // Messung Abstand Links
{
	ADMUX |= ((1 << MUX0)|(1 << MUX1));	// PA3 fuer AD wandlung EIN
	ADCSRA |= (1 << ADSC);			// AD-Wandlung starten
	_delay_us(500);
	int IR[4] = {0,(ADCL+(256*ADCH)),0,0};	// AD-Wandlung 10-Bit
	ADCSRA |= (1 << ADSC);			// AD-Wandlung starten
	_delay_us(500);
	IR[2] = (ADCL+(256*ADCH));		// AD-Wandlung 10-Bit
	ADCSRA |= (1 << ADSC);			// AD-Wandlung starten
	_delay_us(500);
	IR[3] = (ADCL+(256*ADCH));		// AD-Wandlung 10-Bit
	IR[0] = ((IR[1]+IR[2]+IR[3])/3);	// Mittlung der AD-Wandlung
	ADMUX &= ~((1 << MUX0)|(1 << MUX1));	// PC3 fuer AD wandlung AUS
	double d = ((1 - ((IR[0] * 0.0048828125) * (1.0 / 18.0))) / ((IR[0] * 0.0048828125) * (2.0 / 12.0)))*2;	// Distanz Umrechnung in cm fuer SHARP 0A41SK F 33
	if (d < 5)				// Wenn d kleiner als 5 dann ...
	d = 400;				// Minimum 5cm, Meldung 400
	else if (d > 50)			// Wenn d groeßer als 50 dann ...
	d = 300;				// Maximum 50cm, Meldung 300
	return d;				// Rueckgabe d
}

int Usensor(void) // Messung Ultraschall Sensor
{
	int s = 0;
	PORTC |= (1<<PC0);
	_delay_us(10);
	PORTC &= ~(1<<PC0);			// Ultraschall Trigger ausloesen
	TCCR0 |= (1<<CS01);			// Timer0 mit Prescale von 8 starten
	while(!(PINC&(1<<PC7)))			// Wartet auf Steigene Flanke
	{
		_delay_us(0);
		if(zaehler >= ZAELERTRIGGER)
			break;
	}
	TCCR0 &= ~(1<<CS01);			//Timer Stoppen
	if(!(zaehler >= ZAELERTRIGGER))
	{
		zaehler = 0;
		TCCR0 |= (1<<CS01);		// Timer0 mit Prescale von 8 starten
		while(PINC&(1<<PC7))		// Warten solange Echo High
		{
			_delay_us(0);
			if(zaehler >= ZAELERECHO)
				break;
		}
		TCCR0 &= ~(1<<CS01);		//Timer Stoppen
		if (!(zaehler >= ZAELERECHO))
		{
			double t = (((zaehler<<8)+TCNT0)/2)*0.5;
			s = (0.0343*t);
			ulHistory[0] = ulHistory[1];
			ulHistory[1]= s;
		}
		else
		{
			s = ((ulHistory[0]+ulHistory[1])/2);
		}
	}
	else
	{
		s = ((ulHistory[0]+ulHistory[1])/2);
	}
	zaehler = 0;
	return s;
}

int SchrittMotor(void) // Schrittmotor steuerung fuer Ultraschall Radar
{
	PORTB &= ~(1<<PB0);			// Schrittmotor Endstufe ENABLE
	if (firstStart == 1)
	{
		firstStart = 0;
		while (stepperRound < 24)
		{
			if (!(PIND & (1<<PIND6)))
			{
				stepperRound ++;
			} else
			{
				stepperRound ++;
				PORTB |= (1<<PB4);
				_delay_ms(20);
				PORTB &= ~(1<<PB4);
				_delay_ms(200);
			}
		}
		stepperRound = 0;
		PORTB &= ~(1<<PB6);		// Dir Low
	}
	if (stepperRound < 24)
	{
		stepperRound ++;
		PORTB |= (1<<PB4);
		_delay_ms(2);
		PORTB &= ~(1<<PB4);
		if (!(PIND & (1<<PIND6)) && stepperRound >= 3)
		{
			stepperRound = 24;
		}
	}
	else
	{
		PORTB |= (1<<PB6);		// Dir High
		stepperRound ++;
		PORTB |= (1<<PB4);
		_delay_ms(2);
		PORTB &= ~(1<<PB4);
				if (!(PIND & (1<<PIND6)) && stepperRound >= 27)
				{
					stepperRound = 49;
				}
	}
	_delay_ms(2);
	if (stepperRound >= 48)
	{
		stepperRound = 0;
		PORTB &= ~(1<<PB6);		// Dir Low
	}
	_delay_ms(20);
	uart_send_string("	UL:");
	uart_send_int(Usensor(),2);
	uart_send_string("	SM:");
	return stepperRound;
}

int Sensor_Ausgabe(void) //Ausgabe der Sensoren
{
	uart_send_string("SR:");
	uart_send_int(SensorRechts(),2);	// Infrarot Front Sensor ausgeben
	uart_send_string("	SF:");
	uart_send_int(SensorFront(),2);		// Infrarot Rechter Sensor ausgeben
	uart_send_string("	SL:");
	uart_send_int(SensorLinks(),2);		// Infrarot Linker Sensor ausgeben
	uart_send_int(SchrittMotor(),2);	// Schrittmotor Referrers signal der Lichtschranke ausgeben
	uart_send_string("\n\r");		// Zeielnumbruch
	_delay_ms(1);
	return 0;
}

int main(void)
{
	uart_init();				// Serial Inizialisirung
	DDRC = 0b01111111;
	DDRB = 0b11111111;
	DDRA = 0b11110001;
	DDRD &= ~((1 << PD2)|(1<<PD6));		// INT0 und Schrittmotor Endstop input...
	DDRD |= (1<<PD4)|(1<<PD5);		// PWM Ausgaenge festlegen
	ADCSRA = 0b10000111;			// Prescaler 128
	ADMUX |= (1<<REFS0);			// 5V Referrens spannung
	TCCR1A |= (1<<WGM10)|(1<<COM1A1)|(1<<COM1B1);
	TCCR1B |= (1<<WGM12)|(1<<CS12);		// Prescaler 256
	GICR |= (1<<INT0);			// IRS INT0 externer Interrupt aktiviren
	MCUCR |= (1<<ISC00);			// ISR INT0 Reaktion auf jede aenderung
	PORTB |= (1<<PB0);			// Schrittmotor Endstufe aus schalten
	PORTB &= ~(1<<PB2);			// Microstepps Disable
	PORTB |= (1<<PB6);			// Dir High
	PORTD |= (1<<PD6);			// Setze Pullup fuer Schrittmotor Endstop
	PORTD &= ~(1<<PD2);			// set pulldown for ISR Raspbarry PI check
	TIMSK |= (1<<TOIE0);			// Timer0 Overflow Interrupt erlauben
	
    while(1)
    {
		sei();				// Aktiviren aller interrups
		uart_init();			// Serial Inizialisirung
		if (rPI == 1)			// Wenn INT0 == High dann ...
		{
			PORTC |= (1<<PC1);
			Sensor_Ausgabe();	// Pereperie auswerten
			if (I_Wert == 49)	// Wenn eingabe == 49 dann ...
			{
				cli();					// Deaktiviren aller interrups
				I_Wert = 0;
				PORTC |= (1<<PC6);			// anzeigen das daten jetzt uebertragen werden koennen
				int Motor_R = (uart_get_int()-48);	// Richtungs vorgabe der Motoren 1-2 Vor-Rueckwaerts 5 = Motoren Aus
				int Motor_L = (uart_get_int()-48);	// Richtungs vorgabe der Motoren 3-4 Vor-Rueckwaerts 5 = Motoren Aus
				if(Motor_R > 5 || Motor_L > 5)		// Wenn eingabe groeßer als 5 dann ...
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
			PORTB |= (1<<PB0);			// Schrittmotor Endstufe DIESABLE
			Motor_richtung(5,5);			// uebergabe an Motor_richtung
			set_sleep_mode(SLEEP_MODE_IDLE);	// sleep mode Aktiviren
			sleep_mode();				// uC geht in Ideln
		}
    }
}
