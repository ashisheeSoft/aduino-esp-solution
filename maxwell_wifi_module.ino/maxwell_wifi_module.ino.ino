#include <WiFi.h>
#include <WebSocketsServer.h>
#include <EEPROM.h>
#include <Preferences.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <Regexp.h> 

const int EEPROM_SIZE = 64; // Adjust the size based on your password length
const int EEPROM_SSID_ADDRESS = 0; // Starting address for the AP SSID in EEPROM
const int EEPROM_PASSWORD_ADDRESS = 32; // Starting address for the AP password in EEPROM

WebSocketsServer webSocket = WebSocketsServer(81);
WebServer httpServer(80);

const char* ssidRegexPattern = "^[a-zA-Z0-9]+$";
MatchState ms;

Preferences preferences;

char ssid[EEPROM_SIZE / 2] = "MX_11223344"; // Initialize with a default SSID
char password[EEPROM_SIZE / 2] = "DefaultPassword"; // Initialize with a default password

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_TEXT:
      Serial.printf("[%u] Text: %s\n", num, payload);
      handleWebSocketMessage(num, payload, length);
      break;
  }
}

bool isValidValue() {
  ms.Target(ssid,EEPROM_SIZE / 2);
  return ms.Match(ssidRegexPattern) == 0 ? true : false;
}

void handleWebSocketMessage(uint8_t num, uint8_t * payload, size_t length) {
  if (length > 0) {
    DynamicJsonDocument doc(256);
    DeserializationError error = deserializeJson(doc, (const char*)payload);

    if (!error && doc.containsKey("action") && doc.containsKey("payload")) {
      String action = doc["action"];
      String newPass = doc["payload"];

      if (action == "PASSWORD_CHANGE") {
        setPassword(newPass);
        webSocket.sendTXT(num, "Password changed successfully.");
      }
    }
  }
}

void setPassword(const String& newPass) {
  // Update the password in memory
  //newPass.toCharArray(password, EEPROM_SIZE / 2);
  Serial.println("data in store");
  preferences.putString("password", newPass);
  // Write the new password to EEPROM
  //EEPROM.put(EEPROM_PASSWORD_ADDRESS, password);
  //EEPROM.commit();
}

// void handlePasswordChange() {
//   StaticJsonDocument<128> responseJson;
//   String jsonResponse;

//   if (httpServer.method() == HTTP_POST) {
//     String newPass = httpServer.arg("password");
//     Serial.println(newPass);
//     setPassword(newPass);
//     responseJson["status"] = "success";
//     responseJson["message"] = "Password changed successfully";
//     serializeJson(responseJson, jsonResponse);
//     httpServer.send(200, "application/json", jsonResponse);
//   } else {
//     responseJson["status"] = "failed";
//     responseJson["message"] = "Invalid Request";
//     serializeJson(responseJson, jsonResponse);
//     httpServer.send(400, "application/json", jsonResponse);
//   }
// }

void handlePasswordChange(){
  StaticJsonDocument<250> jsonDocument;
  StaticJsonDocument<128> responseJson;
  String jsonResponse;
  if (httpServer.hasArg("plain") == false) {
    responseJson["status"] = "failed";
    responseJson["message"] = "Bad Request";
    serializeJson(responseJson, jsonResponse);
    httpServer.send(400, "application/json", jsonResponse);
  }
  String body = httpServer.arg("plain");
  deserializeJson(jsonDocument, body);

  String new_password = jsonDocument["password"];
  setPassword(new_password);
  responseJson["status"] = "Success";
  responseJson["message"] = "Password Changed Succesfully";
  serializeJson(responseJson, jsonResponse);
  httpServer.send(200, "application/json", jsonResponse);
}

void handleConfirmChange() {
  StaticJsonDocument<250> jsonDocument;
  StaticJsonDocument<128> responseJson;
  String jsonResponse;
  if (httpServer.hasArg("plain") == false) {
    responseJson["status"] = "failed";
    responseJson["message"] = "Bad Request";
    serializeJson(responseJson, jsonResponse);
    httpServer.send(400, "application/json", jsonResponse);
  }
  String body = httpServer.arg("plain");
  deserializeJson(jsonDocument, body);

  String new_password = jsonDocument["password"];
  String passwordCheck = preferences.getString("password", "");
  if(strcmp(passwordCheck.c_str(),new_password.c_str())==0){

    responseJson["status"] = "Success";   
    responseJson["message"] = "Module Reset";
    serializeJson(responseJson, jsonResponse);
    httpServer.send(200, "application/json", jsonResponse);
    ESP.restart();
  }
  else {
    responseJson["status"] = "Failed";   
    responseJson["message"] = "Change Failed";
     serializeJson(responseJson, jsonResponse);
      httpServer.send(400, "application/json", jsonResponse);
  }
 
}

void setup() {
  Serial.begin(115200);

  // Initialize EEPROM
  //EEPROM.begin(EEPROM_SIZE);

  preferences.begin("myApp", false);
  // Read the SSID and password from EEPROM
  //EEPROM.get(EEPROM_SSID_ADDRESS, ssid);
  //EEPROM.get(EEPROM_PASSWORD_ADDRESS, password);

  String derivedPassword = preferences.getString("password", "");
  Serial.println(derivedPassword.);
  

  //Serial.println(isValidValue(ssid));
  // Initialize ESP32 in Access Point (AP) mode
  WiFi.mode(WIFI_AP);
  if(derivedPassword!=""){
    WiFi.softAP(ssid, derivedPassword);
  }
  else{
    WiFi.softAP(ssid, "888888888");
  }

  // Print the AP IP address assigned
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());

  // Set up the WebSocket event handler
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  // Set up the HTTP endpoint for changing the password
  httpServer.on("/change_password", HTTP_POST, handlePasswordChange);
  httpServer.on("/confirm_change", HTTP_POST, handleConfirmChange);

  // Start the HTTP server
  httpServer.begin();
}

void loop() {
  webSocket.loop();
  httpServer.handleClient();
  // Your loop code here
}
