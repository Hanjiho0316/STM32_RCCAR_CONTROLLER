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
        // 1. 테라텀(PC)에서 친 글자를 블루투스(USART1)로 전달
        if(Macro_Check_Bit_Set(USART2->SR, 5)) 
        {
            data = (char)USART2->DR;
            Uart1_Send_Byte(data); 
            // 내가 친 글자를 화면에 보기 위해 로컬 에코(Echo)
            Uart2_Send_Byte(data); 
        }

        // 2. 블루투스에서 온 응답(OK 등)을 테라텀(PC)으로 전달
        if(Macro_Check_Bit_Set(USART1->SR, 5)) 
        {
            data = (char)USART1->DR;
            Uart2_Send_Byte(data); 
        }
    }
}