/**
 * Project 5: Multi-threaded Web Scraper
 * ======================================
 * Features:
 *   - Fetches multiple URLs in parallel using POSIX pthreads
 *   - Each thread writes its output to a separate file
 *   - No thread synchronization needed (each thread is independent)
 *   - Graceful error handling for unreachable URLs
 *   - Uses libcurl for HTTP requests
 *
 * Compile:
 *   gcc -o scraper scraper.c -lpthread -lcurl
 *
 * Usage:
 *   ./scraper urls.txt
 *   (one URL per line in urls.txt)
 *
 * Or edit the DEFAULT_URLS array below and run without arguments.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <curl/curl.h>
#include <errno.h>
#include <sys/stat.h>

// ─── Configuration ────────────────────────────────────────────────────────────
#define MAX_URLS       64
#define MAX_URL_LEN    512
#define OUTPUT_DIR     "scraped_output"
#define CONNECT_TIMEOUT 10L   // seconds
#define TRANSFER_TIMEOUT 30L  // seconds

// ─── Default URLs (used when no file is provided) ─────────────────────────────
static const char* DEFAULT_URLS[] = {
  "https://httpbin.org/html",
  "https://httpbin.org/json",
  "https://httpbin.org/xml",
  "https://example.com",
  NULL
};

// ─── Structures ───────────────────────────────────────────────────────────────

// Buffer for libcurl's write callback
typedef struct {
  char*  data;
  size_t size;
} Buffer;

// Per-thread arguments — each thread gets its own copy
typedef struct {
  int   threadID;
  char  url[MAX_URL_LEN];
  int   success;      // 1 on success, 0 on error
  char  outputFile[256];
  char  errorMsg[256];
} ThreadArgs;

// ─── Prototypes ───────────────────────────────────────────────────────────────
void*  scrape_thread(void* arg);
size_t curl_write_callback(void* contents, size_t size, size_t nmemb, void* userp);
int    load_urls_from_file(const char* path, char urls[][MAX_URL_LEN], int maxUrls);
void   sanitize_filename(const char* url, char* out, int maxLen);
void   print_summary(ThreadArgs* results, int count);
int    ensure_output_dir(void);

// ─── Main ─────────────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
  printf("=== Multi-threaded Web Scraper ===\n");

  // Initialise libcurl globally (once, before any threads)
  if (curl_global_init(CURL_GLOBAL_ALL) != 0) {
    fprintf(stderr, "Failed to initialise libcurl.\n");
    return 1;
  }

  // Ensure output directory exists
  if (!ensure_output_dir()) {
    fprintf(stderr, "Cannot create output directory '%s'.\n", OUTPUT_DIR);
    curl_global_cleanup();
    return 1;
  }

  // ─── Build URL list ───────────────────────────────────────────────────────
  char   urls[MAX_URLS][MAX_URL_LEN];
  int    numURLs = 0;

  if (argc >= 2) {
    numURLs = load_urls_from_file(argv[1], urls, MAX_URLS);
    if (numURLs <= 0) {
      fprintf(stderr, "No valid URLs found in '%s'.\n", argv[1]);
      curl_global_cleanup();
      return 1;
    }
  } else {
    // Use default URLs
    for (int i = 0; DEFAULT_URLS[i] != NULL && i < MAX_URLS; i++) {
      strncpy(urls[numURLs++], DEFAULT_URLS[i], MAX_URL_LEN - 1);
    }
    printf("No URL file specified — using %d built-in test URLs.\n", numURLs);
  }

  printf("Scraping %d URL(s) in parallel...\n\n", numURLs);

  // ─── Allocate thread handles and argument structs ─────────────────────────
  pthread_t*  threads = (pthread_t*)malloc(numURLs * sizeof(pthread_t));
  ThreadArgs* args    = (ThreadArgs*)calloc(numURLs, sizeof(ThreadArgs));
  if (!threads || !args) {
    fprintf(stderr, "Memory allocation failed.\n");
    free(threads); free(args);
    curl_global_cleanup();
    return 1;
  }

  // ─── Launch one thread per URL ────────────────────────────────────────────
  for (int i = 0; i < numURLs; i++) {
    args[i].threadID = i + 1;
    strncpy(args[i].url, urls[i], MAX_URL_LEN - 1);

    // Build output filename from URL
    char base[200];
    sanitize_filename(urls[i], base, sizeof(base));
    snprintf(args[i].outputFile, sizeof(args[i].outputFile),
             "%s/thread_%02d_%s.html", OUTPUT_DIR, i+1, base);

    int rc = pthread_create(&threads[i], NULL, scrape_thread, &args[i]);
    if (rc != 0) {
      fprintf(stderr, "[Thread %d] Failed to create thread: %s\n",
              i+1, strerror(rc));
      args[i].success = 0;
      snprintf(args[i].errorMsg, sizeof(args[i].errorMsg),
               "pthread_create failed: %s", strerror(rc));
      threads[i] = 0;
    }
  }

  // ─── Wait for all threads to complete ────────────────────────────────────
  for (int i = 0; i < numURLs; i++) {
    if (threads[i] != 0) {
      int rc = pthread_join(threads[i], NULL);
      if (rc != 0)
        fprintf(stderr, "[Thread %d] pthread_join error: %s\n", i+1, strerror(rc));
    }
  }

  // ─── Print summary ────────────────────────────────────────────────────────
  print_summary(args, numURLs);

  free(threads);
  free(args);
  curl_global_cleanup();
  return 0;
}

// ─── Thread Function ──────────────────────────────────────────────────────────
// Each thread is fully independent — no shared data, no mutex needed.
void* scrape_thread(void* arg) {
  ThreadArgs* targs = (ThreadArgs*)arg;
  printf("[Thread %d] Starting: %s\n", targs->threadID, targs->url);

  // Allocate response buffer
  Buffer buf;
  buf.data = (char*)malloc(1);
  buf.size = 0;
  if (!buf.data) {
    snprintf(targs->errorMsg, sizeof(targs->errorMsg), "Buffer malloc failed");
    targs->success = 0;
    return NULL;
  }
  buf.data[0] = '\0';

  // Set up CURL handle (each thread uses its own handle — thread-safe)
  CURL* curl = curl_easy_init();
  if (!curl) {
    snprintf(targs->errorMsg, sizeof(targs->errorMsg), "curl_easy_init failed");
    targs->success = 0;
    free(buf.data);
    return NULL;
  }

  curl_easy_setopt(curl, CURLOPT_URL,            targs->url);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,  curl_write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA,      &buf);
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, CONNECT_TIMEOUT);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT,        TRANSFER_TIMEOUT);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);   // follow redirects
  curl_easy_setopt(curl, CURLOPT_USERAGENT,      "multi-thread-scraper/1.0");

  CURLcode res = curl_easy_perform(curl);

  if (res != CURLE_OK) {
    snprintf(targs->errorMsg, sizeof(targs->errorMsg),
             "curl error: %s", curl_easy_strerror(res));
    targs->success = 0;
    printf("[Thread %d] FAILED: %s\n", targs->threadID, targs->errorMsg);
  } else {
    // Write content to per-thread output file
    FILE* f = fopen(targs->outputFile, "w");
    if (!f) {
      snprintf(targs->errorMsg, sizeof(targs->errorMsg),
               "Cannot open output file: %s", targs->outputFile);
      targs->success = 0;
    } else {
      // Write metadata header
      fprintf(f, "<!-- Scraped by Thread %d -->\n", targs->threadID);
      fprintf(f, "<!-- URL: %s -->\n", targs->url);
      fprintf(f, "<!-- Bytes: %zu -->\n\n", buf.size);
      fwrite(buf.data, 1, buf.size, f);
      fclose(f);
      targs->success = 1;
      printf("[Thread %d] Done: %zu bytes -> %s\n",
             targs->threadID, buf.size, targs->outputFile);
    }
  }

  curl_easy_cleanup(curl);
  free(buf.data);
  return NULL;
}

// ─── libcurl Write Callback ───────────────────────────────────────────────────
// Called by libcurl as data arrives; appends to the Buffer.
size_t curl_write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
  size_t   realsize = size * nmemb;
  Buffer*  buf      = (Buffer*)userp;

  char* ptr = (char*)realloc(buf->data, buf->size + realsize + 1);
  if (!ptr) {
    fprintf(stderr, "realloc failed in write callback.\n");
    return 0;   // Signal error to libcurl
  }
  buf->data = ptr;
  memcpy(buf->data + buf->size, contents, realsize);
  buf->size             += realsize;
  buf->data[buf->size]   = '\0';
  return realsize;
}

// ─── Load URLs from File ──────────────────────────────────────────────────────
int load_urls_from_file(const char* path, char urls[][MAX_URL_LEN], int maxUrls) {
  FILE* f = fopen(path, "r");
  if (!f) { perror(path); return -1; }

  int count = 0;
  char line[MAX_URL_LEN];
  while (count < maxUrls && fgets(line, sizeof(line), f)) {
    // Strip newline and trailing whitespace
    line[strcspn(line, "\r\n")] = 0;
    // Skip empty lines and comments
    if (strlen(line) == 0 || line[0] == '#') continue;
    strncpy(urls[count++], line, MAX_URL_LEN - 1);
  }
  fclose(f);
  return count;
}

// ─── Sanitize URL to Safe Filename ───────────────────────────────────────────
void sanitize_filename(const char* url, char* out, int maxLen) {
  int j = 0;
  // Skip protocol (http:// / https://)
  const char* start = url;
  if (strncmp(url, "https://", 8) == 0) start += 8;
  else if (strncmp(url, "http://",  7) == 0) start += 7;

  for (int i = 0; start[i] && j < maxLen - 1; i++) {
    char c = start[i];
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') || c == '-' || c == '_') {
      out[j++] = c;
    } else {
      out[j++] = '_';
    }
  }
  out[j] = '\0';
  if (j == 0) { strncpy(out, "url", maxLen-1); }
}

// ─── Print Summary Table ─────────────────────────────────────────────────────
void print_summary(ThreadArgs* results, int count) {
  int ok = 0, fail = 0;
  printf("\n╔════╦════════════════════════════════════════════╦═══════╗\n");
  printf("║ ID ║ URL                                        ║ Status║\n");
  printf("╠════╬════════════════════════════════════════════╬═══════╣\n");
  for (int i = 0; i < count; i++) {
    printf("║ %2d ║ %-42.42s ║ %-6s║\n",
           results[i].threadID,
           results[i].url,
           results[i].success ? "  OK  " : " FAIL ");
    if (results[i].success) ok++;
    else { fail++; printf("║    ║   Error: %-38.38s ║       ║\n", results[i].errorMsg); }
  }
  printf("╚════╩════════════════════════════════════════════╩═══════╝\n");
  printf("Results: %d succeeded, %d failed. Output in '%s/'\n", ok, fail, OUTPUT_DIR);
}

// ─── Create Output Directory ─────────────────────────────────────────────────
int ensure_output_dir(void) {
  struct stat st;
  if (stat(OUTPUT_DIR, &st) == 0) return 1;  // already exists
#ifdef _WIN32
  return mkdir(OUTPUT_DIR) == 0 || errno == EEXIST;
#else
  return mkdir(OUTPUT_DIR, 0755) == 0 || errno == EEXIST;
#endif
}
