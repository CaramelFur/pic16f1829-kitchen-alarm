#include "display.h"

void setPinOutputs(void);
void enable4BitMode(void);
void writeHalfByte(char data);
void writeFullByte(char data, char mode);
void pulseEnable(void);
void command(char cmd);
void text(char txt);

// Set the display to 4 bit mode, with 4x8 characters and 2 lines
// (The display is 16x1, but internally 8x2, so its 2 lines)
#define FUNCTION_CONFIG (LCD_4BITMODE | LCD_5x8DOTS | LCD_2LINE)
// Turn the display on without a cursor or blinking
#define DISPLAY_CONFIG (LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF)
// Set text direction to left to right
#define MODE_CONFIG (LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT)

#define COLS 16
#define ROWS 1
#define COL_CUTOFF 8

char currentPos = 0;

void initDisplay(void) {
    setPinOutputs();

    // Wait for display to start
    __delay_ms(50);

    // Make sure these are low
    RS_PIN = 0;
    EN_PIN = 0;

    enable4BitMode();

    command(LCD_FUNCTIONSET | FUNCTION_CONFIG);
    command(LCD_DISPLAYCONTROL | DISPLAY_CONFIG);

    clearDisplay();

    command(LCD_ENTRYMODESET | MODE_CONFIG);
}

void setCursor(char x) {
    if (x > COLS) x = COLS - 1;

    currentPos = x;

    // Because the screen is split in half, we need to adjust for that
    if (x >= COL_CUTOFF)
        x += LCD_ROW_INTERNAL_WIDTH - COL_CUTOFF;

    command(LCD_SETDDRAMADDR | x);
}

void clearDisplay(void) {
    command(LCD_CLEARDISPLAY);
    __delay_ms(2);
    currentPos = 0;
}

void homeDisplay(void) {
    command(LCD_RETURNHOME);
    __delay_ms(2);
    currentPos = 0;
}

void setText(char * txt) {
    clearDisplay();
    addText(txt);
}

void addText(char * txt) {
    char txtPtr = 0;
    while (txt[txtPtr] != 0) {
        text(txt[txtPtr++]);
    }
}

void addChar(char character) {
    text(character);
}

void setPinOutputs(void) {
    EN_PIN_T = 0;
    RS_PIN_T = 0;
    D4_PIN_T = 0;
    D5_PIN_T = 0;
    D6_PIN_T = 0;
    D7_PIN_T = 0;
}

void enable4BitMode(void) {
    writeHalfByte(3);
    __delay_ms(5);
    writeHalfByte(3);
    __delay_ms(5);
    writeHalfByte(3);
    __delay_us(150);
    writeHalfByte(2);
}

// Writes one half of a command or data byte
// This is needed because we run in 4 bit mode

void writeHalfByte(char data) {
    D4_PIN = (data >> 0) & 1;
    D5_PIN = (data >> 1) & 1;
    D6_PIN = (data >> 2) & 1;
    D7_PIN = (data >> 3) & 1;

    pulseEnable();
}

// Uses two half byte writes to write a complete byte

void writeFullByte(char data, char mode) {
    RS_PIN = mode;

    writeHalfByte(data >> 4);
    writeHalfByte(data);
}

void command(char cmd) {
    writeFullByte(cmd, 0);
}

void text(char txt) {
    // This will ensure we jump to "second row"
    // at the appropriate time
    if (currentPos == COL_CUTOFF)
        setCursor(currentPos);

    writeFullByte(txt, 1);
    currentPos += 1;
}

// Sends a pulse to the enable pin

void pulseEnable(void) {
    EN_PIN = 0;
    __delay_us(1);
    EN_PIN = 1;
    __delay_us(1);
    EN_PIN = 0;
    __delay_us(100);
}