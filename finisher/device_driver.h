#include "stm32f4xx.h"
#include "option.h"
#include "macro.h"
#include "malloc.h"

// Uart.c

extern void Uart2_Init(int baud);
extern void Uart2_Send_Byte(char data);
extern void Uart2_RX_Interrupt_Enable(int en);

extern void Uart1_Init(int baud);
extern void Uart1_Send_Byte(char data);
extern void Uart1_Send_String(char *pt);
extern void Uart1_Printf(char *fmt,...);
extern char Uart1_Get_Char(void);
extern char Uart1_Get_Pressed(void);

// SysTick.c

extern void SysTick_Run(unsigned int msec);
extern int SysTick_Check_Timeout(void);
extern unsigned int SysTick_Get_Time(void);
extern unsigned int SysTick_Get_Load_Time(void);
extern void SysTick_Stop(void);

// Led.c

extern void LED_Init(void);
extern void LED_On(void);
extern void LED_Off(void);

// Clock.c

extern void Clock_Init(void);

// Key.c

extern void Key_Poll_Init(void);
extern int Key_Get_Pressed(void);
extern void Key_Wait_Key_Released(void);
extern void Key_Wait_Key_Pressed(void);
extern void Key_ISR_Enable(int en);

// Timer.c

extern void TIM2_Delay(int time);
extern void TIM2_Stopwatch_Start(void);
extern unsigned int TIM2_Stopwatch_Stop(void);
extern void TIM4_Repeat(int time);
extern int TIM4_Check_Timeout(void);
extern void TIM4_Stop(void);
extern void TIM4_Change_Value(int time);
extern void TIM4_Repeat_Interrupt_Enable(int en, int time);
extern void TIM3_Out_Init(void);
extern void TIM3_Out_Freq_Generation(unsigned short freq);
extern void TIM3_Out_Stop(void);

// i2c.c

#define SC16IS752_IODIR				0x0A
#define SC16IS752_IOSTATE			0x0B

extern void I2C1_SC16IS752_Init(unsigned int freq);
extern void I2C1_SC16IS752_Write_Reg(unsigned int addr, unsigned int data);
extern void I2C1_SC16IS752_Config_GPIO(unsigned int config);
extern void I2C1_SC16IS752_Write_GPIO(unsigned int data);

// spi.c

extern void SPI1_SC16IS752_Init(unsigned int div);
extern void SPI1_SC16IS752_Write_Reg(unsigned int addr, unsigned int data);
extern void SPI1_SC16IS752_Config_GPIO(unsigned int config);
extern void SPI1_SC16IS752_Write_GPIO(unsigned int data);

// Adc.c

extern void ADC1_IN6_Init(void);
extern void ADC1_Start(void);
extern void ADC1_Stop(void);
extern int ADC1_Get_Status(void);
extern int ADC1_Get_Data(void);


// Buzzer.c

extern void Buzzer_Init(void);
extern void Buzzer_Beep(int on_ms, int off_ms);
extern void Buzzer_Count_Short(void);
extern void Buzzer_Count_Long(void);
extern void Buzzer_Start_Race(void);

// --- [MAX7219 외부 함수 선언] ---
// max7219.c에 정의된 이름과 동일하게 맞췄습니다.

void Delay_uS(volatile unsigned int count);           // static이 아니므로 선언 가능
void GPIO_Init_Dual(void);                            // 새로 만든 초기화 함수
void MAX7219_Init_Dual(int player);                   // 이름 뒤에 _Dual 확인
void MAX7219_SendOne(int player, int module, unsigned char addr, unsigned char data);
void MAX7219_SendAll(int player, unsigned char addr, unsigned char data);
void MAX7219_ShowPattern(int player, int module, unsigned char *pattern);
void MAX7219_ClearAll(int player);