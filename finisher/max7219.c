#include "device_driver.h"
#include "stm32f411xe.h" 

// --- [핀 조작 매크로] ---
#define DIN_HIGH()   (GPIOA->ODR |=  (1 << 7))
#define DIN_LOW()    (GPIOA->ODR &= ~(1 << 7))
#define CLK_HIGH()   (GPIOA->ODR |=  (1 << 5))
#define CLK_LOW()    (GPIOA->ODR &= ~(1 << 5))

#define CS1_HIGH()   (GPIOB->ODR |=  (1 << 6))
#define CS1_LOW()    (GPIOB->ODR &= ~(1 << 6))
#define CS2_HIGH()   (GPIOB->ODR |=  (1 << 12))
#define CS2_LOW()    (GPIOB->ODR &= ~(1 << 12))

// 미세 딜레이
void Delay_uS(volatile unsigned int count) {
    for(; count > 0; count--);
}

// GPIO 및 클럭 초기화 함수 (main.c에서 호출하는 이름과 일치시킴)
void GPIO_Init_Dual(void) {
    // GPIOA, GPIOB 클럭 활성화
    RCC->AHB1ENR |= (1 << 0) | (1 << 1);

    // PA5(CLK), PA7(DIN) 출력 설정
    GPIOA->MODER &= ~((3 << (5 * 2)) | (3 << (7 * 2)));
    GPIOA->MODER |=  ((1 << (5 * 2)) | (1 << (7 * 2)));

    // PB6(CS1), PB12(CS2) 출력 설정
    GPIOB->MODER &= ~((3 << (6 * 2)) | (3 << (12 * 2)));
    GPIOB->MODER |=  ((1 << (6 * 2)) | (1 << (12 * 2)));

    DIN_LOW(); 
    CLK_LOW();
    CS1_HIGH(); 
    CS2_HIGH();
}

// 기초 바이트 전송
void SendByte(unsigned char data) {
    for (int i = 0; i < 8; i++) {
        if (data & 0x80) DIN_HIGH();
        else             DIN_LOW();

        Delay_uS(5);
        CLK_HIGH();
        Delay_uS(5);
        CLK_LOW();
        
        data <<= 1;
    }
}

// 특정 플레이어의 특정 모듈에 전송
void MAX7219_SendOne(int player, int module, unsigned char addr, unsigned char data) {
    if(player == 1)      CS1_LOW();
    else if(player == 2) CS2_LOW();
    else return;

    for (int i = 0; i < 4; i++) {
        if (i == module) {
            SendByte(addr);
            SendByte(data);
        } else {
            SendByte(0x00);
            SendByte(0x00);
        }
    }

    if(player == 1)      CS1_HIGH();
    else if(player == 2) CS2_HIGH();
    
    Delay_uS(10);
}

// 모든 모듈에 전송
void MAX7219_SendAll(int player, unsigned char addr, unsigned char data) {
    if(player == 1)      CS1_LOW();
    else if(player == 2) CS2_LOW();
    else return;

    for (int i = 0; i < 4; i++) {
        SendByte(addr);
        SendByte(data);
    }

    if(player == 1)      CS1_HIGH();
    else if(player == 2) CS2_HIGH();
}

// 초기화 함수 (main.c에서 호출하는 이름과 일치시킴)
void MAX7219_Init_Dual(int player) {
    MAX7219_SendAll(player, 0x0F, 0x00); // Display Test Off
    MAX7219_SendAll(player, 0x0C, 0x01); // Normal Operation
    MAX7219_SendAll(player, 0x09, 0x00); // No Decode
    MAX7219_SendAll(player, 0x0B, 0x07); // 8 rows
    MAX7219_SendAll(player, 0x0A, 0x01); // 밝기 최소 (안전용)
    
    for (int row = 1; row <= 8; row++) {
        MAX7219_SendAll(player, row, 0x00); // Clear All
    }
}