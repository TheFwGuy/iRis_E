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

Adafruit_TSL2561_Unified tsl_R = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);
Adafruit_TSL2561_Unified tsl_L = Adafruit_TSL2561_Unified(TSL2561_ADDR_LOW, 12345);

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

#define NORMAL  0
#define CALIBRATING 1
#define SETTING 2

/*
 *  Global variables
 */

char buffer[20];		/* Generic buffer for display purpose */
char main_status = NORMAL;
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
  tsl_R.enableAutoRange(true);            /* Auto-gain ... switches automatically between 1x and 16x */
  tsl_L.enableAutoRange(true);            /* Auto-gain ... switches automatically between 1x and 16x */
  
  /* Changing the integration time gives you better sensor resolution (402ms = 16-bit data) */
  tsl_R.setIntegrationTime(TSL2561_INTEGRATIONTIME_13MS);      /* fast but low resolution */
  tsl_L.setIntegrationTime(TSL2561_INTEGRATIONTIME_13MS);      /* fast but low resolution */
  // tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_101MS);  /* medium resolution and speed   */
  // tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_402MS);  /* 16-bit data but slowest conversions */

}


void setup()
{
   rightEye.attach(EYE_RIGHT_PWM);	/* attaches the servo to the pin */
   leftEye.attach(EYE_LEFT_PWM);        /* attaches the servo to the pin */

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
   lcd.print("iRis Run        ");  
   lcd.setCursor ( 0, 1 );        // go to the next line
   lcd.print ("TheFwGuy    1.0");      

   rightEye.write(Pwm_R_position);
   leftEye.write(Pwm_L_position);
  
   delay(1000);
  
   attachInterrupt(ENC_A,doEncoder,CHANGE);
}


void loop()
{
   if(KEYIsPressed(SELBTN))
   {
      while(KEYIsPressed(SELBTN));
      switch(main_status)
      {
         case NORMAL:
            lcd.home ();                   // go home
            lcd.print("iRis Calibrating");  
            main_status = CALIBRATING;
            break;

         case CALIBRATING:
            lcd.home ();                   // go home
            lcd.print("iRis Setting     ");  
            main_status = SETTING;
            Encoder_value = 0;
            break;
      
         default:
         case SETTING:
            lcd.home ();                   // go home
            lcd.print("iRis Run        ");  
            main_status = NORMAL;
            break;
      }
   }    
      
   switch(main_status)
   {
      case NORMAL:
         /* Get a new sensor event */ 
         sensors_event_t event_R;
         sensors_event_t event_L;
   
         tsl_R.getEvent(&event_R);
         tsl_L.getEvent(&event_L);

         lcd.setCursor ( 0, 1 );         /* Print PWM values */
         lcd.print ("L:       R:    ");  /* Left at 3, right at 11 */
 
         /* Display the results (light is measured in lux) */
         if (event_R.light)
         {
            Pwm_R_position = map(event_R.light, 0, MAX_LIGHT_VALUE, MIN_PWM_VALUE, MAX_PWM_VALUE);
            
            lcd.setCursor ( 3, 1 );
            lcd.print(Pwm_R_position);
            
            rightEye.write(Pwm_R_position);
         }
   
         if (event_L.light)
         {
            Pwm_L_position = map(event_L.light, 0, MAX_LIGHT_VALUE, MIN_PWM_VALUE, MAX_PWM_VALUE);

            lcd.setCursor ( 11, 1 );
            lcd.print(Pwm_L_position);

            leftEye.write(Pwm_L_position);
         }
         delay(10);
         break;
         
      case CALIBRATING:   
         if(Encoder_value != Old_encoder_value)
         {
            Old_encoder_value = Encoder_value;
            Pwm_R_position = map(Encoder_value, 0, MAX_ENCODER_VALUE, MIN_PWM_VALUE, MAX_PWM_VALUE);
            Pwm_L_position = map(Encoder_value, 0, MAX_ENCODER_VALUE, MIN_PWM_VALUE, MAX_PWM_VALUE);
      
            lcd.setCursor ( 0, 1 );         /* Print PWM values */
            lcd.print ("L:       R:    ");  /* Left at 3, right at 11 */
            lcd.setCursor ( 3, 1 );
            lcd.print(Pwm_R_position);
            lcd.setCursor ( 11, 1 );
            lcd.print(Pwm_L_position);
   
            rightEye.write(Pwm_R_position);
            leftEye.write(Pwm_L_position);
            delay(10);
         }
         break;
         
      case SETTING:   
         if(Encoder_value != Old_encoder_value)
         {
            Old_encoder_value = Encoder_value;
            
            lcd.setCursor ( 0, 1 );         /* Print Menu */
            lcd.print ("Menu TBD       ");  

         }
         break;
         
   }
}

