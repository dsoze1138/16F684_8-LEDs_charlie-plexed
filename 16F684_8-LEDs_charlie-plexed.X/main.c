/*
 * File:   main.c
 * Author: dan1138
 * Target: PIC16F684
 * Compiler: XC8 v2.00
 * 
 * Description:
 * 
 *  Display the upper 8-bits of the ADC conversion 
 *  from analog input from RA0 on 8 charlieplexed LEDs 
 *  connected to outputs RA1,RA2,RA4,RA5.
 *
 *                       PIC16F684
 *             +------------:_:------------+
 *    GND -> 1 : VDD                   VSS : 14 <- 5v0
 *   DRV5 <> 2 : RA5/T1CKI     PGD/AN0/RA0 : 13 <> POT
 *   DRV4 <> 3 : RA4/AN3       PGC/AN1/RA1 : 12 <> DRV1
 *    VPP -> 4 : RA3/VPP           AN2/RA2 : 11 <> DRV2
 *        <> 5 : RC5/CPP1          AN4/RC0 : 10 <> 
 *        <> 6 : RC4/C2OUT         AN5/RC1 : 9  <> 
 *        <> 7 : RC3/AN7           AN6 RC2 : 8  <> 
 *             +---------------------------:
 *                        DIP-14
 * 
 *           150 OHM
 *  DRV4 ----/\/\/\-------+---------+---------+---------+-----------------------------+---------+
 *                        :         :         :         :                             :         :
 *                        :         :         :         :                             :         :
 *                       ---       ---       ---       ---                            :         :
 *                  LED1 / \  LED0 \ /  LED3 / \  LED2 \ /                            :         :
 *                       ---       ---       ---       ---                            :         :  
 *                        :         :         :         :                             :         :
 *           150 OHM      :         :         :         :                             :         :
 *  DRV5 ----/\/\/\-------+---------+-------- : ------- : --------+---------+         :         :
 *                        :         :         :         :         :         :         :         :
 *                        :         :         :         :         :         :         :         :
 *                       ---       ---        :         :         :         :        ---       --- 
 *                  LED5 / \  LED4 \ /        :         :         :         :  LED11 / \ LED10 \ /
 *                       ---       ---        :         :         :         :        ---       --- 
 *                        :         :         :         :         :         :         :         :
 *           150 OHM      :         :         :         :         :         :         :         :
 *  DRV2 ----/\/\/\-------+---------+---------+---------+         :         :         :         :
 *                        :         :                             :         :         :         :
 *                        :         :                             :         :         :         :
 *                       ---       ---                           ---       ---        :         :
 *                  LED7 / \  LED6 \ /                      LED9 / \  LED8 \ /        :         :
 *                       ---       ---                           ---       ---        :         :
 *                        :         :                             :         :         :         :
 *           150 OHM      :         :                             :         :         :         :
 *  DRV1 ----/\/\/\-------+---------+-----------------------------+---------+---------+---------+
 *  
 * 
 *  POT -----/\/\/\--+-------+
 *             1K    :       :
 *                   :      --- 1nF
 *                   :      ---
 *                   v       :
 *  5v0 ----------/\/\/\-----+---- GND
 *                  10K
 * 
 *
 * Notes:
 *  Charlieplexing, see https://en.wikipedia.org/wiki/Charlieplexing
 * 
 * Created on July 13, 2019, 6:09 PM
 */

#pragma config FOSC = INTOSCIO
#pragma config WDTE = OFF
#pragma config PWRTE = OFF
#pragma config MCLRE = ON
#pragma config CP = OFF
#pragma config CPD = OFF
#pragma config BOREN = OFF
#pragma config IESO = OFF
#pragma config FCMEN = OFF

#include <xc.h>
#include <stdint.h>

#define _XTAL_FREQ (8000000ul)
/*
 * Global data
 */
volatile uint8_t gLEDs_0_to_7;
volatile uint8_t gLEDs_8_to_11;
volatile uint8_t gTicks;

void main(void) 
{
    /*
     * Initialize this PIC
     */
    INTCON = 0;
    OSCCON = 0x70;      /* Select 8MHz system oscillator */
    __delay_ms(500);    /* Give ICSP device programming tool a chance to get the PICs attention */

    TRISA = 0xFF;
    TRISC = 0x00;
    ANSEL  = 0;
    OPTION_REG = 0b11000010; /* TIMER0 clock = FOSC/4, prescale 1:8 */
    PORTA = 0;
    PORTC = 0;
    CMCON0 = 7;
    TMR0 = 0;
    TMR0IF = 0;
    TMR0IE = 1;
    gLEDs_0_to_7  = 0b00000000;
    gLEDs_8_to_11 = 0b00000000;
    gTicks = 0;
    GIE = 1;
    /*
     * Initialize ADC on channel 0
     */
    ADCON0 = 0;
    ADCON1 = 0;
    TRISAbits.TRISA0 = 1;       /* Make RA0 an input */
    ANSELbits.ANS0   = 1;       /* Enable AN0 on the RA0 input */
    ADCON1bits.ADCS  = 0b101;   /* Select FOSC/16 as ADC clock source */
    ADCON0bits.CHS   = 0;       /* Select AN0 as ADC input */
    ADCON0bits.ADFM  = 0;       /* Select left justified data */
    ADCON0bits.VCFG  = 0;       /* Select VDD as VREF source */
    ADCON0bits.ADON  = 1;       /* Turn on ADC */
    /*
     * This is the application loop.
     * 
     * Display 8-bit ADC value in charlieplexed LEDs
     */
    while(1)
    {
        ADCON0bits.GO = 1;      /* Start an ADC conversion */
        while(ADCON0bits.GO);   /* Wait for ADC conversion to finish */
        gLEDs_0_to_7 = ADRESH;         /* Put ADC value in LED7 to LED0 */
        if(gTicks == 0)
        {
            gTicks = 250;
            if(((gLEDs_8_to_11 <<= 1) & 0x0F) == 0) gLEDs_8_to_11 = 1;
        }
    }
}
/*
 * Interrupt handlers
 */
void __interrupt() ISR_handler(void)
{
    static uint8_t Timer0Ticks = 0;
    static uint8_t State = 8;
    uint8_t OutBits, HighBits;

    if (TMR0IE && TMR0IF) {  /* TIMER0 asserts and interrupt every 1.024 milliseconds */
        TMR0IF=0;
        if(gTicks != 0) gTicks--;
        if (Timer0Ticks == 0) { /* Select another LED every second TIMER0 interrupt */
            Timer0Ticks = 1;    /* to make LEDs a little brighter make this number larger until you don't like the flickering */

            OutBits  =  0b00000000;
            HighBits =  0b00000000;

            switch (--State)
            {
            case 11:
                if (gLEDs_8_to_11 & 0x08)
                {
                    HighBits |= (1 << 1); /* Drive LED11, DRV4=L DRV1=H */
                    OutBits = ~((1<<1)|(1<<4));
                }
                break;

            case 10:
                if (gLEDs_8_to_11 & 0x04)
                {
                    HighBits |= (1 << 4); /* Drive LED10, DRV4=H DRV1=L */
                    OutBits = ~((1<<1)|(1<<4));
                }
                break;

            case 9:
                if (gLEDs_8_to_11 & 0x02)
                {
                    HighBits |= (1 << 1); /* Drive LED9, DRV1=H DRV5=L */
                    OutBits = ~((1<<1)|(1<<5));
                }
                break;

            case 8:
                if (gLEDs_8_to_11 & 0x01)
                {
                    HighBits |= (1 << 5); /* Drive LED8, DRV1=L DRV5=H */
                    OutBits = ~((1<<1)|(1<<5));
                }
                break;

            case 7:
                if (gLEDs_0_to_7 & 0x80)
                {
                    HighBits |= (1 << 1); /* Drive LED7, DRV1=H DRV2=L */
                    OutBits = ~((1<<1)|(1<<2));
                }
                break;

            case 6:
                if (gLEDs_0_to_7 & 0x40)
                {
                    HighBits |= (1 << 2); /* Drive LED6, DRV1=L DRV2=H */
                    OutBits = ~((1<<1)|(1<<2));
                }
                break;

            case 5:
                if (gLEDs_0_to_7 & 0x20)
                {
                    HighBits |= (1 << 2); /* Drive LED5, DRV5=L DRV2=H */
                    OutBits = ~((1<<5)|(1<<2));
                }
                break;

            case 4:
                if (gLEDs_0_to_7 & 0x10)
                {
                    HighBits |= (1 << 5); /* Drive LED4, DRV5=H DRV2=L */
                    OutBits = ~((1<<5)|(1<<2));
                }
                break;

            case 3:
                if (gLEDs_0_to_7 & 0x08)
                {
                    HighBits |= (1 << 2); /* Drive LED3, DRV4=L DRV2=H */
                    OutBits = ~((1<<4)|(1<<2));
                }
                break;

            case 2:
                if (gLEDs_0_to_7 & 0x04)
                {
                    HighBits |= (1 << 4); /* Drive LED2, DRV4=H DRV2=L */
                    OutBits = ~((1<<4)|(1<<2));
                }
                break;
            case 1:
                if (gLEDs_0_to_7 & 0x02)
                {
                    HighBits |= (1 << 5); /* Drive LED1, DRV4=L DRV5=H */
                    OutBits = ~((1<<4)|(1<<5));
                }
                break;

            default:
                if (gLEDs_0_to_7 & 0x01)
                {
                    HighBits |= (1 << 4); /* Drive LED0, DRV4=H DRV5=L */
                    OutBits = ~((1<<4)|(1<<5));
                }
                State = 12;
            }

            TRISA |= ((1<<5)|(1<<4)|(1<<2)|(1<<1)); /* Turn off all LED output drivers */

            if (OutBits)
            {
                PORTA &= OutBits;      /* Set both LED drivers to low */
                TRISA &= OutBits;      /* Turn on LED output drivers */
                PORTA |= HighBits;     /* Turn on just one of the two LEDs  */
            }
        }
        else
        {
            Timer0Ticks--;
        }
    }
}
