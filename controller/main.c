/**
 * @file main.c
 * @brief RC카 제어용 메인 컨트롤러 애플리케이션
 * @details 조이스틱의 입력을 받아 방향을 계산하고, 블루투스 연결 상태에 따라
 * LCD에 애니메이션 및 직관적인 방향 문자열을 출력하며 제어 패킷을 전송합니다.
 */

#include "device_driver.h"
#include <stdio.h>

int Get_Joystick_Direction(int x, int y);
const char* Get_Direction_String(int dir);

/**
 * @brief 시스템 클럭 및 하드웨어 주변장치 초기화
 */
void Sys_Init(int baud) 
{
    // FPU 및 클럭 설정
    SCB->CPACR |= (0x3 << 10*2)|(0x3 << 11*2); 
    RCC->CR |= (1 << 0); 
    while(!(RCC->CR & (1 << 1)));
    RCC->CFGR = 0; 

    // 하드웨어 초기화
    Uart2_Init(baud);      
    Uart1_Init(baud);      
    I2C1_Init();           
    LCD_Init();            
    Joystick_Init();           
    setvbuf(stdout, NULL, _IONBF, 0);
}

/**
 * @brief 메인 프로그램 루프
 */
void Main(void)
{
    Sys_Init(38400); 
    
    // 1. 실행 초기 화면 출력
    LCD_Send_Cmd(0x01); // 화면 초기화
    TIM2_Delay(2);
    LCD_Print_String("RC Controller");
    LCD_Send_Cmd(0xC0); // 2번째 줄 이동
    LCD_Print_String("System Ready!   ");
    TIM2_Delay(1500); // 사용자가 인지할 수 있도록 1.5초 대기
    
    SysTick_Run(20); 
    
    int x, y, sw, dir;
    int prev_dir = -1; 
    int prev_sw = -1;  
    int heartbeat_cnt = 0; 
    int ani_cnt = 0; // 연결 대기 애니메이션 카운터
    char msg[16];
    
    for(;;)
    {
        // PA8 핀을 통해 블루투스 페어링 상태 읽기 (1: 연결됨, 0: 연결 안됨)
        int is_connected = Macro_Check_Bit_Set(GPIOA->IDR, 8);

        if(SysTick_Check_Timeout())
        {
            if (!is_connected)
            {
                // 2. 블루투스가 연결되지 않았을 때 (페어링 대기 애니메이션)
                // 20ms 주기이므로 25번 반복 시 500ms 경과
                if (ani_cnt % 25 == 0)
                {
                    int dot_state = (ani_cnt / 25) % 4;
                    
                    LCD_Send_Cmd(0x01); 
                    TIM2_Delay(2);
                    LCD_Print_String("Pairing Mode");
                    LCD_Send_Cmd(0xC0); 

                    // 16칸 고정 폭으로 이전 잔상을 덮어씌움
                    if (dot_state == 0)      LCD_Print_String("Connecting      ");
                    else if (dot_state == 1) LCD_Print_String("Connecting.     ");
                    else if (dot_state == 2) LCD_Print_String("Connecting..    ");
                    else if (dot_state == 3) LCD_Print_String("Connecting...   ");
                }
                ani_cnt++;
                
                // 재연결 시 즉각적인 화면 갱신을 위해 상태 변수 초기화
                prev_dir = -1;
                prev_sw = -1;
            }
            else
            {
                // 3. 블루투스가 연결되었을 때 (조종 모드)
                x = Joystick_Get_X();
                y = Joystick_Get_Y();
                sw = Joystick_Get_SW();
                
                dir = Get_Joystick_Direction(x, y);
                
                if(dir != prev_dir || sw != prev_sw || heartbeat_cnt >= 10)
                {
                    // 통신 프로토콜 전송 (숫자 유지)
                    sprintf(msg, "S%d%d\n", dir, sw);
                    Uart1_Send_String(msg);
                    
                    if(dir != prev_dir || sw != prev_sw) 
                    {
                        // 조종 화면 UI 출력
                        LCD_Send_Cmd(0x01);
                        TIM2_Delay(2);
                        LCD_Print_String("RC Controller");
                        LCD_Send_Cmd(0xC0); 
                        
                        // 방향 숫자를 문자열로 변환하여 출력 (잔상 방지를 위해 16칸 고정 반환됨)
                        LCD_Print_String((char*)Get_Direction_String(dir));
                    }
                    
                    prev_dir = dir;
                    prev_sw = sw;
                    heartbeat_cnt = 0; 
                }
                else
                {
                    heartbeat_cnt++; 
                }
            }
        }

        // 수신 데이터 디버깅 전달
        if(Macro_Check_Bit_Set(USART1->SR, 5)) 
        {
            char rx_data = (char)USART1->DR;
            Uart2_Send_Byte(rx_data); 
        }
    }
}

/**
 * @brief 조이스틱 방향 숫자를 문자열로 변환하는 헬퍼 함수
 * @details LCD 출력 시 잔상이 남지 않도록 모든 문자열의 길이를 공백을 포함해 16칸으로 맞춥니다.
 */
const char* Get_Direction_String(int dir) 
{
    switch(dir) 
    {
        case 8: return "Forward         ";
        case 2: return "Backward        ";
        case 4: return "Turn Left       ";
        case 6: return "Turn Right      ";
        case 7: return "Forward Right    ";
        case 9: return "Forward Left   ";
        case 1: return "Backward Right   ";
        case 3: return "Backward Left  ";
        case 5: default: return "Stop            ";
    }
}

/**
 * @brief 조이스틱 아날로그 값을 1~9 방향 데이터로 매핑
 */
int Get_Joystick_Direction(int x, int y) 
{
    int raw_dx = 0; 
    int raw_dy = 0; 

    if (x > 3000) raw_dx = 1;
    else if (x < 1000) raw_dx = -1;

    if (y > 3000) raw_dy = 1;
    else if (y < 1000) raw_dy = -1;

    int dx = raw_dy;
    int dy = -raw_dx;

    if (dy == 1) { 
        if (dx == -1) return 7;
        if (dx == 1) return 9;
        return 8;
    } 
    else if (dy == -1) { 
        if (dx == -1) return 1;
        if (dx == 1) return 3;
        return 2;
    } 
    else { 
        if (dx == -1) return 6;
        if (dx == 1) return 4;
        return 5;
    }
}