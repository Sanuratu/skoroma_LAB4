/*
 * File:   TEST.c
 * Author: Sanuratu Koroma
 * Created on March 10, 2021, 9:07 AM
 */
#include <xc.h>
#define _XTAL_FREQ 8000000
#define RS RD2
#define EN RD3
#define D4 RD4
#define D5 RD5
#define D6 RD6
#define D7 RD7
//Temp
#define DHT22_PIN      RB1
#define DHT22_PIN_DIR  TRISB1


#pragma config FOSC = HS        // Oscillator Selection bits (HS oscillator)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled)
#pragma config PWRTE = ON       // Power-up Timer Enable bit (PWRT enabled)
#pragma config BOREN = ON       // Brown-out Reset Enable bit (BOR enabled)
#pragma config LVP = OFF        // Low-Voltage (Single-Supply) In-Circuit Serial Programming Enable bit (RB3 is digital I/O, HV on MCLR must be used for programming)
#pragma config CPD = OFF        // Data EEPROM Memory Code Protection bit (Data EEPROM code protection off)
#pragma config WRT = OFF        // Flash Program Memory Write Enable bits (Write protection off; all program memory may be written to by EECON control)
#pragma config CP = OFF         // Flash Program Memory Code Protection bit (Code protection off)

//void interrupt_ISR() {
//  if (PIR1bits.TXIF == 1) {
//    PORTBbits.RB0 = 1;
//  PORTAbits.RA0 = 1;
//PIR1bits.TXIF = 0;

// }
//}

unsigned int get_count(void);
unsigned int value;
int ADC_read(void);
void init_ADC(void);
void Lcd_SetBit(char data_bit);
void Lcd_Cmd(char a);
void Lcd_Set_Cursor(char a, char b);
void Lcd_Clear();
void Lcd_Start();
void Lcd_Print_Char(char data);
void Lcd_Print_String(char *a);
void __interrupt() ISR_example();
int arr_dec[17] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
int arr_hex[3] = {}; // ARRAY TO HOLD INDIVIDUAL HEX VALUE
char decimal_data[3] = {}; // ARRAY TO STORE INDIVIDUAL DECIMAL VALUE
int power(int, int); // FUNCTION TO CALCULATE POWER ("RAISE TO")

int number = 0;
int distance = 0;
char ch1, ch2, ch3, ch4;

//Temperature
char Temperature[] = "Temp = 00.0 C  ";
char Humidity[] = "RH   = 00.0 %  ";
unsigned char T_Byte1, T_Byte2, RH_Byte1, RH_Byte2, CheckSum;
unsigned int Temp, RH;

// send start signal to the sensor

void Start_Signal(void) {
    DHT22_PIN_DIR = 0; // configure DHT22_PIN as output
    DHT22_PIN = 0; // clear DHT22_PIN output (logic 0)

    __delay_ms(25); // wait 25 ms
    DHT22_PIN = 1; // set DHT22_PIN output (logic 1)

    __delay_us(30); // wait 30 us
    DHT22_PIN_DIR = 1; // configure DHT22_PIN as input
}

// Check sensor response

__bit Check_Response() {
    TMR1H = 0; // reset Timer1
    TMR1L = 0;
    TMR1ON = 1; // enable Timer1 module

    while (!DHT22_PIN && TMR1L < 100); // wait until DHT22_PIN becomes high (checking of 80µs low time response)

    if (TMR1L > 99) // if response time > 99µS  ==> Response error
        return 0; // return 0 (Device has a problem with response)

    else {
        TMR1H = 0; // reset Timer1
        TMR1L = 0;

        while (DHT22_PIN && TMR1L < 100); // wait until DHT22_PIN becomes low (checking of 80µs high time response)

        if (TMR1L > 99) // if response time > 99µS  ==> Response error
            return 0; // return 0 (Device has a problem with response)

        else
            return 1; // return 1 (response OK)
    }
}

// Data read function

__bit Read_Data(unsigned char* dht_data) {
    *dht_data = 0;

    for (char i = 0; i < 8; i++) {
        TMR1H = 0; // reset Timer1
        TMR1L = 0;

        while (!DHT22_PIN) // wait until DHT22_PIN becomes high
            if (TMR1L > 100) { // if low time > 100  ==>  Time out error (Normally it takes 50µs)
                return 1;
            }

        TMR1H = 0; // reset Timer1
        TMR1L = 0;

        while (DHT22_PIN) // wait until DHT22_PIN becomes low
            if (TMR1L > 100) { // if high time > 100  ==>  Time out error (Normally it takes 26-28µs for 0 and 70µs for 1)
                return 1; // return 1 (timeout error)
            }

        if (TMR1L > 50) // if high time > 50  ==>  Sensor sent 1
            *dht_data |= (1 << (7 - i)); // set bit (7 - i)
    }

    return 0; // return 0 (data read OK)
}

int main() {
    unsigned int count;
    unsigned int countTotal;
    float divideValue = 12.7875;
    int temp;
    TRISA = 0x01;
    TRISA0 = 1; //sets port RA0 as input for the LM35
    int maxtemp;
    maxtemp = 40;
    TRISD = 0x00; //PORTD declared as output for interfacing LCD
    TRISB0 = 1; //DEfine the RB0 pin as input to use as interrupt pin
    OPTION_REG = 0b00000000; // Enables PULL UPs
    //    ADIE = 1;
    GIE = 1; //Enable Global Interrupt
    PEIE = 1; //Enable the Peripheral Interrupt
    INTE = 1; //Enable RB0 as external Interrupt pin
    init_ADC();

    //Temp
    T1CON = 0x10; // set Timer1 clock source to internal with 1:2 prescaler (Timer1 clock = 1MHz)
    TMR1H = 0; // reset Timer1
    TMR1L = 0;

    __delay_ms(2000); // wait 1 second
    Lcd_Start();

    while (1) {
        value = ADC_read(); // READ ADC FROM CHANNEL 1; VALUE WILL BE 
        // STORED IN HEXDECIMAL FORM
        for (int i = 0; i < 3; i++) {
            arr_hex[i] = (value % 16); //SEPERATE INDIVIDUAL HEX BITS 
            temp = arr_hex[i];
            value /= 16;
            decimal_data[i] = arr_dec[temp]; // COMPARE HEX BITS WITH DECIMAL BIT ARRAY

            // TO STORE DECIMAL VALUES IN DECIMAL_DATA 
        }


        count = get_count(); //FUNCTION TO CONVERT HEXADECIMAL VALUE TO DECIMAL  
        countTotal = count / divideValue;
        if (countTotal == 80)
            T0IE = 1;
        Lcd_Clear();
        Lcd_Set_Cursor(1, 1);
        Lcd_Print_String("Speed: ");
        Lcd_Print_Char((countTotal / 1000) + 48); // SEPERATE BITS AND PRINT
        countTotal = countTotal % 1000;
        Lcd_Print_Char((countTotal / 100) + 48);
        countTotal = countTotal % 100;
        Lcd_Print_Char((countTotal / 10) + 48);
        countTotal = countTotal % 10;
        Lcd_Print_Char((countTotal) + 48);


        Lcd_Set_Cursor(2, 1);
        Lcd_Print_String("Distance: ");
        ch1 = (distance / 1000) % 10;
        ch2 = (distance / 100) % 10;
        ch3 = (distance / 10) % 10;
        ch4 = (distance / 1) % 10;

        Lcd_Print_Char(ch1 + '0');
        Lcd_Print_Char(ch2 + '0');
        Lcd_Print_Char(ch3 + '0');
        Lcd_Print_Char(ch4 + '0');

        __delay_ms(2000);
        number++;
        distance = number * count;
        //Temperature
        Start_Signal(); // send start signal to the sensor

        if (Check_Response()) // check if there is a response from sensor (If OK start reading humidity and temperature data)
        {
            // read (and save) data from the DHT22 sensor and check time out errors
            if (Read_Data(&RH_Byte1) || Read_Data(&RH_Byte2) || Read_Data(&T_Byte1) || Read_Data(&T_Byte2) || Read_Data(&CheckSum)) {
                Lcd_Clear();
                Lcd_Print_String("Error ");
            } else // if there is no time out error
            {
                if (CheckSum == ((RH_Byte1 + RH_Byte2 + T_Byte1 + T_Byte2) & 0xFF)) { // if there is no checksum error
                    RH = RH_Byte1;
                    RH = (RH << 8) | RH_Byte2;
                    Temp = T_Byte1;
                    Temp = (Temp << 8) | T_Byte2;
                    __delay_ms(2000);
                    if (Temp > 0X8000) { // if temperature < 0
                        Temperature[6] = '-'; // put minus sign '-'
                        Temp = Temp & 0X7FFF;
                        Lcd_Clear();
                        Lcd_Set_Cursor(1, 1);
                        Lcd_Print_String(Temperature);



                    } else // otherwise (temperature > 0)

                    Temperature[6] = ' '; // put space ' '
                    Temperature[7] = (Temp / 100) % 10 + '0';
                    Temperature[8] = (Temp / 10) % 10 + '0';
                    Temperature[10] = Temp % 10 + '0';
                    Temperature[11] = 223; // put degree symbol (°)
                    Lcd_Clear();
                    Lcd_Set_Cursor(1, 1);
                    Lcd_Print_String(Temperature);
                    //Lcd_Print_String("High Temperature");
                    //Lcd_Set_Cursor(2, 1);
                    //Lcd_Print_String("Check Coolant");
                    __delay_ms(2000);

                }
            }
        }


    }
    Lcd_Clear();
    return 0;
}

void init_ADC(void) {
    ADCON0 = 0x41; // Selecting ADC clock as Fosc/8; Channel 0; Enabling ADC module
    ADCON1 = 0x80; // Setting voltage reference to VSS and VDD: Right justified
}

int ADC_read(void) {
    ADCON0 = 0x41;
    GO_nDONE = 1;
    while (!GO_nDONE);
    return ((ADRESH << 8) + ADRESL);

}

int power(int x, int y) // FUNCTION TO CALCULATE POWER
{
    unsigned int result = 1;
    while (y != 0) {
        result = result*x;
        y--;
    }
    return result;
}

unsigned int get_count(void) // FUNCTION TO CONVERT HEX NUMBER IN DECIMAL
{
    int i;
    unsigned int count = 0;
    for (i = 2; i >= 0; i--) {
        count += (decimal_data[i] * power(16, i)); // FORMULA FOR CONVERSION
    }

    return count;
}

void Lcd_SetBit(char data_bit) //Based on the Hex value Set the Bits of the Data Lines
{
    if (data_bit & 1)
        D4 = 1;
    else
        D4 = 0;

    if (data_bit & 2)
        D5 = 1;
    else
        D5 = 0;

    if (data_bit & 4)
        D6 = 1;
    else
        D6 = 0;

    if (data_bit & 8)
        D7 = 1;
    else
        D7 = 0;
}

void Lcd_Cmd(char a) {
    RS = 0;
    Lcd_SetBit(a); //Incoming Hex value
    EN = 1;
    __delay_ms(4);
    EN = 0;
}

void Lcd_Clear() {
    Lcd_Cmd(0); //Clear the LCD
    Lcd_Cmd(1); //Move the curser to first position
}

void Lcd_Set_Cursor(char a, char b) {
    char temp, z, y;
    if (a == 1) {
        temp = 0x80 + b - 1; //80H is used to move the curser
        z = temp >> 4; //Lower 8-bits
        y = temp & 0x0F; //Upper 8-bits
        Lcd_Cmd(z); //Set Row
        Lcd_Cmd(y); //Set Column
    } else if (a == 2) {
        temp = 0xC0 + b - 1;
        z = temp >> 4; //Lower 8-bits
        y = temp & 0x0F; //Upper 8-bits
        Lcd_Cmd(z); //Set Row
        Lcd_Cmd(y); //Set Column
    }
}

void Lcd_Start() {
    Lcd_SetBit(0x00);
    for (int i = 1065244; i <= 0; i--) NOP();
    Lcd_Cmd(0x03);
    __delay_ms(5);
    Lcd_Cmd(0x03);
    __delay_ms(11);
    Lcd_Cmd(0x03);
    Lcd_Cmd(0x02); //02H is used for Return home -> Clears the RAM and initializes the LCD
    Lcd_Cmd(0x02); //02H is used for Return home -> Clears the RAM and initializes the LCD
    Lcd_Cmd(0x08); //Select Row 1
    Lcd_Cmd(0x00); //Clear Row 1 Display
    Lcd_Cmd(0x0C); //Select Row 2
    Lcd_Cmd(0x00); //Clear Row 2 Display
    Lcd_Cmd(0x06);
}

void Lcd_Print_Char(char data) //Send 8-bits through 4-bit mode
{
    char Lower_Nibble, Upper_Nibble;
    Lower_Nibble = data & 0x0F;
    Upper_Nibble = data & 0xF0;
    RS = 1; // => RS = 1
    Lcd_SetBit(Upper_Nibble >> 4); //Send upper half by shifting by 4
    EN = 1;
    for (int i = 2130483; i <= 0; i--) NOP();
    EN = 0;
    Lcd_SetBit(Lower_Nibble); //Send Lower half
    EN = 1;
    for (int i = 2130483; i <= 0; i--) NOP();
    EN = 0;
}

void Lcd_Print_String(char *a) {
    int i;
    for (i = 0; a[i] != '\0'; i++)
        Lcd_Print_Char(a[i]); //Split the string using pointers and call the Char function 
}

//Interrupt function **/

void __interrupt() ISR_example() {
    if (INTF == 1) //External Interrupt detected
    {
        Lcd_Clear();
        Lcd_Set_Cursor(1, 1);
        Lcd_Print_String("  Ext. ISR");
        Lcd_Set_Cursor(2, 1);
        Lcd_Print_String("  Drive Safely");
        INTF = 0; // clear the interrupt flag after done with it
        __delay_ms(2000);
        Lcd_Clear();
    }

    if (T0IE == 1) {
        Lcd_Clear();
        Lcd_Set_Cursor(1, 1);
        Lcd_Print_String("  Max speed Reached");
        Lcd_Set_Cursor(2, 1);
        Lcd_Print_String("  Drive Safely");
        T0IE = 0; // clear the interrupt flag after done with it
        __delay_ms(2000);
        Lcd_Clear();
    }

    //if (Temperature > 40){
    //Lcd_Clear();
    //Lcd_Set_Cursor(1, 1);
    //Lcd_Print_String("  Temp limit ");
    //__delay_ms(2000);
    //Lcd_Clear();
    //Lcd_Set_Cursor(1, 1);
    //Lcd_Print_String("High Temperature");
    //Lcd_Set_Cursor(2, 1);
    //Lcd_Print_String("Check Coolant");
   // __delay_ms(1000);
    //}
}