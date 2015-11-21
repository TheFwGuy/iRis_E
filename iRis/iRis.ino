/**
 *  iRis project with Energia
 *  @Auth TheFwGuy
 *  November 2015
 *  @Brief Artifical iris control with MSP430g2553
 *
 */

#include <Wire.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>  // F Malpartida's NewLiquidCrystal library
#include <Servo.h> 
#include <Adafruit_Sensor.h>
#include <Adafruit_TSL2561_U.h>
 
/* Function prototypes */


/* Define I2C Address  */

#define I2C_ADDR 0x3F
#define BACKLIGHT_PIN  3

#define Rs_pin  0
#define Rw_pin  1
#define En_pin  2
#define D4_pin  4
#define D5_pin  5
#define D6_pin  6
#define D7_pin  7

#define  LED_OFF  0
#define  LED_ON  1

LiquidCrystal_I2C  lcd(I2C_ADDR,En_pin,Rw_pin,Rs_pin,D4_pin,D5_pin,D6_pin,D7_pin, BACKLIGHT_PIN, POSITIVE);

Servo rightEye;		/* create servo object to control a servo for the right eye */
Servo leftEye;		/* create servo object to control a servo for the left eye */

Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);

/*
 *  I/O defines
 */

#define LED_INFO   2            /* P1_0 */
#define EYE_RIGHT_PWM  3	/* P1_1 */
#define EYE_LEFT_PWM   4	/* P1_2 */

#define SELBTN     13		/* P2_5 */

#define ENC_A   P2_3
#define ENC_B   P2_4

#define KEYIsPressed(key) (!digitalRead(key))
#define KEYIsNotPressed(key) (digitalRead(key))

/*
 *  PWM related defines
 */

#define MAX_LIGHT_VALUE  3000
#define MAX_ENCODER_VALUE  250
#define MIN_PWM_VALUE  14
#define MAX_PWM_VALUE  180

/*
 *  I2C mode 
 *  SDA P1_7 Pin 14
 *  SCL P1_6 Pin 13 
 */


/*
 *  Global variables
 */

char buffer[20];		/* Generic buffer for display purpose */
short Encoder_value = 0;
short Old_encoder_value = 0;
short Pwm_R_position = MIN_PWM_VALUE;
short Pwm_L_position = MIN_PWM_VALUE;


/* Interrupt service encoder function */

void doEncoder() 
{
   /* If pinA and pinB are both high or both low, it is spinning
    * forward. If they're different, it's going backward.
    *
    * For more information on speeding up this process, see
    * [Reference/PortManipulation], specifically the PIND register.
    */
   if (digitalRead(ENC_A) == digitalRead(ENC_B)) 
   {
      if(Encoder_value<MAX_ENCODER_VALUE)
         Encoder_value++;
   }
   else 
   {
      if(Encoder_value>0)
         Encoder_value--;
   }
}

void configureSensor(void)
{
  /* You can also manually set the gain or enable auto-gain support */
  // tsl.setGain(TSL2561_GAIN_1X);      /* No gain ... use in bright light to avoid sensor saturation */
  // tsl.setGain(TSL2561_GAIN_16X);     /* 16x gain ... use in low light to boost sensitivity */
  tsl.enableAutoRange(true);            /* Auto-gain ... switches automatically between 1x and 16x */
  
  /* Changing the integration time gives you better sensor resolution (402ms = 16-bit data) */
  tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_13MS);      /* fast but low resolution */
  // tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_101MS);  /* medium resolution and speed   */
  // tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_402MS);  /* 16-bit data but slowest conversions */

}


void setup()
{
   rightEye.attach(EYE_RIGHT_PWM);	/* attaches the servo to the pin */
   leftEye.attach(EYE_LEFT_PWM);		/* attaches the servo to the pin */

   // put your setup code here, to run once:
   pinMode(LED_INFO,OUTPUT);
   digitalWrite(LED_INFO,LOW);		/* Turn LED OFF */
  
   pinMode(SELBTN,INPUT_PULLUP);
   pinMode(ENC_A,INPUT_PULLUP);
   pinMode(ENC_B,INPUT_PULLUP); 

   /* Setup the sensor gain and integration time */
   configureSensor();

   lcd.setBacklightPin(BACKLIGHT_PIN, POSITIVE); // POSITIVE); //NEGATIVE);
   lcd.setBacklight(LED_ON);
   lcd.backlight();
    
   lcd.begin(16,2);               // initialize the lcd 

//  lcd.createChar (0, smiley);    // load character to the LCD
//  lcd.createChar (1, armsUp);    // load character to the LCD
//  lcd.createChar (2, frownie);   // load character to the LCD

   lcd.clear();
   lcd.home ();                   // go home
   lcd.display();
   lcd.print("     iRis       ");  
   lcd.setCursor ( 0, 1 );        // go to the next line
   lcd.print ("TheFwGuy    1.0");      

   rightEye.write(Pwm_R_position);
   leftEye.write(Pwm_L_position);
  
   delay(1000);
  
   attachInterrupt(ENC_A,doEncoder,CHANGE);
}


void loop()
{
   /* Get a new sensor event */ 
   sensors_event_t event;
   tsl.getEvent(&event);
 
   /* Display the results (light is measured in lux) */
   if (event.light)
   {
      lcd.setCursor ( 0, 1 );        // go to the next line
      lcd.print ("Light :           ");      
      lcd.setCursor ( 8, 1 );
      lcd.print(event.light);

      Pwm_R_position = map(event.light, 0, MAX_LIGHT_VALUE, MIN_PWM_VALUE, MAX_PWM_VALUE);
      Pwm_L_position = map(event.light, 0, MAX_LIGHT_VALUE, MIN_PWM_VALUE, MAX_PWM_VALUE);

      rightEye.write(Pwm_R_position);
      leftEye.write(Pwm_L_position);
      delay(10);

   }
  
   if(Encoder_value != Old_encoder_value)
   {
      Old_encoder_value = Encoder_value;
      Pwm_R_position = map(Encoder_value, 0, MAX_ENCODER_VALUE, MIN_PWM_VALUE, MAX_PWM_VALUE);
      Pwm_L_position = map(Encoder_value, 0, MAX_ENCODER_VALUE, MIN_PWM_VALUE, MAX_PWM_VALUE);
      
      lcd.setCursor ( 0, 1 );        // go to the next line
      lcd.print ("Pwm :           ");      
      lcd.setCursor ( 6, 1 );
      lcd.print(Pwm_R_position);
   
      rightEye.write(Pwm_R_position);
      leftEye.write(Pwm_L_position);
      delay(10);
   }
}

