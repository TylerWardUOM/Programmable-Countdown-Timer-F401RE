#include "mbed.h"


class Joystick{
    private:
        DigitalIn up, down, left, right, fire;
    public:
        Joystick(PinName u, PinName d, PinName l, PinName r, PinName f):up(u), down(d), left(l), right(r), fire(f){
        };
        bool upPressed(void){
            if (up){
                return true;
            }
            else {
                return false;
            }
        };

        bool downPressed(void){
            if (down){
                return true;
            }
            else {
                return false;
            }
        };
        bool leftPressed(void){
            if (left){
                return true;
            }
            else {
                return false;
            }
        };
        bool rightPressed(void){
            if (right){
                return true;
            }
            else {
                return false;
            }
        };
        bool firePressed(void){
            if (fire){
                return true;
            }
            else {
                return false;
            }
        };
};