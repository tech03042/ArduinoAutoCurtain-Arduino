//블루투스 헤더
#include <SoftwareSerial.h>
SoftwareSerial bluetooth(8,9);


//DC 모터
#define MOT1DIR1      2
#define MOT1DIR2      3
#define MOT1PWM      4
#define MOT2DIR1      5
#define MOT2DIR2      6
#define MOT2PWM      7

// 조도 센서
const int cdsInPin = A0;
const int analogInPin = A1;
int cdsValue = 0;
int cdsLevel = 0;
// 초음파 센서
int ultraPin = 23;
long Distance;
unsigned long duration;
long Tempture = 25;
// 부저
const int tonePin = 25;  
// 블루투스 운용
// FLAG들
int autoControl = 0;  // 0=수동, 1=자동
int motorSpeed = 41; // 초기 스피드 100
int soundControl = 0; // 0=사운드x, 1=사운드o
int upsideDown[] = {1}; // upsideDown[0]의 0=위로올라가기, 1=멈춤, 2=아래로내려가기
int upsideDown_temp[] = {1};
int bottomCM = 60;  // 초기 커튼 길이 50cm
int currentCM = 0;  // 현재 커튼 위치


// 조도값 최댓값(1000)으로 총 1~10단계로 나눔)
int return_cdsState(int cdsValue){  
  int cdsState = 0;
  for(int i = 0; i<10; i++){
    if(cdsValue>100*(i+1))
      cdsState = i+1;
  }
  return cdsState;
}

// 모터 제어(동작유무, 방향)
void control_upsideDown(int motorSpeed2){
  currentCM = confirm_UltraCM();
  analogWrite(MOT2PWM, motorSpeed2);
  if(upsideDown[0]==0){
    digitalWrite(MOT2PWM,HIGH);
    digitalWrite(MOT2DIR1,LOW);
    digitalWrite(MOT2DIR2,HIGH); 
  }else if(upsideDown[0]==1){
    digitalWrite(MOT2PWM,LOW);
    digitalWrite(MOT2DIR1,HIGH);
    digitalWrite(MOT2DIR2,HIGH); 
  }else if(upsideDown[0]==2){
    digitalWrite(MOT2PWM,HIGH);
    digitalWrite(MOT2DIR1,HIGH);
    digitalWrite(MOT2DIR2,LOW); 
  }
  //윗 천장까지 가면 강제멈춤
//  if(confirm_UltraCM()>=bottomCM-10){
//    emergency_ControlPower();
//    Serial.println("천장에 근접하여 모터를 멈추었습니다.");
//    Serial.println(confirm_UltraCM());
//    Serial.println(bottomCM-10);
//  }else{
//    upsideDown[0] = upsideDown_temp[0];
//
//  }
  // 바닥 근접시 강제멈춤
  if(confirm_UltraCM()<=10){
    emergency_ControlPower();
    Serial.println("바닥에 근접하여 모터를 멈추었습니다.");
    Serial.println(confirm_UltraCM());
  }else{
//    upsideDown[0] = upsideDown_temp[0];
  }

  //부저음 울리기
  buzzer_Control();
}

// 초기 설정(커텐 길이 확인하여 변수에 저장)
void set_Init(){
  Serial.println("끝까지 올리는 중!(Warning! 커튼을 강제로 붙잡지 마세요.)");
  for(int t=0; t<100; t++){
    upsideDown[0] = 0;
  }
  bottomCM = confirm_UltraCM();
  Serial.println("올리기 종료!\n내려갑니다");
  while(true){
    upsideDown[0] = 2;
    currentCM = confirm_UltraCM();
    if(currentCM <10){
      break;
    }
    control_upsideDown(motorSpeed);
  }
  digitalWrite(MOT2PWM,LOW);
  Serial.println("바닥까지 내려왔습니다. 초기 셋팅이 완료되었습니다.");
}

// 모터 긴급 종료
void emergency_ControlPower(){
//  upsideDown_temp[0] = upsideDown[0];
  upsideDown[0] = 1;
}

// 초음파 센서로 바닥 확인
int confirm_UltraCM_child(){
  delay(10);
  pinMode(ultraPin,OUTPUT);
  digitalWrite(ultraPin,HIGH);
  delayMicroseconds(10);
  digitalWrite(ultraPin,LOW);
  pinMode(ultraPin,INPUT);
  duration = pulseIn(ultraPin, HIGH);
  Distance = (float)(duration / 58.0);
  //Serial.print("*value (cmm) = ");
  //Serial.println(Distance);
    
  return Distance;
}

int confirm_UltraCM(){
  int ret = confirm_UltraCM_child();
  while(ret == 0 || ret == 1)
    ret = confirm_UltraCM_child();
  return ret;
}

void buzzer_Control(){
  if(upsideDown[0] != 1 && soundControl==1){
    tone(tonePin,1000);
  }else if(upsideDown[0] ==1 || soundControl==0){
    noTone(tonePin);
  }
}

void set_BtString(){
   int incomingByte = -1;
    if(Serial3.available() >0 ){
      incomingByte = Serial3.read();
      Serial.print("Recv d: ");
      Serial.println(incomingByte, DEC);
    }
    switch (incomingByte){
    case 0:
      autoControl = 0;
      break;
    case 1:
      autoControl = 1;
      break;
    case 2:
      soundControl = 0;
      break;
    case 3:
      soundControl = 1;
      break;
    case 4:
      upsideDown[0]=0;
      break;
    case 5:
      upsideDown[0]=2;
      break;
    case 6:
      //초기 설정(바닥 길이 확인)
      set_Init();
    case 7:
      upsideDown[0]=1;
    case -1:
      break;
    default:
      motorSpeed = incomingByte/3;
      break;
  }
}


void setup() { 
    Serial.begin(9600);
    //초음파 핀
    pinMode(ultraPin, OUTPUT);
    digitalWrite(ultraPin, LOW);
    
    //부저 핀
    pinMode(tonePin, OUTPUT); 
    
    //블루투스 핀
    Serial3.begin(9600);
//    Serial3.write("AT+NAMElinvor87(PW:1234)");
      
    //DC모터 핀
    pinMode(MOT2DIR1, OUTPUT);    
    pinMode(MOT2DIR2, OUTPUT); 
    pinMode(MOT2PWM, OUTPUT);   
    digitalWrite(MOT2DIR1, HIGH);
    
    Serial.println("Start");
}

void loop() {
  //조도값 읽어오기(analog값)
  cdsLevel = return_cdsState(analogRead(cdsInPin));
  //블루투스 문자 가져와서 각 명령 실행
  set_BtString();
  
  // 오토시 작동
  if(autoControl==1){
    if(cdsLevel*(bottomCM/10)<=currentCM){
      upsideDown[1] = 1;
      upsideDown[0] = 0;
      Serial.println("어두운 상태");
      Serial.println(cdsLevel);
    }else if(cdsLevel*(bottomCM/10)>=currentCM){
      upsideDown[0] = 1;
      upsideDown[1] = 0;
      Serial.println("밝은 상태");
      Serial.println(cdsLevel);
    }
  }else if(autoControl==0){}
   
  //모터 제어(동작유무)
  control_upsideDown(motorSpeed); 
  
}
