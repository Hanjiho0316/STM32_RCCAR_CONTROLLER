#include "device_driver.h"
#include <stdio.h>
#define RCC_AHB1ENR    (*(volatile unsigned int*)0x40023830)
#define GPIOB_MODER    (*(volatile unsigned int*)0x40020400)
#define GPIOB_OTYPER   (*(volatile unsigned int*)0x40020404)
#define GPIOB_ODR      (*(volatile unsigned int*)0x40020414)

void Adc_Init(void);
unsigned int Adc_Read(int channel);
int Check_False_Start_Detail(unsigned int threshold);
int Countdown_State(unsigned int threshold);
void Ready_State(void);

// ---------------------------------
// 패널 번호 기준
// 4번 패널 = 맨위    = module 0
// 3번 패널 = 그 아래 = module 1
// 2번 패널 = 그 아래 = module 2
// 1번 패널 = 맨아래 = module 3
// ---------------------------------

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

unsigned char char_F[8] = {
    0x00,
    0xFF, // 전체 기둥
    0x89, // 맨 위 가로줄 + 중간 가로줄
    0x89,
    0x89,
    0x81, // 맨 위 가로줄 끝부분
    0x81,
    0x00
};

// 'S' 패턴
unsigned char char_S[8] = {
    0x00,
    0x46, // 아랫부분 곡선
    0x89, // 가로줄들
    0x89,
    0x89,
    0x89,
    0x72, // 윗부분 곡선
    0x00
};

// 'P' 패턴
unsigned char char_P[8] = {
    0x00,
    0xFF, // 전체 기둥
    0x09, // 중간 가로줄 + 위 가로줄
    0x09,
    0x09,
    0x09,
    0x06, // 머리 부분 곡선
    0x00
};

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



// 0: 정상, 1: P1 부정, 2: P2 부정, 3: 둘 다 부정
int Check_False_Start_Detail(unsigned int threshold)
{
    int s1 = (Adc_Read(0) > threshold); // PA0가 밝아지면 1
    int s2 = (Adc_Read(1) > threshold); // PA1이 밝아지면 1

    if (s1 && s2) return 3; // 둘 다 출발 (FS)
    if (s1) return 1;       // Player 1만 출발 (FSP1)
    if (s2) return 2;       // Player 2만 출발 (FSP2)
    return 0;               // 둘 다 아직 정지 상태
}

int Countdown_State(unsigned int threshold)
{
    int fs = 0;

    // --- 숫자 3 ---
    MAX7219_ClearAll();
    MAX7219_FillModule(2);
    MAX7219_ShowPattern(3, num_3);
    Buzzer_Count_Short();
    for(int i = 0; i < 10; i++) {
        fs = Check_False_Start_Detail(threshold);
        if(fs > 0) return fs; 
        TIM2_Delay(100);
    }

    // --- 숫자 2 ---
    MAX7219_ClearAll();
    MAX7219_FillModule(1);
    MAX7219_ShowPattern(3, num_2);
    Buzzer_Count_Short();
    for(int i = 0; i < 10; i++) {
        fs = Check_False_Start_Detail(threshold);
        if(fs > 0) return fs; 
        TIM2_Delay(100);
    }

    // --- 숫자 1 ---
    MAX7219_ClearAll();
    MAX7219_FillModule(0);
    MAX7219_ShowPattern(3, num_1);
    Buzzer_Count_Long();
    for(int i = 0; i < 10; i++) {
        fs = Check_False_Start_Detail(threshold);
        if(fs > 0) return fs; 
        TIM2_Delay(15);
    }

    // --- 전체 ON (GO 직전 찰나의 대기) ---
    MAX7219_FillModulesRange(0, 3);
    for(int i = 0; i < 10; i++) {
        fs = Check_False_Start_Detail(threshold);
        if(fs > 0) return fs; 
        TIM2_Delay(100);
    }

    // --- 전체 OFF 및 시작 ---
    MAX7219_ClearAll();
    Buzzer_Start_Race();
    return 0; // 정상 출발 완료
}

void Adc_Init(void)
{
    // 1. GPIOA 및 ADC1 클럭 활성화
    Macro_Set_Bit(RCC->AHB1ENR, 0);   // GPIOA ON
    Macro_Set_Bit(RCC->APB2ENR, 8);   // ADC1 ON

    // 2. PA0, PA1 아날로그 모드 설정 (11)
    Macro_Write_Block(GPIOA->MODER, 0xf, 0xf, 0); // PA0, PA1 => Analog Mode

    // 3. ADC 설정
    ADC1->CR1 = 0; // 초기화
    ADC1->CR2 = (1<<0); // ADON: ADC 켜기
    
    // Sampling Time 설정 (충분히 길게 줘서 안정화)
    ADC1->SMPR2 |= (0x7 << 0) | (0x7 << 3); // PA0, PA1에 대해 최대 샘플링 타임
}

unsigned int Adc_Read(int channel)
{
    ADC1->SQR3 = channel;          // 읽을 채널 선택 (0 또는 1)
    ADC1->CR2 |= (1 << 30);        // SWSTART: 변환 시작
    
    while(!(ADC1->SR & (1 << 1))); // EOC: 변환 완료 대기
    
    return ADC1->DR;               // 결과값 반환 (0~4095)
}

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
    
    setvbuf(stdout, NULL, _IONBF, 0);
}


void Main(void)
{
    unsigned int sensor1, sensor2;
    unsigned int threshold = 2000; 

    // 시스템 초기화
    Clock_Init();
    GPIO_Init();
    Key_Poll_Init();    // 버튼 초기화 추가
    Buzzer_Init();
    MAX7219_Init();
    MAX7219_ClearAll();
    Sys_Init(38400); 
    Adc_Init(); 
    
    printf("Racing Track Start System Initialized...\n");

    while(1)
    {
        sensor1 = Adc_Read(0); // PA0
        sensor2 = Adc_Read(1); // PA1
        printf("S1: %d | S2: %d\n", sensor1, sensor2);

        // 1. 차량 정위치 확인 (조도 센서 둘 다 어두움)
        if(sensor1 < threshold && sensor2 < threshold)
        {
            // 차량 감지 시 "대기" 상태 표시
            Ready_State(); 
            
            // 2. 버튼이 눌렸는지 확인 (PC13)
            if(Key_Get_Pressed()) 
            {
                printf(">> [START] Button Clicked! <<\n");
                
                // 버튼에서 손을 뗄 때까지 대기 (중복 실행 방지)
                Key_Wait_Key_Released(); 
                
                // 3. 카운트다운 및 레이스 시작
                int result = Countdown_State(threshold);
                if(result > 0) // 부정출발 발생 시
                {
                    // 1. 부저 3번 길게 울림 (수동 제어)
                    for(int i = 0; i < 3; i++) {
                        Buzzer_Count_Short();
                    }

                    // 2. 상황에 맞는 메시지 출력
                    MAX7219_ClearAll();
                    if(result == 1) {
                        printf("FSP1 Detected!\n");
                        // 모듈별로 F, S, P, 1을 각각 출력하는 예시
                        MAX7219_ShowPattern(0, char_F); 
                        MAX7219_ShowPattern(1, char_S);
                        MAX7219_ShowPattern(2, char_P);
                        MAX7219_ShowPattern(3, num_1);
                    }
                    else if(result == 2) {
                        printf("FSP2 Detected!\n");
                        MAX7219_ShowPattern(0, char_F);
                        MAX7219_ShowPattern(1, char_S);
                        MAX7219_ShowPattern(2, char_P);
                        MAX7219_ShowPattern(3, num_2);
                    }
                    else if(result == 3) {
                        printf("Both FS Detected!\n");
                        MAX7219_ShowPattern(1, char_F);
                        MAX7219_ShowPattern(2, char_S);
                    }
                    TIM2_Delay(3000); // 3초간 경고 유지
                }
                
                // 카운트다운 직후 차량이 바로 출발하므로, 
                // 센서에서 차량이 완전히 벗어날 때까지 잠시 대기하거나 루프를 조정합니다.
                for(volatile int i = 0; i < 2000000; i++); 
            }
        }
        else
        {
            // 차량이 없으면 대기 화면 유지 혹은 클리어
            MAX7219_ClearAll();
        }

        // TeraTerm 출력 속도 조절
        for(volatile int i = 0; i < 500000; i++); 
    }
}

