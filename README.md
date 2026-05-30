# 📓 Terminal Journal

A fully-featured, beautiful digital journal that lives entirely in your terminal — written in pure C with zero external dependencies.

```
  ╔══════════════════════════════════════════════════════╗
  ║        ✦  T E R M I N A L   J O U R N A L  ✦         ║
  ╚══════════════════════════════════════════════════════╝

  Hello, Bhakti!  Friday, 30 May 2026
  🔥 7-day streak  ·  24 entries  ·  8,312 words total
```

---

## Features

- **Write, browse, edit & delete** journal entries
- **Mood tracker** — tag each entry 😞 to 😄 and view history as a bar chart
- **Keyword search** — case-insensitive, with context snippets
- **Writing streak** tracker — tracks how many days in a row you've written
- **Random past entry** — rediscover old memories at random
- **On This Day** — see entries from the same date in previous years
- **Export** — dump all entries to a plain `.txt` file
- **Password protection** — secured with a djb2 hash at startup
- **ANSI color UI** — box-drawing characters, emoji, color-coded moods, typing animations

---

## Project Structure

```
journal/
├── journal.h   ← shared types, constants, macros, all function declarations
├── main.c      ← entry point, startup sequence, main menu loop
├── ui.c        ← visual rendering: banner, menus, boxes, animations
├── entry.c     ← all CRUD operations on journal entries
├── auth.c      ← password setup, hashing, verification
├── stats.c     ← statistics loading, saving, streak tracking, mood chart
└── util.c      ← shared helpers: word count, mood labels, multiline input
```

All entries are stored as binary files in a `journal_data/` directory that is created automatically on first run.

---

## Build & Run

```bash
# Compile
gcc -o journal main.c ui.c entry.c auth.c stats.c util.c

# Run
./journal
```

Requires a POSIX-compliant system (Linux or macOS). No external libraries needed.

---

## File-by-File Code Explanation

---

### `journal.h` — The Shared Header

This is the backbone of the whole project. Every other `.c` file includes it.

**What it does:**
- Includes all standard library headers in one place (`stdio.h`, `stdlib.h`, `string.h`, `time.h`, `dirent.h`, `termios.h`, etc.)
- Defines all `#define` constants so magic numbers never appear in the code
- Defines the three core data structures
- Declares every function so all files can call each other freely
- Defines all ANSI escape code macros for colors and formatting

**The three structs:**

```c
typedef struct {
    char   filename[MAX_PATH];  // e.g. "2026-05-30_143022.jrn"
    char   title[MAX_TITLE];    // optional title, up to 128 chars
    char   body[MAX_BODY];      // entry text, up to 8192 chars
    char   date_str[32];        // human-readable date
    time_t timestamp;           // Unix timestamp for sorting
    int    mood;                // 1 (terrible) to 5 (great)
    int    word_count;          // auto-calculated on save
    int    tags[8];             // reserved for future use
} Entry;
```

```c
typedef struct {
    int    total_entries;   // total number of entries ever written
    int    total_words;     // running total of all words written
    int    streak;          // current consecutive-day writing streak
    time_t last_entry;      // timestamp of the most recent entry
    int    mood_sum;        // sum of all mood values (for average)
    int    mood_count;      // number of mood-tagged entries
} Stats;
```

```c
typedef struct {
    char pass_hash[128];   // hashed password (djb2 algorithm)
    int  locked;           // 1 = password required on startup
    char author[64];       // user's name shown in the greeting
} Config;
```

**The ANSI macros** — instead of memorising escape codes, the code uses readable names:

```c
#define BGRN  "\033[92m"   // bright green text
#define BYLW  "\033[93m"   // bright yellow text
#define RST   "\033[0m"    // reset all formatting
#define BOLD  "\033[1m"    // bold text
#define DIM   "\033[2m"    // dimmed/grey text
```

---

### `main.c` — Entry Point & Main Loop

This is where the program starts. It's intentionally short — its only job is to orchestrate the startup sequence and keep the menu loop running.

**Startup sequence:**

```c
mkdir(JOURNAL_DIR, 0700);   // create journal_data/ if it doesn't exist
```
`0700` means only the owner can read/write — a small privacy touch.

```c
Config cfg;
load_config(&cfg);

if (cfg.author[0] == '\0') {
    setup_password(&cfg);   // first run: ask for name + password
}
```

On first run, the config file doesn't exist yet, so `author` is empty — that triggers the setup wizard.

```c
if (cfg.locked) {
    int tries = 3;
    while (tries-- > 0) {
        if (check_password(&cfg)) break;
        // lock out after 3 wrong attempts
    }
}
```

**The main loop:**

```c
while (1) {
    print_banner();
    print_menu();
    char buf[8]; fgets(buf, sizeof(buf), stdin);
    switch (atoi(buf)) {
        case 1: write_entry(&stats, &cfg); break;
        case 2: list_entries(NULL);        break;
        // ... etc
        case 0: return 0;  // quit
    }
}
```

It runs forever until the user picks `0`. Each case calls into the relevant module.

---

### `ui.c` — User Interface & Visual Rendering

Handles everything visual: clearing the screen, drawing boxes, the banner, menus, and animations. Uses only `printf` and ANSI escape codes — no `ncurses` or any external library.

**`clear_screen()`**
```c
void clear_screen(void) {
    printf("\033[2J\033[H");  // ESC[2J = clear screen, ESC[H = move cursor to top-left
    fflush(stdout);
}
```

**`print_box()`** — draws a titled box using Unicode box-drawing characters:
```c
printf("╔═══════════════╗\n");
printf("║   YOUR TITLE  ║\n");
printf("╚═══════════════╝\n");
```
The width and padding are calculated dynamically based on the title length.

**`animate_type()`** — creates a typewriter effect by printing one character at a time with a `usleep()` delay between each:
```c
void animate_type(const char *text, int delay_us) {
    for (int i = 0; text[i]; i++) {
        putchar(text[i]);
        fflush(stdout);
        usleep(delay_us);   // pause in microseconds
    }
}
```

**`getch_raw()`** — reads a single keypress without requiring Enter, by temporarily disabling terminal line buffering using `termios`:
```c
struct termios oldt, newt;
tcgetattr(STDIN_FILENO, &oldt);   // save current terminal settings
newt = oldt;
newt.c_lflag &= ~(ICANON | ECHO); // disable line buffering and echo
tcsetattr(STDIN_FILENO, TCSANOW, &newt);
char ch = (char)getchar();
tcsetattr(STDIN_FILENO, TCSANOW, &oldt);  // restore settings
```

---

### `entry.c` — Journal Entry CRUD Operations

The largest and most complex file. Handles writing, listing, viewing, editing, deleting, searching, and the special features (random entry, on this day, export).

**How entries are stored:**

Each entry is serialised as a raw binary dump of the `Entry` struct directly to a `.jrn` file:
```c
FILE *f = fopen(path, "wb");
fwrite(&e, sizeof(e), 1, f);   // write the whole struct as binary
fclose(f);
```
Reading it back is just the reverse:
```c
FILE *f = fopen(path, "rb");
fread(&e, sizeof(e), 1, f);
```
This is fast and simple. The tradeoff is that `.jrn` files are not human-readable — but they don't need to be since the app handles all reading.

**Filename convention:**
```c
strftime(fname, sizeof(fname), "%Y-%m-%d_%H%M%S.jrn", tm_info);
// e.g. "2026-05-30_143022.jrn"
```
Because the format is `YYYY-MM-DD`, sorting filenames alphabetically automatically gives chronological order.

**`collect_entries()`** — scans the directory and collects all `.jrn` filenames:
```c
DIR *dir = opendir(JOURNAL_DIR);
struct dirent *ent;
while ((ent = readdir(dir)) != NULL) {
    if (strstr(ent->d_name, ".jrn"))
        strncpy(filenames[count++], ent->d_name, MAX_PATH - 1);
}
```
Then bubble-sorts them so the list is always in date order.

**`search_entries()`** — case-insensitive search using lowercase copies:
```c
char body_lower[MAX_BODY];
strncpy(body_lower, e.body, sizeof(body_lower) - 1);
for (int j = 0; body_lower[j]; j++)
    body_lower[j] = tolower(body_lower[j]);

if (strstr(body_lower, kw_lower)) {
    // match found — show snippet
}
```

**`on_this_day()`** — filters entries by matching month and day (ignoring year):
```c
struct tm *te = localtime(&e.timestamp);
if (te->tm_mon == cur_month && te->tm_mday == cur_mday
    && te->tm_year != t_now->tm_year) {
    // this entry is from the same date in a different year
}
```

---

### `auth.c` — Password Protection

Implements the lock/unlock system using a simple but effective approach: passwords are never stored in plain text, only their hash.

**`hash_password()`** — the djb2 hash algorithm:
```c
void hash_password(const char *pass, char *out) {
    unsigned long hash = 5381;
    int c;
    while ((c = (unsigned char)*pass++))
        hash = ((hash << 5) + hash) + c;  // hash * 33 + c
    snprintf(out, 128, "%lu", hash);
}
```
This is a classic, fast, non-cryptographic hash. For a local personal journal it's perfectly sufficient. The hash is stored in the `.config` binary file; the original password is never written anywhere.

**`read_password()`** — reads a password without showing it on screen by disabling echo in the terminal:
```c
newt.c_lflag &= ~ECHO;   // turn off character echoing
tcsetattr(STDIN_FILENO, TCSANOW, &newt);
fgets(buf, maxlen, stdin);  // user types but nothing shows
tcsetattr(STDIN_FILENO, TCSANOW, &oldt);  // restore echo
```

**Verifying a password:**
```c
char hashed[128];
hash_password(pass, hashed);                     // hash what user typed
strcmp(hashed, cfg->pass_hash) == 0              // compare to stored hash
```
If they match, access is granted. The original password is never compared directly.

---

### `stats.c` — Statistics & Streak Tracking

Manages the persistent `Stats` struct and renders the mood bar chart.

**Persistence** — stats are stored as a binary file, just like entries:
```c
void save_stats(const Stats *s) {
    FILE *f = fopen(STATS_FILE, "wb");
    fwrite(s, sizeof(*s), 1, f);
    fclose(f);
}
```

**`update_streak()`** — compares today's date to the last entry date:
```c
int diff_days = t_now->tm_yday - t_last->tm_yday
              + (t_now->tm_year - t_last->tm_year) * 365;

if (diff_days == 0)       { /* same day, streak unchanged */  }
else if (diff_days == 1)  { s->streak++;   /* consecutive day */ }
else                      { s->streak = 1; /* streak broken  */ }
```

**The mood bar chart** — reads all `.jrn` files, tallies mood counts, then draws scaled bars using Unicode block characters:
```c
int filled = (max_val > 0) ? (value * bar_width / max_val) : 0;

for (int i = 0; i < filled; i++) printf("█");   // filled portion
for (int i = filled; i < bar_width; i++) printf("░"); // empty portion
```
The bar width scales relative to the most frequent mood so the chart always fits the terminal.

---

### `util.c` — Shared Utilities

Small helper functions used across all the other modules.

**`count_words()`** — counts words by detecting transitions from whitespace to non-whitespace:
```c
int count = 0, in_word = 0;
for (; *text; text++) {
    if (isspace(*text))       { in_word = 0; }
    else if (!in_word)        { in_word = 1; count++; }
}
```

**`read_multiline()`** — reads multiple lines from stdin until the user types `END` alone on a line. This is how journal entry bodies are entered:
```c
while (remaining > 0) {
    fgets(line, sizeof(line), stdin);
    if (strcmp(trimmed, "END") == 0) break;  // sentinel to finish
    strncat(buf, line, llen);
}
```

**`get_mood_str()`** — returns a colour-coded mood label:
```c
const char *colors[] = { "", BRED, RED, YLW, GRN, BGRN };
snprintf(out, 32, "%s%s%s", colors[mood], labels[mood], RST);
```

**`confirm()`** — a simple yes/no prompt:
```c
int confirm(const char *msg) {
    printf("%s (y/N): ", msg);
    char buf[8]; fgets(buf, sizeof(buf), stdin);
    return (buf[0] == 'y' || buf[0] == 'Y');
}
```

---

## Data Storage

```
journal_data/
├── .config               ← binary Config struct (author name, password hash)
├── .stats                ← binary Stats struct (totals, streak, mood data)
├── 2026-05-28_090132.jrn ← binary Entry struct
├── 2026-05-29_213045.jrn ← binary Entry struct
└── 2026-05-30_143022.jrn ← binary Entry struct
```

Files starting with `.` are hidden on Unix systems. Entry filenames use a timestamp format so they sort chronologically without any index file.

---

## Concepts Used

| Concept | Where Used |
|---|---|
| File I/O (`fopen`, `fread`, `fwrite`) | entry.c, stats.c, auth.c |
| Directory scanning (`opendir`, `readdir`) | entry.c |
| Structs & pointers | All files |
| String manipulation (`strstr`, `strncpy`, `strncat`) | entry.c, util.c |
| Terminal control (`termios`) | ui.c, auth.c |
| Time functions (`time`, `localtime`, `strftime`) | entry.c, stats.c |
| ANSI escape codes | ui.c, util.c |
| djb2 hashing | auth.c |
| Bubble sort | entry.c |
| Bit masking | auth.c (`c_lflag &= ~ECHO`) |

---

## License

MIT — free to use, modify, and distribute.
