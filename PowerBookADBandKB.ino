#include <usb_mouse.h>
/*
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
       http://www.apache.org/licenses/LICENSE-2.0
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/
// It will determine how a laptop keyboard matrix is wired using a Teensy 3.2 on an FPC daughterboard.
// Open an editor on a computer and load or create a file that lists every key
// on the laptop keyboard that will be tested. Connect the FPC cable of the test keyboard 
// to the Teensy/FPC board. Connect a USB cable from the Teensy to the computer.
// Wait a few seconds for the computer to see the Teensy as a keyboard. If numbers are reported on the screen
// before any keys are pressed, these pin numbers are shorted together and must be fixed. 
// Press each key one by one on the test keyboard as listed on the editor screen. When a key 
// is pressed on the test keyboard, the program detects which two pins on the FPC connector 
// were connected. Those two pin numbers are sent over USB (separated by a TAB) and displayed 
// on the editor. After sending the numbers, a DOWN ARROW is sent over USB to prepare for
// the next key. Once all keys on the test keyboard have been pressed, the file in
// the editor can be saved to create a row-column matrix.
//
// If your keyboard has diodes, you must pay attention to the order of the two pins that are reported by the Teensy. The code performs 
// a bottom up test first, followed by a top down test so that one of the two tests will forward bias the diode. 
// The first pin reported over USB is the cathode side and the second pin is the anode side. The diode direction must be taken into 
// account when programming the TMK or Teensyduino keyboard routine.   
//
// Revision History
// Rev 1.00 - Nov 18, 2018 - Original Release
//
//
// Load an array with the Teensy 3.2 I/O numbers that correspond to FPC pins 1 thru 34.
 int con_pin[] = {3, 4, 5, 6, 7, 8, 10, 12, 24, 25, 26, 29, 30, 31, 32, 33, 34, 35, 36};
//
// Define maximum and minimum pin numbers that will be tested.
// max_pin is usually set to the FPC connector size. min_pin is usually set to 1. The routine will start testing at pin 1 and go up to the max pin size.
// The max and min pin values can be adjusted to exclude testing the FPC traces at the edges if they are reported as shorted. An example would be if pin 1
// and pin 34 are both grounds. They will be reported as tied together but they are not needed by the key matrix. In this case, set the 
// min_pin to 2 and the max_pin to 33.
//  
int max_pin = 19; // the keyboard FPC connector pin count. If set to 34, unsolder the LED or the code won't work
int min_pin = 1; // the first pin to be tested on the FPC connector (usually pin 1)
//
int key_matrix[37][37]={0};

int SHIFT_PIN = 1;
int CTRL_PIN = 0;
int CAPS_PIN = 2;
int OPTION_PIN = 9;
int GUI_PIN = 11;


//MOUSE CONSTANTD

int mouseLast = 1;
int ADB_DATA_PIN = 27;
int dat = 0;
int ADB_TIMEOUT = 10000;
//

// Function to set a pin as an input with a pullup so it's high unless grounded by a key press
void go_z(int pin)
{
  pinMode(pin, INPUT_PULLUP);
  digitalWrite(pin, HIGH);
}

// Function to set a pin as an output and drive it to a logic low (0 volts)
void go_0(int pin)
{
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
}
//4   19   a
void send_key_from_matrix(int pin0, int pin1, int modifier)
{
  Keyboard.set_modifier(modifier);
  //Keyboard.send_now();
  Keyboard.set_key1(key_matrix[pin0][pin1]);
  Keyboard.send_now();
  //clear the key and modifier
  Keyboard.set_modifier(0);
  Keyboard.set_key1(0);
  Keyboard.send_now();
  
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++/MOUSE STUFF+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
static void send_0bit() {
 digitalWrite(ADB_DATA_PIN, LOW);
 delayMicroseconds(65);
 digitalWrite(ADB_DATA_PIN, HIGH);
 delayMicroseconds(35);
}

static void send_1bit() {
 digitalWrite(ADB_DATA_PIN, LOW);
 delayMicroseconds(35);
 digitalWrite(ADB_DATA_PIN, HIGH);
 delayMicroseconds(65);
}

static void attn() {
  //delay(1);
  digitalWrite(ADB_DATA_PIN, LOW);
  delayMicroseconds(800-35);
  send_0bit();
}

void send_talk_command() {
 //attn
 pinMode(ADB_DATA_PIN, OUTPUT);
 attn();
 //address (3) 0011
 //0
  send_0bit();
  send_0bit();
  send_1bit();
  send_1bit();
 //talk 11
  send_1bit();
  send_1bit();
 //register 00
  send_0bit();
  send_0bit();
 //stop bit
  send_0bit();
 //start to stop time
 delayMicroseconds(140);

}
int receive_data_packet() {
 pinMode(ADB_DATA_PIN, INPUT);
 int data_time = 0;
 bool command_stop = false;
 int adb_data = 0;
 //begin reading ADB datapin
 
 while(digitalRead(ADB_DATA_PIN)!=LOW && data_time < ADB_TIMEOUT){
  //wait for beginning of start bit
  data_time+=1;
  delayMicroseconds(1);
 }

  while(digitalRead(ADB_DATA_PIN)!=HIGH && data_time < ADB_TIMEOUT){
  //wait for end of start bit
  data_time+=1;
  delayMicroseconds(1);
 }

 while(digitalRead(ADB_DATA_PIN)!=LOW && data_time < ADB_TIMEOUT){
  //wait for beginning of first data bit
  data_time+=1;
  delayMicroseconds(1);
 }

 //data packet
 data_time = 0;
 while(!command_stop && data_time < ADB_TIMEOUT) {
  int low = 0;
  int high = 0;
  bool bit_stop = false;
  while(!bit_stop && data_time < ADB_TIMEOUT) {
    
    int adb_stream = digitalRead(ADB_DATA_PIN);
    if(adb_stream == LOW) low+=1;
    if(adb_stream == HIGH) high+=1;
    if(adb_stream == LOW && high > 0) bit_stop = true;
    if(high > 240){
      bit_stop == true;
      break;
    }
    data_time+=1;
    delayMicroseconds(1);
  }
  if(high > 240){
    command_stop = true;
  }
  //append the new data bit to adb_data
  if(high > low && !command_stop) adb_data = (adb_data << 1) + 1;
  if(low > high && !command_stop) adb_data = adb_data << 1;
  
 }
 //stop bit 
 return adb_data;
}
/*data bits:
 * [c][x][x][x][x][x][x][x][c2][y][y][y][y][y][y][y]
*/
void parse_data(int data) {
      if(data !=0) {
       int mousex = data & 0b0000000001111111;
       int mousey = (data >> 8) & 0b01111111;
       //the mouse value is
        //7 bit two's complement
       if((mousey & 0b1000000) != 0) {
          mousey =  ((~mousey + 1) & 0b01111111) * -1;
       }
       if((mousex & 0b1000000) != 0) {
          mousex =  ((~mousex + 1) & 0b01111111) * -1;
       }
       Mouse.move(mousex, mousey);
       Mouse.move(mousex, mousey);

      int mouseClick = data & 0b1000000000000000;
      
      if(mouseClick == 0) {
        Mouse.set_buttons(1,0,0);
      }
      else if(mouseClick != 0) {
        Mouse.set_buttons(0,0,0);
      }
     }
    

}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++End Mouse Stuf++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 //int con_pin[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 24, 25, 26, 29, 30, 31, 32, 33, 34, 35, 36};
// --------------------------------------------------Setup-----------------------------------
void setup() {
  //Set up the key matrix
key_matrix[31][3] = KEY_A;
key_matrix[3][31] = KEY_A;
key_matrix[7][31] = KEY_B;
key_matrix[29][24] = KEY_C;
key_matrix[24][29] = KEY_C;
key_matrix[24][32] = KEY_D; 
key_matrix[24][35] = KEY_E;  
key_matrix[5][31] = KEY_F; 
key_matrix[25][30] = KEY_G; 
key_matrix[10][31] = KEY_H; 
key_matrix[10][35] = KEY_I; 
key_matrix[25][31] = KEY_J; 
key_matrix[25][35] = KEY_K; 
key_matrix[25][32] = KEY_L; 
key_matrix[6][29] = KEY_M;  
key_matrix[25][29] = KEY_N;  
key_matrix[26][34] = KEY_O; 
key_matrix[6][35] = KEY_P; 
key_matrix[12][35] = KEY_Q;  
key_matrix[5][35] = KEY_R; 
key_matrix[12][31] = KEY_S; 
key_matrix[6][36] = KEY_T;
key_matrix[7][32] = KEY_U;
key_matrix[5][29] = KEY_V;
key_matrix[4][35] = KEY_W;
key_matrix[4][29] = KEY_X;
key_matrix[6][30] = KEY_Y;
key_matrix[29][12] = KEY_Z;
key_matrix[12][29] = KEY_Z;
//
key_matrix[25][36] = KEY_TILDE;
key_matrix[10][36] = KEY_1;
key_matrix[10][30] = KEY_2;
key_matrix[10][34] = KEY_3;
key_matrix[25][33] = KEY_4;
key_matrix[6][33] = KEY_5;
key_matrix[8][33] = KEY_6;
key_matrix[8][36] = KEY_7;
key_matrix[8][30] = KEY_8;
key_matrix[8][34] = KEY_9;
key_matrix[26][30] = KEY_0;

key_matrix[26][36] = KEY_MINUS;
key_matrix[26][33] = KEY_EQUAL;
key_matrix[10][32] = KEY_BACKSPACE;
key_matrix[10][33] = KEY_ESC;

key_matrix[32][5] = KEY_RIGHT;
key_matrix[5][32] = KEY_RIGHT; //I don't remember if the lower one needs to be first
key_matrix[3][32] = KEY_LEFT;
key_matrix[4][32] = KEY_UP;
key_matrix[4][31] = KEY_DOWN;

key_matrix[8][31] = KEY_SLASH;
key_matrix[26][31] = KEY_PERIOD;
key_matrix[6][31] = KEY_COMMA;
key_matrix[6][32] = KEY_SEMICOLON;
key_matrix[8][32] = KEY_QUOTE;
key_matrix[7][35] = KEY_ENTER; //enter
key_matrix[26][32] = KEY_ENTER; //return
key_matrix[6][34] = KEY_LEFT_BRACE;
key_matrix[8][35] = KEY_RIGHT_BRACE;
key_matrix[25][34] = KEY_BACKSLASH;
key_matrix[3][35] = KEY_TAB;

key_matrix[7][29] = KEY_SPACE;

//etc...
  for (int k = 0; k < max_pin; k++) {  // loop thru all connector pins 
    go_z(con_pin[k]); // set each pin as an input with a pullup
  }
  go_z(38);
  go_z(SHIFT_PIN);
  go_z(CTRL_PIN);
  go_z(OPTION_PIN);
  go_z(GUI_PIN);
  delay(15000);  // Wait for the host to connect to the Teensy as a keyboard. If 2 pins are shorted,
              // you want the host to be ready to receive the pin numbers.
}
//
// -------------------------------------------Main Loop--------------------------------------
// 38
void loop() {  
//
// ***********Bottom up Test************ 
  for (int i=0; i<18; i++) {   // outer loop pin
    go_0(con_pin[i]); // make the outer loop pin an output and send this pin low
    for (int j=i+1; j<19; j++) {   // inner loop pin
      delayMicroseconds(10); // give time to let the signals settle out
      if (!digitalRead(con_pin[j])) {  // check for connection between inner and outer pins
        go_0(38);      
        if(!digitalRead(SHIFT_PIN)) {
          send_key_from_matrix(con_pin[i],con_pin[j],MODIFIERKEY_SHIFT);  
        }
        else if(!digitalRead(CTRL_PIN)) {
          send_key_from_matrix(con_pin[i],con_pin[j],MODIFIERKEY_CTRL);
        }
        else if(!digitalRead(OPTION_PIN)) {
          send_key_from_matrix(con_pin[i],con_pin[j],MODIFIERKEY_ALT);
        }
        else if(!digitalRead(GUI_PIN)) {
          send_key_from_matrix(con_pin[i],con_pin[j],MODIFIERKEY_GUI);
        }
        else {
          send_key_from_matrix(con_pin[i],con_pin[j],0);
        }
        while(!digitalRead(con_pin[j])) {  // wait until key is released 
           ;                              // handle mouse events while waiting for key release                                         
        }                                                  
      }
    }
  go_z(con_pin[i]); // return the outer loop pin to float with pullup 
  go_z(38);       
  }
    send_talk_command();
   dat = receive_data_packet();
   parse_data(dat);
//delay(2);  // overall keyboard scan rate is about 30 milliseconds
//
}
