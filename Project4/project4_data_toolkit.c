/**
 * Project 4: Data Analysis Toolkit Using Function Pointers and Callbacks
 * =======================================================================
 * Features:
 *   - Dynamic function dispatch via function pointer array (no long if/else)
 *   - Callback-based filtering, transformation, and sorting
 *   - Dynamic dataset with malloc/realloc/free
 *   - File load/save
 *   - Full input validation and error handling
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

// ─── Dataset ──────────────────────────────────────────────────────────────────
typedef struct {
  double* values;
  int     size;
  int     capacity;
} Dataset;

// ─── Callback Types ───────────────────────────────────────────────────────────
typedef int    (*FilterFn)  (double value, double param);
typedef double (*TransformFn)(double value, double param);
typedef int    (*CompareFn) (const void* a, const void* b);

// ─── Function Pointer Dispatcher Entry ───────────────────────────────────────
typedef void (*OperationFn)(Dataset*);

typedef struct {
  const char* label;
  OperationFn fn;
} MenuItem;

// ─── Prototypes ───────────────────────────────────────────────────────────────
// Dataset lifecycle
void   ds_init(Dataset* ds);
int    ds_append(Dataset* ds, double val);
void   ds_reset(Dataset* ds);
void   ds_free(Dataset* ds);
void   ds_print(Dataset* ds);

// Operations (all match OperationFn signature)
void op_computeSumAvg  (Dataset* ds);
void op_findMinMax     (Dataset* ds);
void op_filter         (Dataset* ds);
void op_transform      (Dataset* ds);
void op_sort           (Dataset* ds);
void op_search         (Dataset* ds);
void op_loadFile       (Dataset* ds);
void op_saveFile       (Dataset* ds);
void op_resetDataset   (Dataset* ds);
void op_displayDataset (Dataset* ds);

// Callbacks
int    filter_above    (double v, double p);
int    filter_below    (double v, double p);
int    filter_equal    (double v, double p);
double transform_scale (double v, double p);
double transform_shift (double v, double p);
double transform_square(double v, double p);
int    cmp_asc         (const void* a, const void* b);
int    cmp_desc        (const void* a, const void* b);

// Helpers
int    readInt   (const char* p, int lo, int hi);
double readDouble(const char* p);
void   clearInput(void);

// ─── Dispatcher Table ─────────────────────────────────────────────────────────
//   Each menu option maps directly to a function pointer.
//   Adding a new operation = add one line here. No switch/if needed.
static MenuItem menu[] = {
  { "Load dataset from file",          op_loadFile       },
  { "Display dataset",                 op_displayDataset },
  { "Compute sum & average",           op_computeSumAvg  },
  { "Find minimum & maximum",          op_findMinMax     },
  { "Filter dataset",                  op_filter         },
  { "Apply transformation",            op_transform      },
  { "Sort dataset",                    op_sort           },
  { "Search for a value",              op_search         },
  { "Save dataset to file",            op_saveFile       },
  { "Reset / clear dataset",           op_resetDataset   },
};
#define MENU_SIZE ((int)(sizeof(menu)/sizeof(menu[0])))

// ─── Main ─────────────────────────────────────────────────────────────────────
int main(void) {
  Dataset ds;
  ds_init(&ds);

  int choice;
  do {
    printf("\n╔══════════════════════════════════════╗\n");
    printf("║    Data Analysis Toolkit             ║\n");
    printf("║    Dataset size: %-6d              ║\n", ds.size);
    printf("╠══════════════════════════════════════╣\n");
    for (int i = 0; i < MENU_SIZE; i++)
      printf("║  %2d) %-33s║\n", i+1, menu[i].label);
    printf("║   0) Exit                            ║\n");
    printf("╚══════════════════════════════════════╝\n");

    choice = readInt("Choice", 0, MENU_SIZE);
    if (choice == 0) break;

    // ─── Dynamic Dispatch ────────────────────────────────────────────────────
    // No long if/else chain — the function pointer does the work directly.
    OperationFn op = menu[choice - 1].fn;
    if (op != NULL)
      op(&ds);
    else
      printf("Operation not implemented.\n");

  } while (choice != 0);

  printf("Freeing memory and exiting.\n");
  ds_free(&ds);
  return 0;
}

// ─── Dataset Lifecycle ────────────────────────────────────────────────────────
void ds_init(Dataset* ds) {
  ds->capacity = 16;
  ds->size     = 0;
  ds->values   = (double*)malloc(ds->capacity * sizeof(double));
  if (!ds->values) { fprintf(stderr, "Allocation failed.\n"); exit(1); }
}

int ds_append(Dataset* ds, double val) {
  if (ds->size == ds->capacity) {
    ds->capacity *= 2;
    double* tmp = (double*)realloc(ds->values, ds->capacity * sizeof(double));
    if (!tmp) { fprintf(stderr, "realloc failed.\n"); return 0; }
    ds->values = tmp;
  }
  ds->values[ds->size++] = val;
  return 1;
}

void ds_reset(Dataset* ds) {
  ds->size = 0;
}

void ds_free(Dataset* ds) {
  free(ds->values);
  ds->values = NULL;
  ds->size = ds->capacity = 0;
}

void ds_print(Dataset* ds) {
  if (ds->size == 0) { printf("  (dataset is empty)\n"); return; }
  printf("  [");
  for (int i = 0; i < ds->size; i++) {
    printf("%.4g", ds->values[i]);
    if (i < ds->size - 1) printf(", ");
  }
  printf("]\n  Size: %d\n", ds->size);
}

// ─── Operation Implementations ────────────────────────────────────────────────

void op_displayDataset(Dataset* ds) {
  printf("\nDataset:\n");
  ds_print(ds);
}

void op_computeSumAvg(Dataset* ds) {
  if (ds->size == 0) { printf("Dataset is empty.\n"); return; }
  double sum = 0;
  for (int i = 0; i < ds->size; i++) sum += ds->values[i];
  printf("  Sum:     %.6g\n", sum);
  printf("  Average: %.6g\n", sum / ds->size);
}

void op_findMinMax(Dataset* ds) {
  if (ds->size == 0) { printf("Dataset is empty.\n"); return; }
  double mn = ds->values[0], mx = ds->values[0];
  int    mnIdx = 0, mxIdx = 0;
  for (int i = 1; i < ds->size; i++) {
    if (ds->values[i] < mn) { mn = ds->values[i]; mnIdx = i; }
    if (ds->values[i] > mx) { mx = ds->values[i]; mxIdx = i; }
  }
  printf("  Minimum: %.6g  (index %d)\n", mn, mnIdx);
  printf("  Maximum: %.6g  (index %d)\n", mx, mxIdx);
}

// ─── Filter (callback-based) ─────────────────────────────────────────────────
void op_filter(Dataset* ds) {
  if (ds->size == 0) { printf("Dataset is empty.\n"); return; }

  printf("  Filter type:\n");
  printf("    1) Values above threshold\n");
  printf("    2) Values below threshold\n");
  printf("    3) Values equal to threshold\n");
  int type = readInt("  Choice", 1, 3);

  // Select callback using array of function pointers
  FilterFn filters[] = { filter_above, filter_below, filter_equal };
  FilterFn  fn = filters[type - 1];

  printf("  Threshold value: ");
  double threshold = readDouble("");

  int count = 0;
  printf("  Filtered results:\n  [");
  for (int i = 0; i < ds->size; i++) {
    if (fn(ds->values[i], threshold)) {
      if (count > 0) printf(", ");
      printf("%.4g", ds->values[i]);
      count++;
    }
  }
  printf("]\n  %d value(s) matched.\n", count);
}

int filter_above(double v, double p) { return v >  p; }
int filter_below(double v, double p) { return v <  p; }
int filter_equal(double v, double p) { return fabs(v - p) < 1e-9; }

// ─── Transform (callback-based) ──────────────────────────────────────────────
void op_transform(Dataset* ds) {
  if (ds->size == 0) { printf("Dataset is empty.\n"); return; }

  printf("  Transformation:\n");
  printf("    1) Scale  (multiply by factor)\n");
  printf("    2) Shift  (add offset)\n");
  printf("    3) Square each element\n");
  int type = readInt("  Choice", 1, 3);

  TransformFn transforms[] = { transform_scale, transform_shift, transform_square };
  TransformFn fn = transforms[type - 1];

  double param = 0;
  if (type != 3) {
    printf("  Parameter value: ");
    param = readDouble("");
  }

  // Apply transformation in-place
  for (int i = 0; i < ds->size; i++)
    ds->values[i] = fn(ds->values[i], param);

  printf("  Transformation applied.\n");
  ds_print(ds);
}

double transform_scale (double v, double p) { return v * p; }
double transform_shift (double v, double p) { return v + p; }
double transform_square(double v, double p) { (void)p; return v * v; }

// ─── Sort (comparison function pointer via qsort) ─────────────────────────────
void op_sort(Dataset* ds) {
  if (ds->size == 0) { printf("Dataset is empty.\n"); return; }

  printf("  Sort order:\n    1) Ascending\n    2) Descending\n");
  int order = readInt("  Choice", 1, 2);

  CompareFn comparators[] = { cmp_asc, cmp_desc };
  qsort(ds->values, ds->size, sizeof(double), comparators[order - 1]);

  printf("  Sorted (%s):\n", order == 1 ? "ascending" : "descending");
  ds_print(ds);
}

int cmp_asc (const void* a, const void* b) {
  double da = *(double*)a, db = *(double*)b;
  return (da > db) - (da < db);
}
int cmp_desc(const void* a, const void* b) {
  return cmp_asc(b, a);
}

// ─── Search ───────────────────────────────────────────────────────────────────
void op_search(Dataset* ds) {
  if (ds->size == 0) { printf("Dataset is empty.\n"); return; }
  printf("  Search value: ");
  double target = readDouble("");

  int found = 0;
  printf("  Found at indices: ");
  for (int i = 0; i < ds->size; i++) {
    if (fabs(ds->values[i] - target) < 1e-9) {
      printf("%d ", i);
      found++;
    }
  }
  if (!found) printf("(not found)");
  printf("\n  Occurrences: %d\n", found);
}

// ─── File Operations ─────────────────────────────────────────────────────────
void op_loadFile(Dataset* ds) {
  char path[256];
  printf("  File path: ");
  if (!fgets(path, sizeof(path), stdin)) return;
  path[strcspn(path, "\n")] = 0;

  FILE* f = fopen(path, "r");
  if (!f) { perror("Cannot open file"); return; }

  ds_reset(ds);
  double val; int cnt = 0;
  while (fscanf(f, "%lf", &val) == 1) {
    if (!ds_append(ds, val)) { printf("Memory error at value %d.\n", cnt); break; }
    cnt++;
  }
  fclose(f);
  printf("  Loaded %d values from '%s'.\n", cnt, path);
}

void op_saveFile(Dataset* ds) {
  if (ds->size == 0) { printf("Dataset is empty.\n"); return; }
  char path[256];
  printf("  Save to file path: ");
  if (!fgets(path, sizeof(path), stdin)) return;
  path[strcspn(path, "\n")] = 0;

  FILE* f = fopen(path, "w");
  if (!f) { perror("Cannot open file for writing"); return; }
  for (int i = 0; i < ds->size; i++)
    fprintf(f, "%.10g\n", ds->values[i]);
  fclose(f);
  printf("  Saved %d values to '%s'.\n", ds->size, path);
}

void op_resetDataset(Dataset* ds) {
  ds_reset(ds);
  printf("  Dataset cleared. You can also load new data from a file.\n");

  // Allow quick manual entry
  printf("  Enter values one per line (blank line to finish):\n");
  char buf[64];
  while (1) {
    printf("  > ");
    if (!fgets(buf, sizeof(buf), stdin)) break;
    if (buf[0] == '\n') break;
    double v;
    if (sscanf(buf, "%lf", &v) == 1) ds_append(ds, v);
    else printf("  (skipped non-numeric input)\n");
  }
  printf("  Dataset now has %d values.\n", ds->size);
}

// ─── Input Helpers ────────────────────────────────────────────────────────────
void clearInput(void) {
  int c; while ((c = getchar()) != '\n' && c != EOF) {}
}

int readInt(const char* p, int lo, int hi) {
  int val; char buf[64];
  while (1) {
    printf("%s (%d-%d): ", p, lo, hi);
    if (!fgets(buf, sizeof(buf), stdin)) continue;
    if (sscanf(buf, "%d", &val) == 1 && val >= lo && val <= hi) return val;
    printf("  Invalid. Enter integer %d–%d.\n", lo, hi);
  }
}

double readDouble(const char* p) {
  double val; char buf[64];
  while (1) {
    if (strlen(p) > 0) printf("%s: ", p);
    if (!fgets(buf, sizeof(buf), stdin)) continue;
    if (sscanf(buf, "%lf", &val) == 1) return val;
    printf("  Invalid. Enter a number.\n");
  }
}
