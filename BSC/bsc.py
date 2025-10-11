"""
BlackScript Compiler (BlackScript 1)
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
"""

#!/usr/bin/env python3


import struct
import sys
import os

class SnowCompiler:
    def __init__(self):
        self.platforms = ['esp32', 'esp8266', 'avr', 'arm', 'sam','any']
        self.opcodes = {
            'PRINT': 0x01,
            'DELAY': 0x02,
            'DWRITE': 0x03,
            'AWRITE': 0x04,
            'ASM': 0x05,
            'SYS_CMD': 0x06,

            'SET_VAR': 0x10,
            'GET_VAR': 0x11,
            'MATH_ADD': 0x12,
            'MATH_SUB': 0x13,
            'MATH_MUL': 0x14,
            'MATH_DIV': 0x15,

            'BEGINI2C': 0x07,
            'I2CADDR': 0x08,
            'I2CSEND': 0x09,
            'I2CEND': 0x0A,

            'MATH_ADD_VAR': 0x17,
            'MATH_SUB_VAR': 0x18,
            'MATH_MUL_VAR': 0x19,
            'MATH_DIV_VAR': 0x1A,
            'CONCAT_VAR': 0x1B,
            'GETLINE': 0x1C,
            'CONCAT': 0x1B,

            'SYSCMD_VAR': 0x1D,
            'PRINTNR': 0x1E,

            'BASICCOLOR': 0x20,
            'REDTEXT': 0x21,
            'GREENTEXT': 0x22,
            'YELLOWTEXT': 0x23,
            'BLUETEXT': 0x24,

            'CATCHREQUEST': 0x25,
            'CATCHREQUESTVAR': 0x26,

            'END': 0xFF
        }

    def compile_program(self, source_file, output_file, platform='any'):
        with open(source_file, 'r') as f:
            lines = f.readlines()

        with open(output_file, 'wb') as f:
            # Заголовок программы
            f.write(struct.pack('<I', 0xDEADBEEF))  # Magic number

            # Информация о программе
            platform_bytes = platform.ljust(16).encode('ascii')[:16]
            f.write(platform_bytes)
            f.write(struct.pack('<I', 0x1000))  # Entry point
            f.write(struct.pack('<I', 0))  # Placeholder for size

            # Компиляция команд
            code_start = f.tell()

            for line in lines:
                line = line.strip()
                if not line or line.startswith('#'):
                    continue

                parts = line.split()
                command = parts[0].upper()

                if command == 'PRINT':
                    message = ' '.join(parts[1:])
                    f.write(bytes([self.opcodes['PRINT']]))
                    f.write(message.encode('ascii'))
                    f.write(b'\x00')  # Null terminator

                elif command == 'DELAY':
                    delay_time = int(parts[1])
                    f.write(bytes([self.opcodes['DELAY']]))
                    f.write(struct.pack('<H', delay_time))

                elif command == 'DWRITE':
                    pin = int(parts[1])
                    value = int(parts[2])
                    f.write(bytes([self.opcodes['DWRITE']]))
                    f.write(bytes([pin, value]))

                elif command == 'AWRITE':
                    pin = int(parts[1])
                    value = int(parts[2])
                    f.write(bytes([self.opcodes['AWRITE']]))
                    f.write(bytes([pin, value]))

                elif command == 'ASM':
                    message = ' '.join(parts[1:])
                    f.write(bytes([self.opcodes['ASM']]))
                    f.write(message.encode('ascii'))
                    f.write(b'\x00')  # Null terminator

                elif command == 'SYS_CMD':
                    message = ' '.join(parts[1:])
                    f.write(bytes([self.opcodes['SYS_CMD']]))
                    f.write(message.encode('ascii'))
                    f.write(b'\x00')  # Null terminator

                elif command == 'SET':
                    if len(parts) >= 4 and parts[2] == '=':
                        var_name = parts[1]
                        # Поддерживаем строки и числа
                        if parts[3].startswith('"') and line.count('"') >= 2:
                            # Строковая переменная
                            value_str = ' '.join(parts[3:])
                            if value_str.startswith('"') and value_str.endswith('"'):
                                value_str = value_str[1:-1]
                            f.write(bytes([self.opcodes['SET_VAR']]))
                            f.write(bytes([0]))  # 0 = string type
                            f.write(var_name.encode('ascii'))
                            f.write(b'\x00')  # Null terminator для имени
                            f.write(value_str.encode('ascii'))
                            f.write(b'\x00')  # Null terminator для значения
                        else:
                            # Числовая переменная
                            try:
                                value = int(parts[3])
                                f.write(bytes([self.opcodes['SET_VAR']]))
                                f.write(bytes([1]))  # 1 = integer type
                                f.write(var_name.encode('ascii'))
                                f.write(b'\x00')  # Null terminator для имени
                                f.write(struct.pack('<i', value))  # 4-байтовое целое
                            except ValueError:
                                print(f"Error: Invalid value for SET command: {parts[3]}")
                elif command == 'MATH':
                    if len(parts) >= 4:
                        var_name = parts[1]
                        op = parts[2].upper()
                        value_str = parts[3]

                        # Проверяем, является ли значение числом или переменной
                        try:
                            value = int(value_str)
                            # Это число - используем старые opcodes
                            if op == 'ADD':
                                opcode = self.opcodes['MATH_ADD']
                            elif op == 'SUB':
                                opcode = self.opcodes['MATH_SUB']
                            elif op == 'MUL':
                                opcode = self.opcodes['MATH_MUL']
                            elif op == 'DIV':
                                opcode = self.opcodes['MATH_DIV']
                            else:
                                continue

                            f.write(bytes([opcode]))
                            f.write(var_name.encode('ascii'))
                            f.write(b'\x00')
                            f.write(struct.pack('<i', value))

                        except ValueError:
                            # Это переменная - используем новые opcodes
                            if op == 'ADD':
                                opcode = 0x17  # MATH_ADD_VAR
                            elif op == 'SUB':
                                opcode = 0x18  # MATH_SUB_VAR
                            elif op == 'MUL':
                                opcode = 0x19  # MATH_MUL_VAR
                            elif op == 'DIV':
                                opcode = 0x1A  # MATH_DIV_VAR
                            else:
                                continue

                            f.write(bytes([opcode]))
                            f.write(var_name.encode('ascii'))
                            f.write(b'\x00')
                            f.write(value_str.encode('ascii'))
                            f.write(b'\x00')

                elif command == 'PRINTVAR':
                    if len(parts) >= 2:
                        var_name = parts[1]
                        f.write(bytes([self.opcodes['GET_VAR']]))
                        f.write(var_name.encode('ascii'))
                        f.write(b'\x00')  # Null terminator

                elif command == 'BEGINI2C':
                    f.write(bytes([self.opcodes['BEGINI2C']]))

                elif command == 'I2CADDR':
                    addr_str = parts[1]
                    f.write(bytes([self.opcodes['I2CADDR']]))
                    f.write(addr_str.encode('ascii'))
                    f.write(b'\x00')  # Null terminator ТОЛЬКО для строки

                elif command == 'I2CSEND':
                    # Парсим hex значения
                    hex_values = []
                    for part in parts[1:]:
                        try:
                            hex_val = int(part, 16)
                            hex_values.append(hex_val)
                        except ValueError:
                            print(f"Error: Invalid hex value {part}")
                            continue

                    f.write(bytes([self.opcodes['I2CSEND']]))
                    # Записываем данные БЕЗ null terminator между ними
                    for val in hex_values:
                        f.write(bytes([val]))
                    # НЕ добавляем b'\x00' здесь - интерпретатор сам остановится при чтении следующего opcode

                elif command == 'I2CEND':
                    f.write(bytes([self.opcodes['I2CEND']]))
                elif command == 'CONCAT':
                    if len(parts) >= 3:
                        var_name1 = parts[1]
                        var_name2 = parts[2]
                        f.write(bytes([0x1B]))  # CONCAT_VAR opcode
                        f.write(var_name1.encode('ascii'))
                        f.write(b'\x00')
                        f.write(var_name2.encode('ascii'))
                        f.write(b'\x00')
                elif command == 'GETLINE':
                    if len(parts) >= 2:
                        var_name = parts[1]
                        f.write(bytes([0x1C]))  # GETLINE opcode
                        f.write(var_name.encode('ascii'))
                        f.write(b'\x00')
                elif command == 'SYSCMD_VAR':
                    if len(parts) >= 2:
                        var_name = parts[1]
                        f.write(bytes([self.opcodes['SYSCMD_VAR']]))
                        f.write(var_name.encode('ascii'))
                        f.write(b'\x00')  # Null terminator

                elif command == 'PRINTNR':
                    message = ' '.join(parts[1:])
                    f.write(bytes([self.opcodes['PRINTNR']]))
                    f.write(message.encode('ascii'))
                    f.write(b'\x00')  # Null terminator

                elif command == 'BASICCOLOR':
                    f.write(bytes([self.opcodes['BASICCOLOR']]))
                elif command == 'REDTEXT':
                    f.write(bytes([self.opcodes['REDTEXT']]))
                elif command == 'GREENTEXT':
                    f.write(bytes([self.opcodes['GREENTEXT']]))
                elif command == 'YELLOWTEXT':
                    f.write(bytes([self.opcodes['YELLOWTEXT']]))
                elif command == 'BLUETEXT':
                    f.write(bytes([self.opcodes['BLUETEXT']]))

                elif command == 'CATCHREQUEST':
                    if len(parts) >= 3:
                        var_name1 = parts[1]
                        var_name2 = parts[2]
                        f.write(bytes([self.opcodes['CATCHREQUEST']]))  # CATCHREQUEST opcode
                        f.write(var_name1.encode('ascii'))
                        f.write(b'\x00')
                        f.write(var_name2.encode('ascii'))
                        f.write(b'\x00')
                elif command == 'CATCHREQUESTVAR':
                    if len(parts) >= 3:
                        var_name1 = parts[1]
                        var_name2 = parts[2]
                        f.write(bytes([self.opcodes['CATCHREQUESTVAR']]))  # CATCHREQUEST opcode
                        f.write(var_name1.encode('ascii'))
                        f.write(b'\x00')
                        f.write(var_name2.encode('ascii'))
                        f.write(b'\x00')



                elif command == 'END':
                    f.write(bytes([self.opcodes['END']]))

            # Записываем размер программы
            code_end = f.tell()
            program_size = code_end - code_start
            f.seek(24)  # Позиция размера в заголовке
            f.write(struct.pack('<I', program_size))

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python compiler.py <source.txt> <output.bin> [platform]")
        print("Platforms: esp32, esp8266, avr, any")
        sys.exit(1)

    source = sys.argv[1]
    output = sys.argv[2]
    platform = sys.argv[3] if len(sys.argv) > 3 else 'any'

    compiler = SnowCompiler()
    compiler.compile_program(source, output, platform)
    print(f"Compiled {source} -> {output} for platform {platform}")
