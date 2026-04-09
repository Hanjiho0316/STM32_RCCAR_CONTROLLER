/**
 * @file main.c
 * @brief RC카 차량 제어용 메인 수신 애플리케이션
 * @details 서큘러 큐를 통해 수신된 패킷을 해석하고 XOR 체크섬 검증을 수행합니다.
 * 통신 단절 감지 시 차량을 강제로 정지시키는 페일세이프(Fail-Safe) 로직이 포함되어 있습니다.
 */

#include "device_driver.h"
#include <stdio.h>

/**
 * @brief 시스템 클럭 및 하드웨어 주변장치 초기화
 */
void Sys_Init(int baud) 
{
    SCB->CPACR |= (0x3 << 10*2)|(0x3 << 11*2); 
    RCC->CR |= (1 << 0); 
    while(!(RCC->CR & (1 << 1)));
    RCC->CFGR = 0; 

    Uart2_Init(baud);      
    Uart1_Init(baud);      
    Uart_Buffer_Init();
    Uart1_RX_Interrupt_Enable(1);
    Motor_Init();      
    
    setvbuf(stdout, NULL, _IONBF, 0);
}

/**
 * @brief 메인 프로그램 루프
 * @details 서큘러 큐에서 데이터를 꺼내어 S + 방향 + 버튼 + 체크섬 구조를 검증합니다.
 */
void Main(void)
{
    Sys_Init(38400);
    
    printf("\n==================================\n");
    printf("   Bluetooth Controller Mode      \n");
    printf("   Listening on USART1 (Queue)    \n");
    printf("==================================\n");

    TIM4_Repeat(500); 

    char recv_data;
    int state = 0;
    char temp_joy = '5';
    char temp_btn = '0';
    char temp_chk = 0;

    for(;;)
    {
        // 1. 서큘러 큐에 데이터가 있으면 순차적으로 꺼내어 파싱
        while (Uart_Buffer_Pop(&recv_data))
        {
            if (recv_data == 'S') {
                state = 1;
            } 
            else if (state == 1) {
                temp_joy = recv_data;
                state = 2;
            } 
            else if (state == 2) {
                temp_btn = recv_data;
                state = 3;
            } 
            else if (state == 3) {
                temp_chk = recv_data; // 송신 측에서 보낸 XOR 체크섬 바이트
                state = 4;
            }
            else if (state == 4 && (recv_data == '\n' || recv_data == '\r')) {
                // 패킷 수신 완료 후 무결성 검증
                char calc_chk = 'S' ^ temp_joy ^ temp_btn;
                
                if (calc_chk == temp_chk) {
                    // 데이터가 손상되지 않았을 때만 모터 제어 및 타이머 리셋 수행
                    Control_Motor_By_Joystick(temp_joy);
                    TIM4_Repeat(500); 
                } else {
                    // 체크섬 불일치 (노이즈 변조 발생 시 해당 패킷 폐기)
                    printf(" [Error] Checksum Mismatch! Ignored.\n");
                }
                state = 0;
            }
            else {
                // 노이즈 등으로 패킷 구조가 무너졌을 경우 상태 초기화
                if(recv_data != '\r' && recv_data != '\n') state = 0;
            }
        }
        
        // 2. 500ms 통신 단절 감지 페일세이프
        if(TIM4_Check_Timeout())
        {
            printf(" [Warning] Signal Lost! Force Stop.\n");
            Control_Motor_By_Joystick('5');      
            TIM4_Repeat(500);
        }
    }
}