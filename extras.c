// Extras module for Calculator OS
// I Ching, Asciimojis, and Lasagna

#include "extras.h"

void show_iching(void) {
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

void show_asciimoji(void) {
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

void show_lasagna(void) {
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
