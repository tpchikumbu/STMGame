//********************************************************************
//*                    EEE2046F C template                           *
//*==================================================================*
//* WRITTEN BY: Tapera Chikumbu  	            		             *
//* DATE CREATED: 10/05/2023                                         *
//* MODIFIED: 12/05/2023                                             *
//*==================================================================*
//* PROGRAMMED IN: Visual Studio Code                                *
//* TARGET:        STM32F0                                           *
//*==================================================================*
//* DESCRIPTION:                                                     *
//*                                                                  *
//********************************************************************
// INCLUDE FILES
//====================================================================
#include "stm32f0xx.h"
#include <lcd_stm32f0.c>
//====================================================================
// GLOBAL CONSTANTS
//====================================================================
#define TRUE 1
#define FALSE 0
typedef uint8_t flag_t;
//====================================================================
// GLOBAL VARIABLES
//====================================================================
char time[] = ">     -    -  - -"; //char representation of time
uint8_t minutes = 0, seconds = 0, hundredths = 0;
flag_t startFlag = FALSE, lapFlag = FALSE, stopFlag = FALSE, resetFlag = TRUE;
//====================================================================
// FUNCTION DECLARATIONS
//====================================================================
void display(void);
void checkPB(void);
void initGPIO(void);
void initTIM14(void);
void TIM14_IRQHandler(void);
void convert2BCDASCII(const uint8_t min, const uint8_t sec, const uint8_t hund, char* resultPtr);
//====================================================================
// MAIN FUNCTION
//====================================================================
int main (void) {   
    init_LCD();
    initGPIO();
    initTIM14();
    lcd_command(CLEAR);
    while (1){
        display();
    }
}							// End of main

//====================================================================
// FUNCTION DEFINITIONS
//====================================================================
void initGPIO(void){
    //init input
    RCC -> AHBENR |= RCC_AHBENR_GPIOAEN;
    GPIOA -> MODER &= ~(GPIO_MODER_MODER0|
                        GPIO_MODER_MODER1|
                        GPIO_MODER_MODER2|
                        GPIO_MODER_MODER3);
    GPIOA -> PUPDR |= 0x00000055; 
    //init output
    RCC -> AHBENR |= RCC_AHBENR_GPIOBEN;
    GPIOB -> MODER |= 0x00005555;
    GPIOB -> ODR = 0;
}
void initTIM14(void){
    RCC ->APB1ENR |= RCC_APB1ENR_TIM14EN;
    TIM14 -> PSC = 1;
    TIM14 -> ARR = 39999;
    TIM14 -> DIER |= TIM_DIER_UIE; //Enable interrupts
    TIM14 -> CR1 &= ~(TIM_CR1_CEN);
    NVIC_EnableIRQ(TIM14_IRQn);   
}
void TIM14_IRQHandler(void){
    ++hundredths;
    if (hundredths == 100){ //Overflow in hundredths. Reset and increase seconds
        hundredths = 0;
        ++seconds;
    }
    if (seconds == 60){ //Overflow in seconds. Reset and increase minutes
        seconds = 0;
        ++minutes;
    }
    if (minutes == 100){ //Overflow in minutes. Reset all values
        hundredths = 0;
        seconds = 0;
        minutes = 0;
    }
    TIM14->SR &= ~(TIM_SR_UIF);
}
void display(void){
    checkPB();
    if (resetFlag){ //Clear time and show reset message
        GPIOB ->ODR = 0b1000;
        hundredths = 0;
        seconds = 0;
        minutes = 0;
        //convert2BCDASCII(minutes, seconds, hundredths, time);
        lcd_command(CURSOR_HOME);
        lcd_putstring("Button Game");
        lcd_command(LINE_TWO);
        lcd_putstring("Press SW0...");
    }
    if (startFlag && lapFlag){ //Show current time. Keep counting in background
        GPIOB ->ODR = 0b0010;
        //convert2BCDASCII(minutes, seconds, hundredths, time);
    }
    else if (startFlag && stopFlag){ //Show current time and stop counting
        GPIOB->ODR = 0b0100;
        TIM14 -> CR1 &= ~(TIM_CR1_CEN); //Stop timer
        lcd_command(CLEAR);
        lcd_putstring("Time");
        lcd_command(LINE_TWO);
        lcd_putstring(time);
        delay(10000);
    }
    else if (startFlag){ //Start counter from current value. Show values as they increase
        GPIOB ->ODR = 0b0001;
        TIM14 -> CR1 |= TIM_CR1_CEN; //Start timer
        //convert2BCDASCII(minutes, seconds, hundredths, time);
        lcd_command(CLEAR);
        lcd_putstring("Time");
        lcd_command(LINE_TWO);
        lcd_putstring(time);
        delay(10000);
    }
}
void checkPB(void){
    uint16_t state = (0b1111 & ~(GPIOA -> IDR));
    if (state == 0b0001){ //SWO to start
        startFlag = TRUE;
        stopFlag = FALSE;
        lapFlag = FALSE;
        resetFlag = FALSE;
    }
    else if (state == 0b0010) { //SW1 to lap
        startFlag = TRUE;
        stopFlag = FALSE;
        lapFlag = TRUE;
        resetFlag = FALSE;
    }
    else if (state == 0b0100) { //SW2 to stop
        startFlag = TRUE;
        stopFlag = TRUE;
        lapFlag = FALSE;
        resetFlag = FALSE;
    }
    else if (state == 0b1000) { //SW3 to reset
        startFlag = FALSE;
        stopFlag = FALSE;
        lapFlag = FALSE;
        resetFlag = TRUE;
    }
}
/*4-bit BCD to ASCII conversion*/
void convert2BCDASCII(const uint8_t min, const uint8_t sec, const uint8_t hund, char* resultPtr){
    //Store digits in order: {min1, min0, sec1, sec0, hun1, hun0}
    uint8_t digits[6];
    int8_t time_idx = 0;
    //Get digits
    digits[0] = (min/10) % 10;
    digits[1] = min % 10;
    digits[2] = (sec/10) % 10;
    digits[3] = sec % 10;
    digits[4] = (hund/10) % 10;
    digits[5] = hund % 10;

    for (int8_t i = 0; i < 6; ++i) {
        digits[i] += 48; //Convert to ASCII by adding offset from '0' character
        if (time_idx == 2 || time_idx == 5) {
            ++time_idx; //Skip ‘:’ and ‘.’ in output string
        }
        resultPtr[time_idx] = digits[i];
        ++time_idx;
    }
}
//********************************************************************
// END OF PROGRAM
//********************************************************************
