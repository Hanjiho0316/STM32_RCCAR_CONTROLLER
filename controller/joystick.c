/**
 * @file joystick.c
 * @brief 아날로그 조이스틱 모듈 제어 드라이버
 * @details X축과 Y축의 아날로그 값을 읽기 위한 ADC 기능과 
 * Z축 버튼의 디지털 입력을 처리하는 기능들을 포함하고 있습니다.
 */

#include "device_driver.h"

/**
 * @brief 조이스틱 하드웨어 초기화
 * @details X/Y축 값을 읽기 위해 ADC1 장치를 초기화하고, 
 * 조이스틱의 Z축 버튼(SW) 입력을 받기 위해 PB5 핀(Arduino 커넥터 D4)을 
 * 내부 풀업(Pull-up) 저항이 연결된 입력 모드로 설정합니다.
 */
void Joystick_Init(void)
{
    // 1. X축, Y축을 위한 ADC 장치 초기화 호출
    ADC1_Init();

    // 2. Z축 버튼(SW)을 위한 PB5 핀 초기화 (Arduino 커넥터 D4)
    // GPIOB 클럭 활성화 (비트 1)
    Macro_Set_Bit(RCC->AHB1ENR, 1); 
    
    // PB5를 입력 모드(00)로 설정 (핀 번호 5 * 2 = 10번 비트 위치)
    Macro_Write_Block(GPIOB->MODER, 0x3, 0x0, 10); 
    
    // PB5에 내부 풀업(Pull-up) 저항 연결 (01)
    Macro_Write_Block(GPIOB->PUPDR, 0x3, 0x1, 10); 
}

/**
 * @brief 조이스틱 X축 아날로그 값 읽기
 * @details PA0 핀(ADC1 채널 0)에 연결된 X축의 원시 아날로그(ADC) 값을 변환하여 읽어옵니다.
 * @return int 변환된 X축 아날로그 데이터 (일반적으로 0 ~ 4095)
 */
int Joystick_Get_X(void)
{
    // X축이 연결된 PA0 핀(채널 0)의 아날로그 값을 읽어 반환
    return ADC1_Read(0); 
}

/**
 * @brief 조이스틱 Y축 아날로그 값 읽기
 * @details PA1 핀(ADC1 채널 1)에 연결된 Y축의 원시 아날로그(ADC) 값을 변환하여 읽어옵니다.
 * @return int 변환된 Y축 아날로그 데이터 (일반적으로 0 ~ 4095)
 */
int Joystick_Get_Y(void)
{
    // Y축이 연결된 PA1 핀(채널 1)의 아날로그 값을 읽어 반환
    return ADC1_Read(1); 
}

/**
 * @brief 조이스틱 Z축 버튼(SW) 눌림 상태 확인
 * @details PB5 핀의 논리 상태를 읽어 버튼 조작 여부를 확인합니다.
 * 내부 풀업 저항이 설정되어 있으므로, 버튼이 눌려 핀이 GND와 연결될 때(Low 상태)를
 * 눌린 것으로 간주하여 값을 반전시켜 반환합니다.
 * @return int 버튼이 눌렸을 경우 1, 눌리지 않았을 경우 0
 */
int Joystick_Get_SW(void)
{
    // PB5 버튼이 눌렸을 때 GND와 연결되어 0(Low)이 되므로, 이를 반전시켜 1로 반환
    return Macro_Check_Bit_Clear(GPIOB->IDR, 5);
}