

/*
  Digital Light Wand + SD + LCD + Arduino MEGA - V MRR-3.0 (WS2812 RGB LED Light Strip) 
  by Michael Ross 2014
  Based on original code by myself in 2010 then enhanced by Is0-Mick in 2012 

  The Digital Light Wand is for use in specialized Light Painting Photography
  Applications.

  This code is totally rewritten using code that IsO-Mick created made to specifically 
  support the WS2812 RGB LED strips running with an SD Card, an LCD Display, and the
  Arduino Mega 2560 Microprocessor board.
  
  The functionality that is included in this code is as follows:
  
  Menu System
  1 - File select
  2 - Brightness 
  3 - Initial Delay
  4 - Frame Delay
  5 - Repeat Times (The number of times to repeat the current file playback)
  6 - Repeat Delay (if you want a delay between repeated files)
  7 - Frame Off time if you want a time off between frames (Use 0 in most cases)
  8 - Cycle All (Cycle throuh all images on card, except image labeled "STARTUP.BMP)
  
  This code supports direct reading of a 24bit Windows BMP from the SD card.
  BMP images must be rotated 90 degrees clockwise and the width of the image should match the
  number of pixels you have on your LED strip.  The bottom of the tool will be the INPUT
  end of the strips where the Arduino is connected and will be the left side of the input
  BMP image.    
  
  Mick also added a Gamma Table from adafruit code which gives better conversion of 24 bit to 
  21 bit coloring. 

*/ 

// Library initialization
#include <Wire.h> 
#include <Adafruit_NeoPixel.h>           // Library for the WS2812 Neopixel Strip
#include <SD.h>                          // Library for the SD Card
#include <LiquidCrystal_I2C.h>               // Library for the LCD Display
#include <SPI.h>                         // Library for the SPI Interface

// Pin assignments for the Arduino (Make changes to these if you use different Pins)
//#define BACKLIGHT 10                      // Pin used for the LCD Backlight
#define SDssPin 53                        // SD card CS pin
int NPPin = 5;                           // Data Pin for the NeoPixel LED Strip
int AuxButton = 2;                       // Aux Select Button Pin
int AuxButtonGND = 3;                    // Aux Select Button Ground Pin
int g = 0;                                // Variable for the Green Value
int b = 0;                                // Variable for the Blue Value
int r = 0;                                // Variable for the Red Value

// Intial Variable declarations and assignments (Make changes to these if you want to change defaults)
#define STRIP_LENGTH 144                  // Set the number of LEDs the LED Strip
int frameDelay = 15;                      // default for the frame delay 
int frameBlankDelay = 0;                  // Default Frame blank delay of 0
int menuItem = 1;                         // Variable for current main menu selection
boolean updateScreen = true;              // Added to minimize screen update flicker (Brian Heiland)
int initDelay = 0;                        // Variable for delay between button press and start of light sequence
int repeat = 0;                           // Variable to select auto repeat (until select button is pressed again)
int repeatDelay = 0;                      // Variable for delay between repeats
int updateMode = 0;                       // Variable to keep track of update Modes
int repeatTimes = 1;                      // Variable to keep track of number of repeats
int brightness = 90;                      // Variable and default for the Brightness of the strip
int interuptPressed = 0;                  // variable to keep track if user wants to interupt display session.
int loopCounter = 0;                       // count loops as a means to avoid extra presses of keys
boolean cycleAllImages = false;            //cycle through all images
boolean cycleAllImagesOneshot = false;
int cycleImageCount= 0;
// Other program variable declarations, assignments, and initializations
byte x;
LiquidCrystal_I2C lcd(0x27,16,2);      // Init the LCD

// Declaring the two LED Strips and pin assignments to each 
Adafruit_NeoPixel strip = Adafruit_NeoPixel(STRIP_LENGTH, NPPin, NEO_GRB + NEO_KHZ800);

// BacklightControl to save battery Life
boolean BackLightTimer = false;
int BackLightTimeout =160;                // Adjust this to a larger number if you want a longer delay
int BackLightTemp =  BackLightTimeout;

// Variable assignments for the Keypad
int adc_key_val[5] ={ 30, 170, 390, 600, 800 };  
int NUM_KEYS = 5;
int adc_key_in;
int key=-1;
int oldkey=-1;

// SD Card Variables and assignments
File root;
File dataFile;
String m_CurrentFilename = "";
int m_FileIndex = 0;
int m_NumberOfFiles = 0;
String m_FileNames[200];                  
long buffer[STRIP_LENGTH];
     

// Setup loop to get everything ready.  This is only run once at power on or reset
void setup() {
  
  pinMode(AuxButton, INPUT);
  digitalWrite(AuxButton,HIGH);
  pinMode(AuxButtonGND, OUTPUT);
  digitalWrite(AuxButtonGND,LOW);

  setupLEDs();
  setupLCDdisplay();
  setupSDcard();
  BackLightOn();
  int i;
  for (i = 0; i < m_NumberOfFiles; i++) {     // Loop through all of the files that have been identified on the memory card
    if(m_FileNames[i] == "STARTUP.BMP"){      // Look for a default file to show on initial bootup This file name could be changed to "STARTUP.BMP"
      SendFile(m_FileNames[i]);               // if the startup BMP was found, send it to the LED's
      ClearStrip(0);                          // Clear the strip
      break;                                  // Exit for loop, we found our default file
    }
  }
}

     
// The Main Loop for the program starts here... 
// This will loop endlessly looking for a key press to perform a function
void loop() {
  if (updateScreen){
    updateScreen = false;
    lcd.clear();
  switch (menuItem) {
    case 1:
      lcd.print("1:File Select   ");
      lcd.setCursor(0, 1);
      lcd.print(m_CurrentFilename);
      break;    
    case 2:

      lcd.print("2:Brightness    ");
      lcd.setCursor(0, 1);
      lcd.print(brightness);
      if (brightness == 100) {
        lcd.setCursor(3, 1);
      }
      else {
        lcd.setCursor(2, 1);
      }
      lcd.print("%");
      break;    
    case 3:

      lcd.print("3:Init Delay    ");
      lcd.setCursor(0, 1);
      lcd.print(initDelay);   
      break;    
    case 4:

      lcd.print("4:Frame Delay   ");
      lcd.setCursor(0, 1);
      lcd.print(frameDelay);    
      break;    
    case 5:      

      lcd.print("5:Repeat Times  ");
      lcd.setCursor(0, 1);
      lcd.print(repeatTimes);   
      break;    
    case 6:

      lcd.print("6:Repeat Delay  ");
      lcd.setCursor(0, 1);
      lcd.print(repeatDelay);    
      break;  
    case 7:

      lcd.print("7:Frame Off Time");
      lcd.setCursor(0, 1);
      lcd.print(frameBlankDelay);    
      break;   
    case 8:

      lcd.print("8:Cycle All");
      lcd.setCursor(0, 1);
      if(cycleAllImages){
        lcd.print("Yes");
      }else{
        lcd.print("No"); 
      }    
      break;
    }
  }
  loopCounter +=1;
  if(interuptPressed >=3){          //User requested that display mode be stopped
    delay (250);                    // Delay for a bit, since select button is used to abort and restart this is trying to prevent a restart of display
    dataFile.close();               // Close data file if it was lft open, should have been closed already, trying to prevent open files
    interuptPressed = 0;            // On next loop we won't delay any longer.
  }
  if (loopCounter > 2000){          //  This is a delay that is counting loops, Keypad won't be read until 2000 cycles
  int keypress = -1;                // Set keypress to an invalid value -1
  keypress = ReadKeypad();          // set keypress to keypad reading
  delay(50);                        // take a short delay
  
  if ((keypress == 4) ||(digitalRead(AuxButton) == LOW)) {    // The select key was pressed
    loopCounter = 0;                // tell the loop counter to not read another keypress until the main loop, loops another 2000 times
    BackLightTimer = false;      //turn off lcd backlight the moment the button is pressed so it does not add light to images. (Brian Heiland)
    BackLightTemp = 0;
    BackLightTime();
    delay(initDelay+100);
    cycleImageCount = 0;
    do{
      cycleAllImagesOneshot = cycleAllImages;
      if (cycleAllImages){
        //cycleAllImagesOneshot = cycleAllImages;
        m_CurrentFilename = m_FileNames[cycleImageCount];
        if (m_CurrentFilename == "STARTUP.BMP"){
          cycleImageCount +=1;
          if (cycleImageCount  < m_NumberOfFiles){
            m_CurrentFilename = m_FileNames[cycleImageCount];
          }else{
          break;
          }
        }
      }
    interuptPressed =0;            // Might need removed Brian Heiland   
    if (repeatTimes > 1) {          // If there are repeats selected
      //interuptPressed = 0; // reset checking for interupts
      for (int x = repeatTimes; x > 0; x--) {   // cycle through repeats
        if (interuptPressed >= 3){              // Look for interupt pressed count to be 3 or higher
          interuptPressed = 0;                  // set interupt pressed back to 0 
          cycleAllImagesOneshot = 0;
          break;                                // user has pressed interupt exit the for loop and stop displaying more images
        }
        
        SendFile(m_CurrentFilename);            //display file 
        //delay(frameDelay);
        if (repeatDelay >0){                    // if there is a repeat delay Make sure strip is clear
          ClearStrip(0);                        // Before waiting with stuck pixels
        }
        delay(repeatDelay);                      // delay for the repeat delay before looping 
      }
    }else{
        interuptPressed = 0;                       // User only is displaying image one time 
      SendFile(m_CurrentFilename);               // send the file to display
     
    }
    ClearStrip(0);                              // after last image is displayed clear the strip
    updateScreen = false;                       // No need to update screen, lets keep light off, time exposure may still be happening on camera, so no lights from display.
    
    cycleImageCount +=1;
    if (repeatDelay >0){                    // if there is a repeat delay Make sure strip is clear
       ClearStrip(0);                        // Before waiting with stuck pixels
    }
    delay(repeatDelay);                      // Use repeat delay before shoewing next image 
    }while((cycleAllImagesOneshot)&&(cycleImageCount < m_NumberOfFiles));
    
    
  }
  if (keypress == 0) {                    // The Right Key was Pressed
    loopCounter = 0;                      // tell the loop counter to not read another keypress until the main loop, loops another 2000 times
    updateScreen = true;                  // Screen will need updated
    switch (menuItem) { 
      case 1:                             // Select the Next File
        BackLightOn();
        if (m_FileIndex < m_NumberOfFiles -1) {
          m_FileIndex++;
        }
        else {
          m_FileIndex = 0;                // On the last file so wrap round to the first file
        }
        DisplayCurrentFilename();
        break;
      case 2:                             // Adjust Brightness
        BackLightOn();
        if (brightness < 100) {
          brightness+=1;
        }
        break;
      case 3:                             // Adjust Initial Delay + 1 second
        BackLightOn();
        initDelay+=1000;
        break;
      case 4:                             // Adjust Frame Delay + 1 milliseconds 
        BackLightOn();
        frameDelay+=1;
        break;
      case 5:                             // Adjust Repeat Times + 1
        BackLightOn();
        repeatTimes+=1;
        break;
      case 6:                             // Adjust Repeat Delay + 100 milliseconds
        BackLightOn();
        repeatDelay+=100;
        break;
      case 7:
        BackLightOn();
        frameBlankDelay+=1;
        break;
      case 8:
        BackLightOn();
        cycleAllImages = !cycleAllImages;
        break;
    }
  }

  if (keypress == 3) {                    // The Left Key was Pressed
    loopCounter = 0;
    updateScreen = true;                  // Screen will need updated
    switch (menuItem) {                   // Select the Previous File
      case 1:
        BackLightOn();
        if (m_FileIndex > 0) {
          m_FileIndex--;
        }
        else {
          m_FileIndex = m_NumberOfFiles -1;    // On the last file so wrap round to the first file
        }
        DisplayCurrentFilename();
        delay(500);        
        break;
      case 2:                             // Adjust Brightness
        BackLightOn();
        if (brightness > 1) {
          brightness-=1;
        }
        break;
      case 3:                             // Adjust Initial Delay - 1 second
        BackLightOn();
        if (initDelay > 0) {
          initDelay-=1000;
        }
        break;
      case 4:                             // Adjust Frame Delay - 1 millisecond 
        BackLightOn();
        if (frameDelay > 0) {
          frameDelay-=1;
        }
        break;
      case 5:                             // Adjust Repeat Times - 1
        BackLightOn();
        if (repeatTimes > 1) {
          repeatTimes-=1;
        }
        break;
      case 6:                             // Adjust Repeat Delay - 100 milliseconds
        BackLightOn();
        if (repeatDelay > 0) {
          repeatDelay-=100;
        }
        break;
      case 7:
        BackLightOn();
        if(frameBlankDelay >0){
          frameBlankDelay-=1;
        }
        break;
       case 8:
        BackLightOn();
        cycleAllImages = !cycleAllImages;
        break;
    }    
  }


  if (( keypress == 1)) {                 // The up key was pressed
    loopCounter = 0;
    updateScreen = true;                  // Screen will need updated
    BackLightOn();
    if (menuItem == 1) {
      menuItem = 8;  
    }
    else {
      menuItem -= 1;
    }
  }
  if (( keypress == 2)) {                 // The down key was pressed
    loopCounter = 0;
    updateScreen = true;                  // Screen will need updated
    BackLightOn();
    if (menuItem == 8) {
      menuItem = 1;  
    }
    else {
      menuItem += 1;
    }
  }
  if (BackLightTimer == true) BackLightTime();
}

}

void setupLEDs() {
  strip.begin();
  strip.show();
}

     

void setupLCDdisplay() {
  lcd.init();  
  lcd.backlight();
  lcd.setCursor(1,0);
  lcd.print("*DLW MEGA  V3.6*");
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");
  delay(2000);  
  lcd.clear();
}

     

void setupSDcard() {
  pinMode(SDssPin, OUTPUT);
 
  while (!SD.begin(SDssPin)) {
    BackLightOn();
    lcd.print("SD init failed! ");
    delay(1000);
    lcd.clear();
    delay(500);
  }
  lcd.clear();
  lcd.print("SD init done.   ");
  delay(1000);
  root = SD.open("/");
  lcd.clear();
  lcd.print("Scanning files  ");
  delay(500);
  GetFileNamesFromSD(root);
  isort(m_FileNames, m_NumberOfFiles);
  m_CurrentFilename = m_FileNames[0];
  DisplayCurrentFilename();
}
     


int ReadKeypad() {
  adc_key_in = analogRead(0);             // read the value from the sensor  
  digitalWrite(13, HIGH);  
  key = get_key(adc_key_in);              // convert into key press
     
  if (key != oldkey) {                    // if keypress is detected
    delay(100);                            // wait for debounce time
    adc_key_in = analogRead(0);           // read the value from the sensor  
    key = get_key(adc_key_in);            // convert into key press
    if (key != oldkey) {                  
      oldkey = key;
      if (key >=0){
        return key;
      }
    }
  }
  return key;
}
      

// Convert ADC value to key number
int get_key(unsigned int input) {
  int k;
  for (k = 0; k < NUM_KEYS; k++) {
    if (input < adc_key_val[k]) {        
      return k;
    }
  }
  if (k >= NUM_KEYS)
    k = -1;                               // No valid key pressed
  return k;
}

void BackLightOn() {
 // analogWrite(BACKLIGHT,70);
  BackLightTimer = true;
  BackLightTemp =  BackLightTimeout;
}
     
void BackLightTime() {
  if ((BackLightTemp <= 255) && (BackLightTemp >= 0)) {
   // analogWrite(BACKLIGHT,BackLightTemp);
    delay(1);
  }
     
  if (BackLightTemp <= 0) {
    BackLightTimer = false;
    BackLightTemp =  BackLightTimeout;
   // analogWrite(BACKLIGHT,0);
  }
  else {
    BackLightTemp --;
    delay(1);
  }
}
     
void SendFile(String Filename) {
  char temp[14];
  Filename.toCharArray(temp,14);
     
  dataFile = SD.open(temp);
     
  // if the file is available send it to the LED's
  if (dataFile) {
    ReadTheFile();
    dataFile.close();
    if (interuptPressed >=3){
      //interuptPressed = 0;
      
      delay (100); // add a delay to prevent select button from starting sequence again.
    }
  }  
  else {
    lcd.clear();
    lcd.print("  Error reading ");
    lcd.setCursor(4, 1);
    lcd.print("file");
    BackLightOn();
    delay(1000);
    lcd.clear();
    setupSDcard();
    return;
    }
  }



void DisplayCurrentFilename() {
  m_CurrentFilename = m_FileNames[m_FileIndex];
  lcd.setCursor(0, 1);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  lcd.print(m_CurrentFilename);
}


     
void GetFileNamesFromSD(File dir) {
  int fileCount = 0;
  String CurrentFilename = "";
  while(1) {
    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      m_NumberOfFiles = fileCount;
	  entry.close();
      break;
    }
    else {
      if (entry.isDirectory()) {
        //GetNextFileName(root);
      }
      else {
        CurrentFilename = entry.name();
        if (CurrentFilename.endsWith(".bmp") || CurrentFilename.endsWith(".BMP") ) { //find files with our extension only
          if(CurrentFilename.startsWith("_")){      // mac sidecar files start with _ and should not be included, may be on card if written from Mac
          }else{
          m_FileNames[fileCount] = entry.name();
          fileCount++;
          }
        }
      }
    }
    entry.close();
  }
}
     

void latchanddelay(int dur) {
  strip.show();
  delay(dur);
}



void ClearStrip(int duration) {
  int x;
  for(x=0;x<STRIP_LENGTH;x++) {
    strip.setPixelColor(x, 0);
  }
  strip.show();
}


uint32_t readLong() {
  uint32_t retValue;
  byte incomingbyte;
     
  incomingbyte=readByte();
  retValue=(uint32_t)((byte)incomingbyte);
     
  incomingbyte=readByte();
  retValue+=(uint32_t)((byte)incomingbyte)<<8;
     
  incomingbyte=readByte();
  retValue+=(uint32_t)((byte)incomingbyte)<<16;
     
  incomingbyte=readByte();
  retValue+=(uint32_t)((byte)incomingbyte)<<24;
     
  return retValue;
}



uint16_t readInt() {
  byte incomingbyte;
  uint16_t retValue;
     
  incomingbyte=readByte();
  retValue+=(uint16_t)((byte)incomingbyte);
     
  incomingbyte=readByte();
  retValue+=(uint16_t)((byte)incomingbyte)<<8;
     
  return retValue;
}



int readByte() {
  int retbyte=-1;
  while(retbyte<0) retbyte= dataFile.read();
  return retbyte;
}


void getRGBwithGamma() {
 // g=gamma(readByte())/(101-brightness);
 // b=gamma(readByte())/(101-brightness);
 // r=gamma(readByte())/(101-brightness);
  
  g=gamma(readByte())*(brightness *0.01);          // Brian Heiland Revise.  Old formula
  b=gamma(readByte())*(brightness *0.01);          // reduced brightness 50% when brightness was changed from 
  r=gamma(readByte())*(brightness *0.01);          // 100 to 99 , by 50% brightness was nearly 0 This formula corrects this.
}



void ReadTheFile() {
  #define MYBMP_BF_TYPE           0x4D42
  #define MYBMP_BF_OFF_BITS       54
  #define MYBMP_BI_SIZE           40
  #define MYBMP_BI_RGB            0L
  #define MYBMP_BI_RLE8           1L
  #define MYBMP_BI_RLE4           2L
  #define MYBMP_BI_BITFIELDS      3L

  uint16_t bmpType = readInt();
  uint32_t bmpSize = readLong();
  uint16_t bmpReserved1 = readInt();
  uint16_t bmpReserved2 = readInt();
  uint32_t bmpOffBits = readLong();
  bmpOffBits = 54;
     
  /* Check file header */
  if (bmpType != MYBMP_BF_TYPE || bmpOffBits != MYBMP_BF_OFF_BITS) {
    lcd.setCursor(0, 0);
    lcd.print("not a bitmap");
    delay(1000);
    return;
  }
    
  /* Read info header */
  uint32_t imgSize = readLong();
  uint32_t imgWidth = readLong();
  uint32_t imgHeight = readLong();
  uint16_t imgPlanes = readInt();
  uint16_t imgBitCount = readInt();
  uint32_t imgCompression = readLong();
  uint32_t imgSizeImage = readLong();
  uint32_t imgXPelsPerMeter = readLong();
  uint32_t imgYPelsPerMeter = readLong();
  uint32_t imgClrUsed = readLong();
  uint32_t imgClrImportant = readLong();
   
  /* Check info header */
  if( imgSize != MYBMP_BI_SIZE || imgWidth <= 0 ||
    imgHeight <= 0 || imgPlanes != 1 ||
    imgBitCount != 24 || imgCompression != MYBMP_BI_RGB ||
    imgSizeImage == 0 )
    {
    lcd.setCursor(0, 0);
    lcd.print("Unsupported");
    lcd.setCursor(0, 1);
    lcd.print("Bitmap Use 24bpp");
    delay(1000);
    return;
  }
     
  int displayWidth = imgWidth;
  if (imgWidth > STRIP_LENGTH) {
    displayWidth = STRIP_LENGTH;           //only display the number of led's we have
  }
     
     
  /* compute the line length */
  uint32_t lineLength = imgWidth * 3;
  if ((lineLength % 4) != 0)
    lineLength = (lineLength / 4 + 1) * 4;
    


    // Note:  
    // The x,r,b,g sequence below might need to be changed if your strip is displaying
    // incorrect colors.  Some strips use an x,r,b,g sequence and some use x,r,g,b
    // Change the order if needed to make the colors correct.
    
    for(int y=imgHeight; y > 0; y--) {
      int bufpos=0; 
           if ((interuptPressed <= 3)&& (y <=(imgHeight -5))){     // if the interupt has not been pressed and we are working on column 5 of the image.  (look for no key presses until into 5th column of image)
            int keypress = ReadKeypad();              //Read the keypad, each successive read will be column+1 until 3 is reached.
            if ((keypress == 4) ||(digitalRead(AuxButton) == LOW)) {    // The select key was pressed or the Aux button was pressed
            interuptPressed +=1;                                        // user is trying to interupt display, increment count of frames with button held down.      
          }
         if (interuptPressed >=3){                                      // 3 or more frames have passed with button pressed
            cycleAllImagesOneshot = 0;
            ClearStrip(0);                                               // clear the strip
            break;                                                       // break out of the y for loop.
          }
        }   
      for(int x=0; x < displayWidth; x++) {                              // Loop through the x  values of the image
        uint32_t offset = (MYBMP_BF_OFF_BITS + (((y-1)* lineLength) + (x*3))) ;  
        dataFile.seek(offset);
        getRGBwithGamma();
        strip.setPixelColor(x,r,b,g);        
      }
    latchanddelay(frameDelay);
    if(frameBlankDelay > 0){
      ClearStrip(0);
      delay(frameBlankDelay);
    }

    
        if (interuptPressed >=3){                                        // this code is unreachable clean up and remove when verified not needed Brian Heiland
         // ClearStrip(0);
         dataFile.close();
          break;
        }
    }
  }



// Sort the filenames in alphabetical order
void isort(String *filenames, int n) {
  for (int i = 1; i < n; ++i) {
    String j = filenames[i];
    int k;
    for (k = i - 1; (k >= 0) && (j < filenames[k]); k--) {
      filenames[k + 1] = filenames[k];
    }
    filenames[k + 1] = j;
  }
}
     

   
PROGMEM const unsigned char gammaTable[]  = {
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,
  2,  2,  2,  2,  2,  3,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,
  4,  4,  4,  4,  5,  5,  5,  5,  5,  6,  6,  6,  6,  6,  7,  7,
  7,  7,  7,  8,  8,  8,  8,  9,  9,  9,  9, 10, 10, 10, 10, 11,
  11, 11, 12, 12, 12, 13, 13, 13, 13, 14, 14, 14, 15, 15, 16, 16,
  16, 17, 17, 17, 18, 18, 18, 19, 19, 20, 20, 21, 21, 21, 22, 22,
  23, 23, 24, 24, 24, 25, 25, 26, 26, 27, 27, 28, 28, 29, 29, 30,
  30, 31, 32, 32, 33, 33, 34, 34, 35, 35, 36, 37, 37, 38, 38, 39,
  40, 40, 41, 41, 42, 43, 43, 44, 45, 45, 46, 47, 47, 48, 49, 50,
  50, 51, 52, 52, 53, 54, 55, 55, 56, 57, 58, 58, 59, 60, 61, 62,
  62, 63, 64, 65, 66, 67, 67, 68, 69, 70, 71, 72, 73, 74, 74, 75,
  76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91,
  92, 93, 94, 95, 96, 97, 98, 99,100,101,102,104,105,106,107,108,
  109,110,111,113,114,115,116,117,118,120,121,122,123,125,126,127
};
 
 
inline byte gamma(byte x) {
  return pgm_read_byte(&gammaTable[x]);
}





