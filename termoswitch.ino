#include <EEPROM.h>
#include <DHT.h>
#include <LiquidCrystal.h>

#define PIN_RELAY 10 // вывод для реле
#define DHTPIN 13     // вывод, к которому подключается датчик
#define DHTTYPE DHT22   // DHT 22  (AM2302)

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
    releOn = false; 
  }
  else
  {
    digitalWrite(PIN_RELAY, LOW);
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
  lcd.print("Strarting.");
  delay(700);
  lcd.clear();
  lcd.print("Strarting..");
  delay(700);
  lcd.clear();
  lcd.print("Strarting...");
  delay(700);
}

void printToLCD()
{   
  lcd.clear(); 
  lcd.setCursor(0, 0);
  
  lcd.print(String("H:" + String(humidity , 1) + String("% "))); 
  lcd.print(String("T:" + String(temperature, 1) + (char)223)); 

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
    releOn = true;
  }
  if(temperature > lowerTemperatureLimit + 1)
  {
    digitalWrite(PIN_RELAY, HIGH);
    releOn = false;
  }  
}

//void execManualMode()
//{
//  
//}

void setup() 
{
  attachInterrupt(0, eventModeChange, RISING);
  attachInterrupt(1, eventFunctionalButton, RISING);
 
  pinMode(PIN_RELAY, OUTPUT); // Объявляем пин реле как выход
  digitalWrite(PIN_RELAY, HIGH); // Выключаем реле - посылаем высокий сигнал
  
  Serial.begin(9600);
  dht.begin();
  lcd.begin(16, 2);

  startBanner();

  autoMode = EEPROM.read(AUTOMODE_ADDR);
  releOn = EEPROM.read(RELEON_ADDR);
  if(releOn)
    digitalWrite(PIN_RELAY, LOW);
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
