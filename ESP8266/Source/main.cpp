#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Adafruit_NeoPixel.h>
#include <Ticker.h>
#include <LittleFS.h>
#include <string>

#define LED_PIN D1
#define PWR_pin D2
#define LED_COUNT 20

//Wi-Fi variables
const char* ssid = "BigAmericanTTS";
const char* password = "JohnMadden1337";
ESP8266WebServer server(80);

//LED variables
Adafruit_NeoPixel LED_strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
uint8_t  r;
uint8_t  g;
uint8_t  b;

//State variables
bool light_on = false;
bool no_connection = true;
bool dynamic = false;
bool nv_restart_needed = false;

//Settings variables
String color = "FFFFFF";
String nv_net = "mob2";
int nv_threshold = 50;



int convert_hex_to_int(String hex){

  int i = 0;
  if (hex[0] == 'a') {
    i += 10;
  } else if (hex[0] == 'b'){
    i += 11;
  } else if (hex[0] == 'c'){
    i += 12;
  } else if (hex[0] == 'd'){
    i += 13;
  } else if (hex[0] == 'e'){
    i += 14;
  } else if (hex[0] == 'f'){
    i += 15;
  }
  i *= 16;

  if (hex[1] == 'a') {
    i += 10;
  } else if (hex[1] == 'b'){
    i += 11;
  } else if (hex[1] == 'c'){
    i += 12;
  } else if (hex[1] == 'd'){
    i += 13;
  } else if (hex[1] == 'e'){
    i += 14;
  } else if (hex[1] == 'f'){
    i += 15;
  }
  return i;
}

void convert_color(){
  String red = color.substring(0, 2);
  String green = color.substring(2, 4);
  String blue = color.substring(4, 6);
  r = (uint8_t)convert_hex_to_int(red);
  g = (uint8_t)convert_hex_to_int(green);
  b = (uint8_t)convert_hex_to_int(blue);
}

void load_settings(){
  fs::File color_txt = LittleFS.open("/color.txt", "r");
  color = color_txt.readString();
  color_txt.close();

  Serial.println("Loaded color:\t" + color);

  fs::File net_txt = LittleFS.open("/net.txt", "r");
  nv_net = net_txt.readString();
  net_txt.close();

  Serial.println("Loaded net:\t" + nv_net);

  fs::File threshold_txt = LittleFS.open("/threshold.txt", "r");
  nv_threshold = threshold_txt.readString().toInt();
  threshold_txt.close();

  Serial.println("Loaded thr. :\t" + String(nv_threshold));
  

}

//======================== STATE MANAGEMENT ========================
void enable_lighting(){
  Serial.println("Lights On");
  timer1_write(600000000);
  light_on = true;
  digitalWrite(LED_BUILTIN, LOW);
  digitalWrite(PWR_pin, HIGH);
  delay(50);
  for(int i = 0; i < LED_COUNT; i++) {
    LED_strip.setPixelColor(i, g, r, b);
  }
  LED_strip.show();
}

void disable_lightning(){
  Serial.println("Lights Off");
  timer1_write(600000000);
  light_on = false;
  digitalWrite(LED_BUILTIN, HIGH);

  for(int i = 0; i < LED_COUNT; i++) {
    LED_strip.setPixelColor(i, 0, 0, 0);
  }
  LED_strip.show();
  delay(50);
  digitalWrite(PWR_pin, LOW);

}

void connection_established(){
  Serial.println("Connected with Jetson");
  no_connection = false;
  timer1_write(600000000);
}

void connection_lost(){
  Serial.println("Connection with Jetson lost");
  no_connection = true;
}

//======================== JETSON RESPONSES ========================
void handle_on(){
  if(nv_restart_needed){
    server.send(300);
  } else {
  server.send(200);
  }
  if(no_connection){
    connection_established();
  }
  if(!light_on){
    enable_lighting();
  } else {
    timer1_write(600000000);   //light timer
  }
}
 
void handle_off(){
  if(nv_restart_needed){
    server.send(300);
  }
  else {
  server.send(200);
  }
  Serial.println("request: no person detected");
  if(no_connection){
    connection_established();
  }
  
  if(!light_on){
    timer1_write(600000000);    //timeout timer
  }
}

void handle_dynamic(){
  
}

void handle_establish(){
  server.send(200);
  connection_established();
}


void handle_settings(){
  String response;
  if(dynamic){
    response += "dyn ";
  } else {
    response += "sta ";
  }
  response += nv_net + " ";
  response += String(nv_threshold);
  server.send(200, "text/plain", response);
  Serial.println("Settings sent to Jetson");
  nv_restart_needed = false;
}
//======================== BROWSER RESPONSES ========================
void main_page(){
  fs::File index_html = LittleFS.open("/WebPages/index.html", "r");
  server.send(200, "text/html", index_html.readString());
  index_html.close();
}

void handle_light_submition(){

  server.send(200, "text/plain", color);

  if(server.arg(1) == "dyna") {
    if(!dynamic){
      dynamic = true;
      nv_restart_needed = true;
    }

  } else {

    if(dynamic){
      dynamic = false;
      nv_restart_needed = true;
    }

    color = server.arg(0);
    Serial.println(color);
    fs::File color_txt = LittleFS.open("/color.txt", "w");
    char buff[7];
    color.toCharArray(buff, 7);
    color_txt.write(buff, 7);
    color_txt.close();
    convert_color();
    if (light_on) {
      for(int i = 0; i < LED_COUNT; i++) {
        LED_strip.setPixelColor(i, g, r, b);
      }
      LED_strip.show();
    }
  }
}

void handle_calibration_submittion(){

  int threshold = server.arg(0).toInt();
  String net = server.arg(1);

  if(threshold != nv_threshold){
    nv_threshold = threshold;
    nv_restart_needed = true;
    fs::File threshold_txt = LittleFS.open("/threshold.txt", "w");
    char buff[3];
    server.arg(0).toCharArray(buff, 3);
    threshold_txt.write(buff, 3);
    threshold_txt.close();
  }

  if(net != nv_net){
    nv_net = net;
    nv_restart_needed = true;
    fs::File net_txt = LittleFS.open("/net.txt", "w");
    char buff[5];
    net.toCharArray(buff, 5);
    net_txt.write(buff, 5);
    net_txt.close();
  }

  if(nv_restart_needed){
    server.send(200, "text/plain", "true");
  } else {
    server.send(200, "plain/text", "false");
  }

}

void handle_color(){
  server.send(200, "text/plain", color);
}

void handle_calibration(){

  String response = nv_net;
  response += String(nv_threshold);

  server.send(200, "text/plain", response);
}

void handle_light(){
  server.send(200, "text/plain", String(light_on));
}

void handle_nv(){
  server.send(200, "text/plain", String(!no_connection));
}

//======================== INTERRUPTS ========================
void ICACHE_RAM_ATTR onTimerISR(){
  if(light_on){
    disable_lightning();
  } else if(!no_connection){
    connection_lost();
  } 
}




void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PWR_pin, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);  
  Serial.begin(9600);

  WiFi.begin(ssid, password);             // Connect to the network
  Serial.print("Connecting to ");
  Serial.print(ssid); Serial.println(" ...");

  int i = 0;
  while (WiFi.status() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
    delay(1000);
    Serial.print(++i); 
    Serial.print(' ');
  }



  LED_strip.begin();

  timer1_attachInterrupt(onTimerISR);
  timer1_enable(TIM_DIV256, TIM_EDGE, TIM_SINGLE);

  Serial.println('\n');
  Serial.println("Connection established!");  
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());         // Send the IP address of the ESP8266 to the computer

  if(!LittleFS.begin()){
    Serial.println("Failed initialazing file system!");
  } else {
    Serial.println("File system initialized");
    load_settings();
  }

  convert_color();

  server.on("/", main_page);
  server.on("/On", handle_on);
  server.on("/Off", handle_off);
  server.on("/establish", handle_establish);
  server.on("/settings", handle_settings);

  server.on("/submit_light", handle_light_submition);
  server.on("/submit_calibration", handle_calibration_submittion);
  server.on("/color", handle_color);
  server.on("/calibration", handle_calibration);
  server.on("/light", handle_light);
  server.on("/nv", handle_nv);

  server.begin();
}

void loop() {
  server.handleClient();
}


