/*

FireworkOS 1.0 (Main Script)
Copyright (C) 2025 hidely/ResiChat

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.

*/
#include <Wire.h>
#if defined(ESP8266) || defined(ESP32) || defined(ARDUINO_UNOR4_WIFI)
    #include <WiFi.h>
#endif

#include <SPI.h>
#include <SD.h>
#include "baselib.h"
#include "global_vars.h"
#if defined(ESP8266) || defined(ESP32)
  #include <HTTPClient.h>
  #include <WiFiClient.h>
#elif defined(ARDUINO_UNOR4_WIFI)
  #include <WiFiS3.h>
#else
  // #include <Ethernet.h>
#endif

// Pins for SD
#ifdef ESP32
  #define SD_CS 15
#elif defined(ESP8266)
  #define SD_CS 15
#else
  #define SD_CS 4
#endif

boolean sd_initialized = false;
File root;
String current_dir = "/";
String current_user = "";
String devicetype = ""; 
String kernelver = "fireworkos-1.1.1_snowos-1.1.1"; 
String hostname = "";
boolean storage_mounted = false;
boolean has_wifi = false;
boolean wifi_autoconnect = false;
String wlan_ssid = "";
String wlan_password = "";
String kerndate = "2025-10-12";
boolean wifi_connecting = false;

typedef struct {
    String name;
    int type; // 0 = string, 1 = integer
    String strValue;
    int intValue;
} Variable;

Variable variables[50];
int variableCount = 0;

int findVariable(String name) {
    for (int i = 0; i < variableCount; i++) {
        if (variables[i].name == name) {
            return i;
        }
    }
    return -1;
}

int createVariable(String name, int type) {
    if (variableCount >= 50) {
        return -1;
    }
    
    variables[variableCount].name = name;
    variables[variableCount].type = type;
    variableCount++;
    return variableCount - 1;
}

void setStringVariable(String name, String value) {
    int index = findVariable(name);
    if (index == -1) {
        index = createVariable(name, 0);
    }
    variables[index].strValue = value;
    variables[index].type = 0;
}

void setIntVariable(String name, int value) {
    int index = findVariable(name);
    if (index == -1) {
        index = createVariable(name, 1);
    }
    variables[index].intValue = value;
    variables[index].type = 1;
}

String getStringVariable(String name) {
    int index = findVariable(name);
    if (index != -1 && variables[index].type == 0) {
        return variables[index].strValue;
    }
    return "";
}

int getIntVariable(String name) {
    int index = findVariable(name);
    if (index != -1 && variables[index].type == 1) {
        return variables[index].intValue;
    }
    return 0;
}




bool checkNetwork() {
  #if defined(ESP8266) || defined(ESP32) || defined(ARDUINO_UNOR4_WIFI)
    return WiFi.status() == WL_CONNECTED;
  #else
    //return Ethernet.linkStatus() == LinkON;
    //return false;
  #endif
}
boolean initSD() {
  
  if (!SD.begin(SD_CS)) {
    sd_initialized = false;
    return false;
  }
  
  #if defined(ESP32) || defined(ESP8266)
    uint8_t cardType = SD.cardType();
    
    if (cardType == CARD_NONE) {
      Serial.print("[\033[31mNO CARD\033[0m]");
      sd_initialized = false;
      return false;
    }
    
    Serial.print("[\033[32mOK\033[0m] Type: ");
    switch(cardType) {
      case CARD_MMC: Serial.println("MMC"); break;
      case CARD_SD: Serial.println("SDSC"); break;
      case CARD_SDHC: Serial.println("SDHC"); break;
      default: Serial.println("UNKNOWN"); break;
    }
    
    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.println("Card size: " + String((unsigned long)cardSize) + "MB");
    
  #else
    Serial.println("[\033[32mOK\033[0m]");
    File root = SD.open("/");
    if (!root) {
      Serial.print("[\033[31mNO CARD or READ ERROR\033[0m]");
      sd_initialized = false;
      return false;
    }
    root.close();
    Serial.print("Card: SD (AVR compatible) ");
  #endif
  
  sd_initialized = true;
  storage_mounted = true;
  return true;
}
String readFileToString(String filename) {
  if (!sd_initialized) {
    Serial.println("SD card not initialized!");
    return "";
  }
  
  if (!SD.exists(filename)) {
    Serial.println("File not found: " + filename);
    return "";
  }
  
  File file = SD.open(filename, FILE_READ);
  if (!file) {
    Serial.println("Error opening file: " + filename);
    return "";
  }
  
  String content = "";
  while (file.available()) {
    content += (char)file.read();
  }
  
  file.close();
  return content;
}
int curl(String url) {
  if (!checkNetwork()) {
    Serial.println("Error: No network connection");
    return 1;
  }

  #if defined(ESP8266) || defined(ESP32)
    HTTPClient http;
    WiFiClient client;
    
    Serial.println("Fetching: " + url);
    
    if (http.begin(client, url)) {
      int httpCode = http.GET();
      
      if (httpCode > 0) {
        Serial.println("HTTP Code: " + String(httpCode));
        Serial.println("----------------------------------------");
        
        if (httpCode == HTTP_CODE_OK) {
          String payload = http.getString();
          Serial.println(payload);
        } else {
          Serial.println("Server returned error: " + String(httpCode));
        }
      } else {
        Serial.println("HTTP request failed: " + String(httpCode));
      }
      
      http.end();
    } else {
      Serial.println("Unable to connect to: " + url);
      return 1;
    }
  #elif defined(ARDUINO_UNOR4_WIFI)
    WiFiClient client;
    
    Serial.println("Fetching: " + url);
    
    if (client.connect(url.c_str(), 80)) {
      client.println("GET / HTTP/1.1");
      client.println("Host: " + url);
      client.println("Connection: close");
      client.println();
      
      while (client.connected()) {
        String line = client.readStringUntil('\n');
        if (line == "\r") {
          break;
        }
      }
      
      while (client.available()) {
        String line = client.readStringUntil('\n');
        Serial.println(line);
      }
      
      client.stop();
    } else {
      Serial.println("Unable to connect to: " + url);
      return 1;
    }
  #else
    Serial.println("Curl not supported on this platform");
    return 1;
  #endif
  
  return 0;
}

int listFiles(String path = "/") {
  if (!sd_initialized) {
    Serial.println("SD card not initialized!");
    return 1;
  }
  
  File dir = SD.open(path);
  if (!dir) {
    Serial.println("Directory not found: " + path);
    return 1;
  }
  
  if (!dir.isDirectory()) {
    Serial.println("Not a directory: " + path);
    dir.close();
    return 1;
  }
  
  Serial.println("Contents of " + path + ":");
  Serial.println("Size\t\tName");
  Serial.println("-----------------------");
  
  File entry = dir.openNextFile();
  while (entry) {
    
    // Размер файла
    if (entry.isDirectory()) {
      Serial.print("<DIR>\t\t");
    } else {
      Serial.print(entry.size());
      Serial.print(" bytes\t");
    }
    
    // Имя файла/папки
    Serial.println(entry.name());
    
    entry.close();
    entry = dir.openNextFile();
  }
  
  dir.close();
  return 0;
}
int changedir(String dirname){
  if (dirname.startsWith("/")){
      current_dir = dirname;
    }else if (dirname == "/"){
        current_dir = "/";
    }
    
    else{
      if (dirname.endsWith("/")){
        if (!SD.exists(current_dir + dirname)){
          Serial.println("File not found: " + dirname);
          return 1;
        }
        current_dir = current_dir + dirname;
        return 0;
      }
    else{
        if (!SD.exists(current_dir + dirname + "/")){
          Serial.println("File not found: " + dirname);
          return 1;
        }
        current_dir = current_dir + dirname + "/";
        return 0;
      }
    }
}
int createFile(String filename, String content = "", bool silent = false) {
  if (!sd_initialized) {
    Serial.println("SD card not initialized!");
    return 1;
  }
  
  if (SD.exists(filename)) {
    Serial.println("File already exists: " + filename);
    return 1;
  }
  
  File file = SD.open(filename, FILE_WRITE);
  if (!file) {
    Serial.println("Error creating file: " + filename);
    return 1;
  }
  
  if (content != "") {
    if (file.print(content)) {
      Serial.println("File created and written: " + filename);
    } else {
      Serial.println("Error writing to file: " + filename);
      file.close();
      return 1;
    }
  } else {
    Serial.println("Empty file created: " + filename);
  }
  
  file.close();
  return 0;
}

int appendToFile(String filename, String content, bool addNewline = false, bool silent = true) {
  if (!sd_initialized) {
    Serial.println("SD card not initialized!");
    return 1;
  }
  
  // Если файла нет, создаем его
  if (!SD.exists(filename)) {
    Serial.println("File not found, creating new: " + filename);
    if (createFile(filename, "") != 0) {
      return 1;
    }
  }
  
  // Кроссплатформенное открытие файла
  #if defined(ESP32) || defined(ESP8266)
    // Для ESP32/ESP8266
    File file = SD.open(filename, FILE_APPEND);
  #else
    // Для Arduino Mega2560 и других AVR
    File file = SD.open(filename, O_WRITE | O_APPEND | O_CREAT);
  #endif
  
  if (!file) {
    Serial.println("Error opening file: " + filename);
    return 1;
  }
  
  // Добавляем текст
  if (addNewline) {
    content += "\n";
  }
  
  size_t bytesWritten = file.print(content);
  file.close();
  
  if (bytesWritten == content.length()) {
    if (!silent){
    Serial.println("Successfully appended " + String(bytesWritten) + " bytes to: " + filename);
    }
    return 0;
  } else {
    Serial.println("Error: wrote " + String(bytesWritten) + " of " + String(content.length()) + " bytes");
    return 1;
  }
}

int appendLineToFile(String filename, String content, bool silent=true) {
  return appendToFile(filename, content, silent);
}
String readFirstLineFromFile(String filename) {
  if (!sd_initialized) {
    Serial.println("SD card not initialized!");
    return "";
  }
  
  if (!SD.exists(filename)) {
    Serial.println("File not found: " + filename);
    return "";
  }
  
  File file = SD.open(filename, FILE_READ);
  if (!file) {
    Serial.println("Error opening file: " + filename);
    return "";
  }
  
  String firstLine = "";
  while (file.available()) {
    char ch = file.read();
    if (ch == '\n' || ch == '\r') {
      break;
    }
    firstLine += ch;
  }
  
  file.close();
  firstLine.trim();
  return firstLine;
}
int overwriteFile(String filename, String content) {
  if (!sd_initialized) {
    Serial.println("SD card not initialized!");
    return 1;
  }
  
  // Удаляем старый файл если существует
  if (SD.exists(filename)) {
    if (!SD.remove(filename)) {
      Serial.println("Error deleting old file: " + filename);
      return 1;
    }
  }
  
  File file = SD.open(filename, FILE_WRITE);
  if (!file) {
    Serial.println("Error creating file: " + filename);
    return 1;
  }
  
  if (file.print(content)) {
    file.close();
    return 0;
  } else {
    Serial.println("Error writing to file: " + filename);
    file.close();
    return 1;
  }
}

int logMessage(String message) {
  String timestamp = String(millis());
  String logEntry = "[" + timestamp + "] " + message;
  
  // Исправляем путь к файлу логов
  String logFilename = "/var/log"; // или "log.txt" в зависимости от вашей структуры
  
  return appendLineToFile(logFilename, logEntry, true);
}
int readFile(String filename) {
  if (!sd_initialized) {
    Serial.println("SD card not initialized!");
    return 1;
  }
  
  if (!SD.exists(filename)) {
    Serial.println("File not found: " + filename);
    return 1;
  }
  
  File file = SD.open(filename, FILE_READ);
  if (!file) {
    Serial.println("Error opening file: " + filename);
    return 1;
  }
  
  //Serial.println("Contents of " + filename + ":");
  //Serial.println("----------------------------------------");
  
  while (file.available()) {
    String line = file.readStringUntil('\n');
    Serial.println(line);
  }
  //Serial.println();
  
  //Serial.println("----------------------------------------");
  //Serial.println("File size: " + String(file.size()) + " bytes");
  
  file.close();
  return 0;
}
int deleteFile(String filename) {
  if (!sd_initialized) {
    Serial.println("SD card not initialized!");
    return 1;
  }
  
  if (!SD.exists(filename)) {
    Serial.println("File not found: " + filename);
    return 1;
  }
  
  if (SD.remove(filename)) {
    Serial.println("File deleted: " + filename);
    return 0;
  } else {
    Serial.println("Error deleting file: " + filename);
    return 1;
  }
}
int createDir(String dirname) {
  if (!sd_initialized) {
    Serial.println("SD card not initialized!");
    return 1;
  }
  
  if (SD.exists(dirname)) {
    Serial.println("Directory already exists: " + dirname);
    return 1;
  }
  
  if (SD.mkdir(dirname)) {
    Serial.println("Directory created: " + dirname);
    return 0;
  } else {
    Serial.println("Error creating directory: " + dirname);
    return 1;
  }
}
int deleteDir(String dirname) {
  if (!sd_initialized) {
    Serial.println("SD card not initialized!");
    return 1;
  }
  
  if (!SD.exists(dirname)) {
    Serial.println("Directory not found: " + dirname);
    return 1;
  }
  
  File dir = SD.open(dirname);
  if (!dir.isDirectory()) {
    Serial.println("Not a directory: " + dirname);
    dir.close();
    return 1;
  }
  
  // Рекурсивное удаление содержимого
  File entry = dir.openNextFile();
  while (entry) {
    String entryPath = String(entry.name());
    if (entry.isDirectory()) {
      deleteDir(entryPath); // Рекурсивный вызов для подпапок
    } else {
      SD.remove(entryPath); // Удаление файлов
    }
    entry.close();
    entry = dir.openNextFile();
  }
  
  dir.close();
  
  // Удаление самой папки
  if (SD.rmdir(dirname)) {
    Serial.println("Directory deleted: " + dirname);
    return 0;
  } else {
    Serial.println("Error deleting directory: " + dirname);
    return 1;
  }
}
int sdInfo() {
  if (!sd_initialized) {
    Serial.println("SD card not initialized!");
    return 1;
  }
  
  #if defined(ESP32) || defined(ESP8266)
    // Для ESP платформ
    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    uint64_t totalBytes = SD.totalBytes() / (1024 * 1024);
    uint64_t usedBytes = SD.usedBytes() / (1024 * 1024);
    
    Serial.println("SD Card Information:");
    Serial.println("Total size: " + String((unsigned long)cardSize) + " MB");
    Serial.println("Used space: " + String((unsigned long)usedBytes) + " MB");
    Serial.println("Free space: " + String((unsigned long)(totalBytes - usedBytes)) + " MB");
  #else
    // Для AVR платформ
    Serial.println("SD Card Information:");
    Serial.println("Platform: AVR Arduino - detailed info not available");
    Serial.println("Card is mounted and ready for use");
    
    // Простая проверка свободного места через создание тестового файла
    unsigned long freeSpace = 0;
    File testFile = SD.open("test.tmp", FILE_WRITE);
    if (testFile) {
      freeSpace = testFile.availableForWrite();
      testFile.close();
      SD.remove("test.tmp");
      Serial.println("Approx. free space: " + String(freeSpace) + " bytes");
    }
  #endif
  
  return 0;
}
String getPlatformInfo() {
    String info = "Platform: ";
    
    #if defined(ESP32)
        info += "ESP32";
        info += " | Chip: ESP32";
        #ifdef ESP32_DEFAULT_CPU_FREQ_MHZ
            info += " | Freq: " + String(ESP.getCpuFreqMHz()) + "MHz";
        #endif
        info += " | Flash: " + String(ESP.getFlashChipSize() / 1024 / 1024) + "MB";
        
    #elif defined(ARDUINO_AVR_MEGA2560)
        info += "Arduino Mega 2560";
        info += " | MCU: ATmega2560";
        
    #elif defined(ARDUINO_AVR_UNO)
        info += "Arduino Uno";
        info += " | MCU: ATmega328P";
    #elif defined(__AVR_ATmega2560__)
        return "Arduino Mega 2560";
    #elif defined(ARDUINO_UNOR4_WIFI)
        info += "Arduino UNO R4 WiFi";
        info += " | MCU: RA4M1 (ARM Cortex-M4)";
        info += " | WiFi: Renesas DA16200";
        #ifdef ARDUINO_RENESAS_UNO
            info += " | Freq: 48MHz";
        #endif
    #elif defined(ARDUINO_ARCH_AVR)
        info += "AVR Arduino";
        #ifdef __AVR_ATmega2560__
            info += " | MCU: ATmega2560";
        #elif defined(__AVR_ATmega328P__)
            info += " | MCU: ATmega328P";
        #elif defined(__AVR_ATmega168__)
            info += " | MCU: ATmega168";
        #else
            info += " | MCU: Unknown AVR";
        #endif
        
    #else
        info += "Unknown";
    #endif
    
    return info;
}
String boardinf(){
    Serial.println(getPlatformInfo());
    return "";
}
String getPlatform() {
    #if defined(ESP32)
        return "ESP32";
    #elif defined(ESP8266)
        return "ESP8266";
    #elif defined(ARDUINO_AVR_MEGA2560)
        return "Arduino Mega 2560";
    #elif defined(ARDUINO_AVR_UNO)
        return "Arduino Uno";
    #elif defined(ARDUINO_UNOR4_WIFI) 
        return "Arduino UNO R4 WiFi";
    #elif defined(ARDUINO_AVR_NANO)
        return "Arduino Nano";
    #elif defined(ARDUINO_ARCH_AVR)
        return "AVR Arduino";
    #elif defined(ARDUINO_ARCH_SAM)
        return "SAM Arduino";
    #elif defined(ARDUINO_ARCH_SAMD)
        return "SAMD Arduino";
    #elif defined(ARDUINO_ARCH_RENESAS) 
        return "Renesas Arduino";
    #else
        return "Unknown Platform";
    #endif
}

void(* resetFunc) (void) = 0;

int reboot(){
  resetFunc();
  return 0;
}

int set_hostname(String new_hostname = "", boolean is_starts = false) {
  String hostnameFile = "/etc/hostname"; // Файл для хранения hostname
  
  if (is_starts) {
    // При старте пытаемся прочитать hostname из файла
    hostname = readFirstLineFromFile(hostnameFile);
    
    // Если прочитали пустую строку или файл не существует, устанавливаем значение по умолчанию
    if (hostname.length() == 0 || hostname.length() > 50) {
      hostname = "arduino";
      overwriteFile(hostnameFile, hostname);
    } else {
    }
  } else if (new_hostname != "") {
    // Устанавливаем новый hostname
    if (new_hostname.length() > 0 && new_hostname.length() <= 50) {
      hostname = new_hostname;
      if (overwriteFile(hostnameFile, new_hostname) == 0) {
        return 0;
      } else {
        return 1;
      }
    } else {
      return 1;
    }
  }
  return 0;
}

int build_hello(){
    Serial.println("\e[44m\e[37mFireworkOS v.1.1.1\e[0m (SnowOS Kernel 1.1.1)");
    Serial.println("Powered by \e[35mhidely\e[0m/\e[33mResiChat\e[0m =D ");
    return 0;
}

#if defined(ESP8266) || defined(ESP32) || defined(ARDUINO_UNOR4_WIFI)
    int init_wlan(){
        if (readWiFiConfig(wlan_ssid, wlan_password)) {
            Serial.println("Loaded WiFi config from file");
            Serial.println("SSID: " + wlan_ssid);
            // Не показываем пароль в логах для безопасности
            return 1;
        } else {
            Serial.println("No WiFi config found or error reading file");
            return 0;
        }
    }

    int saveWiFiConfig(String ssid, String password) {
        String wifiConfigFile = "/etc/wlancred";
        
        // Сначала убедимся, что директория существует
        if (!SD.exists("/etc")) {
            if (!SD.mkdir("/etc")) {
                Serial.println("Error: Cannot create /etc directory");
                return 0;
            }
        }
        
        String content = ssid + "\n" + password;
        
        // Удаляем старый файл если существует
        if (SD.exists(wifiConfigFile)) {
            if (!SD.remove(wifiConfigFile)) {
                Serial.println("Error: Cannot remove old config file");
                return 0;
            }
        }
        
        // Создаем новый файл
        File file = SD.open(wifiConfigFile, FILE_WRITE);
        if (!file) {
            Serial.println("Error: Cannot create config file");
            return 0;
        }
        
        if (file.print(content)) {
            file.close();
            Serial.println("WiFi config saved successfully");
            return 1;
        } else {
            Serial.println("Error: Cannot write to config file");
            file.close();
            return 0;
        }
    }

    // Функция для чтения WiFi credentials из файла
    int readWiFiConfig(String &ssid, String &password) {
        String wifiConfigFile = "/etc/wlancred";
        
        if (!sd_initialized) {
            Serial.println("SD card not initialized!");
            return 0;
        }
        
        if (!SD.exists(wifiConfigFile)) {
            Serial.println("WiFi config file not found: " + wifiConfigFile);
            return 0;
        }
        
        String content = readFileToString(wifiConfigFile);
        if (content == "") {
            Serial.println("Error reading WiFi config file");
            return 0;
        }
        
        
        //String user_content = readFileToString("/etc/user");
        String wlan_lines[10];
        int wlancred_lineCount = splitString(content, "\n", wlan_lines, 10);
        String wlan_login = (wlancred_lineCount >= 1) ? wlan_lines[0] : "";
        String wlan_password = (wlancred_lineCount >= 2) ? wlan_lines[1] : "";
        wlan_login.trim();
        wlan_password.trim();
        ssid = wlan_login;
        password = wlan_password;
        
        return 1;
    }
#endif


int stater_wlan_connect(String message = "") {
    if (!readWiFiConfig(wlan_ssid, wlan_password)) {
        return 1;
    }
    
    if (wlan_ssid.length() == 0) {
        return 1;
    }
    if (wifi_connecting) {
        return 1;
    }
    #if defined(ESP8266) || defined(ESP32) || defined(ARDUINO_UNOR4_WIFI)
        // Проверяем, не подключены ли мы уже
        if (WiFi.status() == WL_CONNECTED) {
            WiFi.disconnect();
            delay(500);
        }
        
        #if defined(ESP8266) || defined(ESP32)
            WiFi.mode(WIFI_STA);
            WiFi.setAutoReconnect(true);
            WiFi.persistent(true);
        #endif
        
        WiFi.begin(wlan_ssid.c_str(), wlan_password.c_str());
        
    #else
        return 1;
    #endif
    
    unsigned long startTime = millis();
    int progressBarPos = 1;

    while (WiFi.status() != WL_CONNECTED && millis() - startTime < 20000) { 
        if (progressBarPos == 1){
            Serial.print("\r"+message + "\t[*  ]");
            progressBarPos++;
        } else if (progressBarPos == 2){
            Serial.print("\r"+message + "\t[ * ]");
            progressBarPos++;
        }
        else if (progressBarPos == 3){
            Serial.print("\r"+message + "\t[  *]");
            progressBarPos = 1;
        }
        delay(300);
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        has_wifi = true;
        return 0;
    } else {
        return 1;
    }
}

int wlanctl(String arg){
    arg.trim();
    #if defined(ESP8266) || defined(ESP32) || defined(ARDUINO_UNOR4_WIFI)
        if (arg.startsWith("ssid ")){
            String current_arg = arg.substring(5);
            wlan_ssid = current_arg;
            if (saveWiFiConfig(wlan_ssid, wlan_password)) {
                Serial.println("SSID saved: " + wlan_ssid);
            } else {
                Serial.println("Error saving SSID");
            }
        }
        else if (arg.startsWith("password ")){
            String current_arg = arg.substring(9);
            wlan_password = current_arg;
            if (saveWiFiConfig(wlan_ssid, wlan_password)) {
                Serial.println("Password saved");
            } else {
                Serial.println("Error saving password");
            }
        }
        else if(arg == "connect"){
            if (!readWiFiConfig(wlan_ssid, wlan_password)) {
                Serial.println("Error: No WiFi config found");
                return 1;
            }
            
            if (wlan_ssid.length() == 0) {
                Serial.println("Error: SSID not set. Use 'wlan ssid <name>'");
                return 1;
            }
            
            Serial.println("Connecting to: " + wlan_ssid);
            
            #if defined(ESP8266) || defined(ESP32)
                WiFi.disconnect(true);
                delay(1000);
                WiFi.mode(WIFI_STA);
                WiFi.begin(wlan_ssid.c_str(), wlan_password.c_str());
            #elif defined(ARDUINO_UNOR4_WIFI)
                WiFi.disconnect();
                delay(1000);
                // Для UNO R4 WiFi нужно использовать правильный метод
                int status = WiFi.begin(wlan_ssid.c_str(), wlan_password.c_str());
                if (status != WL_CONNECTED) {
                    Serial.println("WiFi.begin() failed");
                }
            #endif
            
            unsigned long startTime = millis();
            while (WiFi.status() != WL_CONNECTED && millis() - startTime < 20000) { // Увеличил таймаут до 20 сек
                delay(500);
                Serial.print(".");
            }
            
            if (WiFi.status() == WL_CONNECTED) {
                Serial.println("\nConnected!");
                Serial.println("IP address: " + WiFi.localIP().toString());
                Serial.println("RSSI: " + String(WiFi.RSSI()) + " dBm");
                has_wifi = true;
                return 0;
            } else {
                Serial.println("\nConnection failed! Status: " + String(WiFi.status()));
                #if defined(ARDUINO_UNOR4_WIFI)
                    // Дополнительная диагностика для UNO R4
                    Serial.println("UNO R4 WiFi status: " + String(WiFi.status()));
                #endif
                return 1;
            }
        }
        else if(arg == "disconnect"){
            #if defined(ESP8266) || defined(ESP32)
                WiFi.disconnect(true);
            #elif defined(ARDUINO_UNOR4_WIFI)
                WiFi.disconnect();
            #endif
            has_wifi = false;
            Serial.println("WiFi disconnected");
        }
        else if(arg == "reginfo"){
            readWiFiConfig(wlan_ssid, wlan_password);
            Serial.println("SSID: " + wlan_ssid);
            Serial.println("Password: " + String(wlan_password.length() > 0 ? "***" + String(wlan_password.length()) + "***" : "not set"));
        }
        else if(arg == "scan"){
            Serial.println("Scanning networks...");
            int networks = WiFi.scanNetworks();
            if (networks == 0) {
                Serial.println("No networks found");
            } else {
                Serial.println("Found " + String(networks) + " networks:");
                for (int i = 0; i < networks; i++) {
                    Serial.print((i + 1));
                    Serial.print(": ");
                    Serial.print(WiFi.SSID(i));
                    Serial.print(" (");
                    Serial.print(WiFi.RSSI(i));
                    Serial.print(" dBm) ");
                    #if defined(ESP8266) || defined(ESP32)
                        Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "Open" : "Secured");
                    #elif defined(ARDUINO_UNOR4_WIFI)
                        Serial.println((WiFi.encryptionType(i) == 0) ? "Open" : "Secured");
                    #endif
                }
            }
        }
        else if(arg == "status"){
            switch(WiFi.status()) {
                case WL_CONNECTED:
                    Serial.println("Status: Connected");
                    Serial.println("IP: " + WiFi.localIP().toString());
                    Serial.println("RSSI: " + String(WiFi.RSSI()) + " dBm");
                    break;
                case WL_NO_SSID_AVAIL:
                    Serial.println("Status: SSID not available");
                    break;
                case WL_CONNECT_FAILED:
                    Serial.println("Status: Connection failed");
                    break;
                case WL_IDLE_STATUS:
                    Serial.println("Status: Idle");
                    break;
                case WL_DISCONNECTED:
                    Serial.println("Status: Disconnected");
                    break;
                default:
                    Serial.println("Status: Unknown");
            }
        }
        else if(arg == "interactive_setup"){
            Serial.print("\n\rSSID: ");
            wlan_ssid = getline();
            
            Serial.print("\n\rPassword: ");
            wlan_password = getline();
            
            if (saveWiFiConfig(wlan_ssid, wlan_password)) {
                Serial.println("\nWiFi configuration saved!");
            } else {
                Serial.println("\nError saving WiFi configuration!");
            }
        }
        else if(arg == "ip"){
            if (WiFi.status() == WL_CONNECTED) {
                Serial.println("IP: " + WiFi.localIP().toString());
                Serial.println("Gateway: " + WiFi.gatewayIP().toString());
                Serial.println("Subnet: " + WiFi.subnetMask().toString());
                Serial.println("DNS: " + WiFi.dnsIP().toString());
            } else {
                Serial.println("Not connected");
            }
        }
        else if(arg == "reconnect"){
            #if defined(ESP8266) || defined(ESP32)
                WiFi.reconnect();
            #elif defined(ARDUINO_UNOR4_WIFI)
                if (wlan_ssid.length() > 0) {
                    WiFi.begin(wlan_ssid.c_str(), wlan_password.c_str());
                }
            #endif
            Serial.println("Reconnection initiated");
        }
        else if(arg == "save"){
            if (saveWiFiConfig(wlan_ssid, wlan_password)) {
                Serial.println("WiFi configuration saved to file");
            } else {
                Serial.println("Error saving WiFi configuration");
            }
        }
        else if(arg == "load"){
            if (readWiFiConfig(wlan_ssid, wlan_password)) {
                Serial.println("WiFi configuration loaded from file");
                Serial.println("SSID: " + wlan_ssid);
            } else {
                Serial.println("Error loading WiFi configuration");
            }
        }
        
    #else  
        Serial.println("Current device isn't ESP32/ESP8266/UNO R4 WiFi");
    #endif
    return 0;
}
#if defined(ESP8266) || defined(ESP32) || defined(ARDUINO_UNOR4_WIFI)
    int auto_connect_wifi() {
        if (!readWiFiConfig(wlan_ssid, wlan_password)) {
            Serial.println("No WiFi configuration file found");
            return 0;
        }
        
        if (wlan_ssid.length() == 0) {
            Serial.println("No WiFi credentials in config file");
            return 0;
        }
        
        Serial.print("Auto-connecting to WiFi: " + wlan_ssid + "... ");
        
        #if defined(ESP8266) || defined(ESP32)
            WiFi.mode(WIFI_STA);
        #endif
        
        WiFi.begin(wlan_ssid.c_str(), wlan_password.c_str());
        
        // Не ждем долго при авто-подключении
        unsigned long startTime = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - startTime < 5000) {
            delay(100);
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("Connected!");
            Serial.println("IP: " + WiFi.localIP().toString());
            has_wifi = true;
            return 0;
        } else {
            Serial.println("Failed");
            return 1;
        }
    }
#endif

// Структура для хранения информации о программе
typedef struct {
    char name[32];
    char platform[16];
    uint32_t entry_point;
    uint32_t size;
} ProgramInfo;

// Прототипы функций для выполнения программ
int executeProgram(String filename);
#if defined(ESP32) || defined(ESP8266)
void executeESPProgram(File &file, ProgramInfo &info);
#endif
#if defined(ARDUINO_AVR_MEGA2560)
void executeAVRProgram(File &file, ProgramInfo &info);
#endif
#if defined(ARDUINO_ARCH_SAM)
void executeSAMProgram(File &file, ProgramInfo &info);
#endif
#if defined(ARDUINO_UNOR4_WIFI)
void executeR4Program(File &file, ProgramInfo &info);
#endif
void processBytecode(uint8_t opcode, File &file);
int executeProgram(String filename) {
    if (!sd_initialized) {
        Serial.println("SD card not initialized!");
        return 1;
    }
    
    if (!SD.exists(filename)) {
        Serial.println("Program not found: " + filename);
        return 1;
    }
    
    // Читаем заголовок программы
    File file = SD.open(filename, FILE_READ);
    if (!file) {
        Serial.println("Error opening program: " + filename);
        return 1;
    }
    
    // Проверяем магическое число (0xDEADBEEF)
    uint32_t magic;
    file.read((uint8_t*)&magic, sizeof(magic));
    
    if (magic != 0xDEADBEEF) {
        Serial.println("Invalid program format: wrong magic number");
        file.close();
        return 1;
    }
    
    // Читаем информацию о программе
    ProgramInfo progInfo;
    
    // Читаем platform как строку фиксированной длины
    for (int i = 0; i < 16; i++) {
        progInfo.platform[i] = file.read();
    }
    progInfo.platform[15] = '\0'; // Завершающий ноль
    
    file.read((uint8_t*)&progInfo.entry_point, sizeof(progInfo.entry_point));
    file.read((uint8_t*)&progInfo.size, sizeof(progInfo.size));
    
    String currentPlatform = getPlatform();
    String programPlatform = String(progInfo.platform);
    programPlatform.trim(); // Удаляем лишние пробелы

    //Serial.println("Debug: Program platform='" + programPlatform + "', Current='" + currentPlatform + "'");
    bool platformMatch = (programPlatform == currentPlatform) || 
                    (programPlatform == "any") ||
                    (programPlatform == "avr" && currentPlatform.startsWith("Arduino")) ||
                    (programPlatform == "AVR Arduino" && currentPlatform.startsWith("Arduino"));

    /*if (programPlatform != currentPlatform && programPlatform != "any") {*/
    if (!platformMatch) {
        Serial.println("Platform mismatch! Program for: " + programPlatform + ", Current: " + currentPlatform);
        file.close();
        return 1;
    }


    //Serial.println("Executing: " + filename);
    //Serial.println("Platform: " + programPlatform);
    //Serial.println("Size: " + String(progInfo.size) + " bytes");
    
    // Для разных платформ разное выполнение
    #if defined(ESP32) || defined(ESP8266)
        executeESPProgram(file, progInfo);
    #elif defined(ARDUINO_AVR_MEGA2560)
        executeAVRProgram(file, progInfo);
    #elif defined(ARDUINO_ARCH_SAM)
        executeSAMProgram(file, progInfo);
    #if defined(ARDUINO_UNOR4_WIFI)
        executeR4Program(file, progInfo);
    #endif   
    #else
        Serial.println("Unsupported platform for program execution");
    #endif
    
    file.close();
    return 0;
}

#if defined(ARDUINO_UNOR4_WIFI)
void executeR4Program(File &file, ProgramInfo &info) {
    // Аналогично другим платформам
    while (file.available()) {
        uint8_t opcode = file.read();
        processBytecode(opcode, file);
    }
}
#endif

// Выполнение программ для ESP
#if defined(ESP32) || defined(ESP8266)
void executeESPProgram(File &file, ProgramInfo &info) {
    
    // Читаем и выполняем команды байт-кода
    while (file.available()) {
        uint8_t opcode = file.read();
        processBytecode(opcode, file);
    }
    
}
#endif
bool isValidPin(uint8_t pin) {
    #if defined(ARDUINO_AVR_MEGA2560)
        // Для Arduino Mega 2560 - цифровые пины 0-53, аналоговые A0-A15
        return (pin >= 0 && pin <= 53) || (pin >= 54 && pin <= 69); // A0=54, A15=69
    #elif defined(ESP32)
        // Для ESP32 (большинство пинов поддерживают GPIO)
        return (pin >= 0 && pin <= 39);
    #elif defined(ESP8266)
        // Для ESP8266
        return (pin >= 0 && pin <= 16);
    #elif defined(ARDUINO_AVR_UNO)
        // Для Arduino Uno
        return (pin >= 0 && pin <= 19);
    #elif defined(ARDUINO_UNOR4_WIFI)
        // UNO R4: цифровые 0-13, аналоговые A0-A5 (14-19), дополнительные пины
        return (pin >= 0 && pin <= 13) || 
               (pin >= 14 && pin <= 19) || 
               (pin >= 20 && pin <= 21); 
    #else
        return true; // Для неизвестных платформ принимаем все пины
    #endif
}
// Выполнение программ для AVR
#if defined(ARDUINO_AVR_MEGA2560)
void executeAVRProgram(File &file, ProgramInfo &info) {
    
    // Читаем и выполняем команды байт-кода
    while (file.available()) {
        uint8_t opcode = file.read();
        processBytecode(opcode, file);
    }
    
    //Serial.println("AVR program execution finished");
}
#endif

#if defined(ARDUINO_ARCH_SAM)
void executeSAMProgram(File &file, ProgramInfo &info) {
    
    // Читаем и выполняем команды байт-кода
    while (file.available()) {
        uint8_t opcode = file.read();
        processBytecode(opcode, file);
    }
    
}
#endif

void processBytecode(uint8_t opcode, File &file) {
    switch(opcode) {
        case 0x01: { // PRINT_STRING
            String message;
            char ch;
            while (file.available() && (ch = file.read()) != 0) {
                message += ch;
            }
            Serial.println(message);
            break;
        }
        case 0x02: { // DELAY_MS
            if (file.available() >= 2) {
                uint16_t delayTime;
                file.read((uint8_t*)&delayTime, sizeof(delayTime));
                delay(delayTime);
            }
            break;
        }
        case 0x03: { // DIGITAL_WRITE
            if (file.available() >= 2) {
                uint8_t pin = file.read();
                uint8_t value = file.read();
                if (isValidPin(pin)) {
                    pinMode(pin, OUTPUT); // Добавляем автоматическую настройку
                    digitalWrite(pin, value);
                } else {
                }
            }
            break;
        }
        case 0x04: { // ANALOG_WRITE
            if (file.available() >= 2) {
                uint8_t pin = file.read();
                uint8_t value = file.read();
                if (isValidPin(pin)) {
                    pinMode(pin, OUTPUT); // Добавляем автоматическую настройку
                    analogWrite(pin, value);
                    //Serial.println("AWRITE: pin " + String(pin) + " = " + String(value));
                } else {
                    //Serial.println("Error: Invalid pin " + String(pin));
                }
            }
            break;
        }
        case 0x05: { // ASM_RAW - ПРЯМОЙ ВЫЗОВ!
            String asm_code;
            char ch;
            while (file.available() && (ch = file.read()) != 0) {
                asm_code += ch;
            }
            
            //Serial.println("EXEC: " + asm_code);
            #ifdef ARDUINO_ARCH_AVR
            // ТОЛЬКО AVR КОМАНДЫ!
            if (asm_code == "nop") { asm volatile ("nop"); }
            else if (asm_code == "sei") { asm volatile ("sei"); }
            else if (asm_code == "cli") { asm volatile ("cli"); }
            else if (asm_code == "wdr") { asm volatile ("wdr"); }
            else if (asm_code == "sleep") { asm volatile ("sleep"); }
            else if (asm_code == "jmp 0") { asm volatile ("jmp 0"); }
            else if (asm_code == "ret") { asm volatile ("ret"); }
            else if (asm_code == "reti") { asm volatile ("reti"); }
            else if (asm_code == "lpm") { asm volatile ("lpm"); }
            else if (asm_code == "spm") { asm volatile ("spm"); }
            else if (asm_code == "ijmp") { asm volatile ("ijmp"); }
            else if (asm_code == "icall") { asm volatile ("icall"); }
            else if (asm_code == "bset 0") { asm volatile ("bset 0"); }
            else if (asm_code == "bclr 0") { asm volatile ("bclr 0"); }
            else if (asm_code == "break") { asm volatile ("break"); }
            
            else {
                //Serial.println("UNKNOWN ASM: " + asm_code);
            }
            #endif
            break;
        }
        case 0x06: { // SYS_CMD
            String command_txt;
            char ch;
            while (file.available() && (ch = file.read()) != 0) {
                command_txt += ch;
            }
            processCommand(command_txt);
            break;
        }
        case 0x10: { // SET_VAR
            uint8_t varType = file.read(); // 0 = string, 1 = integer
            
            // Читаем имя переменной
            String varName;
            char ch;
            while (file.available() && (ch = file.read()) != 0) {
                varName += ch;
            }
            
            if (varType == 0) {
                // Строковая переменная
                String value;
                while (file.available() && (ch = file.read()) != 0) {
                    value += ch;
                }
                setStringVariable(varName, value);
            } else {
                // Целочисленная переменная - читаем 4 байта в little-endian
                if (file.available() >= 4) {
                    int value = 0;
                    value |= file.read();
                    value |= file.read() << 8;
                    value |= file.read() << 16;
                    value |= file.read() << 24;
                    setIntVariable(varName, value);
                }
            }
            break;
        }
        
        case 0x11: { // GET_VAR
            // Читаем имя переменной
            String varName;
            char ch;
            while (file.available() && (ch = file.read()) != 0) {
                varName += ch;
            }
            
            int index = findVariable(varName);
            if (index != -1) {
                if (variables[index].type == 0) {
                    Serial.println(variables[index].strValue);
                } else {
                    Serial.println(variables[index].intValue);
                }
            } else {
                Serial.println("undefined");
            }
            break;
        }
        case 0x12: { // MATH_ADD
            String varName;
            char ch;
            while (file.available() && (ch = file.read()) != 0) {
                varName += ch;
            }
            
            if (file.available() >= 4) {
                int value = 0;
                value |= file.read();
                value |= file.read() << 8;
                value |= file.read() << 16;
                value |= file.read() << 24;
                
                int current = getIntVariable(varName);
                setIntVariable(varName, current + value);
            }
            break;
        }
        
        case 0x13: { // MATH_SUB
            String varName;
            char ch;
            while (file.available() && (ch = file.read()) != 0) {
                varName += ch;
            }
            
            if (file.available() >= 4) {
                int value;
                file.read((uint8_t*)&value, sizeof(value));
                
                int current = getIntVariable(varName);
                setIntVariable(varName, current - value);
            }
            break;
        }
        
        case 0x14: { // MATH_MUL
            String varName;
            char ch;
            while (file.available() && (ch = file.read()) != 0) {
                varName += ch;
            }
            
            if (file.available() >= 4) {
                int value;
                file.read((uint8_t*)&value, sizeof(value));
                
                int current = getIntVariable(varName);
                setIntVariable(varName, current * value);
            }
            break;
        }
        
        case 0x15: { // MATH_DIV
            String varName;
            char ch;
            while (file.available() && (ch = file.read()) != 0) {
                varName += ch;
            }
            
            if (file.available() >= 4) {
                int value;
                file.read((uint8_t*)&value, sizeof(value));
                
                if (value != 0) {
                    int current = getIntVariable(varName);
                    setIntVariable(varName, current / value);
                }
            }
            break;
        }
        case 0x07: { // BEGINI2C
            Wire.begin();
            break;
        }
        case 0x08: { // I2CADDR
            String addr_iic;
            char ch;
            while (file.available() && (ch = file.read()) != 0) {
                addr_iic += ch;
            }
            
            // Проверка на пустую строку
            if (addr_iic.length() == 0) {
                break;
            }
            
            uint8_t i2c_addr = (uint8_t)strtol(addr_iic.c_str(), NULL, 16);
            
            // Проверка валидности адреса (I2C адреса обычно 0x08-0x77)
            if (i2c_addr >= 0x08 && i2c_addr <= 0x77) {
                Wire.beginTransmission(i2c_addr);
            }
            break;
        }
        case 0x09: { // I2CSEND
            // Читаем данные до нулевого байта
            uint8_t i2cData[32];
            int dataLength = 0;
            
            while (file.available() && dataLength < 31) { // Оставляем место для завершающего нуля
                uint8_t byte = file.read();
                if (byte == 0x00) break; // Конец данных
                i2cData[dataLength++] = byte;
            }
            
            // Отправляем данные
            if (dataLength > 0) {
                #ifdef DEBUG
                Serial.print("I2C Sending ");
                Serial.print(dataLength);
                Serial.println(" bytes:");
                for (int i = 0; i < dataLength; i++) {
                    Serial.print("0x");
                    Serial.print(i2cData[i], HEX);
                    Serial.print(" ");
                }
                Serial.println();
                #endif
                
                Wire.write(i2cData, dataLength);
            }
            break;
        }
        case 0x0A: { // I2CEND
            Wire.endTransmission();
            break;
        }
        case 0x17: { // MATH_ADD_VAR (переменная + переменная)
            String varName1, varName2;
            char ch;
            while (file.available() && (ch = file.read()) != 0) {
                varName1 += ch;
            }
            while (file.available() && (ch = file.read()) != 0) {
                varName2 += ch;
            }
            
            int val1 = getIntVariable(varName1);
            int val2 = getIntVariable(varName2);
            setIntVariable(varName1, val1 + val2);
            break;
        }

        case 0x18: { // MATH_SUB_VAR (переменная - переменная)
            String varName1, varName2;
            char ch;
            while (file.available() && (ch = file.read()) != 0) {
                varName1 += ch;
            }
            while (file.available() && (ch = file.read()) != 0) {
                varName2 += ch;
            }
            
            int val1 = getIntVariable(varName1);
            int val2 = getIntVariable(varName2);
            setIntVariable(varName1, val1 - val2);
            break;
        }

        case 0x19: { // MATH_MUL_VAR (переменная * переменная)
            String varName1, varName2;
            char ch;
            while (file.available() && (ch = file.read()) != 0) {
                varName1 += ch;
            }
            while (file.available() && (ch = file.read()) != 0) {
                varName2 += ch;
            }
            
            int val1 = getIntVariable(varName1);
            int val2 = getIntVariable(varName2);
            setIntVariable(varName1, val1 * val2);
            break;
        }

        case 0x1A: { // MATH_DIV_VAR (переменная / переменная)
            String varName1, varName2;
            char ch;
            while (file.available() && (ch = file.read()) != 0) {
                varName1 += ch;
            }
            while (file.available() && (ch = file.read()) != 0) {
                varName2 += ch;
            }
            
            int val1 = getIntVariable(varName1);
            int val2 = getIntVariable(varName2);
            if (val2 != 0) {
                setIntVariable(varName1, val1 / val2);
            }
            break;
        }
        case 0x1B: { // CONCAT_VAR (конкатенация строковых переменных)
            String varName1, varName2;
            char ch;
            while (file.available() && (ch = file.read()) != 0) {
                varName1 += ch;
            }
            while (file.available() && (ch = file.read()) != 0) {
                varName2 += ch;
            }
            
            String val1 = getStringVariable(varName1);
            String val2 = getStringVariable(varName2);
            setStringVariable(varName1, val1 + val2);
            break;
        }

        case 0x1C: { // GETLINE (чтение строки в переменную)
            String varName;
            char ch;
            while (file.available() && (ch = file.read()) != 0) {
                varName += ch;
            }
            
            // Используем вашу функцию getline() для чтения ввода
            String input = getline();
            setStringVariable(varName, input);
            break;
        }

        case 0x1D: { // SYSCMD_VAR
            String varName;
            char ch;
            while (file.available() && (ch = file.read()) != 0) {
                varName += ch;
            }
            
            String val = getStringVariable(varName);
            processCommand(val);
            break;
        }
        case 0x1E: { // PRINT_NO_RETURN
            String message;
            char ch;
            while (file.available() && (ch = file.read()) != 0) {
                message += ch;
            }
            Serial.print(message);
            break;
        }
        case 0x1F: { // RETURNCHAR
            Serial.print("\r");
            break;
        }
        case 0x20: { // BASICCOLOR
            Serial.print("\e[0m");
            break;
        }
        case 0x21: { // REDTEXT -- Red
            Serial.print("\e[31m");
            break;
        }
        case 0x22: { // GREENTEXT -- Green
            Serial.print("\e[32m");
            break;
        }
        case 0x23: { // YELLOWTEXT -- Green
            Serial.print("\e[33m");
            break;
        }
        case 0x24: { // BLUETEXT -- Blue
            Serial.print("\e[34m");
            break;
        }
        case 0x25: { // CATCHREQUEST
            String varName, url;
            char ch;
            while (file.available() && (ch = file.read()) != 0) {
                varName += ch;
            }
            while (file.available() && (ch = file.read()) != 0) {
                url += ch;
            }



            String request = "";


            if (!checkNetwork()) {
                Serial.println("Error: No network connection");
            }
            else{
            #if defined(ESP8266) || defined(ESP32)
                HTTPClient http;
                WiFiClient client;
            
                
                if (http.begin(client, url)) {
                int httpCode = http.GET();
                
                if (httpCode > 0) {                    
                    if (httpCode == HTTP_CODE_OK) {
                        String payload = http.getString();
                        request = payload;
                    } else {
                        request = String(httpCode);
                    }
                } else {
                    request = String(httpCode);
                }
                
                http.end();
                } else {
                //Serial.println("Unable to connect to: " + url);
                }
            #elif defined(ARDUINO_UNOR4_WIFI)
                WiFiClient client;
                
                //Serial.println("Fetching: " + url);
                
                if (client.connect(url.c_str(), 80)) {
                client.println("GET / HTTP/1.1");
                client.println("Host: " + url);
                client.println("Connection: close");
                client.println();
                
                while (client.connected()) {
                    String line = client.readStringUntil('\n');
                    if (line == "\r") {
                        break;
                    }
                }
                
                while (client.available()) {
                    String line = client.readStringUntil('\n');
                    request += line;
                    //Serial.println(line);
                }
                
                client.stop();
                } else {

                //Serial.println("Unable to connect to: " + url);
                }
            #else
                Serial.println("Curl not supported on this platform");
            #endif
            

            }
            setStringVariable(varName, request);
            break;
        }
        case 0x26: { // CATCHREQUESTVAR
            String varName, url_var;
            char ch;
            
            // Читаем имя переменной для результата
            while (file.available() && (ch = file.read()) != 0) {
                varName += ch;
            }
            
            // Читаем имя переменной с URL
            while (file.available() && (ch = file.read()) != 0) {
                url_var += ch;
            }

            // Получаем URL из переменной
            String url = getStringVariable(url_var);
            String request = "";
            
            if (!checkNetwork()) {
                Serial.println("Error: No network connection");
                setStringVariable(varName, "Error: No network connection");
            } else {
            #if defined(ESP8266) || defined(ESP32)
                HTTPClient http;
                WiFiClient client;
            
                if (http.begin(client, url)) {
                    int httpCode = http.GET();
                    
                    if (httpCode > 0) {                    
                        if (httpCode == HTTP_CODE_OK) {
                            String payload = http.getString();
                            request = payload;
                        } else {
                            request = "HTTP Error: " + String(httpCode);
                        }
                    } else {
                        request = "Request failed";
                    }
                    
                    http.end();
                } else {
                    request = "Connection failed";
                }
            #elif defined(ARDUINO_UNOR4_WIFI)
                WiFiClient client;
                
                if (client.connect(url.c_str(), 80)) {
                    client.println("GET / HTTP/1.1");
                    client.println("Host: " + url);
                    client.println("Connection: close");
                    client.println();
                    
                    while (client.connected()) {
                        String line = client.readStringUntil('\n');
                        if (line == "\r") {
                            break;
                        }
                    }
                    
                    while (client.available()) {
                        String line = client.readStringUntil('\n');
                        request += line;
                    }
                    
                    client.stop();
                } else {
                    request = "Connection failed";
                }
            #else
                request = "Curl not supported on this platform";
            #endif
            }
            
            setStringVariable(varName, request);
            break;
        }
        case 0x27: { // LOOP_START - начало цикла
            // Сохраняем текущую позицию для возврата
            uint32_t loopStart = file.position();
            
            // Читаем количество итераций (0 = бесконечный цикл)
            uint32_t iterations;
            file.read((uint8_t*)&iterations, sizeof(iterations));
            
            // Сохраняем информацию о цикле в переменных
            setIntVariable("__loop_iterations", iterations);
            setIntVariable("__loop_counter", 0);
            setIntVariable("__loop_start", (int)loopStart);
            break;
        }
        case 0x28: { // LOOP_END - конец цикла
            int iterations = getIntVariable("__loop_iterations");
            int counter = getIntVariable("__loop_counter");
            int loopStart = getIntVariable("__loop_start");
            
            // Увеличиваем счетчик
            counter++;
            setIntVariable("__loop_counter", counter);
            
            // Проверяем условие выхода
            if (iterations == 0 || counter < iterations) {
                // Возвращаемся к началу цикла
                file.seek(loopStart);
            }
            // else: выходим из цикла и продолжаем выполнение
            break;
        }
        case 0x29: { // BREAK_LOOP - принудительный выход из цикла
            // Просто продолжаем выполнение после LOOP_END
            // Ищем следующий LOOP_END и пропускаем его
            while (file.available()) {
                uint8_t opcode = file.read();
                if (opcode == 0x28) { // LOOP_END
                    break;
                } else {
                    // Пропускаем остальные опкоды
                    processBytecode(opcode, file);
                }
            }
            break;
        }

        //короче, если кто-то читает мой код сейчас, то знайте, я НЕ буду делать goto-шки, потому что это треш и мозги варятся
        case 0xFF: // END_PROGRAM
            return;
        default:
            return;
            //Serial.println("Unknown opcode: " + String(opcode, HEX));
    }
    
}


// Реализация функций
int compileBlackScript(String sourceFile, String outputFile) {
    if (!sd_initialized) {
        Serial.println("SD card not initialized!");
        return 1;
    }
    
    // Читаем исходный файл
    String sourceCode = readFileToString(sourceFile);
    if (sourceCode == "") {
        Serial.println("Error: Cannot read source file: " + sourceFile);
        return 1;
    }
    
    // Разбиваем на строки
    int lineCount = countNewlines(sourceCode);
    String lines[lineCount + 1];
    splitString(sourceCode, "\n", lines, lineCount + 1);
    
    // Создаем выходной файл
    if (SD.exists(outputFile)) {
        SD.remove(outputFile);
    }
    
    File outFile = SD.open(outputFile, FILE_WRITE);
    if (!outFile) {
        Serial.println("Error creating output file: " + outputFile);
        return 1;
    }
    
    // Записываем заголовок
    outFile.write((const uint8_t*)"\xEF\xBE\xAD\xDE", 4); // Magic number 0xDEADBEEF в little-endian
    
    // Платформа (16 байт)
    String platform = "any";
    char platformBytes[16] = {0};
    platform.toCharArray(platformBytes, 16);
    outFile.write((const uint8_t*)platformBytes, 16);
    
    // Entry point и размер (заглушки)
    uint32_t entryPoint = 0x1000;
    uint32_t programSize = 0;
    outFile.write((const uint8_t*)&entryPoint, 4);
    outFile.write((const uint8_t*)&programSize, 4);
    
    // Компилируем команды
    size_t codeStart = outFile.position();
    
    for (int i = 0; i <= lineCount; i++) {
        String line = lines[i];
        line.trim();
        
        if (line == "" || line.startsWith("#")) {
            continue;
        }
        
        // Разбиваем строку на части
        String parts[10];
        int partCount = splitDynamic(line, " ", parts, 10);
        
        if (partCount == 0) continue;
        
        String command = parts[0];
        command.toUpperCase();
        
        if (command == "PRINT") {
            outFile.write((uint8_t)0x01); // PRINT opcode
            String message = "";
            for (int j = 1; j < partCount; j++) {
                if (j > 1) message += " ";
                message += parts[j];
            }
            outFile.write((const uint8_t*)message.c_str(), message.length());
            outFile.write((uint8_t)0x00); // Null terminator
            
        } else if (command == "DELAY") {
            if (partCount >= 2) {
                outFile.write((uint8_t)0x02); // DELAY opcode
                uint16_t delayTime = parts[1].toInt();
                outFile.write((const uint8_t*)&delayTime, 2);
            }
            
        } else if (command == "DWRITE") {
            if (partCount >= 3) {
                outFile.write((uint8_t)0x03); // DWRITE opcode
                uint8_t pin = parts[1].toInt();
                uint8_t value = parts[2].toInt();
                outFile.write(pin);
                outFile.write(value);
            }
            
        } else if (command == "AWRITE") {
            if (partCount >= 3) {
                outFile.write((uint8_t)0x04); // AWRITE opcode
                uint8_t pin = parts[1].toInt();
                uint8_t value = parts[2].toInt();
                outFile.write(pin);
                outFile.write(value);
            }
            
        } else if (command == "ASM") {
            outFile.write((uint8_t)0x05); // ASM opcode
            String asmCode = "";
            for (int j = 1; j < partCount; j++) {
                if (j > 1) asmCode += " ";
                asmCode += parts[j];
            }
            outFile.write((const uint8_t*)asmCode.c_str(), asmCode.length());
            outFile.write((uint8_t)0x00); // Null terminator
            
        } else if (command == "SYS_CMD") {
            outFile.write((uint8_t)0x06); // SYS_CMD opcode
            String sysCmd = "";
            for (int j = 1; j < partCount; j++) {
                if (j > 1) sysCmd += " ";
                sysCmd += parts[j];
            }
            outFile.write((const uint8_t*)sysCmd.c_str(), sysCmd.length());
            outFile.write((uint8_t)0x00); // Null terminator
            
        }else if (command == "SET") {
                    if (partCount >= 4 && parts[2] == "=") {
                        outFile.write((uint8_t)0x10); // SET_VAR opcode
                        
                        String varName = parts[1];
                        
                        // Определяем тип переменной (строка или число)
                        if (parts[3].startsWith("\"") && line.indexOf("\"", parts[3].length() + 4) != -1) {
                            // Строковая переменная
                            outFile.write((uint8_t)0); // 0 = string type
                            
                            // Записываем имя переменной
                            outFile.write((const uint8_t*)varName.c_str(), varName.length());
                            outFile.write((uint8_t)0x00);
                            
                            // Извлекаем строковое значение (убираем кавычки)
                            String value = "";
                            for (int j = 3; j < partCount; j++) {
                                if (j > 3) value += " ";
                                String part = parts[j];
                                if (part.startsWith("\"")) part = part.substring(1);
                                if (part.endsWith("\"")) part = part.substring(0, part.length() - 1);
                                value += part;
                            }
                            
                            // Записываем значение
                            outFile.write((const uint8_t*)value.c_str(), value.length());
                            outFile.write((uint8_t)0x00);
                            
                        } else {
                            // Числовая переменная
                            outFile.write((uint8_t)1); // 1 = integer type
                            
                            // Записываем имя переменной
                            outFile.write((const uint8_t*)varName.c_str(), varName.length());
                            outFile.write((uint8_t)0x00);
                            
                            // Записываем числовое значение в little-endian формате
                            int value = parts[3].toInt();
                            outFile.write((uint8_t)(value & 0xFF));
                            outFile.write((uint8_t)((value >> 8) & 0xFF));
                            outFile.write((uint8_t)((value >> 16) & 0xFF));
                            outFile.write((uint8_t)((value >> 24) & 0xFF));
                        }
                    }
                    
        }  else if (command == "PRINTVAR") {

            if (partCount >= 2) {
                        outFile.write((uint8_t)0x11); // GET_VAR opcode
                        String varName = parts[1];
                        
                        outFile.write((const uint8_t*)varName.c_str(), varName.length());
                        outFile.write((uint8_t)0x00); // Null terminator
                    }
        }  
                
        else if (command == "MATH") {
                    if (partCount >= 4) {
                        String varName = parts[1];
                        String operation = parts[2];
                        String valueStr = parts[3];
                        
                        // Проверяем, является ли значение числом или переменной
                        bool isNumber = true;
                        for (int i = 0; i < valueStr.length(); i++) {
                            if (!isDigit(valueStr[i]) && valueStr[i] != '-') {
                                isNumber = false;
                                break;
                            }
                        }
                        
                        if (isNumber) {
                        // Старая логика для чисел
                        int value = valueStr.toInt();
                        
                        if (operation == "ADD") {
                            outFile.write((uint8_t)0x12); // MATH_ADD
                        } else if (operation == "SUB") {
                            outFile.write((uint8_t)0x13); // MATH_SUB
                        } else if (operation == "MUL") {
                            outFile.write((uint8_t)0x14); // MATH_MUL
                        } else if (operation == "DIV") {
                            outFile.write((uint8_t)0x15); // MATH_DIV
                        } else {
                            continue;
                        }
                        
                        // Записываем имя переменной
                        outFile.write((const uint8_t*)varName.c_str(), varName.length());
                        outFile.write((uint8_t)0x00);
                        
                        // Записываем значение в little-endian формате
                        outFile.write((uint8_t)(value & 0xFF));
                        outFile.write((uint8_t)((value >> 8) & 0xFF));
                        outFile.write((uint8_t)((value >> 16) & 0xFF));
                        outFile.write((uint8_t)((value >> 24) & 0xFF));
                        
                    }  else {
                            // Новая логика для переменных
                            if (operation == "ADD") {
                                outFile.write((uint8_t)0x17); // MATH_ADD_VAR
                            } else if (operation == "SUB") {
                                outFile.write((uint8_t)0x18); // MATH_SUB_VAR
                            } else if (operation == "MUL") {
                                outFile.write((uint8_t)0x19); // MATH_MUL_VAR
                            } else if (operation == "DIV") {
                                outFile.write((uint8_t)0x1A); // MATH_DIV_VAR
                            } else {
                                continue;
                            }
                            
                            // Записываем первую переменную
                            outFile.write((const uint8_t*)varName.c_str(), varName.length());
                            outFile.write((uint8_t)0x00);
                            
                            // Записываем вторую переменную
                            outFile.write((const uint8_t*)valueStr.c_str(), valueStr.length());
                            outFile.write((uint8_t)0x00);
                        }
            }

        
        }else if (command == "BEGINI2C") {
            outFile.write((uint8_t)0x07); // BEGINI2C opcode
        } else if (command == "I2CADDR") {
            outFile.write((uint8_t)0x08); // ASM opcode
            String addr_iic = "";
            for (int j = 1; j < partCount; j++) {
                if (j > 1) addr_iic += " ";
                addr_iic += parts[j];
            }
            outFile.write((const uint8_t*)addr_iic.c_str(), addr_iic.length());
            outFile.write((uint8_t)0x00); // Null terminator
            
        } else if (command == "I2CSEND") {
            outFile.write((uint8_t)0x09); // I2CSEND opcode
            String data_iic = "";
            for (int j = 1; j < partCount; j++) {
                if (j > 1) data_iic += " ";
                data_iic += parts[j];
            }
            outFile.write((const uint8_t*)data_iic.c_str(), data_iic.length());
            outFile.write((uint8_t)0x00); // Null terminator
        }
        else if (command == "I2CEND") {
            outFile.write((uint8_t)0x0A); // I2CEND opcode
        }

        else if (command == "CONCAT") {
            if (partCount >= 3) {
                outFile.write((uint8_t)0x1B); // CONCAT_VAR opcode
                String varName1 = parts[1];
                String varName2 = parts[2];
                
                // Записываем первую переменную
                outFile.write((const uint8_t*)varName1.c_str(), varName1.length());
                outFile.write((uint8_t)0x00);
                
                // Записываем вторую переменную
                outFile.write((const uint8_t*)varName2.c_str(), varName2.length());
                outFile.write((uint8_t)0x00);
            }
        }

        else if (command == "GETLINE") {
            if (partCount >= 2) {
                outFile.write((uint8_t)0x1C); // GETLINE opcode
                String varName = parts[1];
                
                // Записываем имя переменной
                outFile.write((const uint8_t*)varName.c_str(), varName.length());
                outFile.write((uint8_t)0x00);
            }
        }

        else if (command == "PRINTNR") {
            outFile.write((uint8_t)0x1E); // PRINTNR opcode
            String message = "";
            for (int j = 1; j < partCount; j++) {
                if (j > 1) message += " ";
                message += parts[j];
            }
            outFile.write((const uint8_t*)message.c_str(), message.length());
            outFile.write((uint8_t)0x00); // Null terminator
            
        }

        else if (command == "SYSCMD_VAR") {

            if (partCount >= 2) {
                        outFile.write((uint8_t)0x1D); // SYSCMD_VAR opcode
                        String varName = parts[1];
                        
                        outFile.write((const uint8_t*)varName.c_str(), varName.length());
                        outFile.write((uint8_t)0x00); // Null terminator
                    }
        }
        else if (command == "BASICCOLOR") {
            outFile.write((uint8_t)0x20); // BASICCOLOR opcode
        }
        else if (command == "REDTEXT") {
            outFile.write((uint8_t)0x21); // REDTEXT opcode
        }
        else if (command == "GREENTEXT") {
            outFile.write((uint8_t)0x22); // GREENTEXT opcode
        }
        else if (command == "CATCHREQUEST") {
            if (partCount >= 3) {
                outFile.write((uint8_t)0x25); // CATCHREQUEST opcode
                String varName1 = parts[1];
                String url = parts[2];
                
                // Записываем первую переменную
                outFile.write((const uint8_t*)varName1.c_str(), varName1.length());
                outFile.write((uint8_t)0x00);
                
                // Записываем вторую переменную
                outFile.write((const uint8_t*)url.c_str(), url.length());
                outFile.write((uint8_t)0x00);
            }
        }
        else if (command == "CATCHREQUESTVAR") {
            if (partCount >= 3) {
                outFile.write((uint8_t)0x26); // CATCHREQUESTVAR opcode
                String varName1 = parts[1];
                String url = parts[2];
                
                // Записываем первую переменную
                outFile.write((const uint8_t*)varName1.c_str(), varName1.length());
                outFile.write((uint8_t)0x00);
                
                // Записываем вторую переменную
                outFile.write((const uint8_t*)url.c_str(), url.length());
                outFile.write((uint8_t)0x00);
            }
        }
        else if (command == "END") {
            outFile.write((uint8_t)0xFF); // END opcode
        }else if (command == "LOOP") {
            outFile.write((uint8_t)0x27); // LOOP_START opcode
            
            if (partCount == 1) {
                // Бесконечный цикл
                uint32_t infinite = 0;
                outFile.write((const uint8_t*)&infinite, 4);
            } else if (partCount == 2) {
                // Цикл с количеством итераций
                uint32_t iterations = parts[1].toInt();
                outFile.write((const uint8_t*)&iterations, 4);
            }
            
        } else if (command == "ENDLOOP") {
            outFile.write((uint8_t)0x28); // LOOP_END opcode
            
        } else if (command == "BREAK") {
            outFile.write((uint8_t)0x29); // BREAK_LOOP opcode
        }

    }
    
    // Записываем конечный END если его не было
    outFile.write((uint8_t)0xFF);
    
    // Обновляем размер программы
    size_t codeEnd = outFile.position();
    programSize = codeEnd - codeStart;
    outFile.seek(24); // Позиция размера в заголовке
    outFile.write((const uint8_t*)&programSize, 4);
    
    outFile.close();
    
    Serial.println("Compilation successful: " + sourceFile + " -> " + outputFile);
    //Serial.println("Program size: " + String(programSize) + " bytes");
    
    return 0;
}
int shellscript(String code = "echo hello world"){
  int newlineCount = countNewlines(code);
  String lines[newlineCount]; // Массив для результатов
  splitString(code, "\n", lines, newlineCount);
  for (String t : lines){
    processCommand(t);
  }
  return 0;
}
int shellscript_starter(String fname) {
  // Проверяем расширение файла
  if (!fname.endsWith(".sh")) {
    Serial.println("Error: Not a shell script (must have .sh extension)");
    return 1;
  }
  
  String code = readFileToString(fname);
  if (code == "") {
    Serial.println("Error: Cannot read script file: " + fname);
    return 1;
  }
  
  //Serial.println("Executing shell script: " + fname);
  return shellscript(code);
}
int bsc_command(String args) {
    args.trim();
    
    // Разбиваем аргументы
    String parts[3];
    int partCount = splitDynamic(args, " ", parts, 3);
    
    if (partCount < 2) {
        Serial.println("Usage: bsc <source.bs> <output.bin>");
        Serial.println("Example: bsc test.bs test.bin");
        return 1;
    }
    
    String sourceFile = parts[0];
    String outputFile = parts[1];
    
    // Добавляем расширение .bs если нужно
    if (!sourceFile.endsWith(".bs")) {
        sourceFile += ".bs";
    }
    
    // Добавляем расширение .bin если нужно  
    //if (!outputFile.endsWith(".bin")) {
    //   outputFile += ".bin";
    //}
    
    return compileBlackScript(sourceFile, outputFile);
}

void autoexec(){
    if (SD.exists("/ssr/autorun.sh")) {
         shellscript_starter("/ssr/autorun.sh");
          
    }
}

bool manual_login(){
    String username_input, password_input;
    Serial.print("\n\r"+hostname + " login: ");
    username_input = getline();
            
    Serial.print("Password: ");
    password_input = hiddenGetline();


    String login_orig = readFirstLineFromFile("/etc/user");

    String content = readFileToString("/etc/user");
    String lines[10];
    int lineCount = splitString(content, "\n", lines, 10);
    String password_orig = (lineCount >= 2) ? lines[1] : "";
    

    if (username_input == login_orig && password_input == password_orig){
        current_user = username_input;
        Serial.println();
        return true;

    }
    else{
        return false;
    }
}

void load_all(){
    //Starting Serial port
    Serial.print("Starting serial (speed 115200) :: ");
    Serial.begin(115200);
    Serial.println("[\e[32mSTARTED\e[0m]");
    //logMessage("Starting serial (speed 115200) :: " + "[\e[32mSTARTED\e[0m]");

    //Checking dev type
    Serial.print("Setting devicetype :: ");
    devicetype = getPlatform();
    Serial.println("[\e[32mREADY\e[0m]");
    //initing SD
    Serial.print("Initializing SD card :: ");
    if (initSD()) {
        Serial.println("[\e[32mMOUNTED\e[0m]");
        logMessage("[MOUNTED]");
    } else {
        Serial.println("[\e[31mFAILED\e[0m]");
        return;
    }

    //Stetting hostname
    Serial.print("Setting hostname :: ");
    set_hostname("", true);
    Serial.println("[\e[32mREADY\e[0m]");
    //logMessage("Setting hostname :: " + "[\e[32mREADY\e[0m]");

    //Starting WLAN
    #if defined(ESP8266) || defined(ESP32) || defined(ARDUINO_UNOR4_WIFI)
      Serial.print("Starting WLAN :: ");
      Serial.println("[\e[32mSTARTED\e[0m]");
      //logMessage("Starting WLAN :: " + "[\e[32mSTARTED\e[0m]");
      if (SD.exists("/etc/wlancred")){
        if (stater_wlan_connect("Connecting to WiFi") == 0) {
          Serial.println("\rConnecting to WiFi\t[\e[32mCONNECTED\e[0m]");
        } else {
          Serial.println("[\e[31mFAILED\e[0m]");
          // Продолжаем без WiFi
        }
      }
    #endif
    

    //delay(200);
    
    clear();
    if (sd_initialized){
        if (SD.exists("/etc/aulogin")){
            // Читаем данные автологина
            String aulogin_content = readFileToString("/etc/aulogin");
            String aulogin_lines[10];
            int aulogin_lineCount = splitString(aulogin_content, "\n", aulogin_lines, 10);
            String aulogin_login = (aulogin_lineCount >= 1) ? aulogin_lines[0] : "";
            String aulogin_password = (aulogin_lineCount >= 2) ? aulogin_lines[1] : "";
            
            // Читаем данные пользователя
            String user_content = readFileToString("/etc/user");
            String user_lines[10];
            int user_lineCount = splitString(user_content, "\n", user_lines, 10);
            String user_login = (user_lineCount >= 1) ? user_lines[0] : "";
            String user_password = (user_lineCount >= 2) ? user_lines[1] : "";
            
            // Проверяем совпадение
            if (aulogin_login == user_login && aulogin_password == user_password){
                current_user = user_login;
            } else {
                //Serial.println("Auto-login failed, switching to manual login");
                while (true){
                    if(manual_login()){
                        break;
                    }
                }
            }

            build_hello();
            bash();
        }else if (SD.exists("/etc/user")){
            while (true){
                if(manual_login()){
                    break;
                }
            }
            build_hello();
            bash();
        }
        else{
            set_hostname("installer");
            clear();
            build_hello();
            //installer welcome
            bash();
        }
    }else{
        Serial.println("\e[31mPLEASE INSERT THE SD CARD AND REBOOT\e[0m");
    }
}
int clear(){

    Serial.println("\033[2J");
    return 0;
}
String uname(String arg = "a"){
    switch(arg[0]){
        case 'a':
            Serial.println("fireworkos 1.1.1 "+devicetype+" kernel "+kernelver+" "+kerndate);
            return "fireworkos 1.1.1 "+devicetype+" kernel"+kernelver+" "+kerndate;
        case 'r':
            Serial.print(kernelver);
            return kernelver;
    }
    return "";
}
int bash() {
    String command = "";
    Serial.println("\n\r\e[0m\e[36m┌──(\e[34m"+hostname+"\e[36m)\e[0m");
    Serial.print("\e[0m\e[36m└─\e[34m$\e[0m ");
    
    while (true) {
        if (Serial.available() > 0) {
            char inChar = Serial.read();
            
            // Обработка Enter
            if (inChar == '\n' || inChar == '\r') {
                command.trim();
                if (command.length() > 0) {
                    Serial.println(); // Переход на новую строку
                    processCommand(command); // Обработка команды
                }
                // Перезапуск bash после выполнения команды
                Serial.println("\n\r\e[0m\e[36m┌──(\e[34m"+hostname+"\e[36m)\e[0m");
                Serial.print("\e[0m\e[36m└─\e[34m$\e[0m ");
                command = "";
                continue;
            }
            
            // Обработка Backspace (ASCII 8 или 127)
            else if (inChar == 8 || inChar == 127) {
                if (command.length() > 0) {
                    // Удаляем последний символ из строки
                    command.remove(command.length() - 1);
                    
                    // Стираем символ из терминала
                    Serial.print("\b \b"); // Backspace, пробел, backspace
                }
            }
            
            // Обработка обычных символов
            else if (isPrintable(inChar)) {
                command += inChar;
                Serial.print(inChar); // Эхо символа
            }
        }
    }
    return 0;
}
void man_db(String program_name){
        
}
// Функция для обработки команд
void processCommand(String cmd) {
    //cmd.toLowerCase();
    cmd.trim();
    
    if (cmd == "clear") {
        clear();
    }
    else if (cmd == "pwd") {
        Serial.println(current_dir);
    }
    else if (cmd == "uname") {
        uname();
    }
    else if (cmd == "reboot") {
        reboot();
    }    
    else if (cmd == "boardinf") {
        boardinf();
    }
    else if (cmd.startsWith("uname ")) {
        String text = cmd.substring(6);
        uname(text);
    }
    else if (cmd.startsWith("man ")) {
        String text = cmd.substring(4);
        man_db(text);
    }
    else if (cmd.startsWith("cd ")) {
        String text = cmd.substring(3);
        changedir(text);
    }
    else if (cmd.startsWith("sh ")) {
        String text = cmd.substring(3);
        shellscript_starter(text);
    }

    else if (cmd.startsWith("bsc ")) {
        String args = cmd.substring(4);
        bsc_command(args);
    }

    else if (cmd.startsWith("curl ")) {
        String url = cmd.substring(5);
        url.trim();
        if (url.startsWith("http://") || url.startsWith("https://")) {
            curl(url);
        } else {
            Serial.println("Error: URL must start with http:// or https://");
        }
    }
    else if (cmd.startsWith("wlan ")) {
        String text = cmd.substring(5);
        wlanctl(text);
    }
    else if (cmd.startsWith("sethostname ")) {
        String text = cmd.substring(12);
        set_hostname(text);
    }
    else if (cmd.startsWith("ls")) {
        String path = cmd.substring(2);
        path.trim();
        if (path == "") path = current_dir;
        listFiles(path);
    }
    else if (cmd.startsWith("cat ")) {
        String filename = cmd.substring(4);
        readFile(filename);
    }
    else if (cmd.startsWith("touch ")) {
        String filename = cmd.substring(6);
        filename.trim();
        createFile(filename);
    }
    else if (cmd.startsWith("append ")) {
        // Формат: append filename.txt Text to append
        int spacePos = cmd.indexOf(' ', 7);
        if (spacePos != -1) {
            String filename = cmd.substring(7, spacePos);
            String text = cmd.substring(spacePos + 1);
            appendToFile(filename, text, true);
        } else {
            Serial.println("Usage: append <filename> <text>");
        }
    }
    else if (cmd.startsWith("echo ") && cmd.indexOf(" >> ") != -1) {
    // Формат: echo text >> filename.txt
        int sepPos = cmd.indexOf(" >> ");
        String text = cmd.substring(5, sepPos);
        String filename = cmd.substring(sepPos + 4);
        appendToFile(filename, text, true);
    }
    else if (cmd.startsWith("echo ") && cmd.indexOf(" > ") != -1) {
        int sepPos = cmd.indexOf(" > ");
        String content = cmd.substring(5, sepPos);
        String filename = cmd.substring(sepPos + 3);
        createFile(filename, content);
    }
    else if (cmd.startsWith("rm ")) {
        String filename = cmd.substring(3);
        filename.trim();
        deleteFile(filename);
    }
    else if (cmd.startsWith("mkdir ")) {
        String dirname = cmd.substring(6);
        dirname.trim();
        createDir(dirname);
    }
    else if (cmd.startsWith("rmdir ")) {
        String dirname = cmd.substring(6);
        dirname.trim();
        deleteDir(dirname);
    }
    else if (cmd == "sdinfo") {
        sdInfo();
    }
    else if (cmd == "sdinit") {
        initSD();
    }
    else if (cmd.startsWith("echo ")) {
        
        String text = cmd.substring(5);
        Serial.println(text);
    }


    else {
      if (SD.exists("/bin/"+cmd)) {
          executeProgram("/bin/"+cmd);
          
      }
      else if (SD.exists("/opt/"+cmd)) {
          executeProgram("/opt/"+cmd);
      }
      else if (SD.exists(cmd)) {
          executeProgram(cmd);
      }
      
      else{
          Serial.print("Команда не найдена: ");
          Serial.println(cmd);
        }
    }
}



void setup() {
  load_all();
 
}

void loop() {
  // put your main code here, to run repeatedly:

}
