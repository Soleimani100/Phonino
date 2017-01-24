#include <Adafruit_GFX.h>
#include <gfxfont.h>
#include <MCUFRIEND_kbv.h>
#include <TouchScreen.h>
#include <SoftwareSerial.h>


// Assign human-readable names to some common 16-bit color values:
#define BLACK   0xFFFF - 0x0000
#define BLUE    0xFFFF - 0x001F
#define RED     0xFFFF - 0xF800
#define GREEN   0xFFFF - 0x07E0
#define CYAN    0xFFFF - 0x07FF
#define MAGENTA 0xFFFF - 0xF81F
#define YELLOW  0xFFFF - 0xFFE0
#define WHITE   0xFFFF - 0xFFFF
#define SKYBLUE 0XFFFF - 0XBFFF
#define LANDSCAPE_ORI 1

// MCUFRIEND UNO shield shares pins with the TFT.
#define YP A1   //[A1], A3 for ILI9320, A2 for ST7789V 
#define YM 7    //[ 7], 9             , 7
#define XM A2   //[A2], A2 for ILI9320, A1 for ST7789V
#define XP 6    //[ 6], 8             , 6

enum states {HOME, INBOX, HISTORY, ENTER_NUMBER, ENTER_SMS_TEXT, INCOMING_CALL, TALK, DIALING};
enum states state;

String sms = "";                          // for storing sms message text
String phone_number = "";                 // for storing phone number to send sms or call 
String caller_phone_number = "";          // for storing phone number of who called

bool isMessage = false;                   // for distinguinsh between enter text or phone number functionalities in keyboard
bool keyboard_mode = false;               // shift key in pressed or not
bool last_sms_flag = false;               // to show that last message in inbox is seen

int last_SMS_ID = 1;                      // the ID of last sms is received
int rec_sms_cnt = 0;                      // total number of sms messages that is received and are UNREAD
int inbox_idx = 1;                        // ID of current message that is showed by inbox


TouchScreen myTouch(XP, YP, XM, YM, 300); // instant of touch screen class
TSPoint tp;                               // point that is touched
MCUFRIEND_kbv tft;                        // instant of TFT-screen class
SoftwareSerial mySerial(10, 11);          // virtual serial created in ports 10 and 11, respectively are defined as RX and TX

// keyboard buttons cordinations
uint16_t X1[45];                          
uint16_t X2[45];
uint16_t Y1[45];
uint16_t Y2[45];

// keyboard characters in standard mode
char character1[45] = 
    {'1', '2', '3', '4', '5', '6', '7', '8', '9', '0', 
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l',
    '^' , 'z', 'x', 'c', 'v', 'b', 'n', 'm', '<',
    '@', '!', '?', 32, ',', '.', '>'};

// keyboard characters in shift mode
char character2[45] = 
    {'1', '2', '3', '4', '5', '6', '7', '8', '9', '0', 
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P',
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L',
    '^' , 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<',
    '@', '!', '?', 32, ',', '.', '>'};


void readResistiveTouch(void)                           // reading touch output and storing it into "tp"
{
    tp = myTouch.getPoint();
    pinMode(YP, OUTPUT);                                //restore shared pins
    pinMode(XM, OUTPUT);
    digitalWrite(YP, HIGH);                             //because TFT control pins
    digitalWrite(XM, HIGH);
}

bool ISPRESSED(void)                                    // showing that touch is pressed or not
{
    // now touch has to be stable for 50ms
    int count = 0;
    bool state, oldstate;
    while (count < 5) {
        readResistiveTouch();
        state = tp.z > 20 && tp.z < 1000;
        if (state == oldstate) count++;
        else count = 0;
        oldstate = state;
        delay(5);
    }
    return oldstate;
}

////////////////////////////////////////// for debug
void showpoint(void)
{
    int x = map(tp.y, 844, 95, 0, tft.width());
    int y = map(tp.x, 192, 874, 0, tft.height());
    Serial.print("\r\nx="); Serial.print(x);
    Serial.print(" y="); Serial.print(y);
    Serial.print(" z="); Serial.print(tp.z);
}
//////////////

///////////////////// graphical functions ////////////////////////
void home_screen(){                               // function to making home screen graphically
    tft.fillScreen(BLACK);
    tft.fillRect(70, 100, 150, 40, GREEN);
    tft.fillRect(260, 100, 150, 40, BLUE);
    tft.fillRect(70, 180, 150, 40, RED);
    tft.fillRect(260, 180, 150, 40, MAGENTA);
    tft.setCursor(110 , 110);
    tft.setTextColor(WHITE);
    tft.setTextSize(3);
    tft.print("CALL");
    tft.setCursor(275 , 110);
    tft.print("MESSAGE");
    tft.setCursor(100 , 190);
    tft.print("INBOX");
    tft.setCursor(275 , 190);
    tft.print("HISTORY"); 

    if(rec_sms_cnt > 0)
        sms_notif_screen();      
}

void inbox_screen(){                              // function to making inbox screen graphically  
    tft.fillScreen(BLACK);
    tft.setTextSize(3);
    tft.setTextColor(WHITE);
    if(inbox_idx >= 2)
    {
      tft.fillRect(70, 230, 150, 40, RED);
      tft.setCursor(110 , 240);
      tft.print("BACK");
    }
    if(!last_sms_flag)
    {
       tft.fillRect(260, 230, 150, 40, BLUE);
       tft.setCursor(300 , 240);
       tft.print("NEXT");
    }
    back_home_screen();
}

void incoming_call_screen()                     // function to making incoming call screen graphically
{
    tft.setTextSize(4);
    tft.fillScreen(BLACK);
    tft.setTextColor(WHITE);
    tft.setCursor(90, 90);
    tft.print(caller_phone_number);
    tft.fillRect(70, 180, 150, 40, RED);
    tft.fillRect(260, 180, 150, 40, GREEN);
    tft.setTextSize(3);
    tft.setCursor(95 , 190);
    tft.print("REJECT");
    tft.setCursor(285 , 190);
    tft.print("ACCEPT");
}

void talk_screen()                                // function to making talk screen graphically
{
  tft.fillScreen(BLACK);
  tft.setTextColor(WHITE);
  tft.fillRect(70, 180, 340, 40, RED);
  tft.setTextSize(3);
  tft.setCursor(200 , 190);
  tft.print("FINISH");
}

void back_home_screen()                         // function to making back-home button on screen graphically
{
  tft.setTextColor(WHITE);
  tft.fillRect(445, 0, 35, 35, RED);
  tft.setTextSize(3);
  tft.setCursor( 455 , 8);
  tft.print("X");
}

void loading_screen()                           // function to making loading screen graphically
{
    int t = 3;
    tft.setTextSize(3);
    while(t>0){
      tft.fillRect(70, 100, 340, 120, YELLOW);
      tft.setCursor(140, 150);
      tft.setTextColor(BLUE);
      tft.print("LOADING .");
      delay(500);
      tft.print(".");
      delay(500);
      tft.print(".");
      delay(500);
      t--;
    }
}

void sms_notif_screen()                         // function to making sms notification badge on home screen graphically
{
    tft.setCursor(420, 20);
    tft.fillCircle(425,25, 15, GREEN);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.print(rec_sms_cnt);
}

void call_screen()                            // function to making call screen graphically
{
    int t = 3;
    tft.setTextSize(4);
    tft.fillScreen(BLACK);
    tft.setTextColor(WHITE);
    tft.setCursor(110, 50);
    tft.print(phone_number);
    tft.setTextSize(3);
    while(t>0){
      tft.fillRect(130, 100, 340, 120, BLACK);
      tft.setCursor(140, 150);
      tft.print("CALLING .");
      delay(500);
      tft.print(".");
      delay(500);
      tft.print(".");
      delay(500);
      t--;
    }
    tft.fillRect(70, 180, 340, 40, RED);
    tft.setTextSize(3);
    tft.setCursor(200 , 190);
    tft.print("CANCEL");
}

void make_keyboard(){                           // function to making keyboard on screen graphically
  for(int i=0; i<10; i++){
    X1[i]=48*i;
    X2[i]=48*(i+1);
    Y1[i] = 120;
    Y2[i] = 160; 
    tft.drawRect(X1[i], Y1[i], 48, 40, BLUE);
  }
  for(int i = 0; i < 10; i++){
    X1[i+10]=48*i;
    X2[i+10]=48*(i+1);
    Y1[i+10] = 160;
    Y2[i+10] = 200; 
    tft.drawRect(X1[i+10], Y1[i+10], 48, 40, BLUE);
  }
  for(int i = 0; i< 9; i++){
    X1[i+20] = 24 + 48*i;
    X2[i+20] = 24 + 48*(i+1);
    Y1[i+20] = 200;
    Y2[i+20] = 240;
    tft.drawRect(X1[i+20], Y1[i+20], 48, 40, BLUE);
  }
    X1[29] = 0;
    X2[29] = 24 + 48;
    Y1[29] = 240;
    Y2[29] = 280;
    tft.drawRect(X1[29], Y1[29], 48+24, 40, BLUE);
  for(int i = 1; i< 8; i++){
    X1[i+29] = 24 + 48*i;
    X2[i+29] = 24 + 48*(i+1);
    Y1[i+29] = 240;
    Y2[i+29] = 280;
    tft.drawRect(X1[i+29], Y1[i+29], 48, 40, BLUE);
  }
    X1[37] = 24 + 48*8;
    X2[37] = 480;
    Y1[37] = 240;
    Y2[37] = 280;
    tft.drawRect(X1[37], Y1[37], 48+24, 40, BLUE);
    X1[38] = 0;
    X2[38] = 24 + 48;
    Y1[38] = 280;
    Y2[38] = 320;
    tft.drawRect(X1[38], Y1[38], 48+24, 40, BLUE);
    X1[39] = 24+48;
    X2[39] = 24 + 48*2;
    Y1[39] = 280;
    Y2[39] = 320;
    tft.drawRect(X1[39], Y1[39], 48, 40, BLUE);
    X1[40] = 24+48*2;
    X2[40] = 24 + 48*3;
    Y1[40] = 280;
    Y2[40] = 320;
    tft.drawRect(X1[40], Y1[40], 48, 40, BLUE);
    X1[41] = 24+48*3;
    X2[41] = 24 + 48*6;
    Y1[41] = 280;
    Y2[41] = 320;
    tft.drawRect(X1[41], Y1[41], 48*3, 40, BLUE);
    X1[42] = 24+48*6;
    X2[42] = 24 + 48*7;
    Y1[42] = 280;
    Y2[42] = 320;
    tft.drawRect(X1[42], Y1[42], 48, 40, BLUE);
    X1[43] = 24+48*7;
    X2[43] = 24 + 48*8;
    Y1[43] = 280;
    Y2[43] = 320;
    tft.drawRect(X1[43], Y1[43], 48, 40, BLUE);
    X1[44] = 24 + 48*8;
    X2[44] = 480;
    Y1[44] = 280;
    Y2[44] = 320;
    tft.fillRect(X1[44], Y1[44], 48+24, 40, GREEN);
    tft.setTextSize(3);
    tft.setTextColor(WHITE);
    for(int i=0; i<45 ; i++)
    {
      tft.setCursor((X1[i]+X2[i])/2 - 10, Y1[i]+10);
      if(keyboard_mode)
        tft.print(character1[i]);
      else
        tft.print(character2[i]);
    }
    tft.setCursor(X1[41]+28, Y1[41]+10);
    tft.print("SPACE");
    back_home_screen();
}

//////////////////// END of graphical functions ////////////////////

void gsm_init()                     // sum works to initialize gsm link
{
  boolean at_flag=1;
  while(at_flag)
  {
    mySerial.println("AT");
    while(mySerial.available()>0)
    {
      if(mySerial.find("OK"))
        at_flag=0;
    }
    delay(1000);
  }
  Serial.println("GSM Module is connected");
  delay(1000);
  boolean echo_flag=1;
  while(echo_flag)
  {
    mySerial.println("ATE1");
    while(mySerial.available()>0)
    {
      if(mySerial.find("OK"))
      echo_flag=0;
    }
    delay(1000);
  }
  Serial.println("echo is enabled");
  delay(1000);
  boolean net_flag=1;
  while(net_flag)
  {
    mySerial.println("AT+CPIN?");
    while(mySerial.available()>0)
    {
      if(mySerial.find("+CPIN: READY"))
      net_flag=0;
    }
    delay(1000);
  }
  Serial.println("PIN info is checked");
  boolean text_flag=1;
  while(text_flag)
  {
    mySerial.println("AT+CMGF=1");
    while(mySerial.available()>0)
    {
      if(mySerial.find("OK"))
      text_flag=0;
    }
    delay(1000);
  }
  Serial.println("text mode enabled");
  delay(1000);
}

void sendSMS(String number, String text)                  // send string text to number in form of SMS message
{
  mySerial.print("AT+CMGS=\"");
  mySerial.print(number);
  mySerial.print("\"\r");
  delay(1000);
  
  mySerial.print(text);
  mySerial.write(26);
  
  Serial.println("SMS sent");
}

void print_sms(int index)                               // printing sms message of input index in inbox section
{
  if(index < 1)
      return;
  Serial.println("index: " + String(index));
  tft.setCursor(0 , 0);
  mySerial.println("AT+CMGR=" + String(index) + ",0");  
}

void setup() {
    uint16_t g_identifier;

    Serial.begin(9600);
    mySerial.begin(9600);
    
    digitalWrite(A0, HIGH);
    pinMode(A0, OUTPUT);
    tft.reset();
    
    g_identifier = tft.readID();
    if (g_identifier == 0x00D3 || g_identifier == 0xD3D3) g_identifier = 0x9481;
    if (g_identifier == 0xFFFF) g_identifier = 0x9341;

    tft.begin(g_identifier);
    
    tft.setRotation(LANDSCAPE_ORI);
    tft.fillScreen(BLACK);
    tft.setTextColor(WHITE);
    Serial.println(tft.width());
    Serial.println(tft.height());

    loading_screen();
    gsm_init();
    home_screen();
    state = HOME;
}

void loop() {
  if(mySerial.available())                          // handling data received from gsm module
  {
      String tmp = mySerial.readString();
      if(tmp.indexOf("RING") != -1)
      {
          if(state != INCOMING_CALL)
          {
              mySerial.println("AT+CLCC");
              delay(2000);
              String tmp = mySerial.readString();
              tmp = tmp.substring(tmp.indexOf("\"")+1);
              caller_phone_number = tmp.substring(0,tmp.indexOf("\""));
              incoming_call_screen();
              state = INCOMING_CALL;
          }
      }
      if(tmp.indexOf("NO CARRIER") != -1 || tmp.indexOf("BUSY") != -1)
      {
          state = HOME;
          home_screen();
      }
      if( state == DIALING && (tmp.indexOf("OK") != -1) )
      {
          state = TALK;
          talk_screen();
      }
      if(state == INBOX && (tmp.indexOf("\"") == -1) )
      {
          last_sms_flag = true;
          inbox_screen();
          tft.println("No sms exists");
      }
      else if(state == INBOX && (tmp.indexOf("+CMGR") != -1) )
      {
          inbox_screen();
          int idx = tmp.indexOf("\",");
          //Serial.println("idx: " + String(idx));
          String str = tmp.substring(idx+3);
          //Serial.println(str2);
          tft.setTextColor(WHITE);
          tft.println(str.substring(0, str.indexOf("\"")));
          idx = str.indexOf('\n');
          //Serial.println("idx: " + String(idx));
          tft.println(str.substring(idx, str.length()-5));
          
          if(tmp.indexOf("REC UNREAD") != -1)
              rec_sms_cnt--;
      }
      if(tmp.indexOf("+CMTI") != -1)
      {
          int idx = tmp.indexOf(": \"SM\",");
          idx += 7;
          //Serial.println(tmp[idx]);
          last_SMS_ID = (tmp.substring(idx)).toInt();
          //Serial.println("SMS received on " + String(last_SMS_ID));
          rec_sms_cnt++;
          sms_notif_screen();
      }
      Serial.print(tmp);
  }
  
  if(ISPRESSED()){                                    // handling touch events
      int x = map(tp.y, 844, 95, 0, tft.width());           // convert touch screen horizonal output to x position of touch point
      int y = map(tp.x, 192, 874, 0, tft.height());         // convert touch screen vertical output to y position of touch point
      showpoint();
      switch(state)
      {
        case HOME:
            if (70 < x && x < 220 && 100 < y && y < 140)
            {
                state = ENTER_NUMBER;
                phone_number = "";
                sms = "";
                tft.fillScreen(BLACK);
                make_keyboard();
                tft.setCursor(0,0);
                tft.setTextColor(WHITE);
                tft.print("Enter Number:");
                tft.setCursor(0,40);
            }
            else if(260 < x && x < 410 && 100 < y && y < 140)
            {
                state = ENTER_NUMBER;
                isMessage = true;
                phone_number = "";
                sms = "";
                tft.fillScreen(BLACK);
                make_keyboard();
                tft.setCursor(0,0);
                tft.setTextColor(WHITE);
                tft.print("Enter Number:");
                tft.setCursor(0,40);
            }
            else if (70 < x && x < 220 && 180 < y && y < 220)
            {
                state = INBOX;
                inbox_idx = last_SMS_ID;
                Serial.println("last_SMS_ID: " + String(last_SMS_ID));
                print_sms(last_SMS_ID);
            }
            else if (260 < x && x < 410 && 180 < y && y < 220)
            {
              
            }
            break;
        case ENTER_NUMBER:
            for(int i=0; i<45; i++)
            {
              if(X1[i] < x && x < X2[i] && Y1[i] < y && y < Y2[i])
              {
                if( character1[i] == '>' )
                {
                    if(isMessage)
                    {
                        state = ENTER_SMS_TEXT;
                        tft.fillRect(0,0,480,120,BLACK);
                        back_home_screen();
                        tft.setCursor(0,0);
                        tft.setTextColor(WHITE);
                        tft.print("Enter Text:");
                        tft.setCursor(0,40);
                        isMessage = false;
                    }
                    else
                    {
                        if(phone_number == "")
                        {
                            state = HOME;
                            home_screen();
                        }
                        else
                        {
                            mySerial.println("ATD" + phone_number + ";");
                            call_screen();
                            while(!mySerial.find("OK"));
                            state = DIALING;
                        }
                    }
                }
                else if( character1[i] == '<' )
                {
                    tft.fillRect(0,40,480,80,BLACK);
                    tft.setCursor(0,40);
                    phone_number = phone_number.substring(0,phone_number.length()-1);
                    tft.print(phone_number);
                }
                else if( character1[i] == '^' )
                {
                    tft.fillRect(0,40,480,280,BLACK);
                    keyboard_mode = !keyboard_mode;
                    make_keyboard();
                    tft.setCursor(0,40);
                    tft.print(phone_number);
                }
                else
                {
                    tft.setTextColor(WHITE);
                    if(keyboard_mode)
                    {
                        tft.print(character1[i]);
                        phone_number += character1[i];
                    }
                    else
                    {
                        tft.print(character2[i]);
                        phone_number += character2[i];
                    }
                }
              }
              delay(5);
            }
            if(440 < x && x < 475 && 0 < y && y < 35)
            {
                state = HOME;
                home_screen();
            }              
            break;
        case ENTER_SMS_TEXT:
            for(int i=0; i<45; i++)
            {
              if(X1[i] < x && x < X2[i] && Y1[i] < y && y < Y2[i])
              {
                if( character1[i] == '>' )
                {
                    if(phone_number != "")
                        sendSMS(phone_number, sms);
                    state = HOME;
                    home_screen();
                }
                else if( character1[i] == '<' )
                {
                    tft.fillRect(0,40,480,80,BLACK);
                    tft.setCursor(0,40);
                    sms = sms.substring(0,sms.length()-1);
                    tft.print(sms);
                }
                else if( character1[i] == '^' )
                {
                    tft.fillRect(0,40,480,280,BLACK);
                    keyboard_mode = !keyboard_mode;
                    make_keyboard();
                    tft.setCursor(0,40);
                    tft.print(sms);
                }
                else
                {
                    tft.setTextColor(WHITE);
                    if(keyboard_mode)
                    {
                        tft.print(character1[i]);
                        sms += character1[i];
                    }
                    else
                    {
                        tft.print(character2[i]);
                        sms += character2[i];
                    }
                }
              }
              delay(5);
            }
            if(440 < x && x < 475 && 0 < y && y < 35)
            {
                state = HOME;
                home_screen();
            }
            break;

        case INCOMING_CALL:
              if( 70 < x && x < 220 && 180 < y && y < 220)
              {
                  state = HOME;
                  mySerial.println("ATH");
                  home_screen();
              }
              else if(260 < x && x < 410 && 180 < y && y < 220)
              {
                  state = TALK;
                  mySerial.println("ATA");
                  talk_screen();
              }
              break;
              
         case TALK:
              if( 70 < x && x < 410 && 180 < y && y < 220)
              {
                  state = HOME;
                  mySerial.println("ATH");
                  home_screen();
              }
              break;
          case DIALING:
              if( 70 < x && x < 410 && 180 < y && y < 220)
              {
                  state = HOME;
                  mySerial.println("ATH");
                  home_screen();
              }
              break;
          case INBOX:
              if(440 < x && x < 475 && 0 < y && y < 35)
              {
                  state = HOME;
                  last_sms_flag = false;
                  home_screen();
              }
              else if (70 < x && x < 220 && 230 < y && y < 270)
              {
                  if(last_sms_flag)
                      last_sms_flag = false;
                  if(inbox_idx >= 2)
                  {
                      inbox_idx--;
                      print_sms(inbox_idx);
                  }
                  delay(5);
              }
              else if (260 < x && x < 410 && 230 < y && y < 270)
              {
                  if(!last_sms_flag)
                  {
                      inbox_idx++;
                      print_sms(inbox_idx);
                  }
                  delay(5);
              }
      }
  }
}
