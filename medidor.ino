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
#define RED_P A1
#define GREEN_P A2
#define BLUE_P A3

#define HR_STRING_SIZE 5

enum STATE_DB {READING, WAITING};
enum STATE_HR {READING_HR, READING_AGE};
enum STATE_DP {DISPLAYING};

struct SignalFSM
{
    STATE_DB state;
    unsigned char currentReading;
    unsigned char previousReading;
    unsigned long previousTime;
    unsigned char cleanSignal;
    int inputPin;
};

struct HeartRateFSM
{
    unsigned char current;
    unsigned char previous;
    unsigned long currentEdge;
    unsigned long previousEdge;
    unsigned int heartRateNum;
    STATE_HR state;
    int counter;
    int age;
    int maxHR;
};

struct DisplayFSM
{
    STATE_DP state = DISPLAYING;
    char previousWord[HR_STRING_SIZE] = "0000";
    unsigned char characters[HR_STRING_SIZE - 1];
    char word[HR_STRING_SIZE] = "0000";
    unsigned char decimal;
};

SignalFSM sFSM;
SignalFSM cFSM;
HeartRateFSM hRFSM;
DisplayFSM dFSM;

unsigned char segments[] = {SEGM_A, SEGM_B, SEGM_C, SEGM_D, SEGM_E, SEGM_F, SEGM_G, SEGM_P};
unsigned char digitPins[] = {DIG_1, DIG_2, DIG_3, DIG_4};
unsigned char colorPins[] = {RED_P, GREEN_P, BLUE_P};

/*
Tabla con los bytes que indican el encendido o apagado de un led para cierto caracter
*/
static unsigned char charTable[] =
    {
        0b01110111, //a
        0b01111100, //b
        0b00111001, //c
        0b01011110, //d
        0b01111001, //e
        0b01110001, //f
        0b01101111, //g
        0b01110110, //h
        0b00110000, //i
        0b00011110, //j
        0b00000000,
        0b00111000, //l
        0b00000000,
        0b01010100, //n
        0b00111111, //o
        0b01110011, //p
        0b01100111, //q
        0b01010000, //r
        0b01101101, //s
        0b01111000, //t
        0b00111110, //u
        0b00011100, //v
        0b00000000,
        0b00000000,
        0b01101110, //y
        0b01011011, //z
        0b00111111, //0
        0b00000110, //1
        0b01011011, //2
        0b01001111, //3
        0b01100110, //4
        0b01101101, //5
        0b01111101, //6
        0b00000111, //7
        0b01111111, //8
        0b01101111, //9
        0b00000000 //space
    };

/*
Funcion que transforma una palabra a un set de bytes que indican que 
pines encender o apagar por cada caracter
*/
void wordTo7S(char word[HR_STRING_SIZE], unsigned char characters[HR_STRING_SIZE - 1])
{
    for(int i = 0; i < HR_STRING_SIZE - 1; i++) //Se recorre la palabra
    {
        unsigned char value = word[HR_STRING_SIZE - i - 2]; //value toma el valor de un caracter
        if(value >= 97 && value <= 122) //Si el caracter es una letra
        {
            value -= 97;
        }
        else if(value >= 48 && value <= 57) //Si el caracter es un digito
        {
            value = value - 48 + 26;
        }
        else if(value == 32) //Si el caracter es el de espacio
        {
            value = 26 + 10;
        }
        characters[i] = charTable[value]; //Se busca la secuencia adecuada y se guarda en caracteres[i]
    }
}

void displayCharacters(unsigned char characters[HR_STRING_SIZE - 1])
{
    for (int i = 0; i < HR_STRING_SIZE - 1; i++) //Se recorren los caracteres
    {
        for (int j = 0; j < 4; j++) //Se apagan todos los pines de digitos
        {
            digitalWrite(digitPins[j], LOW);
        }
        if ((3 - i) < 4 && (3 - i) >= 0) //Si el caracter se mostraria en el display
        {
            digitalWrite(digitPins[3 - i], HIGH);
            for (int j = 7; j >= 0; j--) //Se recorren los bits del character
            {
                int turnOn = (characters[i] >> j) & 0x01; //Se guarda el bit en la posicion j del caracter en turnOn
                digitalWrite(segments[j], !turnOn); //Se pone el pin en segmentos[j] en el modo opuesto a turnOn
            }
            delay(4); //Se espera 4 milisegundos
        }
    }
}

/*
Funcion que agrega un decimal al tercer caracter que se muestra en el display
*/
void addDecimal(unsigned char characters[HR_STRING_SIZE - 1])
{
   
    characters[1] = characters[1] | 0b10000000;
        
}

/*
Inicializacion de variable del tipo DisplayFSM
*/
void initializeDisplayFSM(DisplayFSM *displayFSM)
{
    displayFSM->state = DISPLAYING;
    displayFSM->decimal = 0;
}

/*
Funcion que hace display a la palabra que se encuentra como atributo en el argumento
displayFSM
*/
void display(DisplayFSM *displayFSM)
{
    
    switch (displayFSM->state)
    {
    case DISPLAYING:
        if(strcmp(displayFSM->previousWord, displayFSM->word) != 0) //Si la palabra anterior es distinta a la actual
        {
            strcpy(displayFSM->previousWord, displayFSM->word); //La palabra actual se guarda en la palabra anterior
            wordTo7S(displayFSM->word, displayFSM->characters); //Se pasa la palabra al modo para hacer display en el 7 segmentos
            if(displayFSM->decimal) //Si la palabra debe llevar decimal 
            {
                addDecimal(displayFSM->characters); //Se agrega el decimal
            } 
        }
        else
        {
            displayCharacters(displayFSM->characters); //Se le hace display a los caracteres de la palabra
        }
        break;
    default:
        break;
    }
}

/*
Inicializacion de variable del tipo signalFSM
*/
void initializeSignalFSM(SignalFSM *signalFSM, int inputPin)
{
    signalFSM->state = READING;
    signalFSM->inputPin = inputPin;
    pinMode(signalFSM->inputPin, INPUT);
    signalFSM->currentReading = digitalRead(signalFSM->inputPin);
    signalFSM->previousReading = signalFSM->currentReading;
    signalFSM->cleanSignal = LOW;
}

void deBouncer(SignalFSM *signalFSM)
{
    int extintion = 50;

    switch (signalFSM->state)
    {
    case READING:
        if(signalFSM->previousReading == signalFSM->currentReading) //Si la lectura anterior es igual a la lectura actual
        {
            signalFSM->currentReading = digitalRead(signalFSM->inputPin); //Se actualiza el valor de la lectura actual
        }
        else
        {
            signalFSM->previousTime = millis(); //Se actualiza el valor de los milisegundos anteriores
            signalFSM->cleanSignal = signalFSM->currentReading; //Se cambia el valor de la señal limpia al de la lectura actual
            signalFSM->state = WAITING; //Se cambia el estado al de espera
        }
        break;
    case WAITING:
        if(millis() - signalFSM->previousTime >= extintion) //Se espera hasta que se cumpla el requerimiento de tiempo
        {
            signalFSM->previousReading = signalFSM->currentReading;
            signalFSM->currentReading = digitalRead(signalFSM->inputPin);
            signalFSM->state = READING; //Se cambia el estado al de lectura
        }
    default:
        break;
    }
}
/*
Funcion que combierte un numero a un string
*/
void convertNumberToString(int number, char word[HR_STRING_SIZE])
{
    //Depende del tamaño del numero se agregan un numero de ceros al frente
    if(number >= 1000)
    {
        sprintf(word, "%d", number);
    }
    else if(number >= 100)
    {
        sprintf(word, "0%d", number);

    }
    else if(number >= 10)
    {
        sprintf(word, "00%d", number);
    }
    else
    {
        sprintf(word, "000%d", number);
    }
}

/*
Funcion que enciende el led rgb dado la entrada para el rojo, verde y azul
*/
void displayRGB(unsigned char red, unsigned char green, unsigned char blue)
{
    analogWrite(RED_P, red);
    analogWrite(GREEN_P, green);
    analogWrite(BLUE_P, blue);
}

/*
Inicializacion de variable del tipo heartRateFSM
*/
void initialiseHeartRateFSM(HeartRateFSM *heartRateFSM)
{
    heartRateFSM->current = LOW;
    heartRateFSM->previous = heartRateFSM->current;
    heartRateFSM->previousEdge = millis();
    heartRateFSM->heartRateNum = 0;
    heartRateFSM->state = READING_AGE;
    heartRateFSM->counter = 0;
    heartRateFSM->maxHR = 0;
    heartRateFSM->age = 10;
}

/*
Funcion que calcula el ritmo cardiaco dado signalFSM y que guarda el resultado en displayFSM
*/
void heartRateMonitor(HeartRateFSM *heartRateFSM, SignalFSM *signalFSM, DisplayFSM *displayFSM)
{
    switch (heartRateFSM->state)
    {
    case READING_HR:
        if(heartRateFSM->previous == LOW && heartRateFSM->current == HIGH) //Si la señal cambio de LOW a HIGH
        {
            heartRateFSM->currentEdge = millis(); //Se actualiza el tiempo del pico actual a los milisegundos actuales
            heartRateFSM->heartRateNum += heartRateFSM->currentEdge - heartRateFSM->previousEdge; //Se suma la differencia entre el tiempo del pico actual y el pico anterior a hearRateNum
            heartRateFSM->counter++; //Se incrementa el contador
            heartRateFSM->previousEdge = heartRateFSM->currentEdge; //Se actualiza el valor del tiempo del pico anterior al del actual
        }
        if(heartRateFSM->counter == 3) //Si el contador llega a 3
        {
            heartRateFSM->counter = 0; //Se retorna el contador a 0
            heartRateFSM->heartRateNum /= 3; //Se divide lo que hay en heartRateNum en 3 (promedio)
            heartRateFSM->heartRateNum = 600000 / heartRateFSM->heartRateNum; //Se calcula el ritmo cardiaco dado su periodo

            //Se enciende el led rgb de un color dado el ritmo cardiaco actual y el maximo
            if(heartRateFSM->heartRateNum <= heartRateFSM->maxHR * 0.5 * 10)
            {
                displayRGB(0, 0, 255);
            }
            else if(heartRateFSM->heartRateNum <= heartRateFSM->maxHR * 0.7 * 10)
            {
                displayRGB(0, 255, 255);
            }
            else if (heartRateFSM->heartRateNum <= heartRateFSM->maxHR * 0.85 * 10)
            {
                displayRGB(0, 255, 0);
            }
            else if (heartRateFSM->heartRateNum <= heartRateFSM->maxHR * 1 * 10)
            {
                displayRGB(255, 255, 0);
            }
            else
            {
                displayRGB(255, 0, 0);
            }
            convertNumberToString(heartRateFSM->heartRateNum, displayFSM->word); //Se transforma el ritmo cardiaco actual a string y se guarda en la estructura displayFSM
            displayFSM->decimal = 1; //Se le indica a la estructura displayFSM que la palabra se imprimira con decimal
            heartRateFSM->heartRateNum = 0; //Se reinicia hearRateNum en 0
        }
        heartRateFSM->previous = heartRateFSM->current; //La señal previa toma el valor de la actual
        heartRateFSM->current = signalFSM->cleanSignal; //La señal actual toma el valor de la señal limpia en la estructura signalFSM
        break;
    
    default:
        break;
    }
}


void inputAge(HeartRateFSM *heartRateFSM, SignalFSM *signalFSM, DisplayFSM *displayFSM)
{  
    switch (heartRateFSM->state)
    {
    case READING_AGE:
        if(heartRateFSM->previous == LOW && heartRateFSM->current == HIGH) //Si la señal anterior es LOW y la actual es HIGH
        {
            heartRateFSM->age++; //Se incrementa la edad
            convertNumberToString(heartRateFSM->age, displayFSM->word); //Se convierte la edad a un string y se guarda en la estructura displayFSM
        }
        heartRateFSM->previous = heartRateFSM->current; //La señal actual se guarda en la previa
        heartRateFSM->current = signalFSM->cleanSignal; //La señal actual se actualiza a la señal limpia de la estructura signalFSM
        break;
    
    default:
        break;
    }
}

void setup()
{
    for(int i = 0; i < 8; i++)
    {
        pinMode(segments[i], OUTPUT);
    }
    for(int i = 0; i < 4; i++)
    {
        pinMode(digitPins[i], OUTPUT);
    }
    for(int i = 0; i < 3; i++)
    {
        pinMode(colorPins[i], OUTPUT);
    }

    initializeDisplayFSM(&dFSM);
    initializeSignalFSM(&sFSM, A5);
    initializeSignalFSM(&cFSM, A4);
    initialiseHeartRateFSM(&hRFSM);
    convertNumberToString(hRFSM.age, dFSM.word);
    char intro[HR_STRING_SIZE] = "edad";
    unsigned char characters[HR_STRING_SIZE - 1];
    wordTo7S(intro, characters);
    unsigned long previousTime = millis();
    displayRGB(0, 110, 0);
    while(millis() - previousTime < 3000)
    {
        displayCharacters(characters);
    }

}

void loop()
{
    if(hRFSM.maxHR == 0)
    {
        deBouncer(&sFSM);
        deBouncer(&cFSM);
        inputAge(&hRFSM, &sFSM, &dFSM);
        display(&dFSM);
        if(cFSM.cleanSignal == HIGH)
        {
            hRFSM.maxHR = 220 - hRFSM.age;
            convertNumberToString(0, dFSM.word);
            hRFSM.state = READING_HR;
        }
    }
    else if(hRFSM.age > 1)
    {
        deBouncer(&sFSM);
        heartRateMonitor(&hRFSM, &sFSM, &dFSM);
        display(&dFSM);
    }
    
}