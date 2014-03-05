/*
BFGTmega32.h
header-Datei für die Benutzung des µC-Boards "BFGTmega32"
des BZTG Oldenburg für das LCDisplay, die EEPROM-Nutzung,
die Nutzung der seriellen Schnittstelle, ...
am
Bildungszentrum für Technik und Gestaltung der Stadt Oldenburg
Standard: LC-Display an PORTB
*/

// Definitionen für Bitmanipulationen
#define setbit(PORT, bit)	(PORT|=	(1<<bit)	// Bit setzen
#define clearbit(PORT, bit)	(PORT&=~(1<<bit))	// Bit löschen
#define togglebit(PORT, bit)	(PORT^=	(1<<bit))	// Bit invertieren

// Definitionen für LCDisplays, die kompatibel sind zu HD44780			// Pinning adapted to set interrupt pins free
// LCD control commands
#define LCD_CLEAR		0x01	/*Clear display: 	0b 0000 0001	*/
#define LCD_HOME		0x02	/*Cursor home: 		0b 0000 0010	*/
#define LCD_ON			0x0C	/*Cursor invisible:	0b 0000 1100	*/
#define LCD_OFF			0x08	/*Display off: 		0b 0000 1000	*/
#define POS_01			0x80	/*Zeile 1 - Spalte 0	0b 1000 0000 	*/
#define POS_02			0xC0	/*Zeile 2 - Spalte 0	0b 1100 0000 	*/

// Festlegung der PORT-Pins
#define	LCDPORT			PORTB
#define	LCDDDR			DDRB
#define LCD_PIN_RS		2
#define LCD_PIN_E		3
#define LCD_PIN_D4		4
#define LCD_PIN_D5		5
#define LCD_PIN_D6		6
#define LCD_PIN_D7		7
#define	COMMAND			0
#define DATA			1

void toggle_enable_pin(void)			// Unterstützungsfunktion zum Invertieren von Bits
{
	setbit(LCDPORT, LCD_PIN_E);
	clearbit(LCDPORT, LCD_PIN_E);	
}

#ifdef _UTIL_DELAY_H_					// wenn delay.h eingebunden ist dann ...
void lcd_send(unsigned char type, unsigned char c)	// wird von lcd_write() aufgerufen
{
	unsigned char sic_c;				// backup for c
	// send high nibble
	sic_c = c;					// save original c
	sic_c &= ~0x0f;					// set bit 0-3 == 0
	if (type==DATA) sic_c |= (1<<LCD_PIN_RS);	// data: RS = 1
	LCDPORT = sic_c; toggle_enable_pin();		// send high nibble
	// send low nibble
	sic_c = c;					// save original c
	sic_c = sic_c<<4;				// exchange nibbles
	sic_c &= ~0x0f;					// set bit 0-3 == 0
	if (type==DATA) sic_c |= (1<<LCD_PIN_RS);	// data: RS = 1
	LCDPORT = sic_c; toggle_enable_pin();		// send low nibble
	_delay_ms(5);					// Wait for LCD controller
}

void lcd_init()				// Initializing des LCDisplays - vgl. Datenblatt
{
	/* Set Port to Output */
	LCDDDR	= 0xFF;LCDPORT = 0x00;
	_delay_ms(50); // Wait for LCD

	/* 4-bit Modus config */
	setbit(LCDPORT, LCD_PIN_D5);	// Funktion Set : 4-Bit-Modus
	_delay_ms(5); 
	
	/* 2 Lines, 4-Bit Mode */
	lcd_send(COMMAND, 0x28);	// Funktion Set : 2zeilige Display, 5x7 dots 
	lcd_send(COMMAND, LCD_OFF);	lcd_send(COMMAND, LCD_CLEAR);
	lcd_send(COMMAND, 0x06);	// Entry Mode Set : Inkrement, Cursor schieben
	lcd_send(COMMAND, LCD_ON);
}

void lcd_write(char *data)		// Schreibt eine Zeichenkette (String) auf das Display
{
	while(*data){lcd_send(DATA, *data); data++;}
}

void lcd_set_cursor(uint8_t zeile, uint8_t zeichen)	// Cursor auf Zeile und Zeichen setzen
{
	uint8_t i;
	switch (zeile) 
	{
		case 1: i=0x80+0x00+zeichen; break;	// 1. Zeile
		case 2: i=0x80+0x40+zeichen; break;	// 2. Zeile
		default: return;			// invalid line
	}
	lcd_send(COMMAND, i);
}
#endif

void eepromwritebyte(uint16_t adresse, uint8_t data)	// Byte in EEPROM schreiben
{
	while(EECR & (1<<EEWE));
	EEAR = adresse;
	EEDR = data;
	EECR |= (1<<EEMWE);
	EECR |= (1<<EEWE);
}

uint8_t eepromreadbyte(uint16_t adresse)		// Byte aus EEPROM lesen
{
	while(EECR & (1<<EEWE));
	EEAR = adresse;
	EECR |= (1<<EERE);
	return EEDR;
}


#ifdef BAUDRATE					// Wenn BAUDRATE definirt ist dann ...
void uart_init(void)				// Initialisierung der seriellen Schnittstelle
{
	UBRRL = (unsigned char)(F_CPU/(BAUDRATE*16L)-1);	// Formel zur Berechnung der Baudrate variabel zur Tacktferqenz 
	UCSRB |= (1<<TXEN) | (1<<RXEN) | (1<<RXCIE);		// Sendemodul aktivieren, Empfangsmodul aktivieren, // Char Interuppt Aktivieren
	UCSRC |= (1<<UCSZ1) | (1<<UCSZ0) | (1<<URSEL);		// Länge eines Zeichens auf 8 bit setzen
								// URSEL benötigt um auf UCSRC zuzugreifen
}

void uart_send_char(unsigned char c)	// Sendet einen einzelnen char
{
	while (!(UCSRA & (1<<UDRE)));	// Warten bis der UART zum Senden bereit
	UDR = c;			// �c� in Ausgangsregister schreiben
					// übertragung wird automatisch gestartet
}

void uart_send_string(char *s)		// Senset einen String auf die serielle Schnittstelle
{
	while(*s)			//solange s != \0
	{
		uart_send_char(*s);	//einzelnes Zeichen senden
		s++;			//weiter zum nächsten Zeichen
	}
}

int uart_get_int(void)				// Empfängt ein integer
{
	while(bit_is_clear(UCSRA,RXC));		//warten auf Receive Complete
	return UDR;
}

void uart_send_int(unsigned int zahl, int sges)	// Sendet einen integer der Zeichenlänge sges 
{
	//Ausgabe der Integerzahl zahl formatiert mit sges Stellen
	char buffer[17];
	uint8_t l=0,n;
	char *z=buffer;
	utoa(zahl,buffer,10);
	while(*z!=0){l++; z++;}				//Bufferlänge l
	for(n=l;n<sges;n++) uart_send_char(' ');
	uart_send_string(buffer);
}

void uart_send_float(float zahl, int sges, int snach)	// Sendet einen float mit sges Zeichen und snach Nachkommastellen
{
	//Ausgabe einer Fließkommazahl mit sges Gesamtstellen. 
	//Hiervon sind snach Nachkommastellen.
	//Die Nachkollastellen werden gerundet. 
	char buffer[16];
	dtostrf(zahl,sges,snach,buffer);
	uart_send_string(buffer);
}

int ausgabe(void)				// Beispiel einer Ausgabe und Tastaturabfrage
{
	int z;
	uart_send_string("\n\n\r");	// 2x Zeilenumbruch, 1x Rücklauf nach links
	uart_send_string("1)  Digitale Ausgaben an den Ports B und C\n\r");
	uart_send_string("2)  Taster als digitale Eingänge an Port A\n\r");
	uart_send_string("\nGeben Sie Ihre Wahl ein: ");
	z=uart_get_int();
	z=z-48;
	return z;	
}
#endif
