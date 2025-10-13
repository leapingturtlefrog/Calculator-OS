// Simple calculator kernel for 32-bit protected mode
// No standard library, direct hardware access

// VGA text mode buffer address
#define VGA_MEMORY 0xB8000
// Screen dimensions
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
// Colors
#define VGA_COLOR_BLACK 0
#define VGA_COLOR_WHITE 15
#define VGA_COLOR_GREEN 2
#define VGA_COLOR_RED 4
// Standard VGA color byte
#define VGA_COLOR(fg, bg) ((bg << 4) | fg)
// Keyboard port
#define KEYBOARD_PORT 0x60
#define KEYBOARD_STATUS 0x64

// Special key codes
#define KEY_SHIFT 0x2A  // Left shift press
#define KEY_SHIFT_REL 0xAA  // Left shift release
#define KEY_RIGHT_SHIFT 0x36  // Right shift press
#define KEY_RIGHT_SHIFT_REL 0xB6  // Right shift release

// Globals
unsigned short cursor_pos = 0;
char input_buffer[256];
unsigned char input_length = 0;
unsigned char input_position = 0;
int shift_pressed = 0;  // Flag to track shift key state

// Function prototypes
void clear_screen();
void print_char(char c, unsigned char color, int x, int y);
void print_string(const char* str, unsigned char color);
void print_line(const char* str, unsigned char color);
void print_number(int num);
void print_float(float num);
char get_key();
float calculate();
float parse_number();
unsigned char inb(unsigned short port);
void outb(unsigned short port, unsigned char value);
void update_cursor();

// Read a byte from an I/O port
unsigned char inb(unsigned short port) {
    unsigned char result;
    __asm__ volatile("inb %1, %0" : "=a" (result) : "dN" (port));
    return result;
}

// Write a byte to an I/O port
void outb(unsigned short port, unsigned char value) {
    __asm__ volatile("outb %0, %1" : : "a" (value), "dN" (port));
}

// Update hardware cursor position
void update_cursor() {
    unsigned short pos = cursor_pos;
    
    // Send cursor position command and data to VGA controller
    outb(0x3D4, 14);  // Tell the VGA controller we're setting the high byte
    outb(0x3D5, (pos >> 8) & 0xFF); // Send high byte
    outb(0x3D4, 15);  // Tell the VGA controller we're setting the low byte
    outb(0x3D5, pos & 0xFF);       // Send low byte
}

// Entry point - must be aligned properly for the bootloader to jump to
void __attribute__((section(".text.start"))) kernel_main(void) {
    unsigned char* video_memory = (unsigned char*)VGA_MEMORY;
    int i;
    
    // Simple visual indicator that the kernel has started
    for (i = 0; i < 10; i++) {
        video_memory[i * 2] = '*';
        video_memory[i * 2 + 1] = VGA_COLOR(VGA_COLOR_RED, VGA_COLOR_BLACK);
    }
    
    // Clear screen and initialize calculator
    clear_screen();
    print_line("Calculator OS v0.1", VGA_COLOR(VGA_COLOR_GREEN, VGA_COLOR_BLACK));
    print_line("Enter a simple expression (e.g. 5+3, 10-2, 6*7, 8/2)", VGA_COLOR(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    print_line("Press Enter to calculate, ESC to clear", VGA_COLOR(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    print_string("> ", VGA_COLOR(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    update_cursor();
    
    input_length = 0;
    input_buffer[0] = '\0';
    
    // Main loop
    while (1) {
        char key = get_key();
        
        // Enter key - process calculation
        if (key == '\n') {
            // Clear any characters at the current position
            print_char(' ', VGA_COLOR(VGA_COLOR_WHITE, VGA_COLOR_BLACK), cursor_pos % VGA_WIDTH, cursor_pos / VGA_WIDTH);
            
            // Move to the next line
            cursor_pos += VGA_WIDTH - (cursor_pos % VGA_WIDTH);
            update_cursor();
            
            if (input_length > 0) {
                float result = calculate();
                print_string("= ", VGA_COLOR(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
                print_float(result);
                
                // Clear any remaining characters and move to the next line
                print_char(' ', VGA_COLOR(VGA_COLOR_WHITE, VGA_COLOR_BLACK), cursor_pos % VGA_WIDTH, cursor_pos / VGA_WIDTH);
                cursor_pos += VGA_WIDTH - (cursor_pos % VGA_WIDTH);
                print_string("> ", VGA_COLOR(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
                update_cursor();
                
                // Clear input buffer for next calculation
                input_length = 0;
                input_buffer[0] = '\0';
            }
        }
        // Backspace - delete previous character
        else if (key == '\b' && input_length > 0) {
            input_length--;
            input_buffer[input_length] = '\0';
            cursor_pos--;
            print_char(' ', VGA_COLOR(VGA_COLOR_WHITE, VGA_COLOR_BLACK), cursor_pos % VGA_WIDTH, cursor_pos / VGA_WIDTH);
            update_cursor();
        }
        // ESC - clear input
        else if (key == 27) {
            while (input_length > 0) {
                input_length--;
                cursor_pos--;
                print_char(' ', VGA_COLOR(VGA_COLOR_WHITE, VGA_COLOR_BLACK), cursor_pos % VGA_WIDTH, cursor_pos / VGA_WIDTH);
            }
            input_buffer[0] = '\0';
            update_cursor();
        }
        // Regular character input
        else if (key >= 32 && key <= 126 && input_length < 255) {
            input_buffer[input_length] = key;
            input_length++;
            input_buffer[input_length] = '\0';
            print_char(key, VGA_COLOR(VGA_COLOR_WHITE, VGA_COLOR_BLACK), cursor_pos % VGA_WIDTH, cursor_pos / VGA_WIDTH);
            cursor_pos++;
            update_cursor();
        }
    }
}

// Clear the screen
void clear_screen() {
    unsigned char* video_memory = (unsigned char*)VGA_MEMORY;
    
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT * 2; i += 2) {
        video_memory[i] = ' ';               // Character
        video_memory[i + 1] = VGA_COLOR(VGA_COLOR_WHITE, VGA_COLOR_BLACK); // Color
    }
    
    cursor_pos = 0;
    update_cursor();
}

// Print a character at a specific position
void print_char(char c, unsigned char color, int x, int y) {
    unsigned char* video_memory = (unsigned char*)VGA_MEMORY;
    int offset = (y * VGA_WIDTH + x) * 2;
    
    video_memory[offset] = c;
    video_memory[offset + 1] = color;
}

// Print a string at the current cursor position
void print_string(const char* str, unsigned char color) {
    int i = 0;
    while (str[i] != '\0') {
        if (str[i] == '\n') {
            // Clear to the end of the current line
            int current_x = cursor_pos % VGA_WIDTH;
            for (int j = current_x; j < VGA_WIDTH; j++) {
                print_char(' ', color, j, cursor_pos / VGA_WIDTH);
            }
            // Move to the start of the next line
            cursor_pos += VGA_WIDTH - current_x;
        } else {
            print_char(str[i], color, cursor_pos % VGA_WIDTH, cursor_pos / VGA_WIDTH);
            cursor_pos++;
        }
        i++;
    }
}

// Print a string and move to the next line
void print_line(const char* str, unsigned char color) {
    print_string(str, color);
    cursor_pos += VGA_WIDTH - (cursor_pos % VGA_WIDTH);
}

// Print an integer number
void print_number(int num) {
    // Handle negative numbers
    if (num < 0) {
        print_char('-', VGA_COLOR(VGA_COLOR_WHITE, VGA_COLOR_BLACK), cursor_pos % VGA_WIDTH, cursor_pos / VGA_WIDTH);
        cursor_pos++;
        num = -num;
    }
    
    // 0 special case
    if (num == 0) {
        print_char('0', VGA_COLOR(VGA_COLOR_WHITE, VGA_COLOR_BLACK), cursor_pos % VGA_WIDTH, cursor_pos / VGA_WIDTH);
        cursor_pos++;
        return;
    }
    
    // Calculate number length
    int temp = num;
    int length = 0;
    while (temp > 0) {
        temp /= 10;
        length++;
    }
    
    // Allocate buffer on stack
    char buffer[20]; // Max 19 digits for 64-bit integer plus null
    buffer[length] = '\0';
    
    // Fill buffer from end to start
    while (length > 0) {
        buffer[--length] = '0' + (num % 10);
        num /= 10;
    }
    
    // Print the number
    print_string(buffer, VGA_COLOR(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
}

// Print a floating point number
void print_float(float num) {
    // Special case for zero
    if (num == 0.0f) {
        print_string("0.0", VGA_COLOR(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
        return;
    }
    
    // Handle negative numbers
    if (num < 0) {
        print_char('-', VGA_COLOR(VGA_COLOR_WHITE, VGA_COLOR_BLACK), cursor_pos % VGA_WIDTH, cursor_pos / VGA_WIDTH);
        cursor_pos++;
        num = -num;
    }
    
    // Extract integer part
    int integer_part = (int)num;
    print_number(integer_part);
    
    // Extract fractional part (fixed to 4 decimal places)
    float fractional = num - integer_part;
    if (fractional > 0.0f) {
        print_char('.', VGA_COLOR(VGA_COLOR_WHITE, VGA_COLOR_BLACK), cursor_pos % VGA_WIDTH, cursor_pos / VGA_WIDTH);
        cursor_pos++;
        
        // Convert to 4 decimal places 
        int decimal_places = 4;
        for (int i = 0; i < decimal_places; i++) {
            fractional *= 10;
            int digit = (int)fractional;
            print_char('0' + digit, VGA_COLOR(VGA_COLOR_WHITE, VGA_COLOR_BLACK), cursor_pos % VGA_WIDTH, cursor_pos / VGA_WIDTH);
            cursor_pos++;
            fractional -= digit;
        }
    } else {
        // If it's a whole number, add .0
        print_string(".0", VGA_COLOR(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    }
}

// Wait for and get a keypress
char get_key() {
    static unsigned char normal_scancodes[128] = {
        0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
        '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
        0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,
        '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '
    };
    
    static unsigned char shift_scancodes[128] = {
        0, 27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
        '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
        0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0,
        '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' '
    };
    
    unsigned char scancode;
    char asciicode = 0;
    
    // Wait for a key event
    while (asciicode == 0) {
        // Check if there's a new key event
        if ((inb(KEYBOARD_STATUS) & 1) == 0) continue;
        
        // Read the scancode
        scancode = inb(KEYBOARD_PORT);
        
        // Handle shift keys
        if (scancode == KEY_SHIFT || scancode == KEY_RIGHT_SHIFT) {
            shift_pressed = 1;
            continue;
        } else if (scancode == KEY_SHIFT_REL || scancode == KEY_RIGHT_SHIFT_REL) {
            shift_pressed = 0;
            continue;
        }
        
        // Check if it's a key release (bit 7 set)
        if (scancode & 0x80) {
            continue;  // Ignore key releases
        }
        
        // Map scancode to ASCII based on shift state
        if (shift_pressed && scancode < 128) {
            asciicode = shift_scancodes[scancode];
        } else if (scancode < 128) {
            asciicode = normal_scancodes[scancode];
        }
    }
    
    return asciicode;
}

// Simple calculator function
float calculate() {
    input_position = 0;
    
    // Get the first number
    float num1 = parse_number();
    
    // Check if there's an operator
    if (input_position >= input_length) {
        return num1; // Just a number
    }
    
    // Get the operator
    char operator = input_buffer[input_position++];
    
    // Get the second number
    float num2 = parse_number();
    
    // Perform calculation
    switch (operator) {
        case '+': return num1 + num2;
        case '-': return num1 - num2;
        case '*': return num1 * num2;
        case '/': return num2 != 0.0f ? num1 / num2 : 0.0f; // Avoid division by zero
        default: return 0.0f;
    }
}

// Parse a number from the input buffer
float parse_number() {
    float num = 0.0f;
    int decimal_found = 0;
    float decimal_place = 0.1f;
    
    // Skip any spaces
    while (input_position < input_length && input_buffer[input_position] == ' ') {
        input_position++;
    }
    
    // Parse the integer part
    while (input_position < input_length) {
        char current = input_buffer[input_position];
        
        if (current >= '0' && current <= '9') {
            if (!decimal_found) {
                num = num * 10.0f + (current - '0');
            } else {
                num = num + (current - '0') * decimal_place;
                decimal_place *= 0.1f;
            }
        } else if (current == '.' && !decimal_found) {
            decimal_found = 1;
        } else {
            break;
        }
        
        input_position++;
    }
    
    // Skip any spaces after the number
    while (input_position < input_length && input_buffer[input_position] == ' ') {
        input_position++;
    }
    
    return num;
}