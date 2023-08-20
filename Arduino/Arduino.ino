#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>

#define BUTTON_red 2
#define BUTTON_yellow 3
#define BUTTON_green 4
#define BUTTON_white 5
#define LED_red 6
#define LED_green 7
#define buzzer 8
#define trigSensor 10
#define echoSensor 11

LiquidCrystal_I2C lcd(0x27, 16, 2);
SoftwareSerial Arduino(12, 13);


//-------------------HIGH = pullup button-------------------
//RED
int StateRed;              // actual read value from button red
const int oldStateRed = 0; // last read value from button red (NC)

//YELLOW
int StateYellow; 
const int oldStateYellow = 1;

//GREEN
int StateGreen; 
const int oldStateGreen = 1;

//WHITE
int StateWhite;
const int oldStateWhite = 1;

//----------------------------------------------------------
unsigned long start, current, dataTimer;
unsigned long dataRespon, dataStop, dataHandling = 0;
bool hitung = false;

struct PrintDuration {
  float HH;
  float MM;
  float SS;
  float mS;
}Print;

struct PrintDuration last;

int counter = 0;
int Sensor = 0;
int prevSensor = 0;

String Status = ""; //send data status produksi

void transferData(){
  Arduino.print(counter);
  Arduino.print('@');
  Arduino.print(Status);
  Arduino.print('#');
  Arduino.print(dataRespon);
  Arduino.print('$');
  Arduino.print(dataStop);
  Arduino.print('^');
  Arduino.print(dataHandling);
  Arduino.print('*');
  Arduino.print('\n');
}

//-------------------Sensor Reader-------------------

void countProduct(){
  long duration, distance;  //sensor
  digitalWrite(trigSensor, LOW);
  delayMicroseconds(2);
  digitalWrite(trigSensor, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigSensor, LOW);

  duration = pulseIn(echoSensor, HIGH);
  distance = (duration/2)/29.1;
  if (distance <= 15){ 
    Sensor = 1;
  }
  else{
    Sensor = 0;
  }
  if(digitalRead(LED_green) == HIGH){
    if(Sensor != prevSensor){
      if(Sensor == 1){
        counter = counter + 1;
        Status = "Not Found";
        Serial.print(counter);
        Serial.print("\t");
        transferData();
      }
    }
    delay(1000);
  }
}

//-------------------Timer Setup-------------------

void printTime(struct PrintDuration Print){
  if (Print.HH < 10){
    lcd.print("0");
  }
  lcd.print(Print.HH,0);
  lcd.print(":");
  if (Print.MM < 10){
    lcd.print("0");
  }
  lcd.print(Print.MM,0);
  lcd.print(":");
  if (Print.SS < 10){
    lcd.print("0");
  }
  lcd.print(Print.SS,0);
  lcd.print(".");
  if (Print.HH < 10){
    lcd.print(Print.mS,0); 
    lcd.print("   ");
  }
}

void printSerial(struct PrintDuration Print){
  if (Print.HH < 10){
    Serial.print("0");
  }
  Serial.print(Print.HH,0);
  Serial.print(":");
  if (Print.MM < 10){
    Serial.print("0");
  }
  Serial.print(Print.MM,0);
  Serial.print(":");
  if (Print.SS < 10){
    Serial.print("0");
  }
  Serial.println(Print.SS,0);
}

void calculate(unsigned long totalTime){
  struct PrintDuration calc;
  unsigned long sisa;

  calc.HH = int (totalTime/3600000);
  sisa = totalTime % 3600000;
  calc.MM = int (sisa/60000);
  sisa = sisa % 60000;
  calc.SS = int (sisa/1000);
  calc.mS = sisa % 100;

  if (hitung){
    lcd.setCursor(3,1);
    printTime(calc);
  }
  else {
    printSerial(calc);
  }

  last.HH = calc.HH;
  last.MM = calc.MM;
  last.SS = calc.SS;
  last.mS = calc.mS;
}


void setup() {
  Serial.begin(115200);
  Arduino.begin(115200);
  pinMode(trigSensor, OUTPUT);
  pinMode(echoSensor, INPUT);
  pinMode(BUTTON_red, INPUT_PULLUP);
  pinMode(BUTTON_yellow, INPUT_PULLUP);
  pinMode(BUTTON_green, INPUT_PULLUP);
  pinMode(BUTTON_white, INPUT_PULLUP);
  pinMode(LED_red, OUTPUT);
  pinMode(LED_green, OUTPUT);
  pinMode(buzzer, OUTPUT);

  digitalWrite(LED_green, HIGH);
    
  lcd.init();
  lcd.backlight();  
  lcd.setCursor(0,0);
  lcd.print(" Good Condition ");
}

void loop(){
  StateRed = digitalRead(BUTTON_red);
  StateYellow = digitalRead(BUTTON_yellow);
  StateGreen = digitalRead(BUTTON_green);
  StateWhite = digitalRead(BUTTON_white);  
  countProduct();
  
  if (StateRed != oldStateRed){
    start = millis() - dataTimer;
    hitung = true;
    digitalWrite(LED_red, HIGH);
    digitalWrite(buzzer, HIGH);
    digitalWrite(LED_green, LOW);    
    Status = "maintenance";

    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Maintenance Problem");
    Serial.println("");
    Serial.println(Status);
    transferData();
    delay(500);
  }
  
  else if (StateYellow != oldStateYellow){
    start = millis() - dataTimer;
    hitung = true;
    digitalWrite(LED_red, HIGH);
    digitalWrite(buzzer, HIGH);
    digitalWrite(LED_green, LOW);
    Status = "Quality-Control";
    
    lcd.clear();
    delay(20);
    lcd.setCursor(0,0);
    lcd.print("Quality Problem");
    Serial.println("");
    Serial.println(Status);
    transferData();
    delay(500);
  }

  else if (StateGreen != oldStateGreen){
    dataStop = dataTimer;
    hitung = false;
    Serial.println("Durasi Terjadi Permasalahan");
    calculate(dataStop);
    Serial.println(dataStop);
    digitalWrite(LED_red, LOW);
    digitalWrite(buzzer, LOW);
    digitalWrite(LED_green, HIGH);
    delay(1000);
    lcd.clear();   
    
    dataHandling = dataStop - dataRespon;
    Serial.println("Durasi Penanganan");
    calculate(dataHandling);
    Serial.println(dataHandling);
    transferData();
    delay(1000);
    lcd.clear();
    
    dataTimer = 0;
    lcd.setCursor(0,0);
    lcd.print(" Good Condition ");
    lcd.setCursor(3,1);
    last.HH = 0.0;
    last.MM = 0.0;
    last.SS = 0.0;
    last.mS = 0.0;
    printTime(last);  
  }

  else if (StateWhite != oldStateWhite){
    dataRespon = dataTimer;
    hitung = true;
    digitalWrite(buzzer, LOW);
    Serial.println("Durasi Respon Teknisi");
    printSerial(last);
    Serial.println(dataRespon);
    transferData();
    delay(1000);
  }
  
  if (hitung){    //harus dibuat gini, kalo lgsg panggil fungsi calculate bakal error
    current = millis();
    dataTimer = current - start;
    calculate(dataTimer);
  }
}
