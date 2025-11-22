/*

FireworkOS 1.2 (Crypto functions)
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

#ifndef CRYPTO_H
#define CRYPTO_H

String getHash(String data) {
  uint32_t hash = 5381;
  for(int i = 0; i < data.length(); i++) {
    hash = ((hash << 5) + hash) + data.charAt(i);
  }
  
  String result = "";
  for(int i = 0; i < 8; i++) {
    byte nibble = (hash >> (28 - i*4)) & 0x0F;
    result += String(nibble, HEX);
  }
  return result;
}
#endif