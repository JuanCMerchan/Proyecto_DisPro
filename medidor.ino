#include <string.h>

#define DIG_1 13
#define DIG_2 12
#define DIG_3 11
#define DIG_4 10
#define SEGM_A 2
#define SEGM_B 3
#define SEGM_C 4
#define SEGM_D 5
#define SEGM_E 6
#define SEGM_F 7
#define SEGM_G 8
#define SEGM_P 9

#define READ 1

#define HR_STRING_SIZE 5

enum STATE_DB {READING, WAITING};
enum STATE_HR {READING_HR};
enum STATE_DP {DISPLAYING};

unsigned char bouncedSignal;
unsigned char cleanSignal;
char heartRate[HR_STRING_SIZE] = "0000";

unsigned char segments[] = {SEGM_A, SEGM_B, SEGM_C, SEGM_D, SEGM_E, SEGM_F, SEGM_G, SEGM_P};
unsigned char digitPins[] = {DIG_1, DIG_2, DIG_3, DIG_4};

static unsigned char digitTable[] =
    {
        0b01110111,
        0b01111100,
        0b00111001,
        0b01011110,
        0b00111111,
        0b00000110,
        0b01011011,
        0b01001111,
        0b01100110,
        0b01101101,
        0b01111101,
        0b00000111,
        0b01111111,
        0b01101111
    };

void heartRateTo7S(unsigned char digits[HR_STRING_SIZE - 1])
{
    for(int i = 0; i < HR_STRING_SIZE; i++)
    {
        unsigned char value = heartRate[HR_STRING_SIZE - i - 2];
        if(value >= 97 && value <= 100)
        {
            value -= 97;
        }
        else
        {
            value = value - 48 + 4;
        }
        digits[i] = digitTable[value];
        if(i == 1)
        {
            digits[i] = digits[i] | 0b10000000;
        }
        
    }
}

void displayDigits(unsigned char digits[HR_STRING_SIZE - 1])
{
    for (int i = 0; i < HR_STRING_SIZE - 1; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            digitalWrite(digitPins[j], LOW);
        }
        if ((3 - i) < 4 && (3 - i) >= 0)
        {
            digitalWrite(digitPins[3 - i], HIGH);
            for (int j = 7; j >= 0; j--)
            {
                int turnOn = (digits[i] >> j) & 0x01;
                digitalWrite(segments[j], !turnOn);
            }
            delay(4);
        }
    }
}

void display()
{
    static STATE_DP state = DISPLAYING;
    static unsigned long currentMillis = millis();
    static unsigned long previousMillis = currentMillis;
    static char previousHr[HR_STRING_SIZE] = "    ";
    static unsigned char digits[HR_STRING_SIZE - 1];
    switch (state)
    {
    case DISPLAYING:
        if(strcmp(previousHr, heartRate) != 0)
        {
            strcpy(previousHr, heartRate);
            heartRateTo7S(digits);
        }
        else
        {
            displayDigits(digits);
        }
        break;
    default:
        break;
    }
}

void deBouncer()
{
    static STATE_DB state = READING; 
    static unsigned char currentReading = digitalRead(READ);
    static unsigned char previousReading = currentReading;
    static unsigned long previousTime;
    int extintion = 50;

    switch (state)
    {
    case READING:
        if(previousReading == currentReading)
        {
            currentReading = digitalRead(READ);
        }
        else
        {
            previousTime = millis();
            cleanSignal = currentReading;
            state = WAITING;
        }
        break;
    case WAITING:
        if(millis() - previousTime >= extintion)
        {
            previousReading = currentReading;
            currentReading = digitalRead(READ);
            state = READING;
        }
    default:
        break;
    }
}

void convertHeartRateToString(int heartRateNum)
{
    if(heartRateNum >= 1000)
    {
        sprintf(heartRate, "%d", heartRateNum);
    }
    else
    {
        sprintf(heartRate, "0%d", heartRateNum);

    }
}

void heartRateMonitor()
{
    static unsigned char current = LOW;
    static unsigned char previous = current;
    static unsigned long currentEdge;
    static unsigned long previousEdge = millis();
    static unsigned int heartRateNum = 0;
    static STATE_HR state = READING_HR;
    static int counter = 0;

    switch (state)
    {
    case READING_HR:
        if(previous == LOW && current == HIGH)
        {
            currentEdge = millis();
            heartRateNum += currentEdge - previousEdge;
            counter++;
            previousEdge = currentEdge;
        }
        if(counter == 3)
        {
            counter = 0;
            heartRateNum /= 3;
            heartRateNum = 600000 / heartRateNum;
            convertHeartRateToString(heartRateNum);
            heartRateNum = 0;
        }
        previous = current;
        current = cleanSignal;
        break;
    
    default:
        break;
    }
}

void setup()
{
    pinMode(DIG_1, OUTPUT);
    pinMode(DIG_2, OUTPUT);
    pinMode(DIG_3, OUTPUT);
    pinMode(DIG_4, OUTPUT);
    pinMode(SEGM_A, OUTPUT);
    pinMode(SEGM_B, OUTPUT);
    pinMode(SEGM_C, OUTPUT);
    pinMode(SEGM_D, OUTPUT);
    pinMode(SEGM_E, OUTPUT);
    pinMode(SEGM_F, OUTPUT);
    pinMode(SEGM_G, OUTPUT);
    pinMode(SEGM_P, OUTPUT);
    pinMode(READ, INPUT);

    bouncedSignal = 0;
    cleanSignal = 0;
}

void loop()
{
    deBouncer();
    heartRateMonitor();
    display();
}