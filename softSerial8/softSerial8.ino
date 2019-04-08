#include <SoftwareSerial.h>
#include <SPI.h>
#include <SD.h>
#include <EEPROM.h>

#define lidarCtrl 2    //雷达控制引脚
#define IN1  4         //步进马达x4
#define IN2  5
#define IN3  6
#define IN4  7
#define E2ROMaddr 108  //文件序号地址
#define RNDCNT 1536    //2048
#define FULL 280         //280       //197°
bool bl=false;         //是不是扫描状态
u8 fileNo,t[4],in,inChar;
const u8 s[4]={LOW,LOW,LOW,LOW};
const u8 p[4]= {IN1,IN2,IN3,IN4};
u16 cnt=0,Count;
u32 lcnt=0;
File myFile;
char st[10];
static bool flg=false;
void cw(void){u8 i,j;for(j=0;j<4;j++){for(i=0;i<4;i++)t[i]=s[i];t[j]=HIGH;for
(i=0;i<4;i++)digitalWrite(p[i], t[i]); delay(5);}}


void ccw(void){u8 i,j;for(j=0;j<4;j++){for(i=0;i<4;i++)t[i]=s[i];t[3-j]=HIGH;for
(i=0;i<4;i++)digitalWrite(p[i], t[i]); delay(5);}}  

void(* resetFunc) (void) = 0; //系统重启

SoftwareSerial mySerial(8,9); // RX, TX   定义软串口

void setup() {
  pinMode(lidarCtrl, OUTPUT); //定义雷达控制脚
  analogWrite(lidarCtrl, 0);  //雷达stop  
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
 
  Serial.begin(115200);while (!Serial) {; }
  mySerial.begin(115200);

  if (!SD.begin(10)) 
  {
    mySerial.println("SD initialization failed!");while (1);
  }
  mySerial.println("SD initialization done."); 

  fileNo=EEPROM.read(E2ROMaddr); 
  mySerial.print("fileNo="); mySerial.println(fileNo,DEC);

  while(!bl)  //lidar offline
  {
    while(mySerial.available()) 
    {
      in = (char)mySerial.read();
      switch(in)
      {
       case 's':             //scan
        fileNo=EEPROM.read(E2ROMaddr);
        sprintf(st,"%03d.dat",fileNo);
        myFile = SD.open(st, FILE_WRITE); 
        mySerial.println(st);          
        mySerial.println("Moto on");
      
        analogWrite(lidarCtrl, 220);delay(20);  //雷达马达启动   
        Serial.write(0xA5);Serial.write(0x21);  //雷达开始扫描
        Count=FULL;                    
        bl= true;
      break;
 //--------------------------       
      case 'd':               //dump 读出数据
        fileNo=EEPROM.read(E2ROMaddr);sprintf(st,"%03d.dat",fileNo-1);
        if(!SD.exists(st))
        {
         mySerial.println("File not exists!");delay(30); resetFunc(); 
        }
        myFile = SD.open(st, FILE_READ);
        if (myFile) 
        {
          while (myFile.available()) mySerial.write(myFile.read()); 
          myFile.close();                  
        }
        resetFunc();             
      break; 
//--------------------------
      case 'p':               //prev 前一个数据
        fileNo--; fileNo%=256;     
        EEPROM.write(E2ROMaddr,fileNo);delay(30);
        sprintf(st,"Current file :%03d.dat",fileNo);
        mySerial.println(st);
        resetFunc();             
      break; 
      case 'n':               //next  下一个数据
        fileNo++; if(fileNo<0)fileNo=0;     
        EEPROM.write(E2ROMaddr,fileNo);delay(30);
        sprintf(st,"Current file :%03d.dat",fileNo);
        mySerial.println(st);       
        resetFunc();             
      break; 
      case 'c':               //clear all 清除所有数据
        u8 i; 
        for(i=0;i<=fileNo;i++)   
        {
          sprintf(st,"%03d.dat",i);
          if(SD.exists(st))SD.remove(st);
        }
        EEPROM.write(E2ROMaddr,0);delay(30);
        resetFunc();             
      break; 
      }
    }
  }
}

void loop() {
  if(flg) 
  {
    myFile.write(inChar);
    flg=false;   
    cnt++;cnt%=RNDCNT;
    if(cnt==0)
    {
      cw();
      Count--;
      if(Count>0)mySerial.println(Count,DEC);
      if(Count==0)
      {
        analogWrite(lidarCtrl, 0);      //stop lidar's motor
        myFile.close();
        mySerial.println("Finished");   
        fileNo++; 
        if(fileNo<0)fileNo=0;     
        EEPROM.write(E2ROMaddr,fileNo);
        for(int i=0;i<FULL;i++)ccw();   //reset step motor
        if(Serial.available()) {Serial.write(0xA5);Serial.write(0x45); } //stop scan 
        resetFunc(); 
      }
    }
  }
}
void serialEvent() {
    static u8 l=0;
    if(l<7){l++;return;}
    inChar = (char)Serial.read();
    flg=true; 
    return;
}
