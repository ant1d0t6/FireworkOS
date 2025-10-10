# BlackScript Documentation

## Language Syntax

### Basic Structure
Each line contains one command. Lines starting with `#` are comments. Commands are case-insensitive.

### Commands

#### Output Commands
```BlackScript
PRINT Hello World
PRINTNR No newline
BASICCOLOR
REDTEXT
GREENTEXT
```

#### Variable Operations
```BlackScript
SET variable_name = "string value"
SET variable_name = 42
PRINTVAR variable_name
```

#### Mathematical Operations
```BlackScript
# Add number to variable
MATH variable_name ADD 5
# Subtract number from variable
MATH variable_name SUB 3
# Multiply variable by number
MATH variable_name MUL 2
# Divide variable by number
MATH variable_name DIV 2

# Add two variables
MATH var1 ADD var2
# Subtract variables
MATH var1 SUB var2
# Multiply variables
MATH var1 MUL var2
# Divide variables
MATH var1 DIV var2

# Concatenate two string variables
CONCAT str1 str2
```

#### GPIO Operations
```BlackScript
# Digital write: pin 13, value HIGH
DWRITE 13 1
# Digital write: pin 13, value LOW
DWRITE 13 0
# Analog write: pin 9, value 128
AWRITE 9 128
```

#### System Control
```BlackScript
# Delay for 1000 milliseconds
DELAY 1000
# Execute system command
SYS_CMD ls
# Execute command from variable
SYSCMD_VAR variable_name
# Read user input into variable
GETLINE input_var
```

#### Assembly (AVR only)
```BlackScript
# Execute AVR assembly instruction
ASM nop
# Enable interrupts
ASM sei
# Disable interrupts
ASM cli
```

#### Program Control
```BlackScript
END
```

## Data Types

### Variables
- **String**: Text data enclosed in quotes `"text"`
- **Integer**: Whole numbers `123`
- Maximum of 50 variables can be defined

### Supported Platforms
- `esp32` - ESP32 microcontrollers
- `esp8266` - ESP8266 microcontrollers  
- `avr` - AVR Arduino boards (Uno, Mega, etc.)
- `arm` - ARM-based boards
- `sam` - SAM Arduino boards
- `any` - Platform-independent code

## Compilation

### Using Python Compiler
```BlackScript
python bsc.py source.bs output [platform]
```

### Using FireworkOS Built-in Compiler
```BlackScript
bsc source.bs output
```

## File Structure

### Compiled Binary Format
- **Magic Number**: 0xDEADBEEF (4 bytes)
- **Platform**: 16-byte string
- **Entry Point**: 4 bytes (usually 0x1000)
- **Program Size**: 4 bytes
- **Bytecode**: Compiled instructions

### Source File Extension
- `.bs` - BlackScript source files

## Examples

### Basic Hello World
```BlackScript
PRINT Hello World!
DELAY 1000
END
```

### Blink LED
```BlackScript
# Blink LED on pin 13
SET counter = 0
PRINT Starting LED blink program

BEGIN:
DWRITE 13 1
DELAY 500
DWRITE 13 0
DELAY 500
MATH counter ADD 1
PRINT Blink count: 
PRINTVAR counter
END
```

### User Interaction
```BlackScript
PRINT What is your name?
GETLINE name
PRINTNR Hello, 
PRINTVAR name
PRINT Welcome to BlackScript!
```

## Platform-Specific Notes

### Memory Limitations
- Maximum 50 variables
- Variable names: up to 31 characters
- String values: limited by available RAM

## Error Handling
- Invalid commands are ignored
- Platform mismatch prevents execution
- Missing files generate error messages
- Variable errors return default values

## Best Practices
1. Always use `END` command to terminate programs
2. Initialize variables before using them in MATH operations
3. Use comments for complex logic
4. Check platform compatibility before deployment
5. Test GPIO pin numbers for your specific board

This documentation covers the core features of BlackScript v1.0. Refer to the compiler source code for implementation details and advanced usage.
