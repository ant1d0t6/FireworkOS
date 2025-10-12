/*

FireworkOS 1.0 (Base functions)
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

#ifndef BASELIB_H
#define BASELIB_H

#include "global_vars.h"



// Функция для разделения строки по разделителю
int splitString(String input, String delimiter, String results[], int maxResults) {
  int count = 0;
  int index = 0;
  int delimiterLength = delimiter.length();
  
  while (count < maxResults - 1 && index != -1) {
    int newIndex = input.indexOf(delimiter, index);
    
    if (newIndex == -1) {
      // Последний элемент
      results[count] = input.substring(index);
      count++;
      break;
    } else {
      results[count] = input.substring(index, newIndex);
      count++;
      index = newIndex + delimiterLength;
    }
  }
  return count;
}
int countNewlines(String text) {
  int count = 0;
  int position = -1;
  
  while ((position = text.indexOf('\n', position + 1)) != -1) {
    count++;
  }
  
  return count;
}
// Возвращает количество найденных элементов
int splitDynamic(String input, String delimiter, String output[], int maxSize) {
  int count = 0;
  int index = 0;
  int delimiterLen = delimiter.length();
  
  while (count < maxSize && index < input.length()) {
    int delimiterIndex = input.indexOf(delimiter, index);
    
    if (delimiterIndex == -1) {
      output[count] = input.substring(index);
      count++;
      break;
    }
    
    output[count] = input.substring(index, delimiterIndex);
    count++;
    index = delimiterIndex + delimiterLen;
  }
  
  return count;
}
String getline(){
    String active_line = "";
    while (true) {
                if (Serial.available() > 0) {
                    char inChar = Serial.read();
                    
                    // Обработка Enter
                    if (inChar == '\n' || inChar == '\r') {
                        active_line.trim();
                        Serial.println(); // Переход на новую строку
                        return active_line;
                        
                        // Перезапуск bash после выполнения команды
                        break;
                        //active_line = "";
                        //continue;
                    }
                    
                    // Обработка Backspace (ASCII 8 или 127)
                    else if (inChar == 8 || inChar == 127) {
                        if (active_line.length() > 0) {
                            // Удаляем последний символ из строки
                            active_line.remove(active_line.length() - 1);
                            
                            // Стираем символ из терминала
                            Serial.print("\b \b"); // Backspace, пробел, backspace
                        }
                    }
                    
                    // Обработка обычных символов
                    else if (isPrintable(inChar)) {
                        active_line += inChar;
                        Serial.print(inChar); // Эхо символа
                    }
                }
            }
}

String hiddenGetline(){
    String active_line = "";
    while (true) {
                if (Serial.available() > 0) {
                    char inChar = Serial.read();
                    
                    // Обработка Enter
                    if (inChar == '\n' || inChar == '\r') {
                        active_line.trim();
                        Serial.println(); // Переход на новую строку
                        return active_line;
                        
                        // Перезапуск bash после выполнения команды
                        break;
                        //active_line = "";
                        //continue;
                    }
                    
                    // Обработка Backspace (ASCII 8 или 127)
                    else if (inChar == 8 || inChar == 127) {
                        if (active_line.length() > 0) {
                            // Удаляем последний символ из строки
                            active_line.remove(active_line.length() - 1);
                            
                            // Стираем символ из терминала
                            //Serial.print("\b \b"); // Backspace, пробел, backspace
                        }
                    }
                    
                    // Обработка обычных символов
                    else if (isPrintable(inChar)) {
                        active_line += inChar;
                        //Serial.print();
                        //Serial.print(inChar); // Эхо символа
                    }
                }
            }
}

String customGetline(String symbol = "*"){
    String active_line = "";
    while (true) {
                if (Serial.available() > 0) {
                    char inChar = Serial.read();
                    
                    // Обработка Enter
                    if (inChar == '\n' || inChar == '\r') {
                        active_line.trim();
                        Serial.println(); // Переход на новую строку
                        return active_line;
                        
                        // Перезапуск bash после выполнения команды
                        break;
                        //active_line = "";
                        //continue;
                    }
                    
                    // Обработка Backspace (ASCII 8 или 127)
                    else if (inChar == 8 || inChar == 127) {
                        if (active_line.length() > 0) {
                            // Удаляем последний символ из строки
                            active_line.remove(active_line.length() - 1);
                            
                            // Стираем символ из терминала
                            Serial.print("\b \b"); // Backspace, пробел, backspace
                        }
                    }
                    
                    // Обработка обычных символов
                    else if (isPrintable(inChar)) {
                        active_line += inChar;
                        Serial.print(symbol);
                        //Serial.print(inChar); // Эхо символа
                    }
                }
            }
}

int print(String text="", String end="\n\r"){
  Serial.print(text + end);
}
#endif