#include "journal.h"

/* ── Collect all .jrn filenames, sorted by name (= chronological) ── */
static int collect_entries(char filenames[][MAX_PATH], int max) {
    DIR *dir = opendir(JOURNAL_DIR);
    if (!dir) return 0;

    int count = 0;
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL && count < max) {
        if (strstr(ent->d_name, ".jrn"))
            strncpy(filenames[count++], ent->d_name, MAX_PATH - 1);
    }
    closedir(dir);

    /* bubble sort by filename (timestamps → chronological) */
    for (int i = 0; i < count - 1; i++)
        for (int j = i + 1; j < count; j++)
            if (strcmp(filenames[i], filenames[j]) > 0) {
                char tmp[MAX_PATH];
                strncpy(tmp, filenames[i], MAX_PATH - 1);
                strncpy(filenames[i], filenames[j], MAX_PATH - 1);
                strncpy(filenames[j], tmp, MAX_PATH - 1);
            }
    return count;
}

/* ── Write a new entry ── */
int write_entry(Stats *stats, Config *cfg) {
    (void)cfg;
    clear_screen();
    print_box("  NEW ENTRY  ", 58);
    printf("\n");

    Entry e;
    memset(&e, 0, sizeof(e));
   printf("  " BWHT "Use current date/time? (Y/n): " RST);

char choice[8];
fgets(choice, sizeof(choice), stdin);

if (choice[0] == 'n' || choice[0] == 'N') {
    printf("  Enter date (YYYY-MM-DD HH:MM): ");
    char input[64];
    fgets(input, sizeof(input), stdin);

    char *nl = strchr(input, '\n');
    if (nl) *nl = '\0';

    time_t custom = parse_datetime(input);

    if (custom == (time_t)-1) {
        printf(BRED "  Invalid format. Using current time.\n" RST);
        e.timestamp = time(NULL);
    } else {
        e.timestamp = custom;
    }
} else {
    e.timestamp = time(NULL);
}

format_timestamp(e.timestamp, e.date_str);

    printf("  " DIM "%s\n" RST "\n", e.date_str);

    /* Title */
    printf("  " BOLD "Title" RST " (or press Enter to skip): ");
    fgets(e.title, sizeof(e.title), stdin);
    char *nl = strchr(e.title, '\n');
    if (nl) *nl = '\0';

    /* Mood */
    e.mood = pick_mood();

    /* Body */
    printf("\n  " BOLD "Your entry:\n" RST);
    read_multiline(e.body, sizeof(e.body));

    if (strlen(e.body) == 0) {
        printf(YLW "  Nothing written. Entry discarded.\n" RST);
        press_any_key();
        return 0;
    }

    /* Word count */
    e.word_count = count_words(e.body);

    /* Filename: YYYY-MM-DD_HHMMSS.jrn */
    struct tm *tm_info = localtime(&e.timestamp);
    char fname[64];
    strftime(fname, sizeof(fname), "%Y-%m-%d_%H%M%S.jrn", tm_info);
    strncpy(e.filename, fname, sizeof(e.filename) - 1);

    char path[MAX_PATH];
    get_entry_path(fname, path);

    FILE *f = fopen(path, "wb");
    if (!f) {
        printf(BRED "  Error saving entry.\n" RST);
        press_any_key();
        return 0;
    }
    fwrite(&e, sizeof(e), 1, f);
    fclose(f);

    /* Update stats */
    stats->total_entries++;
    stats->total_words += e.word_count;
    stats->mood_sum    += e.mood;
    stats->mood_count++;
    update_streak(stats);
    save_stats(stats);

    char mood_label[32];
    char mood_emoji[16];
    get_mood_str(e.mood, mood_label, 1);
    get_mood_emoji(e.mood, mood_emoji);

    printf("\n");
    print_separator(58);
    printf("  " BGRN "✓ Entry saved!" RST
           "  Words: " BWHT "%d" RST
           "  Mood: %s %s\n",
           e.word_count, mood_emoji, mood_label);
    printf("  🔥 Streak: " BYLW "%d day%s\n" RST,
           stats->streak, stats->streak == 1 ? "" : "s");
    press_any_key();
    return 1;
}

/* ── List entries with index ── */
int list_entries(int *count_out) {
    static char filenames[MAX_ENTRIES][MAX_PATH];
    int count = collect_entries(filenames, MAX_ENTRIES);
    if (count_out) *count_out = count;

    clear_screen();
    print_box("  ALL ENTRIES  ", 58);
    printf("\n");

    if (count == 0) {
        printf(DIM "  No entries yet. Write your first one!\n" RST);
        press_any_key();
        return -1;
    }

    /* show newest first */
    for (int i = count - 1; i >= 0; i--) {
        char path[MAX_PATH];
        get_entry_path(filenames[i], path);
        FILE *f = fopen(path, "rb");
        if (!f) continue;
        Entry e;
        fread(&e, sizeof(e), 1, f);
        fclose(f);

        char emoji[16];
        get_mood_emoji(e.mood, emoji);

        int idx = count - 1 - i; /* display index 0-based */
        printf("  " BOLD CYN "%3d" RST "  " DIM "%-22s" RST "  %s  "
               BWHT "%-30s" RST "  " DIM "%d words" RST "\n",
               idx + 1, e.date_str, emoji,
               strlen(e.title) ? e.title : "(no title)",
               e.word_count);
    }

    printf("\n  " BWHT "Open entry number (0 to go back): " RST);
    char buf[16];
    if (!fgets(buf, sizeof(buf), stdin)) return -1;
    int choice = atoi(buf);
    if (choice < 1 || choice > count) return -1;

    /* Map display index back to sorted filename (newest-first = reverse) */
    int file_idx = count - choice;
    return view_entry(filenames[file_idx]);
}

/* ── View a single entry ── */
int view_entry(const char *filename) {
    char path[MAX_PATH];
    get_entry_path(filename, path);

    FILE *f = fopen(path, "rb");
    if (!f) {
        printf(BRED "  Could not open entry.\n" RST);
        press_any_key();
        return 0;
    }
    Entry e;
    fread(&e, sizeof(e), 1, f);
    fclose(f);

    clear_screen();

    char mood_label[32], mood_emoji[16];
    get_mood_str(e.mood, mood_label, 1);
    get_mood_emoji(e.mood, mood_emoji);

    /* Header */
    printf("\n  " BOLD BWHT "%s\n" RST, strlen(e.title) ? e.title : "(no title)");
    printf("  " DIM "%s" RST "   %s %s   " DIM "%d words\n\n" RST,
           e.date_str, mood_emoji, mood_label, e.word_count);
    print_separator(62);
    printf("\n");

    /* Body — word-wrap at ~60 chars */
    const char *p = e.body;
    int col = 0;
    printf("  ");
    while (*p) {
        if (*p == '\n') {
            printf("\n  ");
            col = 0;
        } else {
            putchar(*p);
            col++;
            if (col >= 62 && *p == ' ') {
                printf("\n  ");
                col = 0;
            }
        }
        p++;
    }
    printf("\n\n");
    print_separator(62);

    /* Actions */
    printf("  " CYN "e" RST " · edit   "
           CYN "d" RST " · delete   "
           CYN "any other key" RST " · back\n  → ");
    fflush(stdout);
    char buf[8];
    if (!fgets(buf, sizeof(buf), stdin)) return 0;

    if (buf[0] == 'e' || buf[0] == 'E') {
        Stats s; load_stats(&s);
        edit_entry(filename, &s);
    } else if (buf[0] == 'd' || buf[0] == 'D') {
        if (confirm("Delete this entry?")) {
            Stats s; load_stats(&s);
            delete_entry(filename, &s);
        }
    }
    return 1;
}

/* ── Edit an existing entry ── */
int edit_entry(const char *filename, Stats *stats) {
    char path[MAX_PATH];
    get_entry_path(filename, path);

    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    Entry e;
    fread(&e, sizeof(e), 1, f);
    fclose(f);

    int old_words = e.word_count;

    clear_screen();
    print_box("  EDIT ENTRY  ", 58);
    printf("\n  " DIM "Current title: %s\n" RST, e.title);
    printf("  " BWHT "New title (Enter to keep): " RST);

    char buf[MAX_TITLE];
    fgets(buf, sizeof(buf), stdin);
    char *nl = strchr(buf, '\n');
    if (nl) *nl = '\0';
    if (strlen(buf) > 0) strncpy(e.title, buf, sizeof(e.title) - 1);

    printf("\n  New mood:\n");
    int new_mood = pick_mood();
    if (new_mood) e.mood = new_mood;

    printf("\n  " BOLD "Rewrite entry body:\n" RST);
    char new_body[MAX_BODY];
    new_body[0] = '\0';
    read_multiline(new_body, sizeof(new_body));

    if (strlen(new_body) > 0) {
        strncpy(e.body, new_body, sizeof(e.body) - 1);
        e.word_count = count_words(e.body);
    }

    f = fopen(path, "wb");
    if (!f) return 0;
    fwrite(&e, sizeof(e), 1, f);
    fclose(f);

    /* fix word count delta in stats */
    stats->total_words += e.word_count - old_words;
    save_stats(stats);

    printf(BGRN "\n  ✓ Entry updated.\n" RST);
    press_any_key();
    return 1;

    printf("\n  " BWHT "Edit date? (y/N): " RST);

char ch[8];
fgets(ch, sizeof(ch), stdin);

if (ch[0] == 'y' || ch[0] == 'Y') {
    printf("  Enter new date (YYYY-MM-DD HH:MM): ");

    char input[64];
    fgets(input, sizeof(input), stdin);

    char *nl = strchr(input, '\n');
    if (nl) *nl = '\0';

    time_t custom = parse_datetime(input);

    if (custom != (time_t)-1) {
        e.timestamp = custom;
        format_timestamp(e.timestamp, e.date_str);
    } else {
        printf(BRED "  Invalid date. Keeping old.\n" RST);
    }
}
}

/* ── Delete an entry ── */
int delete_entry(const char *filename, Stats *stats) {
    char path[MAX_PATH];
    get_entry_path(filename, path);

    /* read word count before deleting */
    FILE *f = fopen(path, "rb");
    if (f) {
        Entry e;
        fread(&e, sizeof(e), 1, f);
        fclose(f);
        stats->total_words   -= e.word_count;
        stats->total_entries -= 1;
        stats->mood_sum      -= e.mood;
        if (stats->mood_count > 0) stats->mood_count--;
        save_stats(stats);
    }

    if (remove(path) == 0)
        printf(BGRN "  ✓ Entry deleted.\n" RST);
    else
        printf(BRED "  Could not delete entry.\n" RST);

    press_any_key();
    return 1;
}

/* ── Search entries by keyword ── */
int search_entries(const char *keyword) {
    char kw[256];
    if (!keyword) {
        printf("  " BWHT "Search: " RST);
        if (!fgets(kw, sizeof(kw), stdin)) return 0;
        char *nl = strchr(kw, '\n');
        if (nl) *nl = '\0';
    } else {
        strncpy(kw, keyword, sizeof(kw) - 1);
    }

    if (strlen(kw) == 0) return 0;

    clear_screen();
    print_box("  SEARCH RESULTS  ", 58);
    printf("  " DIM "Keyword: \"" BWHT "%s" DIM "\"\n\n" RST, kw);

    static char filenames[MAX_ENTRIES][MAX_PATH];
    int count = collect_entries(filenames, MAX_ENTRIES);
    int found = 0;

    /* case-insensitive search via lowercase copies */
    char kw_lower[256];
    strncpy(kw_lower, kw, sizeof(kw_lower) - 1);
    for (int i = 0; kw_lower[i]; i++) kw_lower[i] = (char)tolower((unsigned char)kw_lower[i]);

    for (int i = count - 1; i >= 0; i--) {
        char path[MAX_PATH];
        get_entry_path(filenames[i], path);
        FILE *f = fopen(path, "rb");
        if (!f) continue;
        Entry e;
        fread(&e, sizeof(e), 1, f);
        fclose(f);

        /* lowercase copies for comparison */
        char body_lower[MAX_BODY], title_lower[MAX_TITLE];
        strncpy(body_lower,  e.body,  sizeof(body_lower)  - 1);
        strncpy(title_lower, e.title, sizeof(title_lower) - 1);
        for (int j = 0; body_lower[j];  j++) body_lower[j]  = (char)tolower((unsigned char)body_lower[j]);
        for (int j = 0; title_lower[j]; j++) title_lower[j] = (char)tolower((unsigned char)title_lower[j]);

        if (strstr(body_lower, kw_lower) || strstr(title_lower, kw_lower)) {
            found++;
            char emoji[16];
            get_mood_emoji(e.mood, emoji);
            printf("  " BOLD CYN "%3d" RST "  " DIM "%-22s" RST "  %s  " BWHT "%s\n" RST,
                   found, e.date_str, emoji,
                   strlen(e.title) ? e.title : "(no title)");

            /* show snippet with keyword highlighted */
            char *hit = strstr(body_lower, kw_lower);
            if (hit) {
                int off = (int)(hit - body_lower);
                int start = off - 30; if (start < 0) start = 0;
                char snippet[80] = {0};
                strncpy(snippet, e.body + start, 70);
                printf("       " DIM "…%s…\n" RST, snippet);
            }
        }
    }

    if (found == 0)
        printf(DIM "  No entries found for \"%s\".\n" RST, kw);
    else
        printf(DIM "\n  %d result%s found.\n" RST, found, found == 1 ? "" : "s");

    press_any_key();
    return found;
}

/* ── Random past entry ── */
int random_entry(void) {
    static char filenames[MAX_ENTRIES][MAX_PATH];
    int count = collect_entries(filenames, MAX_ENTRIES);
    if (count == 0) {
        clear_screen();
        printf(DIM "  No entries yet.\n" RST);
        press_any_key();
        return 0;
    }
    srand((unsigned int)time(NULL));
    int idx = rand() % count;
    printf(CYN "\n  🎲 Opening a random memory…\n" RST);
    usleep(800000);
    return view_entry(filenames[idx]);
}

/* ── On this day (same month+day in previous years) ── */
int on_this_day(void) {
    clear_screen();
    print_box("  ON THIS DAY  ", 58);

    time_t now = time(NULL);
    struct tm *t_now = localtime(&now);
    int cur_month = t_now->tm_mon;
    int cur_mday  = t_now->tm_mday;

    printf("\n  " DIM "Entries from %s %d in previous years:\n\n" RST,
           /* month name */
           (const char*[]){"January","February","March","April","May","June",
                           "July","August","September","October","November","December"}[cur_month],
           cur_mday);

    static char filenames[MAX_ENTRIES][MAX_PATH];
    int count = collect_entries(filenames, MAX_ENTRIES);
    int found = 0;

    for (int i = 0; i < count; i++) {
        char path[MAX_PATH];
        get_entry_path(filenames[i], path);
        FILE *f = fopen(path, "rb");
        if (!f) continue;
        Entry e;
        fread(&e, sizeof(e), 1, f);
        fclose(f);

        struct tm *te = localtime(&e.timestamp);
        if (te->tm_mon == cur_month && te->tm_mday == cur_mday
            && te->tm_year != t_now->tm_year) {
            found++;
            char emoji[16];
            get_mood_emoji(e.mood, emoji);
            printf("  %s  " DIM "%d" RST "  " BWHT "%s\n" RST,
                   emoji, 1900 + te->tm_year,
                   strlen(e.title) ? e.title : "(no title)");
        }
    }

    if (found == 0)
        printf(DIM "  Nothing yet — check back next year!\n" RST);

    press_any_key();
    return found;
}

/* ── Export all entries to a plain text file ── */
int export_entries(void) {
    static char filenames[MAX_ENTRIES][MAX_PATH];
    int count = collect_entries(filenames, MAX_ENTRIES);

    if (count == 0) {
        printf(DIM "  Nothing to export.\n" RST);
        press_any_key();
        return 0;
    }

    char out_path[MAX_PATH];
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(out_path, sizeof(out_path), "journal_export_%Y%m%d_%H%M%S.txt", t);

    FILE *out = fopen(out_path, "w");
    if (!out) {
        printf(BRED "  Could not create export file.\n" RST);
        press_any_key();
        return 0;
    }

    fprintf(out, "═══════════════════════════════════════\n");
    fprintf(out, "  TERMINAL JOURNAL  —  EXPORT\n");
    fprintf(out, "  %s\n", out_path);
    fprintf(out, "═══════════════════════════════════════\n\n");

    for (int i = 0; i < count; i++) {
        char path[MAX_PATH];
        get_entry_path(filenames[i], path);
        FILE *f = fopen(path, "rb");
        if (!f) continue;
        Entry e;
        fread(&e, sizeof(e), 1, f);
        fclose(f);

        char mood_str[32];
        get_mood_str(e.mood, mood_str, 0);

        fprintf(out, "───────────────────────────────────────\n");
        fprintf(out, "Date:  %s\n", e.date_str);
        fprintf(out, "Title: %s\n", strlen(e.title) ? e.title : "(no title)");
        fprintf(out, "Mood:  %s  |  Words: %d\n\n", mood_str, e.word_count);
        fprintf(out, "%s\n\n", e.body);
    }

    fclose(out);
    printf(BGRN "  ✓ Exported %d entr%s to: " BWHT "%s\n" RST,
           count, count == 1 ? "y" : "ies", out_path);
    press_any_key();
    return count;
}