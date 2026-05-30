#include "journal.h"

/* ── Very simple djb2 hash (good enough for a local journal) ── */
void hash_password(const char *pass, char *out) {
    unsigned long hash = 5381;
    int c;
    while ((c = (unsigned char)*pass++))
        hash = ((hash << 5) + hash) + c;
    snprintf(out, 128, "%lu", hash);
}

void load_config(Config *cfg) {
    memset(cfg, 0, sizeof(*cfg));
    FILE *f = fopen(CONFIG_FILE, "rb");
    if (!f) return;
    fread(cfg, sizeof(*cfg), 1, f);
    fclose(f);
}

void save_config(const Config *cfg) {
    FILE *f = fopen(CONFIG_FILE, "wb");
    if (!f) return;
    fwrite(cfg, sizeof(*cfg), 1, f);
    fclose(f);
}

/* ── Read a password without echoing characters ── */
static void read_password(char *buf, int maxlen) {
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    if (fgets(buf, maxlen, stdin)) {
        char *nl = strchr(buf, '\n');
        if (nl) *nl = '\0';
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    printf("\n");
}

int setup_password(Config *cfg) {
    clear_screen();
    print_box("  FIRST-TIME SETUP  ", 50);
    printf("\n");

    printf("  " BWHT "Your name: " RST);
    if (!fgets(cfg->author, sizeof(cfg->author), stdin)) return 0;
    char *nl = strchr(cfg->author, '\n');
    if (nl) *nl = '\0';

    printf("\n  " BWHT "Set a password (leave blank for no lock): " RST);
    char pass[MAX_PASS];
    read_password(pass, sizeof(pass));

    if (strlen(pass) > 0) {
        char pass2[MAX_PASS];
        printf("  " BWHT "Confirm password: " RST);
        read_password(pass2, sizeof(pass2));

        if (strcmp(pass, pass2) != 0) {
            printf(BRED "  Passwords do not match.\n" RST);
            return 0;
        }
        hash_password(pass, cfg->pass_hash);
        cfg->locked = 1;
        printf(BGRN "  ✓ Password set.\n" RST);
    } else {
        cfg->locked = 0;
        cfg->pass_hash[0] = '\0';
        printf(DIM "  No password set.\n" RST);
    }

    save_config(cfg);
    printf("\n  " BGRN "Welcome, %s! Your journal is ready.\n" RST, cfg->author);
    press_any_key();
    return 1;
}

int check_password(const Config *cfg) {
    if (!cfg->locked) return 1;

    clear_screen();
    print_box("  JOURNAL LOCKED  ", 50);
    printf("\n  " BOLD "🔒 Enter password: " RST);

    char pass[MAX_PASS];
    read_password(pass, sizeof(pass));

    char hashed[128];
    hash_password(pass, hashed);

    if (strcmp(hashed, cfg->pass_hash) == 0) {
        printf(BGRN "  ✓ Unlocked.\n" RST);
        usleep(600000);
        return 1;
    } else {
        printf(BRED "  ✗ Wrong password.\n" RST);
        usleep(1000000);
        return 0;
    }
}