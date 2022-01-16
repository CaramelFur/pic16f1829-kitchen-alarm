#include <xc.h>

#include "display.h"
#include "speed.h"

#pragma config FOSC=INTOSC, WDTE=OFF, PWRTE=OFF, MCLRE=OFF, CP=OFF, CPD=OFF
#pragma config BOREN=ON, CLKOUTEN=OFF, IESO=OFF, FCMEN=OFF
#pragma config WRT=OFF, PLLEN=OFF, STVREN=OFF, BORV=HI, LVP=OFF

typedef unsigned long ulong;
typedef char bool;
#define true 1
#define false 0

// Alarm
#define ALARM_PIN LATCbits.LATC2
#define ALARM_PIN_T TRISCbits.TRISC2

// Buttons
#define MIN_BUTT_FLAG IOCBFbits.IOCBF4
#define SEC_BUTT_FLAG IOCBFbits.IOCBF5
#define START_BUTT_FLAG IOCBFbits.IOCBF6
#define MIN_BUTT_EN IOCBPbits.IOCBP4
#define SEC_BUTT_EN IOCBPbits.IOCBP5
#define START_BUTT_EN IOCBPbits.IOCBP6
#define MIN_BUTT_VAL PORTBbits.RB4
#define SEC_BUTT_VAL PORTBbits.RB5
#define START_BUTT_VAL PORTBbits.RB6

inline void everySecond(void);
inline void onSecBut(void);
inline void onMinBut(void);
inline void onStartBut(void);
inline void onReset(void);

void updateDisplay(void);
void displayTime(ulong seconds, bool hasColon);
void display2dgtChar(char input);

char getSeconds(ulong seconds);
char getMinutes(ulong seconds);
char getHours(ulong seconds);

// Used to scale down timer even further
char timerCounter = 0;
// Used to execute logic every n seconds
char secondCounter = 0;

typedef enum {
    STOPPED,
    PAUSED,
    RUNNING,
    ALARM,
} Mode;

// Configured seconds at 0 means stopwatch
ulong configuredSeconds = 0;
ulong currentSeconds = 0;
Mode currentMode = STOPPED;

void main(void) {
    OSCCON = 0b01111000; // 16MHZ

    // This does produce some duplicate functions
    // But we have enough space, so it doesnt really matter
    initDisplay();

    // Configure alarm pin
    ALARM_PIN_T = 0;
    ALARM_PIN = 0;

    // Enable interrupts on all 3 buttons
    MIN_BUTT_EN = 1;
    SEC_BUTT_EN = 1;
    START_BUTT_EN = 1;
    ANSELBbits.ANSB4 = 0; // We need to disable analog in
    ANSELBbits.ANSB5 = 0;
    INTCONbits.IOCIE = 1;

    // Set timer 2 to 25 times per second
    PR2 = 250; // Timer 2 triggers at 250
    T2CONbits.T2OUTPS = 0b1001; // 10x postscaler
    T2CONbits.T2CKPS = 0b11; // 64x prescaler
    T2CONbits.TMR2ON = 1;

    // Enable timer interrupt
    PIE1bits.TMR2IE = 1;
    INTCONbits.PEIE = 1;
    INTCONbits.GIE = 1;

    // Immediately trigger timer for faster boot
    PIR1bits.TMR2IF = 1;

    while (1) {
    }
}

void __interrupt() isr_routine(void) {
    // The timer gets called 25 times a second
    // Here it then calls everySecond() every
    // 25 times the timer is triggered
    // This makes sure we call that function once a second
    if (PIR1bits.TMR2IF) {
        PIR1bits.TMR2IF = 0;
        if (timerCounter++ == 25) {
            timerCounter = 0;
            everySecond();
        }
    }

    // Reset if both sec and min button are pressed together
    if (MIN_BUTT_VAL && SEC_BUTT_VAL) {
        __delay_ms(50);
        SEC_BUTT_FLAG = 0;
        MIN_BUTT_FLAG = 0;

        onReset();
    }

    // Otherwise just call the appropriate button handler
    if (START_BUTT_FLAG) {
        onStartBut();
        __delay_ms(150);
        START_BUTT_FLAG = 0;
    } else if (SEC_BUTT_FLAG) {
        onSecBut();
        __delay_ms(150);
        SEC_BUTT_FLAG = 0;
    } else if (MIN_BUTT_FLAG) {
        onMinBut();
        __delay_ms(150);
        MIN_BUTT_FLAG = 0;
    }
}

// These first 5 functions are all inlined
// This can be easily done because they are all called once
// from inside the interrupt handler
// It is used because it makes sure we have less
// chance of accidentally overflowing the stack

inline void everySecond(void) {
    secondCounter++;

    if (currentMode == RUNNING) {
        // If the configuredSeconds is 0 we are a stopwatch
        // So increment instead
        if (configuredSeconds == 0) {
            currentSeconds++;
        } else {
            currentSeconds--;
        }

        // If we hit zero the timer is finished
        // So we switch to alarm mode
        if (currentSeconds == 0) {
            currentMode = ALARM;
            secondCounter = 0;
        }
    }

    // Sound the alarm if we are in alarm mode
    if (currentMode == ALARM) {
        ALARM_PIN = !(secondCounter % 2);
    }

    // Update the display for updated values
    updateDisplay();
}

// For onSecBut and onMinBut we use the same logic

// If the timer is stopped, it means we are configuring
// the timer, so we must change configuredSeconds

// If the timer is not stopped, but either
// running or paused we want to update the currentSeconds,
// this makes it easier to quickly change the timer
// after it has been set off

// However, we don't increment currentSeconds,
// if configuredSeconds is 0, because this means
// we are running in stopwatch mode.
// And in that mode it would be stupid to be able
// to change the value

// Finally we don't increment in alarm mode,
// This would just be useless

inline void onSecBut(void) {
    if (currentMode == STOPPED) {
        configuredSeconds += 1;
    } else if (
            (currentMode == RUNNING || currentMode == PAUSED)
            && configuredSeconds != 0
            ) {
        currentSeconds += 1;
    }
    updateDisplay();
}

inline void onMinBut(void) {
    if (currentMode == STOPPED) {
        configuredSeconds += 60;
    } else if (
            (currentMode == RUNNING || currentMode == PAUSED)
            && configuredSeconds != 0
            ) {
        currentSeconds += 60;
    }
    updateDisplay();
}

inline void onStartBut(void) {
    switch (currentMode) {
        case STOPPED:
            currentMode = RUNNING;
            currentSeconds = configuredSeconds;
            break;
        case PAUSED:
            currentMode = RUNNING;
            break;
        case RUNNING:
            currentMode = PAUSED;
            break;
        case ALARM:
            currentMode = STOPPED;
            ALARM_PIN = 0;
            break;
    }

    // Force secondCounter to zero, to make make button press
    // more clear in display
    secondCounter = 0;
    updateDisplay();
}

// Onreset does what it says, it resets all values
inline void onReset(void) {
    configuredSeconds = 0;
    currentSeconds = 0;
    currentMode = STOPPED;
    ALARM_PIN = 0;

    updateDisplay();
}

// This prints the right value for the current mode
// To the screen

void updateDisplay(void) {
    switch (currentMode) {
        case STOPPED:
            displayTime(configuredSeconds, true);
            break;
        case PAUSED:
            displayTime(currentSeconds, secondCounter % 2);
            break;
        case RUNNING:
            displayTime(currentSeconds, true);
            break;
        case ALARM:
            displayTime(currentSeconds, secondCounter % 2);
            break;
    }
}

void displayTime(ulong seconds, bool hasColon) {
    // Destructure the time
    char sec = getSeconds(seconds);
    char min = getMinutes(seconds);
    char hrs = getHours(seconds);

    // Reset the cursor and update the screen
    setCursor(0);

    display2dgtChar(hrs);
    addChar(hasColon ? ':' : ' ');
    display2dgtChar(min);
    addChar(hasColon ? ':' : ' ');
    display2dgtChar(sec);
}

// This function converts a number to a two digit
// string, and prints it

void display2dgtChar(char input) {
    char buf[3] = "  ";
    // Calculate least significant digit
    char lsd = input % 10;
    // Calculate most significant digit
    char msd = input / 10;
    // Make sure msd is actually a decimal
    // Otherwise we get weird results
    if (msd > 9) msd = 9;

    buf[0] = '0' + msd;
    buf[1] = '0' + lsd;

    addText(buf);
}

// These next three functions are used to calculate
// The sec, min and hour parts out of just seconds

char getSeconds(ulong seconds) {
    return (char) (seconds % 60);
}

char getMinutes(ulong seconds) {
    seconds /= 60;
    return (char) (seconds % 60);
}

char getHours(ulong seconds) {
    seconds /= (60 * 60);
    if (seconds > 255)
        seconds = 255;
    return (char) seconds;
}