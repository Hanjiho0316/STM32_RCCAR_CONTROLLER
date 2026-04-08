/**
 * @file uart.c
 * @brief UART 직렬 통신 제어 드라이버
 * @details STM32F411 MCU의 USART1(HC-05 블루투스용) 및 USART2(PC 디버깅용) 
 * 채널에 대한 초기화, 송수신, 인터럽트 제어 기능을 제공합니다.
 */

#include "device_driver.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

// exception.c에서 참조하는 전역 변수들 실제 정의
volatile int Uart_Data_In = 0;        // 데이터 수신 여부 플래그
volatile unsigned char Uart_Data = 0; // 수신된 데이터 1바이트 저장 변수

/**
 * @brief USART2 하드웨어 초기화 (디버깅용)
 * @details PA2(TX) 및 PA3(RX) 핀을 대체 기능(AF07)으로 설정하고,
 * 지정된 보드레이트에 맞추어 분주비를 계산하여 USART2 장치를 활성화합니다.
 * @param baud 통신 속도 (Baudrate, 예: 38400)
 */
void Uart2_Init(int baud)
{
  double div;
  unsigned int mant;
  unsigned int frac;

  Macro_Set_Bit(RCC->AHB1ENR, 0);                   // PA2,3
  Macro_Set_Bit(RCC->APB1ENR, 17);                   // USART2 ON
  Macro_Write_Block(GPIOA->MODER, 0xf, 0xa, 4);     // PA2,3 => ALT
  Macro_Write_Block(GPIOA->AFR[0], 0xff, 0x77, 8);  // PA2,3 => AF07
  Macro_Write_Block(GPIOA->PUPDR, 0xf, 0x5, 4);     // PA2,3 => Pull-Up  

  volatile unsigned int t = GPIOA->LCKR & 0x7FFF;
  GPIOA->LCKR = (0x1<<16)|t|(0x3<<2);                // Lock PA2, 3 Configuration
  GPIOA->LCKR = (0x0<<16)|t|(0x3<<2);
  GPIOA->LCKR = (0x1<<16)|t|(0x3<<2);
  t = GPIOA->LCKR;

  div = PCLK1/(16. * baud);
  mant = (int)div;
  frac = (int)((div - mant) * 16. + 0.5);
  mant += frac >> 4;
  frac &= 0xf;

  USART2->BRR = (mant<<4)|(frac<<0);
  USART2->CR1 = (1<<13)|(0<<12)|(0<<10)|(1<<3)|(1<<2);
  USART2->CR2 = 0<<12;
  USART2->CR3 = 0;
}

/**
 * @brief USART2를 통한 1바이트 데이터 송신
 * @details 송신 데이터 레지스터가 빌 때까지 대기한 후, 입력받은 1바이트 문자를 전송합니다.
 * 터미널 출력 시 추가적인 개행 문자 변환 없이 순수 데이터를 보냅니다.
 * @param data 전송할 8비트 문자 데이터
 */
void Uart2_Send_Byte(char data)
{
  while(!Macro_Check_Bit_Set(USART2->SR, 7));
  USART2->DR = data;
}

/**
 * @brief USART2 수신 인터럽트 활성화 및 비활성화 제어
 * @details USART2의 RXNE(수신 버퍼 풀음) 인터럽트를 제어하고,
 * NVIC(중첩 벡터 인터럽트 컨트롤러) 상에서 해당 IRQ(38번)를 설정합니다.
 * @param en 1을 입력하면 인터럽트 활성화, 0을 입력하면 비활성화
 */
void Uart2_RX_Interrupt_Enable(int en)
{
  if(en)
  {
		// USART2 RX Interrupt Enable
    Macro_Set_Bit(USART2->CR1,5);

		// NIVC Pending Clear
		NVIC_ClearPendingIRQ((IRQn_Type)38);
    
    // NVIC Interrupt Enable
    NVIC_EnableIRQ((IRQn_Type)38);

  }
  else
  {
    Macro_Clear_Bit(USART2->CR1, 5);
    NVIC_DisableIRQ(38);
  }
}

/**
 * @brief USART1 하드웨어 초기화 (HC-05 블루투스용)
 * @details PA9(TX) 및 PA10(RX) 핀을 대체 기능(AF07)으로 설정하고,
 * 지정된 보드레이트에 맞추어 통신 속도를 설정하여 USART1 장치를 활성화합니다.
 * @param baud 통신 속도 (Baudrate, 예: 38400)
 */
void Uart1_Init(int baud)
{
  double div;
  unsigned int mant;
  unsigned int frac;

  Macro_Set_Bit(RCC->AHB1ENR, 0);                   // PA9,10
  Macro_Set_Bit(RCC->APB2ENR, 4);                   // USART1 ON
  Macro_Write_Block(GPIOA->MODER, 0xf, 0xa, 18);    // PA9,10 => ALT
  Macro_Write_Block(GPIOA->AFR[1], 0xff, 0x77, 4);  // PA9,10 => AF07
  Macro_Write_Block(GPIOA->PUPDR, 0xf, 0x5, 18);    // PA9,10 => Pull-Up
  
  volatile unsigned int t = GPIOA->LCKR & 0x7FFF;
  GPIOA->LCKR = (0x1<<16)|t|(0x3<<9);               // Lock PA9, 10 Configuration
  GPIOA->LCKR = (0x0<<16)|t|(0x3<<9);
  GPIOA->LCKR = (0x1<<16)|t|(0x3<<9);
  t = GPIOA->LCKR;

  div = PCLK2 / (16. * baud);
  mant = (int)div;
  frac = (int)((div - mant) * 16 + 0.5);
  mant += frac >> 4;
  frac &= 0xf;
  USART1->BRR = (mant<<4)|(frac<<0);

  USART1->CR1 = (1<<13)|(0<<12)|(0<<10)|(1<<3)|(1<<2);
  USART1->CR2 = 0 << 12;
  USART1->CR3 = 0;
}

/**
 * @brief USART1을 통한 1바이트 데이터 송신
 * @details 송신 버퍼가 빌 때까지 대기 후 1바이트 문자를 블루투스 모듈로 전송합니다.
 * 패킷 오작동을 막기 위해 강제 개행 문자 변환 로직이 제거되어 있습니다.
 * @param data 전송할 8비트 문자 데이터
 */
void Uart1_Send_Byte(char data)
{
  // \n 체크 로직을 완전히 제거하고 순수하게 데이터만 보냅니다.
  while(!Macro_Check_Bit_Set(USART1->SR, 7));
  USART1->DR = data;
}

/**
 * @brief USART1을 통한 널 종료 문자열 송신
 * @details 문자열의 끝(\0)을 만날 때까지 내부적으로 Uart1_Send_Byte를 반복 호출하여
 * 전체 문자열 패킷을 블루투스 모듈로 전송합니다.
 * @param pt 전송할 문자열의 시작 주소 포인터
 */
void Uart1_Send_String(char *pt)
{
  while(*pt != 0)
  {
    Uart1_Send_Byte(*pt++);
  }
}

/**
 * @brief USART1 수신 버퍼 비동기 확인 및 읽기
 * @details 수신 레지스터에 대기 중인 데이터가 있는지 즉시 확인하고 반환합니다.
 * 데이터가 없으면 대기하지 않고 바로 0을 반환하여 시스템의 멈춤을 방지합니다.
 * @return char 수신된 문자 데이터 (데이터가 없을 경우 0)
 */
char Uart1_Get_Pressed(void)
{
	if(Macro_Check_Bit_Set(USART1->SR, 5))
	{
		return (char)USART1->DR;
	}
	else
	{
		return (char)0;
	}
}

/**
 * @brief USART1 1바이트 동기 수신 대기
 * @details 블루투스 모듈로부터 새로운 데이터가 들어올 때까지 무한 대기(Block)한 후,
 * 수신된 1바이트 문자를 반환합니다.
 * @return char 수신된 8비트 문자 데이터
 */
char Uart1_Get_Char(void)
{
	while(!Macro_Check_Bit_Set(USART1->SR, 5));
	return (char)USART1->DR;
}
