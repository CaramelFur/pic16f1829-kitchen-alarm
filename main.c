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

#define ALARM_PIN LATCbits.LATC2
#define ALARM_PIN_T TRISCbits.TRISC2

#define MIN_BUTT_FLAG IOCBFbits.IOCBF4
#define SEC_BUTT_FLAG IOCBFbits.IOCBF5
#define START_BUTT_FLAG IOCBFbits.IOCBF6

#define MIN_BUTT_EN IOCBPbits.IOCBP4
#define SEC_BUTT_EN IOCBPbits.IOCBP5
#define START_BUTT_EN IOCBPbits.IOCBP6

#define MIN_BUTT_VAL PORTBbits.RB4
#define SEC_BUTT_VAL PORTBbits.RB5
#define START_BUTT_VAL PORTBbits.RB6

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

void display2dgtChar(char input) {
    char buf[3] = "  ";
    char lsd = input % 10;
    char msd = input / 10;
    if (msd > 9) msd = 9;

    buf[0] = '0' + msd;
    buf[1] = '0' + lsd;

    addText(buf);
}

void displayTime(ulong seconds, bool hasColon) {
    char sec = getSeconds(seconds);
    char min = getMinutes(seconds);
    char hrs = getHours(seconds);

    setCursor(0);

    display2dgtChar(hrs);
    addChar(hasColon ? ':' : ' ');
    display2dgtChar(min);
    addChar(hasColon ? ':' : ' ');
    display2dgtChar(sec);
}

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
    ANSELBbits.ANSB4 = 0;
    ANSELBbits.ANSB5 = 0;
    INTCONbits.IOCIE = 1;

    // Set timer 2 to 25 times per second
    PR2 = 250; // Timer 2 triggers at 250
    T2CONbits.T2OUTPS = 0b1001; // 10x postscaler
    T2CONbits.T2CKPS = 0b11; // 64x prescaler
    T2CONbits.TMR2ON = 1;

    PIE1bits.TMR2IE = 1;
    INTCONbits.PEIE = 1;
    INTCONbits.GIE = 1;

    // Immediately trigger timer for faster boot
    PIR1bits.TMR2IF = 1;

    while (1) {
    }
}

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

inline void everySecond(void) {
    secondCounter++;

    if (currentMode == RUNNING) {
        if (configuredSeconds == 0) {
            currentSeconds++;
        } else {
            currentSeconds--;
        }

        if (currentSeconds == 0) {
            currentMode = ALARM;
            secondCounter = 0;
        }
    }

    if (currentMode == ALARM) {
        ALARM_PIN = !(secondCounter % 2);
    }

    updateDisplay();
}

inline void onReset(void) {
    configuredSeconds = 0;
    currentSeconds = 0;
    currentMode = STOPPED;

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
            break;
    }

    secondCounter = 0;
    updateDisplay();
}

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

void __interrupt() isr_routine(void) {
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
