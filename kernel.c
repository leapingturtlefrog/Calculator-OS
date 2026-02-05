// Calculator OS with I Ching, Asciimojis, and Lasagna

#define VGA_MEMORY 0xB8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_COLOR(fg, bg) ((bg << 4) | fg)
#define WHITE_ON_BLACK VGA_COLOR(15, 0)
#define GREEN_ON_BLACK VGA_COLOR(2, 0)
#define YELLOW_ON_BLACK VGA_COLOR(14, 0)

unsigned short cursor_pos = 0;
char input_buffer[256];
unsigned char input_length = 0;
unsigned char input_position = 0;
int shift_pressed = 0;
unsigned int rand_seed = 12345;

unsigned char inb(unsigned short port) {
    unsigned char result;
    __asm__ volatile("inb %1, %0" : "=a" (result) : "dN" (port));
    return result;
}

void outb(unsigned short port, unsigned char value) {
    __asm__ volatile("outb %0, %1" : : "a" (value), "dN" (port));
}

void update_cursor() {
    outb(0x3D4, 14);
    outb(0x3D5, (cursor_pos >> 8) & 0xFF);
    outb(0x3D4, 15);
    outb(0x3D5, cursor_pos & 0xFF);
}

void print_char(char c, unsigned char color, int x, int y) {
    unsigned char* vm = (unsigned char*)VGA_MEMORY;
    int offset = (y * VGA_WIDTH + x) * 2;
    vm[offset] = c;
    vm[offset + 1] = color;
}

void clear_screen() {
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
        if (*str == '\n') {
            cursor_pos += VGA_WIDTH - (cursor_pos % VGA_WIDTH);
        } else {
            print_char(*str, color, cursor_pos % VGA_WIDTH, cursor_pos / VGA_WIDTH);
            cursor_pos++;
        }
        str++;
    }
}

void print_line(const char* str, unsigned char color) {
    print_string(str, color);
    cursor_pos += VGA_WIDTH - (cursor_pos % VGA_WIDTH);
}

void print_number(int num) {
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

void print_result(int num) {
    print_number(num);
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

unsigned int simple_rand() {
    rand_seed = rand_seed * 1103515245 + 12345;
    return (rand_seed >> 16) & 0x7FFF;
}

void show_iching() {
    const char* hexagrams[] = {
        "== ==  Heaven - Great success awaits",
        "==    Earth - Be receptive and patient", 
        "= = = Water - Danger, but persevere",
        "===== Fire - Clarity and awareness",
        "= === Thunder - Shock brings renewal",
        "=== = Wind - Gentle persistence wins",
        "== == Mountain - Keep still, meditate",
        "= = == Lake - Joy through sharing"
    };
    int idx = simple_rand() % 8;
    print_line(hexagrams[idx], YELLOW_ON_BLACK);
}

void show_asciimoji() {
    const char* mojis[] = {
        "(^_^)  - Happy!",
        "(T_T)  - Sad...",
        "(o_O)  - Surprised!",
        "(*_*)  - Amazed!",
        "(>_<)  - Frustrated!",
        "(@_@)  - Dizzy!",
        "(^o^)  - Excited!",
        "(-_-)  - Sleepy..."
    };
    int idx = simple_rand() % 8;
    print_line(mojis[idx], GREEN_ON_BLACK);
}

void show_lasagna() {
    print_line("      ~~~~~~~~~~~~~~~~~~~~", YELLOW_ON_BLACK);
    print_line("     /                    \\", YELLOW_ON_BLACK);
    print_line("    | ==================== |  <- cheese", YELLOW_ON_BLACK);
    print_line("    | -------------------- |  <- meat", YELLOW_ON_BLACK);
    print_line("    | ==================== |  <- pasta", YELLOW_ON_BLACK);
    print_line("    | -------------------- |  <- sauce", YELLOW_ON_BLACK);
    print_line("    | ==================== |  <- more cheese!", YELLOW_ON_BLACK);
    print_line("     \\____________________/", YELLOW_ON_BLACK);
    print_line("         LASAGNA TIME!", GREEN_ON_BLACK);
}

char get_key() {
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
        if (sc & 0x80) continue;
        if (sc < 60) {
            rand_seed ^= sc * 31337;
            return shift_pressed ? shifted[sc] : normal[sc];
        }
    }
}

int parse_number() {
    int num = 0;
    while (input_position < input_length && input_buffer[input_position] == ' ') input_position++;
    while (input_position < input_length) {
        char c = input_buffer[input_position];
        if (c >= '0' && c <= '9') {
            num = num * 10 + (c - '0');
        } else break;
        input_position++;
    }
    while (input_position < input_length && input_buffer[input_position] == ' ') input_position++;
    return num;
}

int calculate() {
    input_position = 0;
    int n1 = parse_number();
    if (input_position >= input_length) return n1;
    char op = input_buffer[input_position++];
    int n2 = parse_number();
    switch (op) {
        case '+': return n1 + n2;
        case '-': return n1 - n2;
        case '*': return n1 * n2;
        case '/': return n2 != 0 ? n1 / n2 : 0;
        default: return 0;
    }
}

void __attribute__((section(".text.start"))) kernel_main(void) {
    clear_screen();
    print_line("Calculator OS v2.0", GREEN_ON_BLACK);
    print_line("Commands: iching, moji, lasagna, or math (e.g. 5+3)", WHITE_ON_BLACK);
    print_line("Enter=run, ESC=clear, Backspace=delete", WHITE_ON_BLACK);
    print_string("> ", WHITE_ON_BLACK);
    update_cursor();
    
    input_length = 0;
    input_buffer[0] = '\0';
    
    while (1) {
        char key = get_key();
        
        if (key == '\n') {
            cursor_pos += VGA_WIDTH - (cursor_pos % VGA_WIDTH);
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
                    print_string("= ", WHITE_ON_BLACK);
                    print_result(calculate());
                    cursor_pos += VGA_WIDTH - (cursor_pos % VGA_WIDTH);
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
        } else if (key >= 32 && key <= 126 && input_length < 255) {
            input_buffer[input_length++] = key;
            print_char(key, WHITE_ON_BLACK, cursor_pos % VGA_WIDTH, cursor_pos / VGA_WIDTH);
            cursor_pos++;
            update_cursor();
        }
    }
}
