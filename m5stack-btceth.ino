#include <WiFi.h>
#include <WiFiAP.h>
#include <WiFiMulti.h>
#include <WiFiUdp.h>
#include <WiFiScan.h>
#include <ETH.h>
#include <WiFiClient.h>
#include <WiFiSTA.h>
#include <WiFiServer.h>
#include <WiFiType.h>
#include <WiFiGeneric.h>
#include <jsmn.h>
#include <M5Stack.h>
WiFiMulti WiFiMulti;

int status = WL_IDLE_STATUS;
int lastPrice = 0;
int currentPrice;
int minPrice = 9999999; 
int maxPrice = 0;
char servername_btc[] = "api.coindesk.com";
char servername_eth[] = "api.etherscan.io";
char servername_gas[] = "api.etherscan.io";
String answer;
WiFiClient client_btc;
WiFiClient client_eth;
WiFiClient client_gas;

// jsonから値抽出するための構造体
class Api_Str{
  public:
    String str; //GETしたデータ
    String amount_str; //最終データ
    String json_str; //抽出するラベル文字列を格納
};
Api_Str eth_str; //ETH価格用
Api_Str gas_str; //Gas用

// the setup routine runs once when M5Stack starts up
void setup(){
  WiFiMulti.addAP("xxxxxxxxx", "xxxxxxxxxxx"); //Wifi名、パスワードを記載する

  // Initialize the M5Stack object
  M5.begin();
  // LCD display
  M5.Lcd.println("Please wait...");

  while (WiFiMulti.run() != WL_CONNECTED) {
    delay(500);
    M5.Lcd.printf(".");
  }
  M5.Lcd.println("wifi connect success");

  //表示する値の初期値を取得する
  ConnectToClient_btc();
  ConnectToClient_eth();
  delay(1000); 
  ConnectToClient_gas();
}

//apiを利用してBTC価格、ETH価格、Gas valueが含まれた文字列をそれぞれ取得する関数
void ConnectToClient_btc(){
  if (client_btc.connect(servername_btc, 80)) {
    // Make a HTTP request:
    client_btc.println("GET https://api.coindesk.com/v1/bpi/currentprice.json HTTP/1.1");
    client_btc.println("Host: api.coindesk.com");
    client_btc.println("Connection: close");
    client_btc.println();
  }
}
void ConnectToClient_eth(){
  if (client_eth.connect(servername_eth, 80)) {
    // Make a HTTP request:
    client_eth.println("GET https://api.etherscan.io/api?module=stats&action=ethprice&apikey=YourApiKeyToken HTTP/1.1"); //etherscan.ioよりAPI keyを取得する必要あり
    client_eth.println("Host: api.etherscan.io");
    client_eth.println("Connection: close");
    client_eth.println();
  }
}
void ConnectToClient_gas(){
  if (client_gas.connect(servername_gas, 80)) {
    // Make a HTTP request:
    client_gas.println("GET https://api.etherscan.io/api?module=gastracker&action=gasoracle&apikey=YourApiKeyToken HTTP/1.1"); //etherscan.ioよりAPI keyを取得する必要あり
    client_gas.println("Host: api.etherscan.io");
    client_gas.println("Connection: close");
    client_gas.println();
  }
}

//ETH価格を抜き出す関数
void jsontrim( Api_Str *t ){
    String jsonAnswer;
    int jsonIndex;

    for (int i = 0; i < t->str.length(); i++) {
      if (t->str[i] == '{') {
        jsonIndex = i;
        break;
      }
    }

    jsonAnswer = t->str.substring(jsonIndex);
    jsonAnswer.trim();

    int rateIndex = jsonAnswer.indexOf(t->json_str);
    int num_eth = t->json_str.length();    
    String priceString = jsonAnswer.substring(rateIndex + num_eth+3, rateIndex + num_eth+10);
    Serial.println(jsonAnswer);
    Serial.println("");
    Serial.println(priceString);
    priceString.trim();
    int decimalplace = priceString.indexOf(".");
    if(decimalplace == -1){ //"."が見つからない場合は0を設定
      decimalplace = 0;
    }
    String Dollars = priceString.substring(0, decimalplace);
    String Cents = priceString.substring(decimalplace+1);
    while (Cents.length() < 2) {
      Cents += "0";
    }
    t->amount_str = "$" + Dollars;  //ETH価格を代入
}

//Gas Valueを抜き出す関数
void jsontrim_gas( Api_Str *t ){
    String jsonAnswer;
    int jsonIndex;

    for (int i = 0; i < t->str.length(); i++) {
      if (t->str[i] == '{') {
        jsonIndex = i;
        break;
      }
    }
    jsonAnswer = t->str.substring(jsonIndex);
    jsonAnswer.trim();

    int rateIndex = jsonAnswer.indexOf(t->json_str); //
    int gas_length = 0;
    int num = t->json_str.length();
    String priceString = jsonAnswer.substring(rateIndex + num+3, rateIndex + num+7);
    // 数字だけを文字列に代入されるようにする。「"」の前までが必要な値（=Gas Value）。
    for (int i = 0; i < priceString.length(); i++) {
      if (priceString[i] == '\"'){
        priceString.replace('\"', NULL);
        break;
      }
    }    
    Serial.println(jsonAnswer);
    Serial.println("");
    Serial.println(priceString);
    priceString.trim();

    t->amount_str = priceString;  //Gas valueを構造体の値に代入
}

// the loop routine runs over and over again forever
void loop() {
  if (client_btc.available() | client_eth.available() | client_gas.available()) {
    String c = client_btc.readString();
    answer += c; //BTC価格格納用変数
    eth_str.str = client_eth.readString(); //ETH価格格納用構造体
    gas_str.str = client_gas.readString(); //Gas Value格納用構造体
  } else {
    delay(10000);
    ConnectToClient_btc();
    ConnectToClient_eth();
    delay(1000); 
    ConnectToClient_gas();
  }
  // if the server's disconnected, stop the client:
  if (!client_btc.connected() | !client_eth.connected() | !client_gas.connected()) {
    m5.update();
    client_btc.stop();
    client_eth.stop();    
    client_gas.stop();
    
    String jsonAnswer;
    int jsonIndex;

    for (int i = 0; i < answer.length(); i++) {
      if (answer[i] == '{') {
        jsonIndex = i;
        break;
      }
    }

    jsonAnswer = answer.substring(jsonIndex);
    jsonAnswer.trim();

    int rateIndex = jsonAnswer.indexOf("rate_float");
    String priceString = jsonAnswer.substring(rateIndex + 12, rateIndex + 19);
    Serial.println(jsonAnswer);
    Serial.println("");
    Serial.println(priceString);
    priceString.trim();
    int decimalplace = priceString.indexOf(".");
    if(decimalplace == -1){ //"."が見つからない場合は0を設定
      decimalplace = 0;
    }
    String Dollars = priceString.substring(0, decimalplace);
    String Cents = priceString.substring(decimalplace+1);
    while (Cents.length() < 2) {
      Cents += "0";
    }
    String Amount = "$" + Dollars; //BTC price

    //ETH price, gas feeのデータ取得
    eth_str.json_str = "ethusd";
    jsontrim(&eth_str);  
     
    gas_str.json_str = "SafeGasPrice";
    jsontrim_gas(&gas_str);
    //String Gas_safe = "40";
    String Gas_safe = gas_str.amount_str;

    gas_str.json_str = "ProposeGasPrice";
    jsontrim_gas(&gas_str);
    String Gas_ave = gas_str.amount_str;

    gas_str.json_str = "FastGasPrice";
    jsontrim_gas(&gas_str);
    String Gas_fast = gas_str.amount_str;

    //Display表示
    //BTC price 
    m5.Lcd.fillScreen(0x0000);
    m5.Lcd.setFont(&FreeSans9pt7b);
    m5.Lcd.setTextColor(WHITE);
    m5.Lcd.setCursor(60, 55);
    m5.Lcd.setFont(&FreeMonoBold18pt7b);
    m5.Lcd.printf("BTC ");
    m5.Lcd.printf(Amount.c_str());
    //ETH price
    m5.Lcd.setCursor(60, 105);
    m5.Lcd.setFont(&FreeMonoBold18pt7b);
    m5.Lcd.printf("ETH  ");
    m5.Lcd.printf(eth_str.amount_str.c_str());
    //Gas Value
    m5.Lcd.setCursor(75, 145);    
    m5.Lcd.setFont(&FreeMonoBold12pt7b);
    m5.Lcd.printf("Gas Low : ");     
    m5.Lcd.printf(Gas_safe.c_str());          
    m5.Lcd.setCursor(75, 175);  
    m5.Lcd.printf("Gas Ave : ");  
    m5.Lcd.printf(Gas_ave.c_str());    
    m5.Lcd.setCursor(75, 205);  
    m5.Lcd.printf("Gas Fast: ");  
    m5.Lcd.printf(Gas_fast.c_str());    

    // wait 60 seconds and key check
    for (int i = 0; i < 60; i++){
      if(M5.BtnA.wasPressed()) {
        m5.lcd.setBrightness(0);
      }
      if(M5.BtnB.wasPressed()) {
        m5.lcd.setBrightness(25);
      }
      if(M5.BtnC.wasPressed()) {
        m5.lcd.setBrightness(150);
      }
      m5.update();
      delay(1000);
    }
    answer = "";
    Amount = "";
    ConnectToClient_btc();
    ConnectToClient_eth();
    delay(1000);
    ConnectToClient_gas();
  }
}
