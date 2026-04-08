/**
 * @file exception.c
 * @brief 인터럽트 서비스 루틴(ISR) 및 예외 처리 함수 정의 파일
 * @details RC카 컨트롤러의 통신(USART2 수신) 및 외부 입력(버튼 등 EXTI)에 대한 
 * 인터럽트 핸들러를 포함하고 있습니다.
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
 * @brief USART2 수신 인터럽트 서비스 루틴(ISR)
 * @details USART2 통신 라인을 통해 데이터가 들어올 때 하드웨어적으로 호출되는 함수입니다.
 * 수신 레지스터(DR)에서 1바이트를 읽어 전역 변수에 저장하고, 메인 루프에서
 * 이를 인지할 수 있도록 수신 플래그를 설정합니다.
 * @note 인터럽트 처리가 끝나기 전에 반드시 NVIC의 Pending 상태를 클리어해야 합니다.
 */
void USART2_IRQHandler(void)
{
	if(Macro_Check_Bit_Set(USART2->SR,5)) {
		Uart_Data = (unsigned char)(USART2->DR & 0xFF);
		Uart_Data_In = 1;
	}
	NVIC_ClearPendingIRQ((IRQn_Type)38);
}
