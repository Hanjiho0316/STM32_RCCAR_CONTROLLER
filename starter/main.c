#include "device_driver.h"

#define RCC_AHB1ENR    (*(volatile unsigned int*)0x40023830)
#define GPIOB_MODER    (*(volatile unsigned int*)0x40020400)
#define GPIOB_OTYPER   (*(volatile unsigned int*)0x40020404)
#define GPIOB_ODR      (*(volatile unsigned int*)0x40020414)

// ---------------------------------
// 패널 번호 기준
// 4번 패널 = 맨위    = module 0
// 3번 패널 = 그 아래 = module 1
// 2번 패널 = 그 아래 = module 2
// 1번 패널 = 맨아래 = module 3
// ---------------------------------

// 부저: A3 = PB0
#define BUZZER_ON()   (GPIOB_ODR |=  (1 << 0))
#define BUZZER_OFF()  (GPIOB_ODR &= ~(1 << 0))

unsigned char num_3[8] = {
    0x00,
    0x6E,
    0x91,
    0x91,
    0x91,
    0x81,
    0x42,
    0x00
};

unsigned char num_2[8] = {
    0x00,
    0x61,
    0x91,
    0x89,
    0x85,
    0x83,
    0x41,
    0x00
};

unsigned char num_1[8] = {
    0x00,
    0x01,
    0x01,
    0xFF,
    0x41,
    0x21,
    0x00,
    0x00
};

void Buzzer_Init(void)
{
    // GPIOB 클럭 활성화
    RCC_AHB1ENR |= (1 << 1);

    // PB0 출력 모드
    GPIOB_MODER &= ~(3 << (0 * 2));
    GPIOB_MODER |=  (1 << (0 * 2));

    // Push-pull
    GPIOB_OTYPER &= ~(1 << 0);

    BUZZER_OFF();
}

void Buzzer_Beep(int on_ms, int off_ms)
{
    BUZZER_ON();

    if (on_ms > 0)
        TIM2_Delay(on_ms);

    BUZZER_OFF();

    if (off_ms > 0)
        TIM2_Delay(off_ms);
}

// 숫자 3, 2용 짧은 삑
void Buzzer_Count_Short(void)
{
    Buzzer_Beep(80, 10);
}

// 숫자 1용 조금 더 긴 삑
void Buzzer_Count_Long(void)
{
    Buzzer_Beep(140, 10);
}

// 스타트용 레이싱 느낌
void Buzzer_Start_Race(void)
{
    Buzzer_Beep(50, 30);
    Buzzer_Beep(50, 30);
    Buzzer_Beep(50, 50);
    Buzzer_Beep(220, 10);
}

void MAX7219_ShowPattern(int module, unsigned char *pattern)
{
    for (int row = 0; row < 8; row++)
    {
        MAX7219_SendOne(module, row + 1, pattern[row]);
    }
}

void MAX7219_FillModulesRange(int from_module, int to_module)
{
    for (int module = from_module; module <= to_module; module++)
    {
        MAX7219_FillModule(module);
    }
}

void Ready_State(void)
{
    for (int i = 0; i < 5; i++)
    {
        MAX7219_ClearAll();
        MAX7219_FillModule(1);
        MAX7219_FillModule(2);
        TIM2_Delay(500);

        MAX7219_ClearAll();
        TIM2_Delay(500);
    }
}

void Countdown_State(void)
{
    // 숫자 3 + 2번 패널만 ON
    MAX7219_ClearAll();
    MAX7219_FillModule(2);
    MAX7219_ShowPattern(3, num_3);
    Buzzer_Count_Short();
    TIM2_Delay(1000);

    // 숫자 2 + 3번 패널만 ON
    MAX7219_ClearAll();
    MAX7219_FillModule(1);
    MAX7219_ShowPattern(3, num_2);
    Buzzer_Count_Short();
    TIM2_Delay(1000);

    // 숫자 1 + 4번 패널만 ON
    MAX7219_ClearAll();
    MAX7219_FillModule(0);
    MAX7219_ShowPattern(3, num_1);
    Buzzer_Count_Long();
    TIM2_Delay(150);

    // 전체 ON
    MAX7219_FillModulesRange(0, 3);
    TIM2_Delay(1000);

    // 전체 OFF
    MAX7219_ClearAll();
    Buzzer_Start_Race();
}

void Main(void)
{
    Clock_Init();
    GPIO_Init();
    Buzzer_Init();
    MAX7219_Init();
    MAX7219_ClearAll();

    while (1)
    {
        Ready_State();
        Countdown_State();

        while (1)
        {
        }
    }
}