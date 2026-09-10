/**
 * @file main.c
 * @brief PWM generator with sinusoidal modulation
 *
 * PWM signal is generated using Timer A0.
 * Duty cycle is periodically updated according to a sine lookup table.
 * Sine amplitude is adjusted using a potentiometer connected to ADC12.
 * Sine frequency is changed using push button S1.
 *
 * @date 2026
 * @author Mihaela Gajic
 */

#include <msp430.h>
#include <stdint.h>
#include "function.h"

/**
 * @brief ACLK frequency
 */
#define ACLK_FREQ               (32768UL)

/** 
 * @brief Baud rate 
 */
#define BR9600      (3)

/**
 * @brief PWM period
 *
 * PWM frequency:
 *
 * fPWM = SMCLK / PWM_PERIOD
 *
 * ≈ 1.024 kHz
 */
#define PWM_PERIOD              (1024)

/**
 * @brief Number of sine samples
 */
#define SINE_POINTS             (64)

/**
 * @brief ADC maximum value
 */
#define ADC_MAX_VALUE           (255)

/**
 * @brief Minimum sine frequency
 */
#define FREQ_MIN                (1)

/**
 * @brief Maximum sine frequency
 */
#define FREQ_MAX                (10)

/**
 * @brief Debounce time
 *
 * About 32 ms using ACLK.
 */
#define DEBOUNCE_PERIOD         (1048)

/**
 * @brief PWM output pin
 *
 * TA0.2 -> P1.3
 */
#define PWM_PIN                 (BIT3)

/**
 * @brief Push button
 *
 * S1 -> P1.4
 */
#define BUTTON_PIN              (BIT4)

/**
 * @brief ADC input
 *
 * Potentiometer -> A0 -> P6.0
 */
#define ADC_PIN                 (BIT0)

/**
 * @brief Timer A2 period
 *
 * Timer is clocked by ACLK (32768Hz). It takes 32768 cycles to count to 1s. If we need a period of X ms, then number of cycles that is written to the CCR0 register is 32768/1000 * X
 */
#define TIMER_A2_PERIOD         (65) /* ~2ms (1.98ms) */

/** 
 * @brief Macro to convert digit to ASCII code 
 */
#define DIGIT2ASCII(x) (x + '0')

/**
 * @brief One sine period
 *
 * Values are scaled to the range 0-255.
 *
 * The table values are calculated using:
 *
 *      y = (sin(x) + 1) * 255 / 2
 *      x = 2pi*i/64
 */
const uint8_t sine_table[SINE_POINTS] =
{
    128, 140, 152, 165, 177, 188, 198, 208,
    218, 227, 236, 243, 249, 253, 254, 255,
    255, 255, 254, 253, 249, 243, 236, 227,
    218, 208, 198, 188, 177, 165, 152, 140,
    128, 115, 103,  90,  78,  67,  57,  47,
     37,  28,  19,  12,   6,   2,   1,   0,
      0,   0,   1,   2,   6,  12,  19,  28,
     37,  47,  57,  67,  78,  90, 103, 115
};


/**
 * @brief Current sine table index
 */
volatile uint8_t sine_index = 0;

/**
 * @brief Current sine amplitude
 *
 * Range: 0-255
 */
volatile uint8_t amplitude = ADC_MAX_VALUE;

/**
 * @brief Previous amplitude value.
 *
 * Used to detect changes in potentiometer position.
 */
volatile uint8_t old_amplitude = 0;

/**
 * @brief Current sine frequency
 *
 * Initial value is 1 Hz.
 */
volatile uint8_t sine_frequency = FREQ_MIN;

/**
 * @brief Array of digits used for display
 */
volatile uint8_t digits[2] = {0};

/**
 * @brief Initializes Timer A0 PWM generator.
 *
 * Timer A0 generates PWM on TA0.2 output.
 *
 * @param none
 *
 * @return none
 *
 * @note PWM output pin is P1.3.
 */
void PWM_INIT(void)
{
    // Init P1.3 pin as alternate function pin
    P1SEL |= PWM_PIN;     // alternate function
    P1DIR |= PWM_PIN;     // P1.3 is TA0.2 pin

    // Init timer A0
    TA0CCR0 = PWM_PERIOD - 1;     // PWM period
    TA0CCR2 = PWM_PERIOD / 2;     // initial duty cycle (50%)
    TA0CCTL2 = OUTMOD_7;     // Reset/Set output mode
    TA0CTL = TASSEL__SMCLK | MC__UP;     // SMCLK clock source
                                         // activate timer
}


/**
 * @brief Initializes ADC12 module.
 *
 * ADC reads potentiometer (pot2) voltage on channel A0.
 *
 * @param none
 *
 * @return none
 *
 * @note ADC resolution is 8 bits.
 */
void ADC_INIT(void)
{
    P6SEL |= ADC_PIN;     // configure P6.0 as analog input
    
    ADC12CTL0 &= ~ADC12ENC;     // disable ADC before configuration
    ADC12CTL0 |= ADC12SHT0_3 + ADC12ON;     // 32 cycles for sampling, ref voltage is set by REF module, ADC ON 
    ADC12CTL1 |= ADC12SHP | ADC12SSEL_3;     // sampling timer, SMCLK source
    ADC12CTL2 &= ~ADC12RES_2;     // 8-bit conversion
    ADC12MCTL0 = ADC12INCH_0;     // channel A0
    ADC12IE |= ADC12IE0;     // enable interrupt
    ADC12CTL0 |= ADC12ENC;     // enable ADC
}


/**
 * @brief Initializes Timer B0.
 *
 * Timer B0 periodically updates PWM duty cycle.
 *
 * @param none
 *
 * @return none
 *
 * @note Interrupt period depends on sine frequency.
 */
void TIMERB_INIT(void)
{
    // Timer interrupt period
    TB0CCR0 = (ACLK_FREQ / (SINE_POINTS * FREQ_MIN)) - 1;
    TB0CCTL0 = CCIE;     // enable CCR0 interrupt
    TB0CTL = TBSSEL__ACLK | MC__UP;     // ACLK clock source
                                        // activate timer
}


/**
 * @brief Initializes push button.
 *
 * Button S1 changes sine frequency.
 *
 * @param none
 *
 * @return none
 */
void BUTTON_INIT(void)
{
    P1REN |= BUTTON_PIN;     // enable pull up/down
    P1OUT |= BUTTON_PIN;     // set pull up
    P1DIR &= ~BUTTON_PIN;     // configure as in
    P1IES |= BUTTON_PIN;     // interrupt on falling edge
    P1IFG &= ~BUTTON_PIN;     // clear flag
    P1IE |= BUTTON_PIN;     // enable interrupt
}


/**
 * @brief Initializes Timer A1.
 *
 * Timer A1 is used for button debouncing.
 *
 * @param none
 *
 * @return none
 */
void DEBOUNCE_TIMER_INIT(void)
{
    TA1CCR0 = DEBOUNCE_PERIOD - 1;    
    TA1CCTL0 = CCIE;     // enable interrupt for CCR0
    TA1CTL = TASSEL__ACLK;     // select ACLK clock source
                               // timer is stopped
}


/**
 * @brief Initializes LED display.
 *
 * LED display shows the current sine frequency.
 *
 * @param none
 *
 * @return none
 */
void DISPLAY_INIT (void)
{
    // sevenseg 1
    P7DIR |= BIT0;              // set P7.0 as out (SEL1)
    P7OUT |= BIT0;             // disable display 1
    // sevenseg 2
    P6DIR |= BIT4;              // set P6.4 as out (SEL2)
    P6OUT |= BIT4;              // disable display 2

    // a,b,c,d,e,f,g
    P2DIR |= BIT6 | BIT3;              // configure P2.3 and P2.6 as out
    P3DIR |= BIT7;                     // configure P3.7 as out
    P4DIR |= BIT3 | BIT0;              // configure P4.0 and P4.3 as out
    P8DIR |= BIT2 | BIT1;              // configure P8.1 and P8.2 as out

    /* initialize Timer A2 for multiplexing*/
    TA2CCR0 = TIMER_A2_PERIOD;
    TA2CCTL0 = CCIE;
    TA2CTL = TASSEL__ACLK | MC__UP;
}

/**
 * @brief Initializes UART.
 *
 * The UART interface is used to send the amplitude value of the generated sine wave.
 *
 * @param none
 *
 * @return none
 */
void UART_INIT (void)
{
    P4SEL |= BIT4 | BIT5;       // P4.4 = UCA1TXD, P4.5 = UCA1RXD

    UCA1CTL1 |= UCSWRST;        // enter reset mode

    UCA1CTL0 = 0;               // UART mode, no parity, 8-bit data
    UCA1CTL1 |= UCSSEL__ACLK;  // use ACLK as clock source

    UCA1BRW = BR9600;                // baud rate 9600
    UCA1MCTL |= UCBRS_3 + UCBRF_0;         // configure 9600 bps

    UCA1CTL1 &= ~UCSWRST;       // leave reset mode
} 


/**
 * @brief Function that populates digit array
 */
void display(const uint8_t number)
{
    uint8_t nr = number;
    uint8_t tmp;
    for (tmp = 0; tmp < 2; tmp++)
    {
        digits[tmp] = nr % 10;
        nr /= 10;
    }
}


void UART_sendNumber(uint16_t value)
{
    uint8_t digit;
    
    // stotine
    digit = value / 100;
    while(!(UCA1IFG & UCTXIFG));
    UCA1TXBUF = DIGIT2ASCII(digit);

    // desetice
    digit = (value / 10) % 10;
    while(!(UCA1IFG & UCTXIFG));
    UCA1TXBUF = DIGIT2ASCII(digit);

    // jedinice
    digit = value % 10;
    while(!(UCA1IFG & UCTXIFG));
    UCA1TXBUF = DIGIT2ASCII(digit);

    // novi red
    while(!(UCA1IFG & UCTXIFG));
    UCA1TXBUF = '\r';

    while(!(UCA1IFG & UCTXIFG));
    UCA1TXBUF = '\n';
}


/**
 * @brief Main function
 *
 * Initializes all peripherals and starts PWM generation.
 * The potentiometer controls the sine amplitude,
 * while the push button changes the sine frequency.
 *
 * @return never returns
 */
int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;           // Stop watchdog timer

    /* Initialize peripherals */
    PWM_INIT();
    ADC_INIT();
    TIMERB_INIT();
    BUTTON_INIT();
    DEBOUNCE_TIMER_INIT();
    DISPLAY_INIT();
    UART_INIT();

    __enable_interrupt();
    
    display(sine_frequency);

    while (1)
    {
        ADC12CTL0 |= ADC12SC;     // start ADC conversion

        __delay_cycles(10000);
    }
}


/**
 * @brief Timer B interrupt service routine.
 *
 * Generates sinusoidal modulation of PWM.
 *
 * Each interrupt:
 * - reads the next sine value from lookup table
 * - calculates PWM duty cycle
 * - updates TA0CCR2 register
 *
 * Timer B interrupt frequency defines how fast the sine table is traversed.
 */
void __attribute__ ((interrupt(TIMER0_B0_VECTOR))) TIMERB_ISR(void)
{
    uint16_t duty;
    uint8_t sine_value;
    
    sine_value = sine_table[sine_index];     // read current sine value from lookup table


    /*
     * Calculate PWM duty cycle.
     *
     * The sine value defines the instantaneous value of the waveform.
     *
     * The amplitude variable scales the waveform according to potentiometer value.
     */
    duty = ((uint32_t)sine_value * amplitude * PWM_PERIOD) / (ADC_MAX_VALUE * ADC_MAX_VALUE);

    TA0CCR2 = duty;     // update PWM duty cycle 
    sine_index++;     // move to next sine sample

    //Restart sine period.
    if(sine_index >= SINE_POINTS)
    {
        sine_index = 0;
    }
}


/**
 * @brief Button interrupt service routine.
 *
 * Starts debounce timer after button press.
 *
 * Frequency is changed only after debounce
 * timer confirms that button is still pressed.
 */
void __attribute__ ((interrupt(PORT1_VECTOR))) BUTTON_ISR(void)
{
    if((P1IFG & BUTTON_PIN) != 0)
    {
        TA1CTL |= MC__UP;     // start debounce timer A1
        P1IFG &= ~BUTTON_PIN;     // clear flag 
    }
}


/**
 * @brief Timer A1 interrupt service routine.
 *
 * If the button is still pressed after debounce period, sine frequency is increased.
 */
void __attribute__ ((interrupt(TIMER1_A0_VECTOR))) DEBOUNCE_ISR(void)
{

    // Check if button is still pressed.
    if((P1IN & BUTTON_PIN) == 0)
    {
        TA1CTL &= ~MC__UP;     // stop debounce timer
        TA1CTL |= TACLR;     // clear debounce timer

        sine_frequency++;     // increase sine frequency

        // Return to minimum frequencyafter maximum value.
        if(sine_frequency > FREQ_MAX)
        {
            sine_frequency = FREQ_MIN;
        }

        display(sine_frequency);

        /*
         * Change Timer B period.
         *
         * Timer B generates: SINE_POINTS samples for one sine period.
         * f_timer = SINE_POINTS * f_sine
         */
        TB0CCR0 = (ACLK_FREQ /(SINE_POINTS * sine_frequency)) - 1;
    }
}


/**
 * @brief ADC12 conversion interrupt.
 *
 * This interrupt is executed after ADC conversion is completed.
 * The converted potentiometer value is read from ADC12MEM0 and stored as the current sine wave amplitude.
 *
 * @note ADC resolution is 8 bits, therefore amplitude values are in the range 0-255.
 */
void __attribute__ ((interrupt(ADC12_VECTOR))) ADC_ISR(void)
{
    if(ADC12IV == ADC12IV_ADC12IFG0)
    {
        amplitude = ADC12MEM0;     // save amplitude value

        if ((amplitude > old_amplitude && (amplitude - old_amplitude) > 3) || (old_amplitude > amplitude && (old_amplitude - amplitude) > 3))
        {
            UART_sendNumber(amplitude);
            old_amplitude = amplitude;
        }
    }
}


/**
 * @brief TA2CCR0 ISR
 *
 * Multiplex the 7seg display. Each ISR activates one digit.
 */
void __attribute__ ((interrupt(TIMER2_A0_VECTOR))) TA2CCR0ISR (void)
{
    static uint8_t current_digit = 0;

    /* algorithm:
     * - turn off previous display (SEL signal)
     * - set a..g for current display
     * - activate current display
     */
    if (current_digit == 1)
    {
        P6OUT |= BIT4;          // turn off SEL2
        WriteLed(digits[current_digit]);    // define seg a..g
        P7OUT &= ~BIT0;         // turn on SEL1
    }
    else if (current_digit == 0)
    {
        P7OUT |= BIT0;
        WriteLed(digits[current_digit]);
        P6OUT &= ~BIT4;
    }
    current_digit = (current_digit + 1) & 0x01;

    return;
}
