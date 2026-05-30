#include "journal.h"

/* ── Trim leading/trailing whitespace in-place ── */
void trim(char *s) {
    if (!s) return;
    char *end;
    while (isspace((unsigned char)*s)) s++;
    end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) *end-- = '\0';
}

/* ── Format a timestamp into a human-readable string ── */
void format_timestamp(time_t t, char *out) {
    struct tm *tm_info = localtime(&t);
    strftime(out, 64, "%A, %d %B %Y  %H:%M", tm_info);
}

/* ── Build full path for an entry filename ── */
void get_entry_path(const char *filename, char *out) {
    snprintf(out, MAX_PATH, "%s/%s", JOURNAL_DIR, filename);
}

/* ── Return a colored mood label ── */
void get_mood_str(int mood, char *out, int color) {
    const char *labels[] = { "", "Terrible", "Bad", "Neutral", "Good", "Great" };
    const char *colors[] = { "", BRED, RED, YLW, GRN, BGRN };
    if (mood < 1 || mood > 5) { strcpy(out, "?"); return; }
    if (color)
        snprintf(out, 32, "%s%s%s", colors[mood], labels[mood], RST);
    else
        snprintf(out, 32, "%s", labels[mood]);
}

/* ── Return a mood emoji string ── */
void get_mood_emoji(int mood, char *out) {
    const char *emojis[] = { "?", "😞", "😟", "😐", "🙂", "😄" };
    if (mood < 1 || mood > 5) { strcpy(out, "?"); return; }
    strcpy(out, emojis[mood]);
}

/* ── Interactive mood picker ── */
int pick_mood(void) {
    printf("\n  " BOLD "How are you feeling?" RST "\n\n");
    printf("    " BRED  " 1 " RST " 😞  Terrible\n");
    printf("    " RED   " 2 " RST " 😟  Bad\n");
    printf("    " YLW   " 3 " RST " 😐  Neutral\n");
    printf("    " GRN   " 4 " RST " 🙂  Good\n");
    printf("    " BGRN  " 5 " RST " 😄  Great\n");
    printf("\n  " BWHT "Mood (1-5): " RST);

    char buf[8];
    if (!fgets(buf, sizeof(buf), stdin)) return MOOD_NEUTRAL;
    int m = atoi(buf);
    if (m < 1 || m > 5) m = MOOD_NEUTRAL;
    return m;
}

/* ── Read multiline text until user types END on its own line ── */
void read_multiline(char *buf, int maxlen) {
    printf(DIM "  (type your entry below; type END on a new line when done)\n" RST);
    print_separator(60);

    buf[0] = '\0';
    int remaining = maxlen - 1;
    char line[512];

    while (remaining > 0) {
        printf(CYN "  " RST);
        fflush(stdout);
        if (!fgets(line, sizeof(line), stdin)) break;

        /* strip trailing newline for comparison only */
        char trimmed[512];
        strncpy(trimmed, line, sizeof(trimmed)-1);
        trimmed[sizeof(trimmed)-1] = '\0';
        char *nl = strchr(trimmed, '\n');
        if (nl) *nl = '\0';

        if (strcmp(trimmed, "END") == 0) break;

        int llen = (int)strlen(line);
        if (llen > remaining) llen = remaining;
        strncat(buf, line, llen);
        remaining -= llen;
    }
}

/* ── Count words in a string ── */
int count_words(const char *text) {
    if (!text || !*text) return 0;
    int count = 0, in_word = 0;
    for (; *text; text++) {
        if (isspace((unsigned char)*text)) {
            in_word = 0;
        } else if (!in_word) {
            in_word = 1;
            count++;
        }
    }
    return count;
}

/* ── Yes/No confirmation ── */
int confirm(const char *msg) {
    printf("  " BYLW "%s" RST " (y/N): ", msg);
    char buf[8];
    if (!fgets(buf, sizeof(buf), stdin)) return 0;
    return (buf[0] == 'y' || buf[0] == 'Y');
}

time_t parse_datetime(const char *str) {
    struct tm t = {0};

    // format: YYYY-MM-DD HH:MM
    if (sscanf(str, "%d-%d-%d %d:%d",
        &t.tm_year,
        &t.tm_mon,
        &t.tm_mday,
        &t.tm_hour,
        &t.tm_min) != 5) {
        return (time_t)-1;
    }

    t.tm_year -= 1900;
    t.tm_mon -= 1;

    return mktime(&t);
}