#include "mbed.h"
#include "C12832.h"
#include "RGBled.h"



class LED                                           //Begin LED class definition
{

protected:                                          //Protected (Private) data member declaration
    DigitalOut outputSignal;                        //Declaration of DigitalOut object
    bool status;                                    //Variable to recall the state of the LED

public:                                             //Public declarations
    LED(PinName pin) : outputSignal(pin)
    {
        off();   //Constructor - user provides the pin name, which is assigned to the DigitalOut
    }

    void on(void)                                   //Public member function for turning the LED on
    {
        outputSignal = 0;                           //Set output to 0 (LED is active low)
        status = true;                              //Set the status variable to show the LED is on
    }

    void off(void)                                  //Public member function for turning the LED off
    {
        outputSignal = 1;                           //Set output to 1 (LED is active low)
        status = false;                             //Set the status variable to show the LED is off
    }

    void toggle(void)                               //Public member function for toggling the LED
    {
        if (status)                                 //Check if the LED is currently on
            off();                                  //Turn off if so
        else                                        //Otherwise...
            on();                                   //Turn the LED on
    }

    bool getStatus(void)                            //Public member function for returning the status of the LED
    {
        return status;                              //Returns whether the LED is currently on or off
    }
};

class Potentiometer                                 //Begin Potentiometer class definition
{
private:                                            //Private data member declaration
    AnalogIn inputSignal;                           //Declaration of AnalogIn object
    float VDD, currentSampleNorm, currentSampleVolts; //Float variables to speficy the value of VDD and most recent samples

public:                                             // Public declarations
    Potentiometer(PinName pin, float v) : inputSignal(pin), VDD(v) {}   //Constructor - user provided pin name assigned to AnalogIn...
                                                                        //VDD is also provided to determine maximum measurable voltage
    float amplitudeVolts(void)                      //Public member function to measure the amplitude in volts
    {
        return (inputSignal.read()*VDD);            //Scales the 0.0-1.0 value by VDD to read the input in volts
    }
    
    float amplitudeNorm(void)                       //Public member function to measure the normalised amplitude
    {
        return inputSignal.read();                  //Returns the ADC value normalised to range 0.0 - 1.0
    }
    
    void sample(void)                               //Public member function to sample an analogue voltage
    {
        currentSampleNorm = inputSignal.read();       //Stores the current ADC value to the class's data member for normalised values (0.0 - 1.0)
        currentSampleVolts = currentSampleNorm * VDD; //Converts the normalised value to the equivalent voltage (0.0 - 3.3 V) and stores this information
    }
    
    float getCurrentSampleVolts(void)               //Public member function to return the most recent sample from the potentiometer (in volts)
    {
        return currentSampleVolts;                  //Return the contents of the data member currentSampleVolts
    }
    
    float getCurrentSampleNorm(void)                //Public member function to return the most recent sample from the potentiometer (normalised)
    {
        return currentSampleNorm;                   //Return the contents of the data member currentSampleNorm  
    }

};
class SamplingPotentiometer : public Potentiometer {
private: 
    float samplingFrequency, samplingPeriod;
    Ticker sampler;  // Ticker object to handle periodic sampling

public:
    // Constructor to set up the sampling process
    SamplingPotentiometer(PinName pin, float vdd, float frequency) 
        : Potentiometer(pin, vdd), samplingFrequency(frequency) {
        
        // Calculate the period (in seconds) from the frequency
        samplingPeriod = 1.0 / samplingFrequency;

        // Attach the sample function of the base class to the Ticker object
        sampler.attach(callback(this, &Potentiometer::sample), samplingPeriod);
    }
};


class Speaker {
private:
    PwmOut speakerPin;
public:
    Speaker(PinName pin) : speakerPin(pin) {
        speakerPin.period(1.0 / 1000.0);  // 1 kHz period for the beep sound
    }

    void beep() {
        speakerPin = 0.5;  // Set 50% duty cycle to generate a tone
    }

    void stop() {
        speakerPin = 0.0;  // Turn off the speaker
    }
};


long map(long x, long in_min, long in_max, long out_min, long out_max){
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
};




typedef enum {
    initialisation,
    set_duration,
    start_timer,
    timer_running,
    timer_paused,
    time_elapsed,
    restart_timer,
    resume_timer
} ProgramState;

ProgramState state = initialisation;

int minutesRemaining = 0;  // Minutes remaining
int secondsRemaining = 0;  // Seconds remaining
int minutesInitial = 0;
int secondsInitial = 0;

C12832 lcd(D11, D13, D12, D7, D10);  // LCD display
RGBLed led(D5,D9,D8);  
SamplingPotentiometer potentiometerLeft(A0,3.3,50);
SamplingPotentiometer potentiometerRight(A1,3.3,50);
Speaker speaker(D6);
InterruptIn fire(D4);  // Interrupt for the center button on the joystick
InterruptIn up(A2);
InterruptIn down(A3);
Ticker lcdUpdateTicker;  // Ticker for periodic LCD updates

// Variable to track when to update the LCD
bool lcdUpdateRequired = false;

void updateLCD() {
    lcdUpdateRequired = true;  // Set flag to update LCD
}

Ticker ledTicker;

void toggleLedgreen() {
    static bool ledState = false;  // Static variable to track the current LED state
    
    if (ledState) {
        led.setGreen();  // Turn the LED green
    } else {
        led.setOff();    // Turn the LED off
    }
    
    ledState = !ledState;  // Toggle the LED state
}

Ticker timerTicker;    // Ticker for regular updates to the timer

void timerUpdate() {
    // If there are seconds remaining
    if (secondsRemaining > 0) {
        secondsRemaining--;
    } 
    // If seconds have reached 0, check if minutes are left
    else if (minutesRemaining > 0) {
        minutesRemaining--;
        secondsRemaining = 59;  // Reset seconds to 59
    }

    // If both minutes and seconds have reached 0, switch to time elapsed state
    if (minutesRemaining == 0 && secondsRemaining == 0) {
        state = time_elapsed;  // Transition to time elapsed state
    }
};


void toggleTimer(){
    if (state == timer_paused){
        state = resume_timer;
    }else if (state == timer_running) {
        state = timer_paused;
    }
};

void startTimer() {
    if (state == set_duration) {
        state = start_timer;  // Move to the start timer state
    }
}

void restartTimer(){
    minutesRemaining=minutesInitial;
    secondsRemaining=secondsInitial;
    state=restart_timer;
}

void quitTimer(){
    state=initialisation;
}

int main() {
    lcdUpdateTicker.attach(&updateLCD, 0.5);  // Update LCD every 500ms

    while (true) {
        switch (state) {
            case initialisation:
                led.setRed();
                speaker.stop();
                if (lcdUpdateRequired){
                    lcd.locate(0, 0);
                    lcd.printf("Initialising");
                    lcdUpdateRequired=false;
                }
                state=set_duration;
                break;

            case set_duration:
                // Read the potentiometer values for minutes and seconds
                float minutesRaw = potentiometerLeft.getCurrentSampleNorm();  // 0 to 1
                float secondsRaw = potentiometerRight.getCurrentSampleNorm(); // 0 to 1

                // Map the potentiometer values to minutes (0-9) and seconds (0-59)
                minutesInitial = map(minutesRaw * 1000, 0, 1000, 0, 10);  // Left potentiometer mapped to 0-9 minutes
                secondsInitial = map(secondsRaw * 1000, 0, 1000, 0, 59);  // Right potentiometer mapped to 0-59 seconds
                minutesRemaining=minutesInitial;
                secondsRemaining=secondsInitial;
                // Update the LCD to show the current set time
                int minuteWidth = map(minutesRaw * 1000, 0, 1000, 0, 40); // Width (0 to screen width)
                int secondWidth = map(secondsRaw * 1000, 0, 1000, 0, 40);
                if (lcdUpdateRequired){
                    lcd.cls();  // Clear the display
                    lcd.fillrect(10, 13, 10+minuteWidth, 16, 1);  // Filled rectangle with dimensions based on pot value
                    lcd.rect(10, 13, 50, 16, 1);
                    lcd.locate(13, 19);
                    lcd.printf("Minutes");
                    lcd.fillrect(75, 13, 75+secondWidth, 16, 1);  // Filled rectangle with dimensions based on pot value
                    lcd.rect(75, 13, 115, 16, 1);
                    lcd.locate(78,19);
                    lcd.printf("Seconds");
                    lcd.locate(8, 0);
                    lcd.printf("Set Timer Duration: %02d:%02d", minutesRemaining, secondsRemaining);
                    lcdUpdateRequired=false;
                }

                // Set the LED to Red to show that we are in the set duration phase
                led.setRed();
                // Wait for the user to press the center button to start the timer
                fire.fall(callback(&startTimer));  // This assumes startTimer is a global function or add the logic inline here
                break;

            case start_timer:
                led.setGreen();
                state = timer_running;
                    // Clear the display first
                int x = 3;
                while (x>0){
                    lcd.cls();

                    int x1 = 5, x2 = 25;  // Hourglass width
                    int y1 = 5, y2 = 27;  // Hourglass height
                    int midY = (y1 + y2) / 2;  // Midpoint of the hourglass (vertically aligned center)
                    int midX= (x1+x2)/2;

                    // Fill the top triangle (pointing down)
                    for (int y = y1; y <= midY; y++) {
                        int xStart = x1 + (y - y1);
                        int xEnd = x2 - (y - y1);
                        lcd.line(xStart, y, xEnd, y, 1);
                    }

                    // Draw the bottom half
                    lcd.line(x2, y2, midX, midY, 1);  // right diagonal
                    lcd.line(x1, y2, midX, midY, 1);  // left diagonal
                    lcd.line(x1, y1, x2, y1, 1);    // Top horizontal
                    lcd.line(x1, y2, x2, y2, 1);    // Bottom horizontal
                    lcd.locate(50, 5);
                    lcd.printf("Timer Starting");
                    lcd.locate(75,15);
                    lcd.printf("%d",x);
                    x--;
                    wait(1);
                };
                ledTicker.attach(&toggleLedgreen,2.0);
                timerTicker.attach(callback(&timerUpdate), 1);  // Update every 1 second
                break;

            case resume_timer:
                up.fall(NULL);
                down.fall(NULL);
                if (lcdUpdateRequired){
                    lcd.cls();
                    lcdUpdateRequired=false;
                }
                
                led.setGreen();
                state = timer_running;
                ledTicker.attach(&toggleLedgreen,2.0);
                timerTicker.attach(callback(&timerUpdate), 1);  // Update every 1 second

            case timer_running:
                int total_seconds_initial = minutesInitial*60+secondsInitial;
                int total_seconds_remaining = minutesRemaining*60+secondsRemaining;
                int progress_width=map(float(total_seconds_remaining),0,float(total_seconds_initial),0,86);
                if (lcdUpdateRequired){
                    lcd.cls();
                    lcd.locate(0, 0);
                    lcd.printf("00:00");
                    lcd.locate(106,0);
                    lcd.printf("%02d:%02d", minutesInitial, secondsInitial);
                    lcd.locate(21, 22);
                    lcd.printf("Remaining Time: %d:%02d\n", minutesRemaining, secondsRemaining);
                    lcd.fillrect(21, 13, 21+progress_width, 18, 1);
                    lcd.rect(21, 13, 21+86, 18, 1);
                    lcdUpdateRequired=false;
                }
                fire.fall(callback(&toggleTimer));
                break;

            case timer_paused:
                if (lcdUpdateRequired){
                    lcd.cls();
                    lcd.locate(30,15);
                    lcd.printf("paused");
                    lcdUpdateRequired=false;
                }
                led.setOrange();
                timerTicker.detach();
                up.fall(&restartTimer);
                down.fall(&quitTimer);
                fire.fall(callback(&toggleTimer));
                break;

            case time_elapsed:
                if (lcdUpdateRequired){
                    lcd.cls();
                    lcd.locate(30, 15);
                    lcd.printf("Timer Finished");
                    lcdUpdateRequired=false;
                }
                led.setBlue();
                speaker.beep();
                fire.fall(&quitTimer);
                //update timer display then tranistion back to the timer running
                break;

            case restart_timer:
                up.fall(NULL);
                down.fall(NULL);
                state=resume_timer;
                break;

            default:
                // Handle unexpected states
                printf("Unknown state!\n");
                break;
        }
    }
}

