#include "device_driver.h"
#include <stdio.h>

void Sys_Init_Chat(int baud) 
{
    // FPU 및 클럭(HSI 16MHz) 설정
    SCB->CPACR |= (0x3 << 10*2)|(0x3 << 11*2); 
    RCC->CR |= (1 << 0); 
    while(!(RCC->CR & (1 << 1)));
    RCC->CFGR = 0; 

    Uart2_Init(baud);      
    Uart1_Init(baud);      
    
    setvbuf(stdout, NULL, _IONBF, 0);
}

void Main(void)
{
    Sys_Init_Chat(38400); 
    
    printf("\n==============================\n");
    printf("   Bluetooth Chatting Ready   \n");
    printf("      Baudrate: 38400         \n");
    printf("==============================\n");
    
    char data;
    for(;;)
    {
        // 1. 내 PC(테라텀)에서 입력 -> 블루투스로 전송
        if(Macro_Check_Bit_Set(USART2->SR, 5)) 
        {
            data = (char)USART2->DR;
            Uart1_Send_Byte(data); 
            // 중복 입력을 막기 위해 Uart2_Send_Byte(data)는 하지 않습니다.
            // (글자가 안 보이면 테라텀 설정에서 Local Echo를 켜세요)
        }

        // 2. 블루투스로부터 수신 -> 내 PC(테라텀)에 출력
        if(Macro_Check_Bit_Set(USART1->SR, 5)) 
        {
            data = (char)USART1->DR;
            Uart2_Send_Byte(data); 
        }
    }
}