#include "journal.h"

void load_stats(Stats *s) {
    memset(s, 0, sizeof(*s));
    FILE *f = fopen(STATS_FILE, "rb");
    if (!f) return;
    fread(s, sizeof(*s), 1, f);
    fclose(f);
}

void save_stats(const Stats *s) {
    FILE *f = fopen(STATS_FILE, "wb");
    if (!f) return;
    fwrite(s, sizeof(*s), 1, f);
    fclose(f);
}

void update_streak(Stats *s) {
    time_t now = time(NULL);
    struct tm *t_now = localtime(&now);
    struct tm *t_last;

    if (s->last_entry == 0) {
        s->streak = 1;
        s->last_entry = now;
        return;
    }

    t_last = localtime(&s->last_entry);

    /* compare calendar dates */
    int diff_days = t_now->tm_yday - t_last->tm_yday
                  + (t_now->tm_year - t_last->tm_year) * 365;

    if (diff_days == 0) {
        /* same day - streak unchanged */
    } else if (diff_days == 1) {
        s->streak++;
    } else {
        s->streak = 1;
    }
    s->last_entry = now;
}

/* ── Draw a simple bar chart row ── */
static void draw_bar(const char *label, int value, int max_val, const char *color) {
    int bar_width = 30;
    int filled = (max_val > 0) ? (value * bar_width / max_val) : 0;
    printf("  %s%-10s" RST " " BOLD "%s", DIM, label, color);
    for (int i = 0; i < filled; i++) printf("█");
    printf(RST DIM);
    for (int i = filled; i < bar_width; i++) printf("░");
    printf(RST "  " BWHT "%d" RST "\n", value);
}

void show_stats(const Stats *s) {
    clear_screen();
    print_box("  JOURNAL STATS & MOOD HISTORY  ", 58);
    printf("\n");

    /* ── General stats ── */
    printf(BOLD CYN "  General\n" RST);
    print_separator(58);
    printf("  📖  Total entries    " BWHT "%d\n" RST, s->total_entries);
    printf("  ✏️   Total words      " BWHT "%d\n" RST, s->total_words);
    printf("  🔥  Current streak   " BYLW "%d day%s\n" RST,
           s->streak, s->streak == 1 ? "" : "s");

    if (s->last_entry) {
        char date_buf[64];
        format_timestamp(s->last_entry, date_buf);
        printf("  🕐  Last entry       " DIM "%s\n" RST, date_buf);
    }

    float avg_mood = (s->mood_count > 0)
                   ? (float)s->mood_sum / s->mood_count
                   : 0.0f;
    printf("  💭  Avg mood         ");
    if      (avg_mood >= 4.5f) printf(BGRN);
    else if (avg_mood >= 3.5f) printf(GRN);
    else if (avg_mood >= 2.5f) printf(YLW);
    else if (avg_mood >= 1.5f) printf(RED);
    else                       printf(BRED);
    printf("%.1f / 5.0" RST "\n\n", avg_mood);

    /* ── Mood breakdown by reading all entries ── */
    printf(BOLD CYN "  Mood Breakdown\n" RST);
    print_separator(58);

    int mood_counts[6] = {0};
    DIR *dir = opendir(JOURNAL_DIR);
    if (dir) {
        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL) {
            if (!strstr(ent->d_name, ".jrn")) continue;
            char path[MAX_PATH];
            get_entry_path(ent->d_name, path);
            FILE *f = fopen(path, "rb");
            if (!f) continue;
            Entry e;
            fread(&e, sizeof(e), 1, f);
            fclose(f);
            if (e.mood >= 1 && e.mood <= 5)
                mood_counts[e.mood]++;
        }
        closedir(dir);
    }

    int max_m = 1;
    for (int i = 1; i <= 5; i++) if (mood_counts[i] > max_m) max_m = mood_counts[i];

    const char *mood_labels[] = {"", "Terrible", "Bad", "Neutral", "Good", "Great"};
    const char *mood_colors[] = {"", BRED, RED, YLW, GRN, BGRN};
    for (int i = 5; i >= 1; i--)
        draw_bar(mood_labels[i], mood_counts[i], max_m, mood_colors[i]);

    /* ── Streak encouragement ── */
    printf("\n");
    print_separator(58);
    if (s->streak >= 30)
        printf("  " BGRN BOLD "🏆  Incredible! 30+ day streak. You're unstoppable!" RST "\n");
    else if (s->streak >= 7)
        printf("  " BYLW BOLD "⭐  Great job! A whole week of journaling!" RST "\n");
    else if (s->streak >= 3)
        printf("  " GRN   "🌱  Keep it up — %d days in a row!" RST "\n", s->streak);
    else if (s->streak >= 1)
        printf("  " CYN   "✨  You started! Keep the streak going." RST "\n");
    else
        printf("  " DIM   "Write your first entry to start a streak." RST "\n");

    press_any_key();
}