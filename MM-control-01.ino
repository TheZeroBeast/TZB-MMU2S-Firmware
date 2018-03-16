// MM-control-01
// HW arduino Leonardo
//
// peripherals:
//
// 3 x TMC2130
// pinout:
//  pin apin port bit signal
//  0   0    C    0   CS0
//  0   0    C    0   CS1
//  0   0    C    0   CS2
//  0   0    C    0   STP0
//  0   0    C    0   STP1
//  0   0    C    0   STP2
//
// 16-bit shift register (2x 74595)
// pinout:
//  pin apin port bit signal
//  0   0    C    0   CLK
//  0   0    C    0   DATA
//  0   0    C    0   LATCH







/////////////////// SETUP  ////////////////////////
const int NumberOfRegister =  2;    // 0-x

const int ButtonPin = A2;

const int ShiftOutClockPin = 13;
const int ShiftOutDataPin  =  9;
const int ShiftOutLatchPin = 10;
/////////////////// SETUP  ////////////////////////

const int BAUDRATE0 = 115200; // USB
const int BAUDRATE1 = 115200; // SERIAL UART
byte ShiftByte[5];
int SelectOut = 0;
int x;



int ReadButton(){
  int raw = analogRead(ButtonPin);
  // Button 1 - 0
  // Button 2 - 344
  // Button 3 - 516
  if(raw<10){
    return(1);
  }else if(raw>300 && raw<400){
    return(2);
  }else if(raw>450 && raw<600){
    return(3);
  }else{
    return(0);
  }
}
