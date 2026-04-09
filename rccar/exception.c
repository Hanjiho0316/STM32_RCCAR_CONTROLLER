/**
 * @file exception.c
 * @brief RC카 수신부 인터럽트 서비스 루틴(ISR) 및 예외 처리
 * @details 외부 인터럽트 및 UART 통신(USART1, USART2) 수신 인터럽트를 처리합니다.
 * USART1 인터럽트는 지연 없이 수신된 데이터를 서큘러 큐에 밀어 넣습니다.
 */

#include "device_driver.h"
#include <stdio.h>

void _Invalid_ISR(void)
{
    unsigned int r = Macro_Extract_Area(SCB->ICSR, 0x1ff, 0);
    printf("\nInvalid_Exception: %d!\n", r);
    printf("Invalid_ISR: %d!\n", r - 16);
    for(;;);
}

extern volatile int Key_Pressed;

void EXTI15_10_IRQHandler(void)
{
    Key_Pressed = 1;
    
    EXTI->PR = 0x1 << 13;
    NVIC_ClearPendingIRQ(40);
}

extern volatile int Uart_Data_In;
extern volatile unsigned char Uart_Data;

/**
 * @brief USART2 수신 인터럽트 서비스 루틴 (디버깅용)
 */
void USART2_IRQHandler(void)
{
    if(Macro_Check_Bit_Set(USART2->SR,5)){
        Uart_Data = (unsigned char)(USART2->DR & 0xFF);
        Uart_Data_In = 1;
    }
    NVIC_ClearPendingIRQ((IRQn_Type)38);
}

/**
 * @brief USART1 수신 인터럽트 서비스 루틴 (블루투스 제어 명령 수신)
 * @details 시스템 지연을 방지하기 위해 데이터 파싱 로직을 제거하고,
 * 수신된 1바이트를 즉시 서큘러 큐(환형 버퍼)에 삽입합니다.
 */
void USART1_IRQHandler(void)
{
    if(Macro_Check_Bit_Set(USART1->SR, 5))
    {
        char recv = (char)(USART1->DR & 0xFF);
        Uart_Buffer_Push(recv);
    }

    NVIC_ClearPendingIRQ((IRQn_Type)37);
}