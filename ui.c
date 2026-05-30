#include "journal.h"

#define UI_WIDTH 60   

void clear_screen(void) {
    printf("\033[2J\033[H");
    fflush(stdout);
}

int get_terminal_width(void) {
    return UI_WIDTH;
}

void print_separator(int width) {
    printf(DIM CYN);
    for (int i = 0; i < width; i++) printf("─");
    printf(RST "\n");
}


void print_box(const char *title, int width) {
    int inner = width - 2;
    int tlen = strlen(title);

    if (tlen > inner) tlen = inner;

    int pad_left = (inner - tlen) / 2;
    int pad_right = inner - tlen - pad_left;

    printf(BOLD CYN "╔");
    for (int i = 0; i < inner; i++) printf("═");
    printf("╗\n");

    printf("║" RST);
    for (int i = 0; i < pad_left; i++) printf(" ");
    printf(BOLD BWHT "%.*s" RST, tlen, title);
    for (int i = 0; i < pad_right; i++) printf(" ");
    printf(BOLD CYN "║\n");

    printf("╚");
    for (int i = 0; i < inner; i++) printf("═");
    printf("╝" RST "\n");
}

void print_banner(void) {
    clear_screen();

    printf("\n");
    printf(BOLD MAG "  ╔════════════════════════════════════════════════════╗\n");
    printf(        "  ║                                                    ║\n");
    printf(        "  ║" BMAG "        ✦  T E R M I N A L   J O U R N A L  ✦       " MAG  "║\n");
    printf(        "  ║                                                    ║\n");
    printf(        "  ╚════════════════════════════════════════════════════╝" RST "\n\n");
}

void print_menu(void) {
    int width = 50;
    int inner = width - 2; // 48

    const char *items[] = {
        "1 . Write new entry",
        "2 . Browse entries",
        "3 . Search entries",
        "4 . Stats & mood history",
        "5 . Random past entry",
        "6 . On this day",
        "7 . Export all entries",
        "0 . Quit"
    };
    int item_count = 8;

    printf("  " BOLD CYN "┌");
    for (int i = 0; i < inner; i++) printf("─");
    printf("┐\n");

    const char *title = "MAIN MENU";
    int tlen = (int)strlen(title);
    int pad_left = (inner - tlen) / 2;
    int pad_right = inner - tlen - pad_left;

    printf("  " BOLD CYN "│" RST);
    for (int i = 0; i < pad_left; i++) printf(" ");
    printf(BOLD BWHT "%s" RST, title);
    for (int i = 0; i < pad_right; i++) printf(" ");
    printf(BOLD CYN "│\n");

    printf("  " BOLD CYN "├");
    for (int i = 0; i < inner; i++) printf("─");
    printf("┤\n");

    for (int i = 0; i < item_count; i++) {
    
        int len = (int)strlen(items[i]);

        int spaces = (width - 3) - len; 

        printf("  " BOLD CYN "│ " RST); 
        printf("%s", items[i]);
        for (int j = 0; j < spaces; j++) printf(" ");
        printf(BOLD CYN "│\n" RST);
    }

    printf("  " BOLD CYN "└");
    for (int i = 0; i < inner; i++) printf("─");
    printf("┘\n" RST);

    printf("\n  " BWHT "→ " RST);
}

/* typing animation */
void animate_type(const char *text, int delay_us) {
    for (int i = 0; text[i]; i++) {
        putchar(text[i]);
        fflush(stdout);
        usleep(delay_us);
    }
}

void press_any_key(void) {
    printf(DIM "\n  [press enter to continue]" RST);
    fflush(stdout);
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

char getch_raw(void) {
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    char ch = (char)getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}