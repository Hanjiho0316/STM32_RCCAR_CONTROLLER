#include "device_driver.h"
#include "stm32f411xe.h" // CMSIS 표준 헤더 추가
#include "macro.h"
#include <stdio.h>

// --- [매크로 기반 핀 제어] ---
// 초음파 센서 (1P: PA10/PB5, 2P: PA8/PA9)
#define TRIG1_HIGH()   Macro_Set_Bit(GPIOA->ODR, 10)
#define TRIG1_LOW()    Macro_Clear_Bit(GPIOA->ODR, 10)
#define ECHO1_PIN      Macro_Check_Bit_Set(GPIOB->IDR, 5)
#define TRIG2_HIGH()   Macro_Set_Bit(GPIOA->ODR, 8)
#define TRIG2_LOW()    Macro_Clear_Bit(GPIOA->ODR, 8)
#define ECHO2_PIN      Macro_Check_Bit_Set(GPIOA->IDR, 9)

// MAX7219 (DIN: PA7, CLK: PA5, CS1: PB6, CS2: PB12)
#define DIN_HIGH()     Macro_Set_Bit(GPIOA->ODR, 7)
#define DIN_LOW()      Macro_Clear_Bit(GPIOA->ODR, 7)
#define CLK_HIGH()     Macro_Set_Bit(GPIOA->ODR, 5)
#define CLK_LOW()      Macro_Clear_Bit(GPIOA->ODR, 5)
#define CS1_HIGH()     Macro_Set_Bit(GPIOB->ODR, 6)
#define CS1_LOW()      Macro_Clear_Bit(GPIOB->ODR, 6)
#define CS2_HIGH()     Macro_Set_Bit(GPIOB->ODR, 12)
#define CS2_LOW()      Macro_Clear_Bit(GPIOB->ODR, 12)

// --- 시스템 초기화 ---
void Sys_Init(int baud) 
{
    // FPU 및 기본 클럭 설정
    SCB->CPACR |= (0x3 << 10*2)|(0x3 << 11*2); 
    RCC->CR |= (1 << 0); 
    while(!(RCC->CR & (1 << 1)));
    RCC->CFGR = 0; 

    // AHB1 클럭 활성화 (구조체 및 표준 비트 명칭 사용)
    RCC->AHB1ENR |= (RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN);

    Uart2_Init(baud);      
    setvbuf(stdout, NULL, _IONBF, 0);

    // GPIO 모드 설정 (GPIOA->MODER, GPIOB->MODER 직접 사용)
    // 1P: PA10(Output), PB5(Input)
    Macro_Write_Block(GPIOA->MODER, 0x3, 0x1, 10*2); // PA10 -> Output(01)
    Macro_Clear_Area(GPIOB->MODER, 0x3, 5*2);        // PB5 -> Input(00)

    // 2P: PA8(Output), PA9(Input)
    Macro_Write_Block(GPIOA->MODER, 0x3, 0x1, 8*2);  // PA8 -> Output(01)
    Macro_Clear_Area(GPIOA->MODER, 0x3, 9*2);        // PA9 -> Input(00)
}

// --- [MAX7219 전송 함수 수정] ---
void Matrix_SendByte(unsigned char data) {
    for (int i = 0; i < 8; i++) {
        // DIN (PA7) 제어: 다른 핀 영향 없이 7번 비트만 조작
        if (data & 0x80) GPIOA->ODR |= (1 << 7); 
        else             GPIOA->ODR &= ~(1 << 7);
        
        for(volatile int j=0; j<20; j++); // 안정화 딜레이

        // CLK (PA5) 제어: 5번 비트만 조작
        GPIOA->ODR |= (1 << 5);
        for(volatile int j=0; j<20; j++);
        GPIOA->ODR &= ~(1 << 5);
        for(volatile int j=0; j<20; j++);

        data <<= 1;
    }
}

void Matrix_Write(int player, unsigned char addr, unsigned char data) {
    // 1. CS 핀 활성화 (Low)
    // CS1: PB6, CS2: PB12
    if(player == 1)      GPIOB->ODR &= ~(1 << 6);
    else if(player == 2) GPIOB->ODR &= ~(1 << 12);

    // 2. 데이터 전송 (8x32 모듈 4개 칩 기준)
    for(int i=0; i<4; i++) {
        Matrix_SendByte(addr);
        Matrix_SendByte(data);
    }

    // 3. CS 핀 비활성화 (High)
    if(player == 1)      GPIOB->ODR |= (1 << 6);
    else if(player == 2) GPIOB->ODR |= (1 << 12);
}

void Matrix_Init(int player) {
    Matrix_Write(player, 0x0C, 0x01); // Normal Operation
    Matrix_Write(player, 0x09, 0x00); // No Decode
    Matrix_Write(player, 0x0B, 0x07); // Scan Limit (All digits)
    Matrix_Write(player, 0x0A, 0x01); // Brightness
    for(int i=1; i<=8; i++) Matrix_Write(player, i, 0x00); // Clear All
}

// --- [함수 1] 1번 센서용 측정 ---
float Get_Distance_1P(void) {
    uint32_t count = 0, timeout = 0;

    TRIG1_LOW(); for(volatile int i=0; i<10; i++);
    TRIG1_HIGH(); for(volatile int i=0; i<50; i++);
    TRIG1_LOW();

    // 레지스터 주소 대신 GPIOB->IDR 사용
    while(Macro_Check_Bit_Clear(GPIOB->IDR, 5)) { 
        if(++timeout > 50000) return -1.0f;
    }
    while(Macro_Check_Bit_Set(GPIOB->IDR, 5)) {   
        count++;
        if(count > 100000) break;
    }
    return count * 0.017f;
}

// --- [함수 2] 2번 센서용 측정 ---
float Get_Distance_2P(void) {
    uint32_t count = 0, timeout = 0;

    TRIG2_LOW(); for(volatile int i=0; i<10; i++);
    TRIG2_HIGH(); for(volatile int i=0; i<50; i++);
    TRIG2_LOW();

    // 레지스터 주소 대신 GPIOA->IDR 사용
    while(Macro_Check_Bit_Clear(GPIOA->IDR, 9)) { 
        if(++timeout > 50000) return -1.0f;
    }
    while(Macro_Check_Bit_Set(GPIOA->IDR, 9)) {   
        count++;
        if(count > 100000) break;
    }
    return count * 0.017f;
}

// --- 메인 함수 ---
void Main(void)
{
    Sys_Init(38400); 
    // 매트릭스 초기화
    Matrix_Init(1); 
    Matrix_Init(2);
    printf("--- Game Start! ---\n");

    for(;;)
    {
        float d1 = Get_Distance_1P();
        for(volatile int i=0; i<30000; i++); 
        float d2 = Get_Distance_2P();

        printf("\r1P: %5.1f cm | 2P: %5.1f cm", (double)d1, (double)d2);

        // 예시: 10cm 이내면 매트릭스에 불 켜기 (간단한 테스트용)
        //if(d1 > 0 && d1 < 10.0f) Matrix_Write(1, 1, 0xFF); 
        //else Matrix_Write(1, 1, 0x00);

        for(volatile int i=0; i<500000; i++); 
    }
}