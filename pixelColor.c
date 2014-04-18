 #include "pixelColor.h"

unsigned char stepsToRed (int steps){return steps;}
unsigned char stepsToBlue (int steps){return steps % 128;}
unsigned char stepsToGreen (int steps){return steps / 64;}
