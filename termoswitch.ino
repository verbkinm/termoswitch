#include <EEPROM.h>
#include <DHT.h>
#include <LiquidCrystal.h>

static const String VERSION = "0.3";

#define PIN_RELAY 10 // вывод для реле
#define DHTPIN 13     // вывод, к которому подключается датчик
#define DHTTYPE DHT22   // DHT 22  (AM2302)
#define LED_PIN 11 //вывод для LED

static LiquidCrystal lcd(4, 5, 6, 7, 8, 9);
static DHT dht(DHTPIN, DHTTYPE);

static const int AUTOMODE_ADDR = 0;
static const int RELEON_ADDR  = 1;
static const int LOWERTEMPERATURELIMIT_ADDR = 2;

//переменные состояний
static volatile boolean autoMode = true;
static volatile boolean releOn = false;
static volatile float lowerTemperatureLimit = 20;

//лимиты выставления температуры
static const float minTemperature = 15.00;
static const float maxTemperature = 25.00;

//влажность и тепература
static float humidity  = 0;
static float temperature = 0;

void eventModeChange()
{
  static unsigned long millis_prev; //для борьбы с дребезгом контактов
  if(millis() - 300 > millis_prev)
  {
    if(autoMode)
      autoMode = false;
    else
      autoMode = true;
    printData();
    EEPROM.update(AUTOMODE_ADDR, autoMode);
    EEPROM.update(RELEON_ADDR, releOn); 
  }
  millis_prev = millis();
}

void eventFunctionalButton()
{
  static unsigned long millis_prev; //для борьбы с дребезгом контактов
  if(millis() - 300 > millis_prev)
  {
    if(autoMode)
      itereateLowerTemperatureLimit();
    else
      switchRele();
    printData();
  }
  millis_prev = millis();
}

void itereateLowerTemperatureLimit()
{
   lowerTemperatureLimit++;
   if(lowerTemperatureLimit > maxTemperature)
      lowerTemperatureLimit = minTemperature;
   EEPROM.update(LOWERTEMPERATURELIMIT_ADDR, lowerTemperatureLimit);
}

void switchRele()
{
  if(releOn)
  {
    digitalWrite(PIN_RELAY, HIGH);
    digitalWrite(LED_PIN, LOW);
    releOn = false; 
  }
  else
  {
    digitalWrite(PIN_RELAY, LOW);
    digitalWrite(LED_PIN, HIGH);
    releOn = true;
  }
  EEPROM.update(RELEON_ADDR, releOn);
}

bool checkForError()
{
  if (isnan(humidity) || isnan(temperature)) 
  {
    Serial.println("Failed to read from DHT sensor!");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Failed to read");
    lcd.setCursor(0, 1);
    lcd.print("from DHT sensor!"); 
    digitalWrite(PIN_RELAY, HIGH);
    return true;
  }
  return false;
}

void printData()
{
  Serial.print("H");Serial.print(humidity );
  Serial.print("T");Serial.print(temperature);
  Serial.print("\n");
  printToLCD();
}

void startBanner()
{
  lcd.setCursor(0, 0);
  lcd.print(String("TermoSwitch v" + VERSION));
  
  lcd.setCursor(0, 1);
  lcd.print("Strarting");
  delay(700);

  for(int i = 9; i < 16; ++i)
  {
    lcd.setCursor(i, 1);
    lcd.print(".");
    delay(700);
  }
}

void printDrop()
{
  if(humidity < 20)
    lcd.write((uint8_t)0);
  else if(humidity > 20 && humidity < 40)
    lcd.write((uint8_t)1);
  else if(humidity > 40 && humidity < 60)
    lcd.write((uint8_t)2);
  else if(humidity > 80)
    lcd.write((uint8_t)3);
}

void printThermomentr()
{
  if(temperature < 0)
    lcd.write((uint8_t)4);
  else if(temperature > 0 && temperature < 10)
    lcd.write((uint8_t)5);
  else if(temperature > 10 && temperature < 25)
    lcd.write((uint8_t)6);
  else if(temperature > 40)
    lcd.write((uint8_t)7);
}

void printToLCD()
{   
  lcd.clear(); 
  lcd.setCursor(0, 0);

  printDrop();
  lcd.print(String(":" + String(humidity , 1) + String("% "))); 
  printThermomentr();
  lcd.print(String(":" + String(temperature, 1) + (char)223)); 

  lcd.setCursor(0, 1);
  
  if(autoMode)
    lcd.print( String("M:auto(" + String(lowerTemperatureLimit, 1) + (char)223 +")") );
  else
  {
    String _status;
    if(releOn) _status = "on";
    else _status = "off";
    lcd.print( String("M:manual(" + _status + ")") );
  }
}

void execModeType()
{
  if(autoMode)
    execAutoMode();
//  else
//    execManualMode();
}

void execAutoMode()
{
  if(temperature < lowerTemperatureLimit)
  {
    digitalWrite(PIN_RELAY, LOW);
    digitalWrite(LED_PIN, HIGH);
    releOn = true;
  }
  if(temperature > lowerTemperatureLimit + 1)
  {
    digitalWrite(PIN_RELAY, HIGH);
    digitalWrite(LED_PIN, LOW);
    releOn = false;
  }  
}

//void execManualMode()
//{
//  
//}

void lcdChars()
{
  byte drop20[8] = {
    0b00100,
    0b00100,
    0b01010,
    0b01010,
    0b10001,
    0b10001,
    0b10001,
    0b01110
  };
  byte drop40[8] = {
    0b00100,
    0b00100,
    0b01010,
    0b01010,
    0b10001,
    0b10001,
    0b11111,
    0b01110
  };
  byte drop60[8] = {
    0b00100,
    0b00100,
    0b01010,
    0b01010,
    0b10001,
    0b11111,
    0b11111,
    0b01110
  };
  byte drop80[8] = {
    0b00100,
    0b00100,
    0b01110,
    0b01110,
    0b11111,
    0b11111,
    0b11111,
    0b01110
  };

  byte thermometer0[8] = {
    0b00100,
    0b01010,
    0b01010,
    0b01010,
    0b01010,
    0b10001,
    0b10001,
    0b01110
  };
  byte thermometer10[8] = {
    0b00100,
    0b01010,
    0b01010,
    0b01010,
    0b01010,
    0b10001,
    0b11111,
    0b01110
  };
  byte thermometer25[8] = {
    0b00100,
    0b01010,
    0b01010,
    0b01010,
    0b01110,
    0b11111,
    0b11111,
    0b01110
  };
  byte thermometer40[8] = {
    0b00100,
    0b01110,
    0b01110,
    0b01110,
    0b01110,
    0b11111,
    0b11111,
    0b01110
  };
  lcd.createChar(0, drop20);
  lcd.createChar(1, drop40);
  lcd.createChar(2, drop60);
  lcd.createChar(3, drop80);
  lcd.createChar(4, thermometer0);
  lcd.createChar(5, thermometer10);
  lcd.createChar(6, thermometer25);
  lcd.createChar(7, thermometer40);
}
void setup() 
{
  attachInterrupt(0, eventModeChange, RISING);
  attachInterrupt(1, eventFunctionalButton, RISING);
 
  pinMode(PIN_RELAY, OUTPUT); // Объявляем пин реле как выход
  digitalWrite(PIN_RELAY, HIGH); // Выключаем реле - посылаем высокий сигнал

  pinMode(LED_PIN, OUTPUT); // Объявляем пин led
  digitalWrite(LED_PIN, LOW); // Выключаем led
  
  Serial.begin(9600);
  dht.begin();  
  lcdChars();
  lcd.begin(16, 2);

  startBanner();

  autoMode = EEPROM.read(AUTOMODE_ADDR);
  releOn = EEPROM.read(RELEON_ADDR);
  if(releOn)
  {
    digitalWrite(PIN_RELAY, LOW);
    digitalWrite(LED_PIN, HIGH);
  }
  lowerTemperatureLimit = EEPROM.read(LOWERTEMPERATURELIMIT_ADDR);
}

void loop() 
{
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();

  if(checkForError())
    return;
    
  execModeType();
  
  printData();

  delay(2000);
}
