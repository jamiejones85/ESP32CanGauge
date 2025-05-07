#include <TFT_eSPI.h>
#include <ACAN_ESP32.h>
#include <esp_task_wdt.h>

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sprite = TFT_eSprite(&tft);

#define BRIGHTNESS 14
#define CAN_BAUD 500000
#define WDT_TIMEOUT 3

//......................................colors
#define backColor 0x0026
#define gaugeColor 0x055D
#define dataColor 0x0311
#define purple 0xEA16
#define needleColor 0xF811

//............................dont edit this
int cx=160;
int cy=120;
int r=102;
int ir=100;
int n=0;
int angle=0;

float x2[360]; //outer points of soc gauges
float y2[360];
float px2[360]; //ineer point of soc gauges
float py2[360];
float lx2[360]; //text of soc gauges
float ly2[360];
float nx2[360]; //needle low of soc gauges
float ny2[360];

double rad=0.01745;
unsigned short color1;
unsigned short color2;
float sA;
float rA;
int brightnesses[5]={40,80,120,150,240};
int selectedBrightness=3;
long previousMillis = 0; 
CANMessage inFrame;

//........................................................colors
unsigned short blockCurrentColor[15]={0x4000, 0x4800, 0x5000, 0x5800, 0x6000, 0x6800, 0x7000, 0x7800, 0x8000, 0x8800, 0x9000, 0xa000, 0xb000, 0xc000, 0xd000};
unsigned short blockChargingColor[15]={0x0280, 0x0300, 0x0380, 0x0400, 0x0480, 0x0500, 0x0580, 0x0600, 0x0680, 0x0700, 0x0780, 0x0780, 0x0800, 0x0800, 0x0880};
unsigned short blockColor[4]={0x0312,0x0290,0x01EC,0x016A};
int16_t gaugeValues[20]={-30, -20, -10, 0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120, 130, 140, 150, 160};


// .........important variables
int socAngle=100;  //.....SOC variable 0-10
int temperature = 0;
int current = 0;
int voltage = 0;
float remainingKHW = 0;
unsigned int batMin, batMax;

void setup() {
  Serial.begin(115200);
  tft.init();

  tft.setRotation(3);
  tft.fillScreen(backColor);
  sprite.createSprite(320,240);
  sprite.setSwapBytes(true);
  sprite.setTextDatum(4);
  sprite.setTextColor(TFT_WHITE,backColor);
  sprite.setTextDatum(4);
    

  int a=120;
  for(int i=0;i<360;i++)
    {
       x2[i]=((r-10)*cos(rad*a))+320-cx;
       y2[i]=((r-10)*sin(rad*a))+cy;
       px2[i]=((r-14)*cos(rad*a))+320-cx;
       py2[i]=((r-14)*sin(rad*a))+cy;
       lx2[i]=((r-24)*cos(rad*a))+320-cx;
       ly2[i]=((r-24)*sin(rad*a))+cy;
       nx2[i]=((r-36)*cos(rad*a))+320-cx;
       ny2[i]=((r-36)*sin(rad*a))+cy;

       a++;
       if(a==360)
       a=0;
    }
  Serial.println("Can0 Setup") ;

  ACAN_ESP32_Settings canSettings(CAN_BAUD);
  canSettings.mRxPin = GPIO_NUM_4;
  canSettings.mTxPin = GPIO_NUM_5;

  const ACAN_ESP32_Filter filter = ACAN_ESP32_Filter::dualStandardFilter (
    ACAN_ESP32_Filter::data, 0x420, 0x42F, // ?
    ACAN_ESP32_Filter::data, 0x155, 0x0 //exact match
  ) ;
  uint16_t errorCode = ACAN_ESP32::can.begin(canSettings, filter);
  if (errorCode > 0) {
    Serial.print ("Can0 Configuration error 0x") ;
    Serial.println (errorCode, HEX) ;
  }
  Serial.println("Can0 Setup Done") ;
 
  // enable WDT
  noInterrupts();                                         // don't allow interrupts while setting up WDOG
  esp_task_wdt_init(WDT_TIMEOUT, true); //enable panic so ESP32 restarts
  esp_task_wdt_add(NULL); //add current thread to WDT watch
  interrupts();

}

void resetwdog()
{
  noInterrupts();                                     //   No - reset WDT
  esp_task_wdt_reset();
  interrupts();
}

void handleCanFrame() {

  if (inFrame.id == 0x155) {
    socAngle = (float) ((inFrame.data[4] << 8) + inFrame.data[5]) * 0.0025;
    voltage = (float) ((inFrame.data[6] << 8) + inFrame.data[7]) / 2;
    int16_t rawCurrent = ((inFrame.data[1] << 8) + inFrame.data[2]) & 0xFFF;
    rawCurrent = (float)(rawCurrent * 0.25);
    rawCurrent = rawCurrent - 500;
    current = rawCurrent * -1;
    if (current > 160) {
      current = 160;
    }
    if (current < -30) {
      current = -30;
    }
  } else if (inFrame.id == 0x424) {
     uint8_t minTempC = (uint8_t)(inFrame.data[4]) - 40;
     uint8_t maxTempC = (uint8_t)(inFrame.data[7]) - 40;
     temperature = (minTempC + maxTempC) / 2;
  } else if (inFrame.id == 0x425) {
    batMin = (((float)(((inFrame.data[6] & 0x01) << 8) + inFrame.data[7]) + 100) * 10);
    batMax = ((float)(((inFrame.data[4] & 0x03) << 7) + ((inFrame.data[5] >> 1) + 100)) * 10);
    remainingKHW = (float)((((inFrame.data[0] & 0x01) << 8) + inFrame.data[1]) * 0.1);

  }
}

void draw()
{
  sprite.fillSprite(backColor);

  for(int i=0;i<4;i++) {
   sprite.fillRect(0, 10+i*55, 320, 50, blockColor[i]);  
  }


  //brightness levels
  // for(int i=0;i<selectedBrightness;i++) {
  //     sprite.fillSmoothRoundRect(8+(i*4),6,2,9,1,TFT_ORANGE,backColor);   
  // }

  sprite.fillSmoothCircle(320-cx, cy, r+2, backColor);

 
  sprite.drawSmoothArc(320-cx, cy, r, ir, 30, 330, gaugeColor, backColor);

  sprite.drawSmoothArc(320-cx, cy, r-5, r-6, 30, 330, TFT_WHITE, backColor);  
  sprite.drawSmoothArc(320-cx, cy, r-38, ir-37, 10, 350, gaugeColor, backColor);
  

  for(int i=0;i<20;i++){
    if(i<3) {
      color1=gaugeColor; color2=TFT_GREEN;
    } else if(i<14) {
      color1=gaugeColor; color2=TFT_WHITE;
    } else {
      color1=gaugeColor; color2=TFT_RED;
    }

    if(i%2==0) {
      sprite.drawWedgeLine(x2[i*16],y2[i*16],px2[i*16],py2[i*16],2,1,color1);
      sprite.setTextColor(color2,backColor);
      sprite.drawString((String)gaugeValues[i],lx2[i*16],ly2[i*16], 2);
    } else {
      sprite.drawWedgeLine(x2[i*16],y2[i*16],px2[i*16],py2[i*16],1,1,color2);
    }
  }

  // ........................................needles draw
   long gaugeValue = map(current, -30, 170, 0, 100);
  
   rA=2*gaugeValue*1.6;
   sprite.drawWedgeLine(px2[(int)rA],py2[(int)rA],nx2[(int)rA],ny2[(int)rA],2,2,needleColor);


  char strBuf[10];

  //.....................................drawing  TEXT

  if (socAngle > 40) {
    sprite.setTextColor(TFT_GREEN,backColor);
  } else if (socAngle > 20) {
    sprite.setTextColor(TFT_ORANGE,backColor);
  } else {
    sprite.setTextColor(TFT_RED,backColor);
  }
  sprintf(strBuf, "%i%%", socAngle);
  sprite.drawString((String)strBuf, 320-cx, cy,4);

  sprite.setTextColor(TFT_WHITE,backColor);
  sprintf(strBuf, "%iv", voltage);
  sprite.drawString((String)strBuf,320-cx,cy-40,2);



  sprite.setTextColor(TFT_ORANGE,backColor);


  sprite.drawString("Battery",320-cx,cy + 20, 2);

  sprite.drawString("Amps",320-cx, 220, 4);

  // sprintf(strBuf, "%iA", current);
  // sprite.drawString((String)strBuf, 20, 230, 4);


  sprite.fillSmoothCircle(248, 40, 5, TFT_WHITE);
  sprite.fillRect(246, 22, 5, 20, TFT_WHITE);  

  sprite.setTextColor(TFT_WHITE, blockColor[0]);
  sprintf(strBuf, "%iC", temperature);
  sprite.drawString((String)strBuf,285 ,36,4);

  sprintf(strBuf, "%.0f", remainingKHW);
  sprite.drawString((String)strBuf,20 ,100,4);
  
  sprite.setTextColor(TFT_WHITE, blockColor[3]);

  sprite.drawString("Min Cell",30, 185, 2);
  sprintf(strBuf, "%.3fv", ((float)batMin/1000));
  sprite.drawString((String)strBuf, 50, 210, 4);

  sprite.drawString("Max Cell",290, 185, 2);
  sprintf(strBuf, "%.3fv", ((float)batMax/1000));
  sprite.drawString((String)strBuf, 275, 210, 4);

  sprite.setTextColor(TFT_WHITE, blockColor[0]);
  sprite.drawString("Delta", 20, 20, 2);
  sprintf(strBuf, "%.0fmV", (float)(batMax - batMin));
  sprite.drawString(strBuf, 40, 45, 4);

   //..............................................push Sprite to screen  
  sprite.pushSprite(0,0);
  
}

void loop() {
  
  draw();
  while (ACAN_ESP32::can.receive(inFrame)) {
    handleCanFrame();
  }
  resetwdog();

}
