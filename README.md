# C Programming Summative Assessment

A collection of five systems programming projects covering embedded systems, shell scripting, data structures, function pointers, and multi-threading.

---

## Table of Contents

- [Project Structure](#project-structure)
- [Project 1 — Smart Traffic Light Controller](#project-1--smart-traffic-light-controller)
- [Project 2 — Linux Server Health Monitor](#project-2--linux-server-health-monitor)
- [Project 3 — Academic Records Analyzer](#project-3--academic-records-analyzer)
- [Project 4 — Data Analysis Toolkit](#project-4--data-analysis-toolkit)
- [Project 5 — Multi-threaded Web Scraper](#project-5--multi-threaded-web-scraper)
- [Assessment Outcomes](#assessment-outcomes)

---

## Project Structure

```
.
├── README.md
├── project1_traffic_light/
│   └── traffic_light_FIXED.ino
├── project2_health_monitor/
│   └── health_monitor.sh
├── project3_academic/
│   └── academic_records.c
├── project4_toolkit/
│   └── data_toolkit.c
└── project5_scraper/
    └── scraper.c
```

---

## Project 1 — Smart Traffic Light Controller

**Language:** Arduino C++ | **Platform:** Tinkercad + EasyEDA

### Description

A smart traffic light controller that manages two intersections simultaneously. Vehicle presence is detected using push buttons, and the system automatically adjusts green light timing based on traffic volume. All operations are non-blocking using `millis()`.

### Features

- Two fully independent traffic intersections (Red, Yellow, Green LEDs each)
- Dynamic green light timing — adjusts automatically based on detected vehicles:
  - 1–3 vehicles → 10 seconds
  - 4–8 vehicles → 12 seconds
  - 9+ vehicles → 15 seconds
- Non-blocking concurrency using `millis()` — no `delay()` used anywhere
- Dynamic memory allocation using `new` for `Intersection` and `TrafficStats` structs
- Serial monitor interface with full command menu
- Emergency stop and system reset functionality
- Real-time traffic statistics logging

### Wiring (Tinkercad)

| Component     | Intersection 1 Pin | Intersection 2 Pin | Resistor |
|---------------|--------------------|--------------------|----------|
| Red LED       | D2                 | D8                 | 220Ω     |
| Yellow LED    | D3                 | D9                 | 220Ω     |
| Green LED     | D4                 | D10                | 220Ω     |
| Push Button   | D7                 | D11                | 10kΩ     |
| GND           | GND rail           | GND rail           | —        |

### Serial Commands

| Command | Action |
|---------|--------|
| `1` | Toggle manual mode — Intersection 1 |
| `2` | Toggle manual mode — Intersection 2 |
| `R` | Set RED (manual mode only) |
| `G` | Set GREEN (manual mode only) |
| `Y` | Set YELLOW (manual mode only) |
| `E` | Emergency stop — all lights go RED |
| `S` | Reset system to automatic mode |
| `V` | View detailed statistics |
| `M` | Show command menu |

### How to Run

1. Open [Tinkercad](https://tinkercad.com) and create a new Circuit
2. Add: 1× Arduino Uno, 6× LEDs (2 Red, 2 Yellow, 2 Green), 2× Push Buttons, 6× 220Ω resistors, 2× 10kΩ resistors
3. Wire components according to the table above
4. Click **Code** → switch to **Text** mode
5. Paste the contents of `traffic_light_FIXED.ino`
6. Click **Start Simulation**
7. Open the Serial Monitor to interact via commands

---

## Project 2 — Linux Server Health Monitor

**Language:** Bash | **Platform:** Linux (Ubuntu/Debian)

### Description

An automated shell script that continuously monitors server health by tracking CPU usage, memory consumption, disk utilization, and active process count. Generates timestamped alerts when thresholds are exceeded and provides an interactive terminal dashboard.

### Features

- Real-time monitoring of CPU, memory, disk, and process metrics
- Colour-coded output: green (normal), yellow (warning), red (critical)
- Configurable alert thresholds per resource
- Timestamped log file with view and clear options
- Background monitoring loop with configurable interval (default: 60 seconds)
- Start/stop background monitoring with PID file management
- Comprehensive input validation and error handling
- Automatic fallback log path if `/var/log` is not writable

### How to Run

```bash
# Make executable
chmod +x health_monitor.sh

# Run
bash health_monitor.sh
```

### Menu Options

```
1) Display current system health
2) Configure monitoring thresholds
3) View activity logs
4) Clear logs
5) Start background monitoring
6) Stop background monitoring
7) Exit
```

### Linux Tools Used

| Tool | Purpose |
|------|---------|
| `/proc/stat` | Accurate CPU usage calculation |
| `free` | Memory usage |
| `df` | Disk utilization |
| `ps aux` | Active process count |
| `sleep` | Timed monitoring intervals |

---

## Project 3 — Academic Records Analyzer

**Language:** C | **Platform:** Linux / Windows

### Description

A menu-driven C program for managing student academic records. Uses dynamic memory allocation to handle an unknown number of records, implements three manual sorting algorithms, and persists data to a binary file.

### Features

- Dynamic array using `malloc()`, `realloc()`, and `free()`
- Full CRUD operations (Create, Read, Update, Delete)
- Case-insensitive partial name search
- Three manual sorting algorithms:
  - **Insertion sort** — by GPA (descending)
  - **Bubble sort** — by name (A–Z)
  - **Selection sort** — by student ID (ascending)
- Statistical reports:
  - Class average GPA
  - Highest and lowest GPA
  - Median GPA
  - Top-N performing students
  - Best performer per course
  - Course-based average performance
- Binary file persistence with integrity checking
- Duplicate ID prevention and grade range validation

### Compile and Run

```bash
gcc academic_records.c -o records -lm
./records
```

### Data File

Records are saved automatically to `records.dat` in binary format. The file loads on startup if it exists.

### Struct Definition

```c
typedef struct {
    int   studentID;
    char  name[64];
    char  course[64];
    int   age;
    float grades[8];     // up to 8 subjects
    int   numSubjects;
    float gpa;           // computed average
} Student;
```

---

## Project 4 — Data Analysis Toolkit

**Language:** C | **Platform:** Linux / Windows

### Description

A numerical data processing toolkit built around a **function pointer dispatch table**. Each menu option maps directly to a function pointer — there is no long `if/else` or `switch` chain in the dispatcher. Filtering, transformation, and sorting all use callback functions.

### Features

- Function pointer dispatcher table (`OperationFn` array of structs)
- Callback-based operations:
  - **Filters:** above threshold, below threshold, equal to value
  - **Transforms:** scale (multiply), shift (add), square each element
  - **Comparators:** ascending and descending sort via `qsort()`
- Supported operations:
  - Sum and average
  - Minimum and maximum
  - Filter by condition
  - Apply transformation
  - Sort (ascending/descending)
  - Search for value
- Dynamic dataset using `malloc()` and `realloc()`
- File load and save
- NULL pointer guards, empty dataset checks, full input validation

### Compile and Run

```bash
gcc data_toolkit.c -o toolkit -lm
./toolkit
```

### Key Design Pattern

```c
// Each menu item maps label → function pointer
typedef struct {
    const char* label;
    OperationFn fn;       // function pointer
} MenuItem;

// Dispatch — zero if/else needed
OperationFn op = menu[choice - 1].fn;
op(&ds);
```

### Dataset File Format

Plain text, one number per line:

```
3.14
2.71
1.41
42.0
```

---

## Project 5 — Multi-threaded Web Scraper

**Language:** C | **Platform:** Linux | **Libraries:** pthreads, libcurl

### Description

A parallel web scraper that fetches multiple URLs simultaneously using POSIX threads. Each thread operates fully independently with its own `CURL*` handle and its own output file — no mutex or thread synchronization required.

### Features

- One `pthread` per URL — true parallel downloads
- Each thread saves HTML content to a separate file in `scraped_output/`
- Per-thread metadata header written to each output file
- Graceful error handling for unreachable or timed-out URLs
- URL list loaded from a text file (one URL per line)
- Built-in default test URLs if no file is provided
- Summary table printed after all threads complete
- Memory managed with `malloc()`, `realloc()`, and `free()`

### Install Dependencies

```bash
# Ubuntu / Debian
sudo apt-get install gcc libcurl4-openssl-dev

# Fedora / CentOS
sudo dnf install gcc libcurl-devel
```

### Compile and Run

```bash
# Compile
gcc scraper.c -o scraper -lpthread -lcurl

# Run with a URL file
./scraper urls.txt

# Run with built-in test URLs
./scraper
```

### URL File Format (`urls.txt`)

```
https://example.com
https://httpbin.org/html
https://httpbin.org/json
# Lines starting with # are ignored
```

### Output

Downloaded files are saved to `scraped_output/`:

```
scraped_output/
├── thread_01_example_com.html
├── thread_02_httpbin_org_html.html
└── thread_03_httpbin_org_json.html
```

### Thread Design

```c
// Each thread gets its own independent struct — no shared state
typedef struct {
    int  threadID;
    char url[512];
    int  success;
    char outputFile[256];
    char errorMsg[256];
} ThreadArgs;

// Each thread uses its own CURL handle — thread-safe, no mutex needed
CURL* curl = curl_easy_init();
```
