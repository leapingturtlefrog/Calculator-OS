// Calculator OS v0.2
// Supports Level 1 & 2 math operations

#include "math.h"
#include "extras.h"

#define VGA_MEMORY 0xB8000
#define SERIAL_PORT 0x3F8
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_COLOR(fg, bg) ((bg << 4) | fg)
#define WHITE_ON_BLACK VGA_COLOR(15, 0)
#define GREEN_ON_BLACK VGA_COLOR(2, 0)
#define YELLOW_ON_BLACK VGA_COLOR(14, 0)

// Scroll buffer configuration
#define HEADER_LINES 4
#define CONTENT_LINES (VGA_HEIGHT - HEADER_LINES)
#define SCROLL_BUFFER_LINES 200

// Scroll buffer: stores lines of text with colors
char scroll_buffer[SCROLL_BUFFER_LINES][VGA_WIDTH];
unsigned char scroll_colors[SCROLL_BUFFER_LINES][VGA_WIDTH];
int buffer_write_line = 0;   // Next line to write in buffer
int buffer_total_lines = 0;  // Total lines written
int scroll_offset = 0;       // How many lines scrolled back (0 = latest)

unsigned short cursor_pos = 0;
char input_buffer[256];
unsigned char input_length = 0;
int shift_pressed = 0;
unsigned int rand_seed = 12345;

// I/O functions
unsigned char inb(unsigned short port) {
    unsigned char result;
    __asm__ volatile("inb %1, %0" : "=a" (result) : "dN" (port));
    return result;
}

void outb(unsigned short port, unsigned char value) {
    __asm__ volatile("outb %0, %1" : : "a" (value), "dN" (port));
}

// Serial port functions for debugging
void serial_init(void) {
    outb(SERIAL_PORT + 1, 0x00);  // Disable interrupts
    outb(SERIAL_PORT + 3, 0x80);  // Enable DLAB
    outb(SERIAL_PORT + 0, 0x03);  // Divisor low byte (38400 baud)
    outb(SERIAL_PORT + 1, 0x00);  // Divisor high byte
    outb(SERIAL_PORT + 3, 0x03);  // 8 bits, no parity, 1 stop
    outb(SERIAL_PORT + 2, 0xC7);  // Enable FIFO
    outb(SERIAL_PORT + 4, 0x0B);  // IRQs enabled, RTS/DSR set
}

void serial_putc(char c) {
    while ((inb(SERIAL_PORT + 5) & 0x20) == 0);  // Wait for empty transmit
    outb(SERIAL_PORT, c);
}

void serial_puts(const char* s) {
    while (*s) {
        if (*s == '\n') serial_putc('\r');
        serial_putc(*s++);
    }
}

void serial_putint(int num) {
    if (num < 0) { serial_putc('-'); num = -num; }
    if (num == 0) { serial_putc('0'); return; }
    char buf[12];
    int i = 0;
    while (num > 0) { buf[i++] = '0' + (num % 10); num /= 10; }
    while (i > 0) serial_putc(buf[--i]);
}

void serial_putdouble(double num) {
    if (num < 0) { serial_putc('-'); num = -num; }
    // Scale to integer to avoid float-to-int conversion issues
    // Multiply by 10000 for 4 decimal places
    int scaled = (int)(num * 10000.0 + 0.5);  // with rounding
    int int_part = scaled / 10000;
    int frac_part = scaled % 10000;
    
    serial_putint(int_part);
    serial_putc('.');
    
    // Print fractional part with leading zeros
    if (frac_part < 1000) serial_putc('0');
    if (frac_part < 100) serial_putc('0');
    if (frac_part < 10) serial_putc('0');
    if (frac_part > 0) serial_putint(frac_part);
    else serial_putc('0');
}

// Initialize FPU with proper control word
void init_fpu(void) {
    unsigned short cw = 0x037F;  // Default FPU control word: all exceptions masked
    __asm__ volatile(
        "fninit\n"
        "fldcw %0\n"
        : : "m" (cw)
    );
}

void update_cursor(void) {
    // Only show cursor if not scrolled back
    if (scroll_offset == 0) {
        outb(0x3D4, 14);
        outb(0x3D5, (cursor_pos >> 8) & 0xFF);
        outb(0x3D4, 15);
        outb(0x3D5, cursor_pos & 0xFF);
    } else {
        // Hide cursor when scrolled back
        outb(0x3D4, 14);
        outb(0x3D5, 0xFF);
        outb(0x3D4, 15);
        outb(0x3D5, 0xFF);
    }
}

// Initialize scroll buffer
void init_scroll_buffer(void) {
    for (int i = 0; i < SCROLL_BUFFER_LINES; i++) {
        for (int j = 0; j < VGA_WIDTH; j++) {
            scroll_buffer[i][j] = ' ';
            scroll_colors[i][j] = WHITE_ON_BLACK;
        }
    }
    buffer_write_line = 0;
    buffer_total_lines = 0;
    scroll_offset = 0;
}

// Get buffer line index (handles circular buffer)
int get_buffer_line(int offset_from_latest) {
    if (buffer_total_lines <= SCROLL_BUFFER_LINES) {
        return buffer_total_lines - 1 - offset_from_latest;
    }
    int idx = buffer_write_line - 1 - offset_from_latest;
    if (idx < 0) idx += SCROLL_BUFFER_LINES;
    return idx;
}

// Redraw the content area from scroll buffer
void redraw_content(void) {
    unsigned char* vm = (unsigned char*)VGA_MEMORY;
    
    for (int y = 0; y < CONTENT_LINES; y++) {
        int screen_y = HEADER_LINES + y;
        int line_offset = scroll_offset + (CONTENT_LINES - 1 - y);
        
        if (line_offset < buffer_total_lines) {
            int buf_idx = get_buffer_line(line_offset);
            for (int x = 0; x < VGA_WIDTH; x++) {
                int offset = (screen_y * VGA_WIDTH + x) * 2;
                vm[offset] = scroll_buffer[buf_idx][x];
                vm[offset + 1] = scroll_colors[buf_idx][x];
            }
        } else {
            // Clear line if no content
            for (int x = 0; x < VGA_WIDTH; x++) {
                int offset = (screen_y * VGA_WIDTH + x) * 2;
                vm[offset] = ' ';
                vm[offset + 1] = WHITE_ON_BLACK;
            }
        }
    }
}

// Add a new line to the scroll buffer
void buffer_newline(void) {
    buffer_write_line = (buffer_write_line + 1) % SCROLL_BUFFER_LINES;
    buffer_total_lines++;
    
    // Clear the new line
    for (int x = 0; x < VGA_WIDTH; x++) {
        scroll_buffer[buffer_write_line][x] = ' ';
        scroll_colors[buffer_write_line][x] = WHITE_ON_BLACK;
    }
}

// Write a character to the current buffer line
void buffer_putchar(int x, char c, unsigned char color) {
    if (buffer_total_lines == 0) {
        buffer_total_lines = 1;
    }
    int buf_idx = (buffer_write_line) % SCROLL_BUFFER_LINES;
    if (x >= 0 && x < VGA_WIDTH) {
        scroll_buffer[buf_idx][x] = c;
        scroll_colors[buf_idx][x] = color;
    }
}

// Scroll up the content area (when we reach bottom)
void scroll_content_up(void) {
    buffer_newline();
    redraw_content();
}

// Scroll view up (Page Up) - shows older content
void scroll_view_up(void) {
    int max_scroll = buffer_total_lines - CONTENT_LINES;
    if (max_scroll < 0) max_scroll = 0;
    if (scroll_offset < max_scroll) {
        scroll_offset += (CONTENT_LINES / 2);  // Scroll half page
        if (scroll_offset > max_scroll) scroll_offset = max_scroll;
        redraw_content();
        update_cursor();
    }
}

// Scroll view down (Page Down) - shows newer content
void scroll_view_down(void) {
    if (scroll_offset > 0) {
        scroll_offset -= (CONTENT_LINES / 2);  // Scroll half page
        if (scroll_offset < 0) scroll_offset = 0;
        redraw_content();
        update_cursor();
    }
}

void print_char(char c, unsigned char color, int x, int y) {
    unsigned char* vm = (unsigned char*)VGA_MEMORY;
    int offset = (y * VGA_WIDTH + x) * 2;
    vm[offset] = c;
    vm[offset + 1] = color;
    
    // Also store in scroll buffer if in content area
    if (y >= HEADER_LINES && scroll_offset == 0) {
        buffer_putchar(x, c, color);
    }
}

void clear_screen(void) {
    unsigned char* vm = (unsigned char*)VGA_MEMORY;
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT * 2; i += 2) {
        vm[i] = ' ';
        vm[i + 1] = WHITE_ON_BLACK;
    }
    cursor_pos = 0;
    update_cursor();
}

void print_string(const char* str, unsigned char color) {
    while (*str) {
        int y = cursor_pos / VGA_WIDTH;
        int x = cursor_pos % VGA_WIDTH;
        
        // Check if we need to scroll
        if (y >= VGA_HEIGHT) {
            scroll_content_up();
            cursor_pos = (VGA_HEIGHT - 1) * VGA_WIDTH;
            y = VGA_HEIGHT - 1;
            x = 0;
        }
        
        if (*str == '\n') {
            cursor_pos += VGA_WIDTH - (cursor_pos % VGA_WIDTH);
        } else {
            print_char(*str, color, x, y);
            cursor_pos++;
        }
        str++;
    }
}

void print_line(const char* str, unsigned char color) {
    print_string(str, color);
    
    // Add current line to buffer before moving to next
    if (cursor_pos / VGA_WIDTH >= HEADER_LINES && scroll_offset == 0) {
        buffer_newline();
    }
    
    // Move to next line
    cursor_pos += VGA_WIDTH - (cursor_pos % VGA_WIDTH);
    
    // Check if we need to scroll
    if (cursor_pos / VGA_WIDTH >= VGA_HEIGHT) {
        scroll_content_up();
        cursor_pos = (VGA_HEIGHT - 1) * VGA_WIDTH;
    }
}

void print_int(int num) {
    if (num < 0) {
        print_char('-', WHITE_ON_BLACK, cursor_pos % VGA_WIDTH, cursor_pos / VGA_WIDTH);
        cursor_pos++;
        num = -num;
    }
    if (num == 0) {
        print_char('0', WHITE_ON_BLACK, cursor_pos % VGA_WIDTH, cursor_pos / VGA_WIDTH);
        cursor_pos++;
        return;
    }
    char buffer[12];
    int i = 0;
    while (num > 0) {
        buffer[i++] = '0' + (num % 10);
        num /= 10;
    }
    while (i > 0) {
        print_char(buffer[--i], WHITE_ON_BLACK, cursor_pos % VGA_WIDTH, cursor_pos / VGA_WIDTH);
        cursor_pos++;
    }
}

void print_float(double num) {
    // Handle negative
    if (num < 0) {
        print_char('-', WHITE_ON_BLACK, cursor_pos % VGA_WIDTH, cursor_pos / VGA_WIDTH);
        cursor_pos++;
        num = -num;
    }
    
    // Print integer part
    long int_part = (long)num;
    if (int_part == 0) {
        print_char('0', WHITE_ON_BLACK, cursor_pos % VGA_WIDTH, cursor_pos / VGA_WIDTH);
        cursor_pos++;
    } else {
        char buffer[20];
        int i = 0;
        long temp = int_part;
        while (temp > 0) {
            buffer[i++] = '0' + (temp % 10);
            temp /= 10;
        }
        while (i > 0) {
            print_char(buffer[--i], WHITE_ON_BLACK, cursor_pos % VGA_WIDTH, cursor_pos / VGA_WIDTH);
            cursor_pos++;
        }
    }
    
    // Print decimal part (4 digits)
    double frac = num - int_part;
    if (frac > 0.00005) {
        print_char('.', WHITE_ON_BLACK, cursor_pos % VGA_WIDTH, cursor_pos / VGA_WIDTH);
        cursor_pos++;
        
        for (int i = 0; i < 4; i++) {
            frac *= 10;
            int digit = (int)frac;
            print_char('0' + digit, WHITE_ON_BLACK, cursor_pos % VGA_WIDTH, cursor_pos / VGA_WIDTH);
            cursor_pos++;
            frac -= digit;
        }
    }
}

int str_eq(const char* a, const char* b) {
    while (*a && *b) {
        char ca = *a, cb = *b;
        if (ca >= 'A' && ca <= 'Z') ca += 32;
        if (cb >= 'A' && cb <= 'Z') cb += 32;
        if (ca != cb) return 0;
        a++; b++;
    }
    return *a == *b;
}

unsigned int simple_rand(void) {
    rand_seed = rand_seed * 1103515245 + 12345;
    return (rand_seed >> 16) & 0x7FFF;
}

// Special key codes returned by get_key
#define KEY_PAGE_UP 128
#define KEY_PAGE_DOWN 129

char get_key(void) {
    static unsigned char normal[] = {
        0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
        '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
        0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,
        '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '
    };
    static unsigned char shifted[] = {
        0, 27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
        '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
        0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0,
        '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' '
    };
    
    while (1) {
        if ((inb(0x64) & 1) == 0) continue;
        unsigned char sc = inb(0x60);
        if (sc == 0x2A || sc == 0x36) { shift_pressed = 1; continue; }
        if (sc == 0xAA || sc == 0xB6) { shift_pressed = 0; continue; }
        if (sc & 0x80) continue;  // Key release
        
        // Page Up (scancode 0x49)
        if (sc == 0x49) {
            rand_seed ^= sc * 31337;
            return KEY_PAGE_UP;
        }
        // Page Down (scancode 0x51)
        if (sc == 0x51) {
            rand_seed ^= sc * 31337;
            return KEY_PAGE_DOWN;
        }
        
        if (sc < 60) {
            rand_seed ^= sc * 31337;
            return shift_pressed ? shifted[sc] : normal[sc];
        }
    }
}

void __attribute__((section(".text.start"))) kernel_main(void) {
    // Initialize serial port for debugging
    serial_init();
    serial_puts("\n[DEBUG] Calculator OS v0.2 starting...\n");
    
    // Initialize FPU for floating point support
    init_fpu();
    serial_puts("[DEBUG] FPU initialized\n");
    
    // Quick FPU test
    serial_puts("[DEBUG] Testing FPU: 2.5 + 3.5 = ");
    volatile double a = 2.5;
    volatile double b = 3.5;
    volatile double c = a + b;
    serial_putdouble(c);
    serial_puts("\n");
    serial_puts("[DEBUG] FPU test passed!\n");
    
    // Initialize scroll buffer
    init_scroll_buffer();
    
    clear_screen();
    // Print fixed header (lines 0-3)
    print_line("Calculator OS v0.2", GREEN_ON_BLACK);
    print_line("Math: +, -, *, /, %, ^, (), sqrt(), abs(), root(n,x)", WHITE_ON_BLACK);
    print_line("Extras: iching, moji, lasagna | PgUp/PgDn: scroll", WHITE_ON_BLACK);
    print_line("Enter=run, ESC=clear, Backspace=delete", WHITE_ON_BLACK);
    
    // Start content area at line 4
    cursor_pos = HEADER_LINES * VGA_WIDTH;
    print_string("> ", WHITE_ON_BLACK);
    update_cursor();
    
    input_length = 0;
    input_buffer[0] = '\0';
    
    while (1) {
        char key = get_key();
        
        if (key == '\n') {
            // Return to current if scrolled back
            if (scroll_offset > 0) {
                scroll_offset = 0;
                redraw_content();
            }
            
            // Move to next line, scroll if needed
            cursor_pos += VGA_WIDTH - (cursor_pos % VGA_WIDTH);
            if (cursor_pos / VGA_WIDTH >= VGA_HEIGHT) {
                scroll_content_up();
                cursor_pos = (VGA_HEIGHT - 1) * VGA_WIDTH;
            }
            update_cursor();
            
            if (input_length > 0) {
                input_buffer[input_length] = '\0';
                
                if (str_eq(input_buffer, "iching")) {
                    show_iching();
                } else if (str_eq(input_buffer, "moji")) {
                    show_asciimoji();
                } else if (str_eq(input_buffer, "lasagna")) {
                    show_lasagna();
                } else {
                    serial_puts("[DEBUG] Evaluating: ");
                    serial_puts(input_buffer);
                    serial_puts("\n");
                    
                    print_string("= ", WHITE_ON_BLACK);
                    serial_puts("[DEBUG] Calling evaluate()...\n");
                    
                    double result = evaluate(input_buffer, input_length);
                    
                    serial_puts("[DEBUG] evaluate() returned: ");
                    serial_putdouble(result);
                    serial_puts("\n");
                    
                    serial_puts("[DEBUG] Calling print_float()...\n");
                    print_float(result);
                    serial_puts("[DEBUG] print_float() done\n");
                    
                    // Move to next line, scroll if needed
                    cursor_pos += VGA_WIDTH - (cursor_pos % VGA_WIDTH);
                    if (cursor_pos / VGA_WIDTH >= VGA_HEIGHT) {
                        scroll_content_up();
                        cursor_pos = (VGA_HEIGHT - 1) * VGA_WIDTH;
                    }
                }
                
                // Print prompt, scroll if needed
                if (cursor_pos / VGA_WIDTH >= VGA_HEIGHT) {
                    scroll_content_up();
                    cursor_pos = (VGA_HEIGHT - 1) * VGA_WIDTH;
                }
                print_string("> ", WHITE_ON_BLACK);
                update_cursor();
                input_length = 0;
                input_buffer[0] = '\0';
            }
        } else if (key == '\b' && input_length > 0) {
            input_length--;
            cursor_pos--;
            print_char(' ', WHITE_ON_BLACK, cursor_pos % VGA_WIDTH, cursor_pos / VGA_WIDTH);
            update_cursor();
        } else if (key == 27) {
            while (input_length > 0) {
                input_length--;
                cursor_pos--;
                print_char(' ', WHITE_ON_BLACK, cursor_pos % VGA_WIDTH, cursor_pos / VGA_WIDTH);
            }
            update_cursor();
        } else if (key == KEY_PAGE_UP) {
            scroll_view_up();
        } else if (key == KEY_PAGE_DOWN) {
            scroll_view_down();
        } else if (key >= 32 && key <= 126 && input_length < 255) {
            // If scrolled back, return to current before typing
            if (scroll_offset > 0) {
                scroll_offset = 0;
                redraw_content();
            }
            input_buffer[input_length++] = key;
            
            // Check if we need to scroll before printing
            int y = cursor_pos / VGA_WIDTH;
            if (y >= VGA_HEIGHT) {
                scroll_content_up();
                cursor_pos = (VGA_HEIGHT - 1) * VGA_WIDTH;
            }
            
            print_char(key, WHITE_ON_BLACK, cursor_pos % VGA_WIDTH, cursor_pos / VGA_WIDTH);
            cursor_pos++;
            update_cursor();
        }
    }
}
