/*
  WiFiEsp test: ClientTest
  http://www.kccistc.net/
  작성일 : 2022.12.19
  작성자 : IoT 임베디드 KSH
*/
#define DEBUG
//#define DEBUG_WIFI

#include <WiFiEsp.h>
#include <SoftwareSerial.h>
#include <MsTimer2.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <DHT.h>

#define AP_SSID "embA"
#define AP_PASS "embA1234"
#define SERVER_NAME "10.10.141.65"
#define SERVER_PORT 5000
#define LOGID "BOX10"
#define BOXID "10"
#define BOXPW "357357"
#define PASSWD "PASSWD"

#define CDS_PIN A0
#define BUTTON_PIN 2
#define LED_LAMP_PIN 3
#define DHTPIN 4
#define SERVO_PIN 5
#define WIFIRX 6  //6:RX-->ESP8266 TX
#define WIFITX 7  //7:TX -->ESP8266 RX
//#define LED_BUILTIN_PIN 13
#define OPEN LOW
#define CLOSE HIGH
#define SENSOR 8
#define LED 9

#define CMD_SIZE 50
#define ARR_CNT 5
#define DHTTYPE DHT11
bool timerIsrFlag = false;
boolean lastButton = LOW;     // 버튼의 이전 상태 저장
boolean currentButton = LOW;    // 버튼의 현재 상태 저장
boolean ledOn = false;      // LED의 현재 상태 (on/off)
boolean cdsFlag = false;

char sendId[10] = "BOX10";
char sendBuf[CMD_SIZE];
char lcdLine1[17] = "Smart IoT By YSK";
char lcdLine2[17] = "WiFi Connecting!";

int cds = 0;
unsigned int secCount;
unsigned int myservoTime = 0;

unsigned long lastSensorSendTime = 0;
const unsigned long sensorInterval = 300000; // 100초

char getSensorId[10];
int sensorTime;
float temp = 0.0;
float humi = 0.0;
bool updatTimeFlag = false;
typedef struct {
  int year;
  int month;
  int day;
  int hour;
  int min;
  int sec;
} DATETIME;
DATETIME dateTime = {0, 0, 0, 12, 0, 0};
DHT dht(DHTPIN, DHTTYPE);
SoftwareSerial wifiSerial(WIFIRX, WIFITX);
WiFiEspClient client;
LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo myservo;

void setup() {
  lcd.init();
  lcd.backlight();
  lcdDisplay(0, 0, lcdLine1);
  lcdDisplay(0, 1, lcdLine2);

  //pinMode(CDS_PIN, INPUT);    // 조도 핀을 입력으로 설정 (생략 가능)
  //pinMode(BUTTON_PIN, INPUT);    // 버튼 핀을 입력으로 설정 (생략 가능)
  //pinMode(LED_LAMP_PIN, OUTPUT);    // LED 핀을 출력으로 설정
  //pinMode(LED_BUILTIN_PIN, OUTPUT); //D13

#ifdef DEBUG
  Serial.begin(115200); //DEBUG
#endif
  wifi_Setup();

  MsTimer2::set(1000, timerIsr); // 1000ms period
  MsTimer2::start();

  myservo.attach(SERVO_PIN);
  myservo.write(0);
  myservoTime = secCount;
  dht.begin();
  Serial.println("DHT sensor initialized");
}

void loop() {

  int data =digitalRead(SENSOR);
  if(data == OPEN){ 
    digitalWrite(LED,1);
    
  }else if(data == CLOSE){

    digitalWrite(LED,0);
  }

  if (client.available()) {
    socketEvent();
  }

  if (timerIsrFlag) //1초에 한번씩 실행
  {
    timerIsrFlag = false;
    if (!(secCount % 60)) //60초에 한번씩 실행
    {

     
      humi = dht.readHumidity();
      temp = dht.readTemperature();

if (isnan(humi) || isnan(temp)) {
  Serial.println("Failed to read from DHT sensor!");
} else {
  Serial.print("Humidity: ");
  Serial.print(humi);
  Serial.print("%  Temperature: ");
  Serial.print(temp);
  Serial.println("°C");
  sprintf(lcdLine2, "Humi:%d Temp:%d°C",(int)temp, (int)humi);
  lcdDisplay(0, 1, lcdLine2);
}

//#ifdef DEBUG
          //  Serial.print("Cds: ");
          //   Serial.print(cds);
          //   Serial.print(" Humidity: ");
          //   Serial.print(humi);
          //   Serial.print(" Temperature: ");
          //   Serial.println(temp);
      
//#endif
      if(!client.connected()) {
        lcdDisplay(0, 1, "Server Down");
        server_Connect();
      }
    }
    if (myservo.attached() && myservoTime + 2 == secCount)
    {
      myservo.detach();
    }
    //if (sensorTime != 0 && !(secCount % sensorTime ))
    //{
      //sprintf(sendBuf, "[%s]SENSOR@%d@%d@%d\r\n", getSensorId, cds, (int)temp, (int)humi);
      /*      char tempStr[5];
            char humiStr[5];
            dtostrf(humi, 4, 1, humiStr);  //50.0 4:전체자리수,1:소수이하 자리수
            dtostrf(temp, 4, 1, tempStr);  //25.1
            sprintf(sendBuf,"[%s]SENSOR@%d@%s@%s\r\n",getSensorId,cdsValue,tempStr,humiStr);
      */
      //client.write(sendBuf, strlen(sendBuf));
      //client.flush();
    //}
    sprintf(lcdLine1, "%02d.%02d  %02d:%02d:%02d", dateTime.month, dateTime.day, dateTime.hour, dateTime.min, dateTime.sec );
    lcdDisplay(0, 0, lcdLine1);
    if (updatTimeFlag)
    {
      client.print("[GETTIME]\n");
      updatTimeFlag = false;
    }
  }

  // currentButton = debounce(lastButton);   // 디바운싱된 버튼 상태 읽기
  // if (lastButton == LOW && currentButton == HIGH)  // 버튼을 누르면...
  // {
  //   ledOn = !ledOn;       // LED 상태 값 반전
  //   digitalWrite(LED_BUILTIN_PIN, ledOn);     // LED 상태 변경
  //   //    sprintf(sendBuf,"[%s]BUTTON@%s\n",sendId,ledOn?"ON":"OFF");
  //   sprintf(sendBuf, "[HM_CON]GAS%s\n", ledOn ? "ON" : "OFF");
  //   client.write(sendBuf, strlen(sendBuf));
  //   client.flush();
  // }
  // lastButton = currentButton;     // 이전 버튼 상태를 현재 버튼 상태로 설정

  if(true) {  // 5분마다 습도,온도 서버에 전송
    // 10초마다 센서 데이터를 서버로 전송
    if (millis() - lastSensorSendTime >= sensorInterval) {
        lastSensorSendTime = millis();  // 마지막 전송 시간 업데이트

        // 🔹 센서 값 읽기
        humi = dht.readHumidity();
        temp = dht.readTemperature();

        // 🔹 NAN 체크 (센서 값 읽기 실패 방지)
        if (isnan(humi) || isnan(temp)) {
            Serial.println("Failed to read from DHT sensor!");
            temp = -1;
            humi = -1;
        }

        // 문자열 변환 (소수점 1자리 유지)
        char tempStr[10], humiStr[10];
        dtostrf(temp, 4, 1, tempStr);
        dtostrf(humi, 4, 1, humiStr);

        // 서버로 데이터 전송
        sprintf(sendBuf, "[SGR_SQL]SETHT@BOX@10@%s@%s\n", tempStr,humiStr);
        client.write(sendBuf, strlen(sendBuf));
        client.flush();

        //  디버깅 출력
        // Serial.print("Sent Data -> Temperature: ");
        // Serial.print(temp);
        // Serial.print("°C, Humidity: ");
        // Serial.print(humi);
        // Serial.println("%");
    }
}

}
void socketEvent()
{
  int i = 0;
  char * pToken;
  char * pArray[ARR_CNT] = {0};
  char recvBuf[CMD_SIZE] = {0};
  int len;

  sendBuf[0] = '\0';
  len = client.readBytesUntil('\n', recvBuf, CMD_SIZE);
  client.flush();
#ifdef DEBUG
  Serial.print("recv : ");
  Serial.print(recvBuf);
#endif
  pToken = strtok(recvBuf, "[@]");
  while (pToken != NULL)
  {
    pArray[i] =  pToken;
    if (++i >= ARR_CNT)
      break;
    pToken = strtok(NULL, "[@]");
  }
  //[KSH_ARD]LED@ON : pArray[0] = "KSH_ARD", pArray[1] = "LED", pArray[2] = "ON"
  if ((strlen(pArray[1]) + strlen(pArray[2])) < 16)
  {
    sprintf(lcdLine2, "%s %s", pArray[1], pArray[2]);
    lcdDisplay(0, 1, lcdLine2);
  }
  if (!strncmp(pArray[1], " New", 4)) // New Connected
  {
#ifdef DEBUG
    Serial.write('\n');
#endif
    strcpy(lcdLine2, "Server Connected");
    lcdDisplay(0, 1, lcdLine2);
    updatTimeFlag = true;
    return ;
  }
  else if (!strncmp(pArray[1], " Alr", 4)) //Already logged
  {
#ifdef DEBUG
    Serial.write('\n');
#endif
    client.stop();
    server_Connect();
    return ;
  }
  else if (!strcmp(pArray[1], "SERVO"))
  {
    myservo.attach(SERVO_PIN);
    myservoTime = secCount;
    if (!strcmp(pArray[2], "ON"))
    {
      if(!strcmp(pArray[3],"CUSTOMER"))
      {
        myservo.write(180);
        sprintf(sendBuf, "[SGR_SQL]SETINFO@BOX@%s@%s@CUSTOMER\n", BOXID, pArray[4]);
        client.write(sendBuf, strlen(sendBuf));
      }
      else if(!strcmp(pArray[3],"RETURN"))
      {
        myservo.write(180);
        sprintf(sendBuf, "[SGR_SQL]SETINFO@BOX@%s@%s@RETURN\n", BOXID, pArray[4]);
        client.write(sendBuf, strlen(sendBuf));

      }
      else if(!strcmp(pArray[3],"WORKER"))
      {
        myservo.write(180);
        sprintf(sendBuf, "[%s]DOOROPEN\n", pArray[0]);

      }
    lcdDisplay(0, 1, "DOOR OPEN!");
    }
    else if(!strcmp(pArray[2], "OFF"))
    {
      myservo.write(0);
      sprintf(sendBuf, "[%s]DOORCLOSE\n", pArray[0]);
      lcdDisplay(0, 1, "DOOR CLOSE!");
    }
    else
    {
      sprintf(sendBuf, "[%s]INVAILD\n", pArray[0]);
    }
  }
  else if(!strcmp(pArray[0],"GETTIME")) {  //GETTIME
    dateTime.year = (pArray[1][0]-0x30) * 10 + pArray[1][1]-0x30 ;
    dateTime.month =  (pArray[1][3]-0x30) * 10 + pArray[1][4]-0x30 ;
    dateTime.day =  (pArray[1][6]-0x30) * 10 + pArray[1][7]-0x30 ;
    dateTime.hour = (pArray[1][9]-0x30) * 10 + pArray[1][10]-0x30 ;
    dateTime.min =  (pArray[1][12]-0x30) * 10 + pArray[1][13]-0x30 ;
    dateTime.sec =  (pArray[1][15]-0x30) * 10 + pArray[1][16]-0x30 ;
#ifdef DEBUG
//    sprintf(sendBuf,"\nTime %02d.%02d.%02d %02d:%02d:%02d\n\r",dateTime.year,dateTime.month,dateTime.day,dateTime.hour,dateTime.min,dateTime.sec );
//    Serial.println(sendBuf);
#endif
    return;
  } 
  else
    return;

  client.write(sendBuf, strlen(sendBuf));
  client.flush();

#ifdef DEBUG
  Serial.print(", send : ");
  Serial.print(sendBuf);
#endif
}
void timerIsr()
{
  timerIsrFlag = true;
  secCount++;
  clock_calc(&dateTime);
}
void clock_calc(DATETIME *dateTime)
{
  int ret = 0;
  dateTime->sec++;          // increment second

  if(dateTime->sec >= 60)                              // if second = 60, second = 0
  { 
      dateTime->sec = 0;
      dateTime->min++; 
             
      if(dateTime->min >= 60)                          // if minute = 60, minute = 0
      { 
          dateTime->min = 0;
          dateTime->hour++;                               // increment hour
          if(dateTime->hour == 24) 
          {
            dateTime->hour = 0;
            updatTimeFlag = true;
          }
       }
    }
}

void wifi_Setup() {
  wifiSerial.begin(38400);
  wifi_Init();
  server_Connect();
}
void wifi_Init()
{
  do {
    WiFi.init(&wifiSerial);
    if (WiFi.status() == WL_NO_SHIELD) {
// #ifdef DEBUG_WIFI
//       Serial.println("WiFi shield not present");
// #endif
    }
    else
      break;
  } while (1);

#ifdef DEBUG_WIFI
  Serial.print("Attempting to connect to WPA SSID: ");
  Serial.println(AP_SSID);
#endif
  while (WiFi.begin(AP_SSID, AP_PASS) != WL_CONNECTED) {
#ifdef DEBUG_WIFI
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(AP_SSID);
#endif
  }
  sprintf(lcdLine1, "ID:%s", LOGID);
  lcdDisplay(0, 0, lcdLine1);
  sprintf(lcdLine2, "%d.%d.%d.%d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
  lcdDisplay(0, 1, lcdLine2);

#ifdef DEBUG_WIFI
  Serial.println("You're connected to the network");
  printWifiStatus();
#endif
}
int server_Connect()
{
#ifdef DEBUG_WIFI
  Serial.println("Starting connection to server...");
#endif

  if (client.connect(SERVER_NAME, SERVER_PORT)) {
#ifdef DEBUG_WIFI
    Serial.println("Connect to server");
#endif
    client.print("["LOGID":"PASSWD"]");
  }
  else
  {
#ifdef DEBUG_WIFI
    Serial.println("server connection failure");
#endif
  }
}
void printWifiStatus()
{
  // print the SSID of the network you're attached to

  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength
  long rssi = WiFi.RSSI();
  // Serial.print("Signal strength (RSSI):");
  // Serial.print(rssi);
  // Serial.println(" dBm");
}
void lcdDisplay(int x, int y, char * str)
{
  int len = 16 - strlen(str);
  lcd.setCursor(x, y);
  lcd.print(str);
  for (int i = len; i > 0; i--)
    lcd.write(' ');
}
// boolean debounce(boolean last)
// {
//   boolean current = digitalRead(BUTTON_PIN);  // 버튼 상태 읽기
//   if (last != current)      // 이전 상태와 현재 상태가 다르면...
//   {
//     delay(5);         // 5ms 대기
//     current = digitalRead(BUTTON_PIN);  // 버튼 상태 다시 읽기
//   }
//   return current;       // 버튼의 현재 상태 반환
// }
