#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>


#define MAX_NAME    64
#define MAX_COURSE  64
#define MAX_SUBJECT 8
#define DATA_FILE   "records.dat"


typedef struct {
  int    studentID;
  char   name[MAX_NAME];
  char   course[MAX_COURSE];
  int    age;
  float  grades[MAX_SUBJECT];  // per-subject grades (0–100)
  int    numSubjects;
  float  gpa;                  // computed average of grades
} Student;

typedef struct {
  Student* data;    
  int      count;
  int      capacity;
} Database;


// DB management
void     db_init(Database* db);
int      db_add(Database* db, Student s);
int      db_delete(Database* db, int id);
Student* db_findByID(Database* db, int id);
void     db_free(Database* db);

// CRUD
void     addRecord(Database* db);
void     displayAll(Database* db);
void     updateRecord(Database* db);
void     deleteRecord(Database* db);

// Search
void     searchByID(Database* db);
void     searchByName(Database* db);

// Sort (manual algorithms)
void     sortByGPA(Database* db);
void     sortByName(Database* db);
void     sortByID(Database* db);

// Analytics
void     reportAverageGPA(Database* db);
void     reportTopN(Database* db);
void     reportCourseStats(Database* db);
void     reportMinMax(Database* db);
void     reportMedian(Database* db);

// File I/O
void     saveToFile(Database* db);
void     loadFromFile(Database* db);

// Helpers
float    computeGPA(float* grades, int n);
void     printStudent(Student* s);
void     clearInput(void);
int      readInt(const char* prompt, int lo, int hi);
float    readFloat(const char* prompt, float lo, float hi);
void     readString(const char* prompt, char* buf, int maxLen);
int      idExists(Database* db, int id);

// ─── Main ─────────────────────────────────────────────────────────────────────
int main(void) {
  Database db;
  db_init(&db);
  loadFromFile(&db);

  int choice;
  do {
    printf("\n\n");
    printf("  Academic Records Analyzer       \n");
    printf("\n_________________________________");
    printf("  1) Add student record           \n");
    printf("  2) Display all records          \n");
    printf("  3) Update record                \n");
    printf("  4) Delete record                \n");
    printf("  5) Search by ID                 \n");
    printf("  6) Search by name               \n");
    printf("  7) Sort by GPA                  \n");
    printf("  8) Sort by name                 \n");
    printf("  9) Sort by ID                   \n");
    printf(" 10) Report: Average GPA          \n");
    printf(" 11) Report: Top-N students       \n");
    printf(" 12) Report: Min/Max GPA          \n");
    printf(" 13) Report: Median GPA           \n");
    printf(" 14) Report: Per-course stats     \n");
    printf(" 15) Save and exit                \n");
   

    choice = readInt("Choice", 1, 15);

    switch (choice) {
      case  1: addRecord(&db);         break;
      case  2: displayAll(&db);        break;
      case  3: updateRecord(&db);      break;
      case  4: deleteRecord(&db);      break;
      case  5: searchByID(&db);        break;
      case  6: searchByName(&db);      break;
      case  7: sortByGPA(&db);         break;
      case  8: sortByName(&db);        break;
      case  9: sortByID(&db);          break;
      case 10: reportAverageGPA(&db);  break;
      case 11: reportTopN(&db);        break;
      case 12: reportMinMax(&db);      break;
      case 13: reportMedian(&db);      break;
      case 14: reportCourseStats(&db); break;
      case 15:
        saveToFile(&db);
        printf("Records saved. Goodbye!\n");
        break;
    }
  } while (choice != 15);

  db_free(&db);
  return 0;
}

void db_init(Database* db) {
  db->capacity = 8;
  db->count    = 0;
  db->data     = (Student*)malloc(db->capacity * sizeof(Student));
  if (!db->data) { fprintf(stderr, "Memory allocation failed.\n"); exit(1); }
}

int db_add(Database* db, Student s) {
  if (db->count == db->capacity) {
    db->capacity *= 2;
    Student* tmp = (Student*)realloc(db->data, db->capacity * sizeof(Student));
    if (!tmp) { fprintf(stderr, "realloc failed.\n"); return 0; }
    db->data = tmp;
  }
  db->data[db->count++] = s;
  return 1;
}

int db_delete(Database* db, int id) {
  for (int i = 0; i < db->count; i++) {
    if (db->data[i].studentID == id) {
      // Shift left
      for (int j = i; j < db->count - 1; j++)
        db->data[j] = db->data[j + 1];
      db->count--;
      return 1;
    }
  }
  return 0;
}

Student* db_findByID(Database* db, int id) {
  for (int i = 0; i < db->count; i++)
    if (db->data[i].studentID == id) return &db->data[i];
  return NULL;
}

void db_free(Database* db) {
  free(db->data);
  db->data = NULL;
  db->count = db->capacity = 0;
}

float computeGPA(float* grades, int n) {
  if (n <= 0) return 0.0f;
  float sum = 0;
  for (int i = 0; i < n; i++) sum += grades[i];
  return sum / n;
}

//Print One Student 
void printStudent(Student* s) {
  printf("  ID: %-6d  Name: %-20s  Course: %-15s  Age: %-3d  GPA: %.2f\n",
         s->studentID, s->name, s->course, s->age, s->gpa);
  printf("  Grades: ");
  for (int i = 0; i < s->numSubjects; i++)
    printf("[S%d:%.1f] ", i+1, s->grades[i]);
  printf("\n");
}


void addRecord(Database* db) {
  Student s;
  memset(&s, 0, sizeof(s));

  s.studentID = readInt("Student ID", 1, 999999);
  if (idExists(db, s.studentID)) {
    printf("Error: ID %d already exists.\n", s.studentID);
    return;
  }

  readString("Name",   s.name,   MAX_NAME);
  readString("Course", s.course, MAX_COURSE);
  s.age = readInt("Age", 16, 80);
  s.numSubjects = readInt("Number of subjects (1-8)", 1, MAX_SUBJECT);

  for (int i = 0; i < s.numSubjects; i++) {
    char prompt[32];
    snprintf(prompt, sizeof(prompt), "  Grade for subject %d (0-100)", i+1);
    s.grades[i] = readFloat(prompt, 0.0f, 100.0f);
  }
  s.gpa = computeGPA(s.grades, s.numSubjects);

  if (db_add(db, s))
    printf("Record added. GPA = %.2f\n", s.gpa);
  else
    printf("Error: could not add record.\n");
}

void displayAll(Database* db) {
  if (db->count == 0) { printf("No records found.\n"); return; }
  printf("\n%-6s  %-20s  %-15s  %-4s  %s\n",
         "ID", "Name", "Course", "Age", "GPA");
  for (int i = 0; i < db->count; i++) {
    printStudent(&db->data[i]);
  }
  printf("Total records: %d\n", db->count);
}

void updateRecord(Database* db) {
  int id = readInt("Enter student ID to update", 1, 999999);
  Student* s = db_findByID(db, id);
  if (!s) { printf("Student ID %d not found.\n", id); return; }

  printf("Updating: "); printStudent(s);
  printf("(Press Enter to keep current value)\n");

  char buf[MAX_NAME];
  printf("New name [%s]: ", s->name);
  fgets(buf, sizeof(buf), stdin);
  buf[strcspn(buf, "\n")] = 0;
  if (strlen(buf) > 0) strncpy(s->name, buf, MAX_NAME-1);

  printf("New course [%s]: ", s->course);
  fgets(buf, sizeof(buf), stdin);
  buf[strcspn(buf, "\n")] = 0;
  if (strlen(buf) > 0) strncpy(s->course, buf, MAX_COURSE-1);

  printf("Update grades? [y/N]: ");
  fgets(buf, sizeof(buf), stdin);
  if (buf[0] == 'y' || buf[0] == 'Y') {
    s->numSubjects = readInt("Number of subjects (1-8)", 1, MAX_SUBJECT);
    for (int i = 0; i < s->numSubjects; i++) {
      char prompt[32];
      snprintf(prompt, sizeof(prompt), "  Grade S%d (0-100)", i+1);
      s->grades[i] = readFloat(prompt, 0.0f, 100.0f);
    }
    s->gpa = computeGPA(s->grades, s->numSubjects);
  }
  printf("Record updated. New GPA = %.2f\n", s->gpa);
}

void deleteRecord(Database* db) {
  int id = readInt("Enter student ID to delete", 1, 999999);
  if (db_delete(db, id))
    printf("Student %d deleted.\n", id);
  else
    printf("Student ID %d not found.\n", id);
}

void searchByID(Database* db) {
  int id = readInt("Enter student ID", 1, 999999);
  Student* s = db_findByID(db, id);
  if (s) { printf("Found:\n"); printStudent(s); }
  else   printf("Student ID %d not found.\n", id);
}

void searchByName(Database* db) {
  char query[MAX_NAME];
  readString("Enter name (or partial name)", query, MAX_NAME);

  // Convert to lowercase for case-insensitive search
  char lq[MAX_NAME];
  for (int i = 0; query[i]; i++) lq[i] = tolower((unsigned char)query[i]);
  lq[strlen(query)] = 0;

  int found = 0;
  for (int i = 0; i < db->count; i++) {
    char ln[MAX_NAME];
    for (int j = 0; db->data[i].name[j]; j++)
      ln[j] = tolower((unsigned char)db->data[i].name[j]);
    ln[strlen(db->data[i].name)] = 0;

    if (strstr(ln, lq)) {
      printStudent(&db->data[i]);
      found++;
    }
  }
  if (!found) printf("No match found for '%s'.\n", query);
  else printf("%d result(s).\n", found);
}

// Insertion sort by GPA descending
void sortByGPA(Database* db) {
  for (int i = 1; i < db->count; i++) {
    Student key = db->data[i];
    int j = i - 1;
    while (j >= 0 && db->data[j].gpa < key.gpa) {
      db->data[j+1] = db->data[j];
      j--;
    }
    db->data[j+1] = key;
  }
  printf("Sorted by GPA (descending):\n");
  displayAll(db);
}


// Bubble sort by name ascending
void sortByName(Database* db) {
  for (int i = 0; i < db->count - 1; i++)
    for (int j = 0; j < db->count - 1 - i; j++)
      if (strcmp(db->data[j].name, db->data[j+1].name) > 0) {
        Student tmp    = db->data[j];
        db->data[j]   = db->data[j+1];
        db->data[j+1] = tmp;
      }
  printf("Sorted by name (A-Z):\n");
  displayAll(db);
}

// Selection sort by ID ascending
void sortByID(Database* db) {
  for (int i = 0; i < db->count - 1; i++) {
    int minIdx = i;
    for (int j = i+1; j < db->count; j++)
      if (db->data[j].studentID < db->data[minIdx].studentID) minIdx = j;
    if (minIdx != i) {
      Student tmp        = db->data[i];
      db->data[i]        = db->data[minIdx];
      db->data[minIdx]   = tmp;
    }
  }
  printf("Sorted by ID (ascending):\n");
  displayAll(db);
}

void reportAverageGPA(Database* db) {
  if (db->count == 0) { printf("No records.\n"); return; }
  float sum = 0;
  for (int i = 0; i < db->count; i++) sum += db->data[i].gpa;
  printf("Class average GPA: %.2f  (over %d students)\n", sum/db->count, db->count);
}

void reportMinMax(Database* db) {
  if (db->count == 0) { printf("No records.\n"); return; }
  int maxIdx = 0, minIdx = 0;
  for (int i = 1; i < db->count; i++) {
    if (db->data[i].gpa > db->data[maxIdx].gpa) maxIdx = i;
    if (db->data[i].gpa < db->data[minIdx].gpa) minIdx = i;
  }
  printf("Highest GPA: %.2f  — %s (ID %d)\n",
         db->data[maxIdx].gpa, db->data[maxIdx].name, db->data[maxIdx].studentID);
  printf("Lowest  GPA: %.2f  — %s (ID %d)\n",
         db->data[minIdx].gpa, db->data[minIdx].name, db->data[minIdx].studentID);
}

void reportMedian(Database* db) {
  if (db->count == 0) { printf("No records.\n"); return; }
  // Copy GPAs and sort
  float* gpas = (float*)malloc(db->count * sizeof(float));
  if (!gpas) { fprintf(stderr, "Allocation failed.\n"); return; }
  for (int i = 0; i < db->count; i++) gpas[i] = db->data[i].gpa;
  // Simple bubble sort on copy
  for (int i = 0; i < db->count - 1; i++)
    for (int j = 0; j < db->count - 1 - i; j++)
      if (gpas[j] > gpas[j+1]) { float t = gpas[j]; gpas[j] = gpas[j+1]; gpas[j+1] = t; }

  float median;
  if (db->count % 2 == 0)
    median = (gpas[db->count/2 - 1] + gpas[db->count/2]) / 2.0f;
  else
    median = gpas[db->count/2];

  printf("Median GPA: %.2f\n", median);
  free(gpas);
}

void reportTopN(Database* db) {
  if (db->count == 0) { printf("No records.\n"); return; }
  int n = readInt("How many top students to display", 1, db->count);
  // Insertion sort descending into a copy
  Student* copy = (Student*)malloc(db->count * sizeof(Student));
  if (!copy) { fprintf(stderr, "Allocation failed.\n"); return; }
  memcpy(copy, db->data, db->count * sizeof(Student));
  for (int i = 1; i < db->count; i++) {
    Student key = copy[i]; int j = i-1;
    while (j >= 0 && copy[j].gpa < key.gpa) { copy[j+1] = copy[j]; j--; }
    copy[j+1] = key;
  }
  printf("Top %d students:\n", n);
  for (int i = 0; i < n; i++) {
    printf("  #%d  ", i+1);
    printStudent(&copy[i]);
  }
  free(copy);
}

void reportCourseStats(Database* db) {
  if (db->count == 0) { printf("No records.\n"); return; }
  // Collect unique course names
  char courses[64][MAX_COURSE];
  int  numCourses = 0;
  for (int i = 0; i < db->count; i++) {
    int found = 0;
    for (int c = 0; c < numCourses; c++)
      if (strcmp(courses[c], db->data[i].course) == 0) { found = 1; break; }
    if (!found && numCourses < 64)
      strncpy(courses[numCourses++], db->data[i].course, MAX_COURSE-1);
  }
  for (int c = 0; c < numCourses; c++) {
    float sum = 0; int cnt = 0; float best = -1;
    char  bestName[MAX_NAME] = "";
    for (int i = 0; i < db->count; i++) {
      if (strcmp(db->data[i].course, courses[c]) == 0) {
        sum += db->data[i].gpa; cnt++;
        if (db->data[i].gpa > best) { best = db->data[i].gpa; strncpy(bestName, db->data[i].name, MAX_NAME-1); }
      }
    }
    printf("Course: %-20s  Students: %-3d  Avg GPA: %.2f  Top: %s (%.2f)\n",
           courses[c], cnt, sum/cnt, bestName, best);
  }
}


void saveToFile(Database* db) {
  FILE* f = fopen(DATA_FILE, "wb");
  if (!f) { perror("Cannot open file for writing"); return; }
  fwrite(&db->count, sizeof(int), 1, f);
  fwrite(db->data, sizeof(Student), db->count, f);
  fclose(f);
  printf("Saved %d records to %s\n", db->count, DATA_FILE);
}

void loadFromFile(Database* db) {
  FILE* f = fopen(DATA_FILE, "rb");
  if (!f) { printf("No existing data file found. Starting fresh.\n"); return; }
  int count;
  if (fread(&count, sizeof(int), 1, f) != 1 || count < 0 || count > 100000) {
    printf("File integrity error. Starting fresh.\n");
    fclose(f); return;
  }
  for (int i = 0; i < count; i++) {
    Student s;
    if (fread(&s, sizeof(Student), 1, f) != 1) { printf("Read error at record %d.\n", i); break; }
    db_add(db, s);
  }
  fclose(f);
  printf("Loaded %d records from %s\n", db->count, DATA_FILE);
}

void clearInput(void) {
  int c; while ((c = getchar()) != '\n' && c != EOF) {}
}

int readInt(const char* prompt, int lo, int hi) {
  int val; char buf[64];
  while (1) {
    printf("%s (%d-%d): ", prompt, lo, hi);
    if (!fgets(buf, sizeof(buf), stdin)) continue;
    if (sscanf(buf, "%d", &val) == 1 && val >= lo && val <= hi) return val;
    printf("  Invalid input. Enter an integer between %d and %d.\n", lo, hi);
  }
}

float readFloat(const char* prompt, float lo, float hi) {
  float val; char buf[64];
  while (1) {
    printf("%s (%.1f-%.1f): ", prompt, lo, hi);
    if (!fgets(buf, sizeof(buf), stdin)) continue;
    if (sscanf(buf, "%f", &val) == 1 && val >= lo && val <= hi) return val;
    printf("  Invalid input. Enter a number between %.1f and %.1f.\n", lo, hi);
  }
}

void readString(const char* prompt, char* buf, int maxLen) {
  while (1) {
    printf("%s: ", prompt);
    if (fgets(buf, maxLen, stdin)) {
      buf[strcspn(buf, "\n")] = 0;
      if (strlen(buf) > 0) return;
    }
    printf("  Input cannot be empty.\n");
  }
}

int idExists(Database* db, int id) {
  return db_findByID(db, id) != NULL;
}
