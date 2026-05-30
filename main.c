#include "journal.h"

/* declared in entry.c but not in header (internal to module) */
int export_entries(void);

int main(void) {
    /* ── Ensure journal directory exists ── */
    mkdir(JOURNAL_DIR, 0700);

    /* ── Load or create config ── */
    Config cfg;
    load_config(&cfg);

    /* First run — no author set */
    if (cfg.author[0] == '\0') {
        if (!setup_password(&cfg)) return 1;
    }

    /* ── Password check ── */
    if (cfg.locked) {
        int tries = 3;
        while (tries-- > 0) {
            if (check_password(&cfg)) break;
            if (tries == 0) {
                printf(BRED "  Too many failed attempts. Goodbye.\n" RST);
                return 1;
            }
            printf(BRED "  %d attempt%s left.\n" RST, tries, tries == 1 ? "" : "s");
        }
    }

    /* ── Load stats ── */
    Stats stats;
    load_stats(&stats);

    /* ── Main loop ── */
    while (1) {
        print_banner();

        /* Greeting with streak */
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        char date_str[64];
        strftime(date_str, sizeof(date_str), "%A, %d %B %Y", t);

        printf("  " BWHT "Hello, %s!" RST "  " DIM "%s\n" RST, cfg.author, date_str);

        if (stats.streak >= 1)
            printf("  " BYLW "🔥 %d-day streak" RST "  " DIM "·" RST
                   "  " BWHT "%d" RST DIM " entries" RST
                   "  " DIM "·" RST
                   "  " BWHT "%d" RST DIM " words total\n" RST,
                   stats.streak, stats.total_entries, stats.total_words);
        else
            printf(DIM "  Start writing to build your streak!\n" RST);

        printf("\n");
        print_menu();

        char buf[8];
        if (!fgets(buf, sizeof(buf), stdin)) break;
        int choice = atoi(buf);

        switch (choice) {
            case 1: write_entry(&stats, &cfg);  break;
            case 2: list_entries(NULL);          break;
            case 3:
                clear_screen();
                print_box("  SEARCH  ", 50);
                printf("\n  " BWHT "Keyword: " RST);
                search_entries(NULL);
                break;
            case 4: show_stats(&stats);          break;
            case 5: random_entry();              break;
            case 6: on_this_day();               break;
            case 7: export_entries();            break;
            case 0:
                clear_screen();
                animate_type("  Goodbye. Keep writing. ✨\n\n", 25000);
                return 0;
            default:
                printf(DIM "  Unknown option.\n" RST);
                usleep(500000);
        }
    }
    return 0;
}
