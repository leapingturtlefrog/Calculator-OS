#ifndef EXTRAS_H
#define EXTRAS_H

// Display functions need to be provided by kernel
extern void print_line(const char* str, unsigned char color);
extern unsigned int simple_rand(void);

// Color definitions
#define YELLOW_ON_BLACK 0x0E
#define GREEN_ON_BLACK 0x02

// Show I Ching fortune
void show_iching(void);

// Show random asciimoji
void show_asciimoji(void);

// Show lasagna art
void show_lasagna(void);

#endif
