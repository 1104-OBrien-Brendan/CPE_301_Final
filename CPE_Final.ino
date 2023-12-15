//Brendan OBrien and Fernando Quintero
//CPE 301 Final Project

#include <dht.h> //dht library
dht DHT;
#include <Stepper.h> //stepper motor library
#include <LiquidCrystal.h> //lcd library

#define RDA 0x80
#define TBE 0x20  
#define DHT11_PIN 7

//Declarations for Stepper Motor
const int stepsPerRevolution = 2038; // Total steps per revolution
Stepper myStepper(stepsPerRevolution, 8, 10, 9, 11);


// UART Pointers
volatile unsigned char *myUCSR0A  = (unsigned char *)0x00C0;
volatile unsigned char *myUCSR0B  = (unsigned char *)0x00C1;
volatile unsigned char *myUCSR0C  = (unsigned char *)0x00C2;
volatile unsigned int  *myUBRR0   = (unsigned int *) 0x00C4;
volatile unsigned char *myUDR0    = (unsigned char *)0x00C6;
// GPIO Pointers
volatile unsigned char *portA     = (unsigned char *) 0x22;
volatile unsigned char *portDDRA = (unsigned char *) 0x21;
volatile unsigned char *portE     = (unsigned char *) 0x2E;
volatile unsigned char *portDDRE  = (unsigned char *) 0x2D;



//water sensor pointers
volatile unsigned char* my_ADMUX = (unsigned char*) 0x7C;
volatile unsigned char* my_ADCSRB = (unsigned char*) 0x7B;
volatile unsigned char* my_ADCSRA = (unsigned char*) 0x7A;
volatile unsigned int* my_ADC_DATA = (unsigned int*) 0x78;


//Button for Stepper Motor
int lastButtonState = 0; // last button state
int motorPosition = 0; //  track the motor position
volatile unsigned char* pinB = (unsigned char*) 0x23;
volatile unsigned char *ddrB =  (unsigned char*) 0x24;  
volatile unsigned char* portB = (unsigned char*) 0x25;

//pin 2 and 3 are chip inputs 5 is enabler and A10 is button input checking for ground input to set low
// UART Pointers
volatile unsigned char* portK = (unsigned char*) 0x108;
volatile unsigned char* ddrK  = (unsigned char*) 0x107;
volatile unsigned char* pinK  = (unsigned char*) 0x106;


//RTC Clock Module:
#include <Wire.h> //For the clock Module
#include <RTClib.h> //Clock module library
RTC_DS1307 rtc;

//Servo Functions
void openVent(); 
void closeVent();
void fanMotor(); //Fan motor function

//Sets mode variable to 0 for boot up
int mode=0;

//Creates int used to monitor temp
unsigned int temp;

//Sets LCD pins
const int RS= 42, EN= 43, D4 = 44, D5 = 45, D6 = 46, D7 = 47;
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);

//Sets up vatiables for millis function used in Temp sensor display
unsigned long previousMillis = 0;
const long interval = 60000;

//Int variables used for clock module
unsigned int hour;
unsigned int minute;
unsigned int second;

//Variable that controls state of Arduino
unsigned int state=0;

//variable used for temperature threhold changing between states
int set_temp=30;


void setup(){      
           
  Serial.begin(9600);
  // Start the UART
  U0Init(9600);


  //set up ADC:
  adc_init();


  // Set the speed of the stepper motor:
  myStepper.setSpeed(10);
  
  //Sets port B to output for 
  *ddrB &= 0x7F;
  *portB |= 0x80;

 //Sets ports E and K to output for use in motor and buttons
  *portDDRE |= 0x38;
  *portK |= 0x04;
  *ddrK &= 0x04;
  *portE |= 0x20;

//LED's setup for output
  *portDDRA |= 0x0F;


//reset
attachInterrupt(digitalPinToInterrupt(18), start_reset, CHANGE);
attachInterrupt(digitalPinToInterrupt(19), stp, CHANGE);


//LCD setup
lcd.begin(16, 2);
lcd.setCursor(0,0);

//RTC setup
rtc.begin();
rtc.isrunning(); {
rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
}

//Prints bootup time and display it is in disabled mode
DateTime now = rtc.now();
hour = now.hour();
minute = now.minute();
second = now.second();
 U0putchar(((hour % 100) / 10) + '0');
 U0putchar((hour % 10) + '0');
 U0putchar(':');
 U0putchar(((minute%100)/10)+'0');
 U0putchar((minute % 10) + '0');
 U0putchar(':');
 U0putchar(((second%100)/10)+'0');
 U0putchar((second % 10) + '0');
 U0putchar(' ');
  U0putchar('D');
  U0putchar('i');
  U0putchar('s');
  U0putchar('a');
  U0putchar('b');
  U0putchar('l');
  U0putchar('e');
  U0putchar('d');  
  U0putchar('\n');

//sets up dht to read temperature 
int chk = DHT.read11(DHT11_PIN);
unsigned int temp= DHT.temperature;
unsigned int temperature = DHT.temperature; //creates int variable with temperature value

//Sets up dht to read humidity
unsigned int hum = DHT.humidity;
unsigned int humidity = DHT.humidity;//creates int variable with humidity value
}

void loop(){
  //checks mode and runs these as long as it is not disabled
if (mode !=0){
   temp = temperature();
   waterSensorLoop();
    LoopDHT11();
    //Stepper Motor
    int button = (*pinB & 0x80);
    //checks for servo avtivation to print closed if previously opened
    if (!(*pinB & 0x80) && motorPosition == 0) {
        myStepper.step(stepsPerRevolution / 2); // Turn 180 degrees forwards
        motorPosition = 1; // Update the motor position
        lastButtonState = 1;
        DateTime now = rtc.now();
        hour = now.hour();
        minute = now.minute();
        second = now.second();
        U0putchar(((hour % 100) / 10) + '0');
        U0putchar((hour % 10) + '0');
        U0putchar(':');
        U0putchar(((minute%100)/10)+'0');
        U0putchar((minute % 10) + '0');
        U0putchar(':');
        U0putchar(((second%100)/10)+'0');
        U0putchar((second % 10) + '0');
        U0putchar(' ');
        U0putchar('V');
        U0putchar('e');
        U0putchar('n');
        U0putchar('t');
        U0putchar(' ');
        U0putchar('C');
        U0putchar('l');
        U0putchar('o');  
        U0putchar('s');  
        U0putchar('e');
        U0putchar('d');
        U0putchar('\n');


               
    }
    //checks for servo activation and prints open if previously closed
       else if (!(*pinB & 0x80) && motorPosition == 1){  
        myStepper.step(-stepsPerRevolution / 2); // Return to original position
        motorPosition = 0; // Update the motor position
        lastButtonState = 0; // Update the lastButtonState
                DateTime now = rtc.now();
        hour = now.hour();
        minute = now.minute();
        second = now.second();
        U0putchar(((hour % 100) / 10) + '0');
        U0putchar((hour % 10) + '0');
        U0putchar(':');
        U0putchar(((minute%100)/10)+'0');
        U0putchar((minute % 10) + '0');
        U0putchar(':');
        U0putchar(((second%100)/10)+'0');
        U0putchar((second % 10) + '0');
        U0putchar(' ');
        U0putchar('V');
        U0putchar('e');
        U0putchar('n');
        U0putchar('t');
        U0putchar(' ');
        U0putchar('O');
        U0putchar('p');
        U0putchar('e');
        U0putchar('n');
        U0putchar('e');
        U0putchar('d');  
        U0putchar('\n');

       }

  }

//checks mode for disabled and sets leds off and turns on yellow, turns off motor and prints time disabled was activated
  if (mode ==0){
    //set pin ALL LED's LOW
   *portA = 0x00;
   //Yellow LED high
   *portA |= 0x02;
   //Fan off
   *portE &= 0xF7;
    while(mode!=state){
DateTime now = rtc.now();
hour = now.hour();
minute = now.minute();
second = now.second();
 U0putchar(((hour % 100) / 10) + '0');
 U0putchar((hour % 10) + '0');
 U0putchar(':');
 U0putchar(((minute%100)/10)+'0');
 U0putchar((minute % 10) + '0');
 U0putchar(':');
 U0putchar(((second%100)/10)+'0');
 U0putchar((second % 10) + '0');
 U0putchar(' ');
  U0putchar('D');
  U0putchar('i');
  U0putchar('s');
  U0putchar('a');
  U0putchar('b');
  U0putchar('l');
  U0putchar('e');
  U0putchar('d');  
  U0putchar('\n');
  state = mode;
    }


  }
  //mode 1 is activated by start/reset button, if mode one then according to temp will either run motor or stop motor. 
  if(mode ==1){
    if ( temp < set_temp){
      mode=2;
    }
    else{
      mode = 3;
    }
 
 }
 // turns off motor, all leds, turns on green led and prints time that mode 2 was activated
 if (mode ==2){      
  //turn off fan motor
      *portE &= 0xF7;
      //set pin ALL LED's LOW
     *portA = 0x00;
        //Green LED High
     *portA |= 0x04;
         while(mode!=state){
          DateTime now = rtc.now();
          hour = now.hour();
          minute = now.minute();
          second = now.second();
          U0putchar(((hour % 100) / 10) + '0');
          U0putchar((hour % 10) + '0');
          U0putchar(':');
          U0putchar(((minute%100)/10)+'0');
          U0putchar((minute % 10) + '0');
          U0putchar(':');
          U0putchar(((second%100)/10)+'0');
          U0putchar((second % 10) + '0');
          U0putchar(' ');
          U0putchar('I');
          U0putchar('d');
          U0putchar('l');  
          U0putchar('e');  
          U0putchar('\n');
          state = mode;
    }
    //sends sytem mode 3 if temp is met
  if(temp>set_temp){
    mode=3;
  }
 }
 // sends system into running
 if (mode == 3){
        //turn on fan motor
     *portE |= 0x28;
      //set pin ALL LED's LOW
     *portA = 0x00;
     //turn on blue led
      *portA |= 0x08;
        while(mode!=state){
          DateTime now = rtc.now();
          hour = now.hour();
          minute = now.minute();
          second = now.second();
          U0putchar(((hour % 100) / 10) + '0');
          U0putchar((hour % 10) + '0');
          U0putchar(':');
          U0putchar(((minute%100)/10)+'0');
          U0putchar((minute % 10) + '0');
          U0putchar(':');
          U0putchar(((second%100)/10)+'0');
          U0putchar((second % 10) + '0');
          U0putchar(' ');
          U0putchar('R');
          U0putchar('u');
          U0putchar('n');  
          U0putchar('n');
          U0putchar('i');
          U0putchar('n');
          U0putchar('g');
          U0putchar('\n');
          state = mode;
    }
    //sends back to mode 2, idle, when temp is met
  if (temp<set_temp){
    mode = 2;
  }
 }

//when water level is low and mode 4 is activated, prints error message and waits for reset or stop
if(mode == 4){
  //error message
  lcd_error();
  //turn off fan motor
  *portE &= 0xF7;
  //set pin ALL LED's LOW
  *portA = 0x00;
  //set red led high
  *portA |= 0x01;
  while(mode!=state){
    DateTime now = rtc.now();
    hour = now.hour();
    minute = now.minute();
    second = now.second();
    U0putchar(((hour % 100) / 10) + '0');
    U0putchar((hour % 10) + '0');
    U0putchar(':');
    U0putchar(((minute%100)/10)+'0');
    U0putchar((minute % 10) + '0');
    U0putchar(':');
    U0putchar(((second%100)/10)+'0');
    U0putchar((second % 10) + '0');
    U0putchar(' ');
    U0putchar('E');
    U0putchar('r');
    U0putchar('r');
    U0putchar('o');
    U0putchar('r');
    U0putchar('\n');
    state=mode;
  }
}



}


void U0Init(int U0baud){
 unsigned long FCPU = 16000000;
 unsigned int tbaud;
 tbaud = (FCPU / 16 / U0baud - 1);
 // Same as (FCPU / (16 * U0baud)) - 1;
 *myUCSR0A = 0x20;
 *myUCSR0B = 0x18;
 *myUCSR0C = 0x06;
 *myUBRR0  = tbaud;
}
unsigned char kbhit(){
  return *myUCSR0A & RDA;
}
unsigned char getChar(){
  return *myUDR0;
}
void putChar(unsigned char U0pdata){
  while((*myUCSR0A & TBE)==0);
  *myUDR0 = U0pdata;
}


//function activated every time start/ reset button is used
void start_reset (){
  mode = 1;
  int chk = DHT.read11(DHT11_PIN);
unsigned int temp= DHT.temperature;
unsigned int temperature = DHT.temperature; //creates int variable with temperature value


unsigned int hum = DHT.humidity;
unsigned int humidity = DHT.humidity;//creates int variable with humidity value


 lcd.clear();
 lcd.setCursor(0,0);
 lcd.print("Temperature:");
 lcd.print(temperature);


 chk = DHT.read11(DHT11_PIN);


 lcd.setCursor(0,2);
 lcd.print("Humidity:");
 lcd.print('humidity');


}




//function for stop button that sends to mode 0 for disabled
void stp (){
  mode = 0;
  lcd.clear();
}




void adc_init(){
  // setup the A register
  *my_ADCSRA |= 0b10000000; // set bit   7 to 1 to enable the ADC
  *my_ADCSRA &= 0b11011111; // clear bit 6 to 0 to disable the ADC trigger mode
  *my_ADCSRA &= 0b11110111; // clear bit 5 to 0 to disable the ADC interrupt
  *my_ADCSRA &= 0b11111000; // clear bit 0-2 to 0 to set prescaler selection to slow reading
  // setup the B register
  *my_ADCSRB &= 0b11110111; // clear bit 3 to 0 to reset the channel and gain bits
  *my_ADCSRB &= 0b11111000; // clear bit 2-0 to 0 to set free running mode
  // setup the MUX Register
  *my_ADMUX  &= 0b01111111; // clear bit 7 to 0 for AVCC analog reference
  *my_ADMUX  |= 0b01000000; // set bit   6 to 1 for AVCC analog reference
  *my_ADMUX  &= 0b11011111; // clear bit 5 to 0 for right adjust result ~ ADLAR=0
  *my_ADMUX  &= 0b11100000; // clear bit 4-0 to 0 to reset the channel and gain bits
}
unsigned int adc_read(unsigned char adc_channel_num){
  // clear the channel selection bits (MUX 4:0)
  *my_ADMUX  &= 0b11100000;
  // clear the channel selection bits (MUX 5)
  *my_ADCSRB &= 0b11110111;
  // set the channel number
  if(adc_channel_num > 7){
    // set the channel selection bits, but remove the most significant bit (bit 3)
    adc_channel_num -= 8;
    // set MUX bit 5
    *my_ADCSRB |= 0b00001000;
  }
  // set the channel selection bits
  *my_ADMUX  += adc_channel_num;
  // set bit 6 of ADCSRA to 1 to start a conversion
  *my_ADCSRA |= 0x40;
  // wait for the conversion to complete
  while((*my_ADCSRA & 0x40) != 0);
  // return the result in the ADC data register
  return *my_ADC_DATA;
}


//set up baud
void U0init(int U0baud){
  unsigned long FCPU = 16000000;
  unsigned int tbaud;
  tbaud = (FCPU / 16 / U0baud - 1);
  // Same as (FCPU / (16 * U0baud)) - 1;
  *myUCSR0A = 0x20;
  *myUCSR0B = 0x18;
  *myUCSR0C = 0x06;
  *myUBRR0  = tbaud;
}
//set up char reads and prints
unsigned char U0kbhit(){
  return *myUCSR0A & RDA;
}
unsigned char U0getchar(){
  return *myUDR0;
}
void U0putchar(unsigned char U0pdata){
  while((*myUCSR0A & TBE)==0);
  *myUDR0 = U0pdata;
}


//function for water sensor that goes to error if two low
void waterSensorLoop(){
  unsigned int cs1 = adc_read(0);



  if (cs1<150){
    mode = 4;
  }
  return mode;
}

//Function for temp/humidity sensor that constantly check temp and humidity but only prints once per minute
void LoopDHT11(){
int chk = DHT.read11(DHT11_PIN);
unsigned int temp= DHT.temperature;
unsigned int temperature = DHT.temperature; //creates int variable with temperature value


unsigned int hum = DHT.humidity;
unsigned int humidity = DHT.humidity;//creates int variable with humidity value


unsigned long currentMillis = millis();
previousMillis = currentMillis;


if (currentMillis - previousMillis >= interval) {
 previousMillis = currentMillis;
 lcd.clear();
 lcd.setCursor(0,0);
 lcd.print("Temperature:");
 lcd.print(temperature);


 chk = DHT.read11(DHT11_PIN);


 lcd.setCursor(0,2);
 lcd.print("Humidity:");
 lcd.print('humidity');


  }
}
unsigned int temperature(){
int chk = DHT.read11(DHT11_PIN);
  unsigned int temperature = DHT.temperature; //creates int variable with temperature value
  return temperature;
}

//fan motor funciton for on and off
void fanMotor(){
  if(*pinK & 0x04){
    *portE |= 0x28;
  }
  else{
    *portE &= 0xF7;//Sets pin 5 low when button is pressed
  }
}

//function for displaying error to lcd when water is too
void lcd_error(){
  if(mode==4){
 lcd.clear();
 lcd.setCursor(1,0);
 lcd.print("Water level is");
 lcd.setCursor(4,2);
 lcd.print("too low!");
  }
}
