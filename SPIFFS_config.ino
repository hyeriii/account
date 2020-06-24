#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>
#include <ETH.h>

#define ETH_ADDR        1
#define ETH_POWER_PIN   5
#define ETH_MDC_PIN     23
#define ETH_MDIO_PIN    18
#define ETH_TYPE        ETH_PHY_IP101

AsyncWebServer server(80);
static bool eth_connected = false;

// REPLACE WITH YOUR NETWORK CREDENTIALS
const char* ssid = "ever";
const char* password = "06300630";

const char* PARAM_ID = "input_ID";
const char* PARAM_PW = "input_PW";

const char* PARAM_ID_CHECK = "input_id_check";
const char* PARAM_PW_CHECK = "input_pw_check";

IPAddress local_ip(192, 168, 0, 41);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns(210, 94, 0, 73);

int account = 0 ;

struct info { //구조체 정의

  char id[20];
  char pw[20];

};

struct info a[10]; // 10명의 사용자를 받겠다.

/***********JSON형식*************/
String Name(String a) {
  String temp = "\"{v}\":";
  temp.replace("{v}", a);
  return temp;
}

String strVal(String a) {
  String temp = "\"{v}\",";
  temp.replace("{v}", a);
  return temp;
}

String strVal1(String a) {
  String temp = "\"{v}\"&";
  temp.replace("{v}", a);
  return temp;
}


void WiFiEvent(WiFiEvent_t event)
{
  switch (event) {
    case SYSTEM_EVENT_ETH_START:
      Serial.println("ETH Started");
      //set eth hostname here
      ETH.setHostname("esp32-ethernet");
      break;

    case SYSTEM_EVENT_ETH_CONNECTED:
      Serial.println("ETH Connected");
      break;

    case SYSTEM_EVENT_ETH_GOT_IP:
      Serial.print("ETH MAC: ");
      Serial.print(ETH.macAddress());
      Serial.print(", IPv4: ");
      Serial.print(ETH.localIP());
      Serial.print(", ");
      Serial.print(ETH.subnetMask());
      Serial.print(", ");
      Serial.print(ETH.gatewayIP());
      if (ETH.fullDuplex()) {
        Serial.print(", FULL_DUPLEX");
      }
      Serial.print(", ");
      Serial.print(ETH.linkSpeed());
      Serial.println("Mbps");
      eth_connected = true;
      break;

    case SYSTEM_EVENT_ETH_DISCONNECTED:
      Serial.println("ETH Disconnected");
      eth_connected = false;
      break;

    case SYSTEM_EVENT_ETH_STOP:
      Serial.println("ETH Stopped");
      eth_connected = false;
      break;

    default:
      break;
  }
}

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}
/*void stringTo(String ssidTemp, String passTemp) { // 스트링 SSID / PASS 배열에 저장

  for (int i = 0; i < ssidTemp.length(); i++) a[i].id = ssidTemp[i];
  a[ssidTemp.length()].id = '\0';
  for (int i = 0; i < passTemp.length(); i++) a[i].pw = passTemp[i];
  a.pw[passTemp.length()] = '\0';
  }
*/
/* 데이터 저장 형식 - JSON 형식
   첫 번째, "스트링"처럼 큰 따옴표 기호(")로 묶인 스트링은 변수 명이고 고유하다.
   두 번째, 변수에 대한 값은 연이어 나오는':'문자 다음에 위치한다.
   세 번째, 값이 스트링이면 큰 따옴표(")로 묶이고, 값이 숫자(소수/정수)이면 큰 따옴표(")가 없다
   네 번째, 값 다음에 반드시 콤마(,)가 있다. */

bool saveConfig() { // "SSID":"YOUR_SSID","PASS":"YOUR_PASS"
  String value;
  value = Name("SSID") +  strVal(a[0].id) ;
  Serial.println(value);

  value +=  Name("PASS") + strVal1(a[0].pw)  ;
  Serial.println(value);

  File configFile = SPIFFS.open("/config.txt", "w"); // a로 하면 저장이 안됨 왜지?
  Serial.println("쓰기위해 파일을 열었다. ");

  if (!configFile) {
    Serial.println("쓰기 위해 구성 파일을 열지 못했습니다.");
    return false;
  }
  configFile.println(value); // SPIFF config.txt에 데이터 저장, '\n'포함
  configFile.close();
  return true;
}

String json_parser(String s, String a) {
  String val;
  if (s.indexOf(a) != -1) { // 문자열 s에  a가 있다면
    int st_index = s.indexOf(a);
    int val_index = s.indexOf(':', st_index);
    if (s.charAt(val_index + 1) == '"') {     // 값이 스트링 형식이면
      int ed_index = s.indexOf('"', val_index + 2);
      val = s.substring(val_index + 2, ed_index);
    }
    else {                                   // 값이 스트링 형식이 아니면
      int ed_index = s.indexOf(',', val_index + 1);
      val = s.substring(val_index + 1, ed_index);
    }
  }
  else {
    Serial.print(a); Serial.println(F(" is not available"));
  }
  return val;
}

bool loadConfig() {
  File configFile = SPIFFS.open("/config.txt", "r"); // 파일을 읽는다
  if (!configFile) {
    Serial.println("Failed to open config file");
    return false;
  }
  String line = configFile.readStringUntil('\n');//&문자까지 읽겠다.
  configFile.close(); // 다 읽고 파일 닫음
  String ssidTemp = json_parser(line, "SSID");
  String passTemp = json_parser(line, "PASS");

  strcpy(a[0].id, ssidTemp.c_str());
  strcpy(a[0].pw, passTemp.c_str()); // String을 배열에 저장

  Serial.print("파일에 저장된 아이디는");
  Serial.println(a[0].id);

  Serial.print("파일에 저장된 비밀번호는");
  Serial.println(a[0].pw);
  return true;
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("Mounting FS...");

  if (!SPIFFS.begin()) {
    Serial.println("Failed to mount file system");
    return;
  }
  if (SPIFFS.exists("/config.txt"))  loadConfig();
  else saveConfig();
  Serial.print("재부팅시 아이디는");
  Serial.print(a[0].id);
  Serial.println("입니다.");

  Serial.print("재부팅시 비밀번호는");
  Serial.print(a[0].pw);
  Serial.println("입니다.");

  WiFi.onEvent(WiFiEvent);
  ETH.begin(ETH_ADDR, ETH_POWER_PIN, ETH_MDC_PIN, ETH_MDIO_PIN, ETH_TYPE);
  ETH.config(local_ip, gateway, subnet, dns);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("WiFi Failed!");
    return;
  }
  Serial.println();
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Send web page with input fields to client
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/SPIFFS.html");
  });

  server.on("/next", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/login.html");
  });


  server.on("/check", HTTP_GET, [](AsyncWebServerRequest * request) {
    String input_id_check;
    String input_pw_check;
    char input_ch[10];

    if (request->hasParam( PARAM_ID_CHECK)) {
      input_id_check = request->getParam( PARAM_ID_CHECK )->value();
      input_pw_check = request->getParam( PARAM_PW_CHECK )->value();

      Serial.print("입력한 아이디는");
      Serial.println(input_id_check);



      File configFile = SPIFFS.open("/config.txt", "r");
      String line = configFile.readStringUntil('&');
      configFile.close();

      String ssidTemp = json_parser(line, "SSID");
      String passTemp = json_parser(line, "PASS");

      if (input_id_check == ssidTemp) {

        Serial.print("입력한 비밀번호는");
        Serial.println(input_pw_check);

        if (input_pw_check == passTemp) request->send(SPIFFS, "/hello.html");
        else  request->send(SPIFFS, "/fail.html");
      }

      else  request->send(SPIFFS, "/fail.html");
    }
    else  request->send(SPIFFS, "/fail.html");
  });



  server.on("/new_account", HTTP_GET, [] (AsyncWebServerRequest * request) { // ID입력
    String input_id;
    String input_pw;
    account++;

    //File configFile = SPIFFS.open("/config.txt", "r");
    //configFile.close();

    // GET inputString value on <ESP_IP>/get?inputString=<inputMessage>
    if ((request->hasParam(PARAM_ID)) && (request->hasParam(PARAM_ID))) {
      input_id = request->getParam(PARAM_ID)->value();

      Serial.print("현재 등록한 계정의 번호는 ");
      Serial.println(account);

      strcpy(a[0].id, input_id.c_str());
      Serial.print("아이디는");
      Serial.println(a[0].id);

      input_pw = request->getParam(PARAM_PW)->value(); //string 으로 파라미터 값 받음
      strcpy(a[0].pw, input_pw.c_str());// string에서 char(C스트링)으로 변환
      Serial.print("비밀번호는");

      Serial.println( a[0].pw);

      saveConfig();
      loadConfig();
      // stringTo(input_id,input_pw);
      //saveConfig();
      //inputMessage.toCharArray(id, inputMessage.length());
      //  writeFile(SPIFFS, "/inputString.txt",input_id .c_str()); // 입력한 ID 파일에 저장


    }

    else {
      input_id = "No message sent";
 request->send(200, "text/text", "Hello" );
    }
    //Serial.println(input_id );
   

  });

  server.onNotFound(notFound);
  server.begin();
}

void loop() {
  /* if(Serial.available() > 0){
     String temp = Serial.readStringUntil('\n');
     if (temp == "1") {
       Serial.println("File System Format....");
       if(SPIFFS.format())  Serial.println("File System Formated");  //Format File System
       else   Serial.println("File System Formatting Error");
     }
     else if (temp.startsWith("id:")) {
       Serial.println("SPIFF test");
       temp.remove(0, 3);
       int index = temp.indexOf(",");
       String ssidTemp = temp.substring(0, index);
       temp.remove(0, index+1);
       String passTemp = temp;
       stringTo(ssidTemp, passTemp);
       timeZone = -1.5;
       summerTime = 30;
       saveConfig();
       loadConfig();
       Serial.println(ssid);
       Serial.println(pass);
       Serial.println(timeZone);
       Serial.println(summerTime);
     }
    }*/
}
