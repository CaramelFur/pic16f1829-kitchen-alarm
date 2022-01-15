#ifndef DISPLAY_H
#define	DISPLAY_H

#include <xc.h>
#include "speed.h"

// =================== Config  ===================

#define EN_PIN LATBbits.LATB7
#define RS_PIN LATCbits.LATC7
#define D4_PIN LATCbits.LATC5
#define D5_PIN LATCbits.LATC4
#define D6_PIN LATCbits.LATC3
#define D7_PIN LATCbits.LATC6

#define EN_PIN_T TRISBbits.TRISB7
#define RS_PIN_T TRISCbits.TRISC7
#define D4_PIN_T TRISCbits.TRISC5
#define D5_PIN_T TRISCbits.TRISC4
#define D6_PIN_T TRISCbits.TRISC3
#define D7_PIN_T TRISCbits.TRISC6

// ================= End Config ==================

// commands
#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME 0x02
#define LCD_ENTRYMODESET 0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_CURSORSHIFT 0x10
#define LCD_FUNCTIONSET 0x20
#define LCD_SETCGRAMADDR 0x40
#define LCD_SETDDRAMADDR 0x80

// flags for display entry mode
#define LCD_ENTRYRIGHT 0x00
#define LCD_ENTRYLEFT 0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

// flags for display on/off control
#define LCD_DISPLAYON 0x04
#define LCD_DISPLAYOFF 0x00
#define LCD_CURSORON 0x02
#define LCD_CURSOROFF 0x00
#define LCD_BLINKON 0x01
#define LCD_BLINKOFF 0x00

// flags for display/cursor shift
#define LCD_DISPLAYMOVE 0x08
#define LCD_CURSORMOVE 0x00
#define LCD_MOVERIGHT 0x04
#define LCD_MOVELEFT 0x00

// flags for function set
#define LCD_8BITMODE 0x10
#define LCD_4BITMODE 0x00
#define LCD_2LINE 0x08
#define LCD_1LINE 0x00
#define LCD_5x10DOTS 0x04
#define LCD_5x8DOTS 0x00

// constants
#define LCD_ROW_INTERNAL_WIDTH 0x40

void initDisplay(void);
void setCursor(char x);
void clearDisplay(void);
void homeDisplay(void);
void setText(char * txt);
void addText(char * txt);
void addChar(char character);

#endif

