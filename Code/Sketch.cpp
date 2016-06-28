/**
 * \file Sketc.cpp
 * \brief Stopwatch using IR light barriers and Adafruit i2c 7-segment LED display
 * \author Remko Welling, pe1mew@gmail.com
 * \date june 6, 2016
 * 
 * This code is imported from Arduino IDE in to Atmel Studio 7.
 *
 * Dependencies:
 * - library for I2C LED Backpacks written by Limor Fried/Ladyada for Adafruit Industries.  
 * - Simple StopWatch library for Arduino, Rob Tillaart, http://playground.arduino.cc/Code/StopWatchClass
 *
 */

/*Beginning of Auto generated code by Atmel studio */
#include <Arduino.h>
/*End of auto generated code by Atmel studio */

#include <Wire.h> // Enable this line if using Arduino Uno, Mega, etc.

#include "Adafruit_LEDBackpack.h"	
#include "Adafruit_GFX.h"
#include <StopWatch.h>

// defines that connect functions to pins.
#define LED_IR		11		// Timer 2 "A" output: OC2A
#define SENSOR_A	8		// Input from IR sensor at track A
#define SENSOR_B	9		// Input from IR sensor at track B
#define SWITCH		2		// Switch to start and stop timer

// enum
enum runMode { STOP=1, RUN, FINISH };
int  Mode { 1 };

//Beginning of Auto generated function prototypes by Atmel Studio
void setup();
void loop();
//End of Auto generated function prototypes by Atmel Studio

// Prototypes of helper functions .

/* \brief send timer result over serial port.
 * Helper function to simplify code.
 * \param pointer to text (line name) 
 * \param timer result as double
 */
void serialResult(char* line, double result);

/* \brief send timer result to 7-segment display.
 * Helper function to simplify code.
 * \param timer result as double
 */
void displayResult(double result);

// Objects 
StopWatch sw_millis_A;					// Stopwatch track A in MILLIS (default)
StopWatch sw_millis_B;					// Stopwatch track B in MILLIS (default)
Adafruit_7segment matrix = Adafruit_7segment();	// object for driving Adafruit 7-segment diplay.

/**
 IR barrier
 ----------
 The timer is using IR to detect if a lane has finished.
 This is initially don by sending a IR signal to a IR sensor. When the IR signal dissapeares a finish-event is assumed.
 In this configuration IR sender and IR detector are oposite of each other and detection takes place when a object goes
 trough between sender and detector.
 Testing for dissapearens of the IR signal done testing on pin SENSOR_A and SENSOR_B to be HIGH.
 
 The IR detection can also be done by reflection. In this case a active reflection assumes finishing a lane.
 To detect reflection testing on pin SENSOR_A and SENSOR_B shall be LOW.
 
 To make the IR sensor insensitive for other signals the IR transmitter is modulated with a 450 KHz signal.
 The 450 KHz signal is generated using timer 2 where OCR is used as a driver for the IR leds.
 */
void setup() 
{
#ifndef __AVR_ATtiny85__
	Serial.begin(9600);
#endif
	// Configure Adafruit 7-segment diplay.
	matrix.begin(0x70);					// i2c address of 7-segment display
	matrix.clear();						// Clear display buffer
	matrix.writeDigitRaw(4, 0x80);		// Set most left decimal point on.
	matrix.writeDisplay();				// Command 7-segment display to show buffer content.
  
	// initialize digital pins.
	pinMode(LED_IR,		OUTPUT);		// Output for IR-leds to drive them
	pinMode(SENSOR_A,	INPUT);			// Input for IR-sensor on line A
	pinMode(SENSOR_B,	INPUT);			// Input for IR-sensor on line B
	pinMode(SWITCH,		INPUT_PULLUP);	// Input for start(stop) switch of stopwatch.
  	
	// Timer 2 configuration for generating 450 KHz signal to drive IR-leds
	TCCR2A = _BV (COM2A0) | _BV(WGM21);	// CTC, toggle OC2A on Compare Match
	TCCR2B = _BV (CS20);				// No prescaler
	OCR2A  = 209;						// compare A register value (210 * clock speed)
  	
	//Initialize serial and wait for port to open:
	Serial.begin(9600);
	while (!Serial) 
	{
		; // wait for serial port to connect. Needed for native USB port only
	}
  	
	// print title to serial port with ending line break
	Serial.println("HamRadio Ship-Stop-watch");
	Serial.println("Ready to start");
	
	// Set state machine of stopwatch to stop
	Mode = STOP;
}

void loop() 
{
	/**
	Global description of main loop.
	--------------------------------
	 The stopwatch is using a state machine for it's operation. 
	 After a reset it goes to STOP. Here it tests for the start button to be pressed.
	 When in STOP state and SWITCH = LOW (pressed), the two stopwatch timers for each lane
	 are reset and started. Information about the state change is sent on both the serial port
	 and the 7-segment display. Now the stopwatches are running the state is changed to RUN.
	 
	 While in RUN state the IR-sensors of both lanes is tested to see if one is finished.
	 When IR for a lane is detected the timer for that lane is stopped and kept as memory. 
	 The result is sent to the 7-segment display.
	 If the start button is pressed after one second the timer stops and the result is displayed.
	 Waiting for one second to evaluate the button state, the contact of the start button is debounced.
	 When the both lines are finished or the start button is pressed the state changes to FINISH.
	 
	 In FINISH state the 7-segment display is updatet in a 1 second interval displaying the finishing 
	 order of the lanes and the time of each lane in finishing order.
	 */
	switch(Mode)
  	{
	  	case STOP:
	  		//digitalWrite(LED_MODE, LOW);	// turn the LED off to indicate that measurement is started for debug
	  		if (digitalRead(SWITCH) == LOW)
	  		{
		  		// Reset both stopwatches
		  		sw_millis_A.reset();	// reset lane A
		  		sw_millis_B.reset();	// reset lane B
		  	
		  		// Start both stopwatches
		  		sw_millis_A.start();	// start lane A
		  		sw_millis_B.start();	// start lane B
		  	
		  		// Change state machine state to RUN
		  		Mode = RUN;
				  
				// send information to serial port
		  		Serial.println("Stopwatch running...");
				  
				// indicate on 7-segment display that stopwatch is running by showing colon
				matrix.writeDigitRaw(4, 0x00);	// erase display buffer
				matrix.drawColon(true);			// set colon to on
				matrix.writeDisplay();			// show on display
	  		}
	  break;

	  	case RUN:
	  		//digitalWrite(LED_MODE, HIGH);   // turn the LED on to indicate that measurement is finished for debug
			  
	  		// test if lane A is finished
	  		if (digitalRead(SENSOR_A) == HIGH)
	  		{
		  		sw_millis_A.stop();						// stop stopwatch for lane A, keep value for memory
				displayResult(sw_millis_A.value());		// Display result on 7-segment display.
	  		}
	  	
	  		// test if lane B is finished
	  		if (digitalRead(SENSOR_B) == HIGH)
	  		{
		  		sw_millis_B.stop();						// stop stopwatch for lane B, keep value for memory
				displayResult(sw_millis_B.value());		// Display result on 7-segment display.
	  		}
	  	
		    // test if both lanes are finished or if start button is pressed.
			// the button state is only evaluated when the timer is running for more than 1 second
			// Doing so the contact of the start button is debounced.
	  		if ( (sw_millis_A.isRunning() != true && sw_millis_B.isRunning() != true) ||
	  			 (digitalRead(SWITCH) == LOW && sw_millis_A.value() > 1000) )
	  		{
		  		// \todo this code shall be cleaned as the test is there for no reason anymore.
				// \todo implement better usage of  temporary variable
				if(sw_millis_A.isRunning() != true)
		  		{
			  		double result = sw_millis_A.value()/1000.0;	// Create temporary variable and fill with timer A result
																// Devide by 1000 to convert form milliseconds to seconds
			  		serialResult("Line A: ", result);			// send timer A result to serial port.
		  		}
		  	
		  		if(sw_millis_B.isRunning() != true)
		  		{
			  		double result = sw_millis_B.value()/1000.0;	// Create temporary variable and fill with timer B result, 
																// Devide by 1000 to convert form milliseconds to seconds
			  		serialResult("Line B: ", result);			// send timer B result to serial port.
		  		}
				  
		  		Mode = FINISH;							// Change state machine state to FINISH
				matrix.drawColon(false);				// Erase colon form buffer of 7-segment display
				matrix.writeDisplay();					// Write buffer to 7-segment display
		  		Serial.println("Stopwatch stopped.");	// Send message that stopwatch stopped to serial port.
	  		}
	  	break;

	  	case FINISH:
	  		//digitalWrite(LED_MODE, LOW);   // turn the LED on to indicate RUN state
			
			// Test which lane has fastest result.
			if (sw_millis_A.value() < sw_millis_B.value())	// Lane A wins
			{
				matrix.print(0x00AB, HEX);					// display that lane A has shorter time than lane B. 
				matrix.writeDisplay();
				delay(1000);								// Wait 1 second
				displayResult(sw_millis_A.value());			// Show stopwatch time for lane A on 7-segment display
				delay(1000);								// Wait 1 second
				displayResult(sw_millis_B.value());			// Show stopwatch time for lane B on 7-segment display
				delay(1000);								// Wait 1 second
			}
			else
			{
				matrix.print(0x00BA, HEX);					// display that lane B has shorter time than lane A. 
				matrix.writeDisplay();
				delay(1000);								// Wait 1 second
				displayResult(sw_millis_B.value());			// Show stopwatch time for lane B on 7-segment display
				delay(1000);								// Wait 1 second
				displayResult(sw_millis_A.value());			// Show stopwatch time for lane A on 7-segment display
				delay(1000);								// Wait 1 second
			}

	  	break;

	  	default:
	  		Serial.println("Switch-case failure");
	  	break;
  	}
}

void serialResult(char* line, double result)
{
	Serial.print(line);
	Serial.print(result);
	Serial.print(" mS.");
	Serial.print( "\r\n");
}

void displayResult(double result)
{
	matrix.println(result/1000.0);			// write time to 7-segment buffer.
											// Devide by 1000 to convert form milliseconds to seconds
	matrix.writeDisplay();					// Write buffer to 7-segment display
}
