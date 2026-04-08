/**
 * @file main.c
 * @brief RC카 제어용 메인 컨트롤러 애플리케이션
 * @details 조이스틱의 입력을 받아 방향을 계산하고, 상태 변화 또는 하트비트 주기에 따라
 * HC-05 블루투스 모듈(UART1)을 통해 제어 패킷을 전송하며, LCD에 현재 상태를 실시간으로 출력합니다.
 */

#include "device_driver.h"
#include <stdio.h>

int Get_Joystick_Direction(int x, int y);

/**
 * @brief 시스템 클럭 및 하드웨어 주변장치 초기화
 * @details FPU 및 16MHz 클럭을 설정하고 UART, I2C, LCD, 조이스틱 등
 * 프로젝트 구동에 필요한 모든 하드웨어 드라이버를 초기화합니다.
 * @param baud UART 통신에 사용할 보드레이트 (예: 38400)
 */
void Sys_Init(int baud) 
{
    // FPU 및 클럭 설정
    SCB->CPACR |= (0x3 << 10*2)|(0x3 << 11*2); 
    RCC->CR |= (1 << 0); 
    while(!(RCC->CR & (1 << 1)));
    RCC->CFGR = 0; 

    // 하드웨어 초기화
    Uart2_Init(baud);      // 디버그용
    Uart1_Init(baud);      // HC-05 블루투스용
    I2C1_Init();           // LCD 통신용
    LCD_Init();            // LCD 모듈 초기화
    Joystick_Init();       // 조이스틱(ADC 및 PB5) 초기화
    
    setvbuf(stdout, NULL, _IONBF, 0);
}

/**
 * @brief 메인 프로그램 루프
 * @details 20ms 주기로 조이스틱 데이터를 샘플링하여 제어 방향을 계산합니다.
 * 통신 버퍼 포화를 방지하기 위해 조이스틱의 상태(방향 또는 버튼)가 변했을 때만 통신을 수행하며,
 * 상태가 변하지 않더라도 수신 측의 안전(페일세이프) 기능을 위해 200ms(10주기)마다 하트비트 패킷을 전송합니다.
 */
void Main(void)
{
    Sys_Init(38400); 
    
    LCD_Send_Cmd(0x01); // 화면 초기화
    TIM2_Delay(2);
    LCD_Print_String("RC Controller");
    
    // 20ms 주기 타이머 시작 (ADC 읽기 주기 유지)
    SysTick_Run(20); 
    
    int x, y, sw, dir;
    int prev_dir = -1; 
    int prev_sw = -1;  
    int heartbeat_cnt = 0; // 주기적 전송을 위한 카운터 변수 추가
    char msg[16];
    
    for(;;)
    {
        if(SysTick_Check_Timeout())
        {
            x = Joystick_Get_X();
            y = Joystick_Get_Y();
            sw = Joystick_Get_SW();
            
            dir = Get_Joystick_Direction(x, y);
            
            // 상태가 변했거나, 상태가 변하지 않았어도 10주기(200ms)가 경과했다면 전송
            if(dir != prev_dir || sw != prev_sw || heartbeat_cnt >= 10)
            {
                sprintf(msg, "S%d%d\n", dir, sw);
                Uart1_Send_String(msg);
                
                // LCD 업데이트는 상태가 실제로 변했을 때만 수행하여 불필요한 깜빡임 방지
                if(dir != prev_dir || sw != prev_sw) 
                {
                    LCD_Send_Cmd(0xC0); 
                    sprintf(msg, "DIR:%d BTN:%d    ", dir, sw);
                    LCD_Print_String(msg);
                }
                
                prev_dir = dir;
                prev_sw = sw;
                heartbeat_cnt = 0; // 데이터 전송 후 카운터 리셋
            }
            else
            {
                heartbeat_cnt++; // 상태가 변하지 않았다면 카운터 증가
            }
        }

        if(Macro_Check_Bit_Set(USART1->SR, 5)) 
        {
            char rx_data = (char)USART1->DR;
            Uart2_Send_Byte(rx_data); 
        }
    }
}

/**
 * @brief 조이스틱 아날로그 값을 1~9 방향 데이터로 매핑
 * @details X축과 Y축의 원시 ADC 값을 평가하여 데드존(1000~3000)을 적용하고,
 * 하드웨어 장착 방향을 고려하여 논리적인 90도 회전을 수행한 뒤
 * 숫자 키패드 배열(1~9, 5는 중립)에 맞춘 방향 코드를 반환합니다.
 * @param x 조이스틱 X축 원시 ADC 값 (0 ~ 4095)
 * @param y 조이스틱 Y축 원시 ADC 값 (0 ~ 4095)
 * @return int 1부터 9까지의 방향 상태 코드
 */
int Get_Joystick_Direction(int x, int y) 
{
    int raw_dx = 0; 
    int raw_dy = 0; 

    // X축 원시 데이    터 분석
    if (x > 3000) raw_dx = 1;
    else if (x < 1000) raw_dx = -1;

    // Y축 원시 데이터 분석
    if (y > 3000) raw_dy = 1;
    else if (y < 1000) raw_dy = -1;

    // 방향 맞추기
    int dx = raw_dy;
    int dy = -raw_dx;

    // 숫자 키패드 방식 매핑 (789 / 456 / 123)
    if (dy == 1) { // 전진 라인
        if (dx == -1) return 7;
        if (dx == 1) return 9;
        return 8;
    } 
    else if (dy == -1) { // 후진 라인
        if (dx == -1) return 1;
        if (dx == 1) return 3;
        return 2;
    } 
    else { // 중립 라인
        if (dx == -1) return 4;
        if (dx == 1) return 6;
        return 5;
    }
}