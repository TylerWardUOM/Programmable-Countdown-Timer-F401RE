#include "mbed.h"
class RGBLed{
    private:
        PwmOut red, green, blue;
        bool state;
    public:
        RGBLed(PinName r, PinName g, PinName b):red(r),green(g),blue(b),state(false){

        };

        void setOn(){
            red = 0.0;
            blue = 0.0;
            green = 0.0;
        }

        void setOff(){
            red = 1.0;
            blue = 1.0;
            green = 1.0;
        };

        void setRed(){
            red = 0.0;
            blue = 1.0;
            green = 1.0;
        };
        
        void setGreen(){
            red = 1.0;
            blue = 1.0;
            green = 0.0;
        };

        void setBlue(){
            red = 1.0;
            blue = 0.0;
            green = 1.0;
        };

        void setYellow(){
            red = 0.0;
            blue = 1.0;
            green = 0.0;
        };

        void setWhite(){
            red = 0.0;
            blue = 0.0;
            green = 0.0;
        };

        void setOrange(){
            red = 0.0;
            blue = 1;
            green = 0.36;
        }
};