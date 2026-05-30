#ifndef JOURNAL_H
#define JOURNAL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <termios.h>
#include <ctype.h>

/* ── Paths ── */
#define JOURNAL_DIR     "journal_data"
#define CONFIG_FILE     "journal_data/.config"
#define STATS_FILE      "journal_data/.stats"

/* ── Limits ── */
#define MAX_BODY        8192
#define MAX_TITLE       128
#define MAX_PATH        512
#define MAX_ENTRIES     1024
#define MAX_PASS        64

/* ── Moods ── */
#define MOOD_GREAT      5
#define MOOD_GOOD       4
#define MOOD_NEUTRAL    3
#define MOOD_BAD        2
#define MOOD_TERRIBLE   1

/* ── ANSI Colors ── */
#define RST   "\033[0m"
#define BOLD  "\033[1m"
#define DIM   "\033[2m"
#define ITAL  "\033[3m"

#define RED   "\033[31m"
#define GRN   "\033[32m"
#define YLW   "\033[33m"
#define BLU   "\033[34m"
#define MAG   "\033[35m"
#define CYN   "\033[36m"
#define WHT   "\033[37m"

#define BRED  "\033[91m"
#define BGRN  "\033[92m"
#define BYLW  "\033[93m"
#define BBLU  "\033[94m"
#define BMAG  "\033[95m"
#define BCYN  "\033[96m"
#define BWHT  "\033[97m"

#define BG_BLU  "\033[44m"
#define BG_MAG  "\033[45m"
#define BG_CYN  "\033[46m"
#define BG_DRK  "\033[40m"

/* ── Entry struct ── */
typedef struct {
    char  filename[MAX_PATH];   /* e.g. 2026-05-04_143022.jrn */
    char  title[MAX_TITLE];
    char  body[MAX_BODY];
    char  date_str[32];         /* human-readable */
    time_t timestamp;
    int   mood;                 /* 1-5 */
    int   word_count;
    int   tags[8];              /* reserved */
} Entry;

/* ── Stats struct ── */
typedef struct {
    int  total_entries;
    int  total_words;
    int  streak;
    time_t last_entry;
    int  mood_sum;
    int  mood_count;
} Stats;

/* ── Config struct ── */
typedef struct {
    char  pass_hash[128];
    int   locked;
    char  author[64];
} Config;

/* ── Function declarations ── */

/* ui.c */
void  clear_screen(void);
void  print_banner(void);
void  print_menu(void);
void  print_box(const char *title, int width);
void  print_separator(int width);
void  animate_type(const char *text, int delay_us);
void  press_any_key(void);
int   get_terminal_width(void);
char  getch_raw(void);

/* entry.c */
int   write_entry(Stats *stats, Config *cfg);
int   list_entries(int *count_out);
int   view_entry(const char *filename);
int   search_entries(const char *keyword);
int   delete_entry(const char *filename, Stats *stats);
int   edit_entry(const char *filename, Stats *stats);
int   random_entry(void);
int   on_this_day(void);

/* stats.c */
void  load_stats(Stats *s);
void  save_stats(const Stats *s);
void  show_stats(const Stats *s);
void  update_streak(Stats *s);
int   count_words(const char *text);

/* auth.c */
void  load_config(Config *cfg);
void  save_config(const Config *cfg);
int   setup_password(Config *cfg);
int   check_password(const Config *cfg);
void  hash_password(const char *pass, char *out);

/* util.c */
void  get_mood_str(int mood, char *out, int color);
void  get_mood_emoji(int mood, char *out);
int   pick_mood(void);
void  read_multiline(char *buf, int maxlen);
void  trim(char *s);
int   confirm(const char *msg);
void  format_timestamp(time_t t, char *out);
void  get_entry_path(const char *filename, char *out);

time_t parse_datetime(const char *str);

#endif