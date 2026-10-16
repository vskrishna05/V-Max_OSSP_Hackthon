#ifndef SEARCH_H
#define SEARCH_H

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <time.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fnmatch.h>

#define MAX_PATH_LEN 4096
#define MAX_THREADS 32

/* Clean ANSI Color Palette */
#define CLR_RESET   "\033[0m"
#define CLR_BOLD    "\033[1m"
#define CLR_DIM     "\033[2m"
#define CLR_CYAN    "\033[36m"
#define CLR_GREEN   "\033[32m"
#define CLR_YELLOW  "\033[33m"
#define CLR_RED     "\033[31m"
#define CLR_BLUE    "\033[34m"
#define CLR_MAGENTA "\033[35m"
#define CLR_GRAY    "\033[90m"

/* Monotonic High-Precision Timer */
static inline double get_time_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

/* System Hardware CPU Cores Detection */
static inline int get_cpu_cores(void) {
    long n = sysconf(_SC_NPROCESSORS_ONLN);
    if (n < 1) return 4;
    if (n > MAX_THREADS) return MAX_THREADS;
    return (int)n;
}

/* Format Bytes into Human-Readable String */
static inline void format_bytes(uint64_t bytes, char *buf, size_t len) {
    if (bytes < 1024ULL) {
        snprintf(buf, len, "%llu B", (unsigned long long)bytes);
    } else if (bytes < 1024ULL * 1024ULL) {
        snprintf(buf, len, "%.2f KB", (double)bytes / 1024.0);
    } else if (bytes < 1024ULL * 1024ULL * 1024ULL) {
        snprintf(buf, len, "%.2f MB", (double)bytes / (1024.0 * 1024.0));
    } else {
        snprintf(buf, len, "%.2f GB", (double)bytes / (1024.0 * 1024.0 * 1024.0));
    }
}

/* Safe Path Join Helper (Avoids double slashes "//" and truncation warnings) */
static inline void join_path(char *out, size_t out_sz, const char *dir, const char *name) {
    if (!out || out_sz == 0) return;
    out[0] = '\0';
    if (!dir) dir = ".";
    if (!name) name = "";

    size_t dlen = strlen(dir);
    if (dlen == 0) {
        snprintf(out, out_sz, "%.4000s", name);
        return;
    }

    bool has_slash = (dir[dlen - 1] == '/');
    if (has_slash) {
        snprintf(out, out_sz, "%.3500s%.500s", dir, name);
    } else {
        snprintf(out, out_sz, "%.3500s/%.500s", dir, name);
    }
}

/* Windows Path Normalization (e.g. "C:\Users\..." -> /mnt/c/Users/...) */
static inline void normalize_path(const char *input, char *output, size_t out_len) {
    if (!input || !output || out_len == 0) return;

    /* Trim leading spaces, tabs, and quotes */
    while (*input == ' ' || *input == '\t' || *input == '"' || *input == '\'') input++;

    /* Copy and trim trailing quotes and whitespace */
    char temp[MAX_PATH_LEN];
    snprintf(temp, sizeof(temp), "%.4000s", input);
    size_t len = strlen(temp);
    while (len > 0 && (temp[len - 1] == ' ' || temp[len - 1] == '\t' ||
                       temp[len - 1] == '\r' || temp[len - 1] == '\n' ||
                       temp[len - 1] == '"' || temp[len - 1] == '\'')) {
        temp[--len] = '\0';
    }

    /* Check for Windows drive letter: C:\ or c:\ or D:\ */
    if (isalpha((unsigned char)temp[0]) && temp[1] == ':') {
        char drive = (char)tolower((unsigned char)temp[0]);
        const char *rest = temp + 2;
        while (*rest == '/' || *rest == '\\') rest++;

        char converted[3500];
        size_t d = 0;
        for (size_t s = 0; rest[s] != '\0' && d + 1 < sizeof(converted); s++) {
            converted[d++] = (rest[s] == '\\') ? '/' : rest[s];
        }
        converted[d] = '\0';

        snprintf(output, out_len, "/mnt/%c/%.3500s", drive, converted);
        return;
    }

    /* Convert backslashes to forward slashes */
    size_t d = 0;
    for (size_t s = 0; temp[s] != '\0' && d + 1 < out_len; s++) {
        output[d++] = (temp[s] == '\\') ? '/' : temp[s];
    }
    output[d] = '\0';

    /* Remove trailing slash if not root */
    size_t out_l = strlen(output);
    if (out_l > 1 && output[out_l - 1] == '/') {
        output[out_l - 1] = '\0';
    }
}

/* Detect Windows User Profile Directory */
static inline void get_windows_user_path(char *out_path, size_t out_len) {
    struct stat check_st;
    /* 1. Direct check for known user profile */
    if (stat("/mnt/c/Users/vsktu/Desktop", &check_st) == 0 && S_ISDIR(check_st.st_mode)) {
        snprintf(out_path, out_len, "/mnt/c/Users/vsktu");
        return;
    }

    /* 2. Enumerate /mnt/c/Users dynamically */
    DIR *dir = opendir("/mnt/c/Users");
    if (dir) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_name[0] == '.') continue;
            if (strcasestr(entry->d_name, "Public") ||
                strcasestr(entry->d_name, "Default") ||
                strcasestr(entry->d_name, "All Users") ||
                strcasestr(entry->d_name, "Codex") ||
                strcasestr(entry->d_name, "ini")) {
                continue;
            }
            char test_desk[MAX_PATH_LEN];
            snprintf(test_desk, sizeof(test_desk), "/mnt/c/Users/%.3500s/Desktop", entry->d_name);
            if (stat(test_desk, &check_st) == 0 && S_ISDIR(check_st.st_mode)) {
                snprintf(out_path, out_len, "/mnt/c/Users/%.3500s", entry->d_name);
                closedir(dir);
                return;
            }
        }
        closedir(dir);
    }
    snprintf(out_path, out_len, "/mnt/c/Users");
}

/* Binary File Check Heuristic */
static inline bool is_binary_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return false;
    unsigned char buf[1024];
    size_t n = fread(buf, 1, sizeof(buf), f);
    fclose(f);
    if (n == 0) return false;
    size_t non_printable = 0;
    for (size_t i = 0; i < n; i++) {
        if (buf[i] == '\0') return true;
        if (buf[i] < 32 && buf[i] != '\t' && buf[i] != '\n' && buf[i] != '\r') {
            non_printable++;
        }
    }
    return (non_printable * 100 / n > 15);
}

/* Inode set to prevent circular symlink loops */
struct inode_entry {
    dev_t dev;
    ino_t ino;
    struct inode_entry *next;
};

/* Dynamic Task Queue */
typedef struct task_node {
    char path[MAX_PATH_LEN];
    struct task_node *next;
} task_node_t;

typedef struct {
    task_node_t *head;
    task_node_t *tail;
    int count;
    int active_workers;
    bool shutdown;
    pthread_mutex_t lock;
    pthread_cond_t has_work;
    pthread_cond_t all_done;
} work_queue_t;

/* Search Engine Modes */
typedef enum {
    SEARCH_FILENAME   = 1,
    SEARCH_CONTENT    = 2,
    SEARCH_EXTENSION  = 3,
    SEARCH_SIZE_GT    = 4,
    SEARCH_SIZE_LT    = 5,
    SEARCH_HIDDEN     = 6
} search_mode_t;

typedef struct {
    char root_dir[MAX_PATH_LEN];
    char query[MAX_PATH_LEN];
    search_mode_t mode;
    int thread_count;
    bool silent;
    uint64_t size_threshold_bytes;
    
    uint64_t files_scanned;
    uint64_t matches_found;
    double elapsed_sec;
    
    work_queue_t queue;
    pthread_mutex_t stats_lock;
    pthread_mutex_t inode_lock;
    struct inode_entry *visited_inodes;
} search_ctx_t;

/* Symlink loop prevention */
static bool check_and_add_inode(search_ctx_t *ctx, dev_t dev, ino_t ino) {
    pthread_mutex_lock(&ctx->inode_lock);
    struct inode_entry *curr = ctx->visited_inodes;
    while (curr) {
        if (curr->dev == dev && curr->ino == ino) {
            pthread_mutex_unlock(&ctx->inode_lock);
            return true; /* already visited */
        }
        curr = curr->next;
    }
    struct inode_entry *entry = (struct inode_entry *)malloc(sizeof(struct inode_entry));
    if (entry) {
        entry->dev = dev;
        entry->ino = ino;
        entry->next = ctx->visited_inodes;
        ctx->visited_inodes = entry;
    }
    pthread_mutex_unlock(&ctx->inode_lock);
    return false;
}

static void free_visited_inodes(search_ctx_t *ctx) {
    struct inode_entry *curr = ctx->visited_inodes;
    while (curr) {
        struct inode_entry *next = curr->next;
        free(curr);
        curr = next;
    }
    ctx->visited_inodes = NULL;
}

/* Queue Operations */
static void queue_push(work_queue_t *q, const char *path) {
    task_node_t *node = (task_node_t *)malloc(sizeof(task_node_t));
    if (!node) return;
    snprintf(node->path, sizeof(node->path), "%.4000s", path);
    node->next = NULL;

    pthread_mutex_lock(&q->lock);
    if (q->tail) {
        q->tail->next = node;
        q->tail = node;
    } else {
        q->head = node;
        q->tail = node;
    }
    q->count++;
    pthread_cond_signal(&q->has_work);
    pthread_mutex_unlock(&q->lock);
}

static void free_queue(work_queue_t *q) {
    task_node_t *curr = q->head;
    while (curr) {
        task_node_t *tmp = curr->next;
        free(curr);
        curr = tmp;
    }
    q->head = NULL;
    q->tail = NULL;
    q->count = 0;
}

/* Search Worker Thread */
static void *search_worker(void *arg) {
    search_ctx_t *ctx = (search_ctx_t *)arg;
    work_queue_t *q = &ctx->queue;

    while (1) {
        pthread_mutex_lock(&q->lock);
        while (q->head == NULL && !q->shutdown) {
            pthread_cond_wait(&q->has_work, &q->lock);
        }

        if (q->shutdown && q->head == NULL) {
            pthread_mutex_unlock(&q->lock);
            break;
        }

        task_node_t *node = q->head;
        q->head = node->next;
        if (!q->head) q->tail = NULL;
        q->count--;
        q->active_workers++;
        pthread_mutex_unlock(&q->lock);

        char current_dir[MAX_PATH_LEN];
        snprintf(current_dir, sizeof(current_dir), "%.4000s", node->path);
        free(node);

        DIR *dir = opendir(current_dir);
        if (dir) {
            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL) {
                if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                    continue;
                }

                /* Skip hidden files/folders unless in SEARCH_HIDDEN mode */
                if (ctx->mode != SEARCH_HIDDEN && entry->d_name[0] == '.') {
                    continue;
                }

                char fullpath[MAX_PATH_LEN];
                join_path(fullpath, sizeof(fullpath), current_dir, entry->d_name);

                struct stat st;
                if (lstat(fullpath, &st) != 0) continue;

                bool is_dir = S_ISDIR(st.st_mode);
                bool is_reg = S_ISREG(st.st_mode);

                /* Safe Symlink Resolution & Circular Loop Prevention */
                if (S_ISLNK(st.st_mode)) {
                    struct stat target_st;
                    if (stat(fullpath, &target_st) == 0) {
                        if (S_ISDIR(target_st.st_mode)) {
                            /* Explore symlink directory safely if target inode is not visited */
                            if (!check_and_add_inode(ctx, target_st.st_dev, target_st.st_ino)) {
                                queue_push(q, fullpath);
                            }
                            /* Fall through to directory name match check */
                            is_dir = true;
                            st = target_st;
                        } else if (S_ISREG(target_st.st_mode)) {
                            is_reg = true;
                            st = target_st;
                        }
                    } else {
                        /* Broken symlink: safely skip */
                        continue;
                    }
                }

                if (is_dir) {
                    if (!S_ISLNK(st.st_mode)) {
                        if (!check_and_add_inode(ctx, st.st_dev, st.st_ino)) {
                            queue_push(q, fullpath);
                        }
                    }

                    /* Match directory name if searching by filename or hidden filter */
                    bool dir_matched = false;
                    if (ctx->mode == SEARCH_FILENAME) {
                        if (strcasestr(entry->d_name, ctx->query) != NULL ||
                            fnmatch(ctx->query, entry->d_name, FNM_CASEFOLD) == 0) {
                            dir_matched = true;
                        }
                    } else if (ctx->mode == SEARCH_HIDDEN) {
                        if (entry->d_name[0] == '.' || strstr(fullpath, "/.") != NULL) {
                            if (ctx->query[0] == '\0' ||
                                strcasestr(entry->d_name, ctx->query) != NULL ||
                                fnmatch(ctx->query, entry->d_name, FNM_CASEFOLD) == 0) {
                                dir_matched = true;
                            }
                        }
                    }

                    if (dir_matched) {
                        pthread_mutex_lock(&ctx->stats_lock);
                        ctx->matches_found++;
                        if (!ctx->silent) {
                            printf("  %s%s%s  (Directory)\n", CLR_CYAN, fullpath, CLR_RESET);
                        }
                        pthread_mutex_unlock(&ctx->stats_lock);
                    }
                } else if (is_reg) {
                    pthread_mutex_lock(&ctx->stats_lock);
                    ctx->files_scanned++;
                    pthread_mutex_unlock(&ctx->stats_lock);

                    bool matched = false;

                    if (ctx->mode == SEARCH_FILENAME) {
                        if (strcasestr(entry->d_name, ctx->query) != NULL ||
                            fnmatch(ctx->query, entry->d_name, FNM_CASEFOLD) == 0) {
                            matched = true;
                        }
                    } else if (ctx->mode == SEARCH_EXTENSION) {
                        size_t nlen = strlen(entry->d_name);
                        const char *ext_q = ctx->query;
                        if (ext_q[0] == '*') ext_q++;
                        size_t qlen = strlen(ext_q);
                        if (nlen >= qlen && strcasecmp(entry->d_name + (nlen - qlen), ext_q) == 0) {
                            matched = true;
                        }
                    } else if (ctx->mode == SEARCH_SIZE_GT) {
                        if ((uint64_t)st.st_size >= ctx->size_threshold_bytes) {
                            matched = true;
                        }
                    } else if (ctx->mode == SEARCH_SIZE_LT) {
                        if ((uint64_t)st.st_size <= ctx->size_threshold_bytes) {
                            matched = true;
                        }
                    } else if (ctx->mode == SEARCH_HIDDEN) {
                        if (entry->d_name[0] == '.' || strstr(fullpath, "/.") != NULL) {
                            if (ctx->query[0] == '\0' ||
                                strcasestr(entry->d_name, ctx->query) != NULL ||
                                fnmatch(ctx->query, entry->d_name, FNM_CASEFOLD) == 0) {
                                matched = true;
                            }
                        }
                    } else if (ctx->mode == SEARCH_CONTENT) {
                        if (!is_binary_file(fullpath)) {
                            FILE *file = fopen(fullpath, "r");
                            if (file) {
                                char *line = NULL;
                                size_t cap = 0;
                                ssize_t len;
                                long line_num = 0;

                                while ((len = getline(&line, &cap, file)) != -1) {
                                    (void)len;
                                    line_num++;
                                    if (strcasestr(line, ctx->query) != NULL) {
                                        line[strcspn(line, "\r\n")] = '\0';
                                        char *preview = line;
                                        while (*preview == ' ' || *preview == '\t') preview++;

                                        pthread_mutex_lock(&ctx->stats_lock);
                                        ctx->matches_found++;
                                        if (!ctx->silent) {
                                            printf("  %s%s%s:%s%ld%s: %.*s\n",
                                                   CLR_CYAN, fullpath, CLR_RESET,
                                                   CLR_YELLOW, line_num, CLR_RESET,
                                                   140, preview);
                                        }
                                        pthread_mutex_unlock(&ctx->stats_lock);
                                    }
                                }
                                free(line);
                                fclose(file);
                            }
                        }
                    }

                    if (matched) {
                        char sz_buf[32];
                        format_bytes((uint64_t)st.st_size, sz_buf, sizeof(sz_buf));

                        pthread_mutex_lock(&ctx->stats_lock);
                        ctx->matches_found++;
                        if (!ctx->silent) {
                            printf("  %s  (%s)\n", fullpath, sz_buf);
                        }
                        pthread_mutex_unlock(&ctx->stats_lock);
                    }
                }
            }
            closedir(dir);
        }

        pthread_mutex_lock(&q->lock);
        q->active_workers--;
        if (q->count == 0 && q->active_workers == 0) {
            q->shutdown = true;
            pthread_cond_broadcast(&q->has_work);
            pthread_cond_broadcast(&q->all_done);
        }
        pthread_mutex_unlock(&q->lock);
    }
    return NULL;
}

/* Execute Search */
static void run_search(search_ctx_t *ctx) {
    ctx->files_scanned = 0;
    ctx->matches_found = 0;
    ctx->visited_inodes = NULL;

    pthread_mutex_init(&ctx->stats_lock, NULL);
    pthread_mutex_init(&ctx->inode_lock, NULL);
    pthread_mutex_init(&ctx->queue.lock, NULL);
    pthread_cond_init(&ctx->queue.has_work, NULL);
    pthread_cond_init(&ctx->queue.all_done, NULL);

    ctx->queue.head = NULL;
    ctx->queue.tail = NULL;
    ctx->queue.count = 0;
    ctx->queue.active_workers = 0;
    ctx->queue.shutdown = false;

    /* Register root_dir inode initially to prevent circular loops back to root */
    struct stat root_st;
    if (stat(ctx->root_dir, &root_st) == 0) {
        check_and_add_inode(ctx, root_st.st_dev, root_st.st_ino);
    }

    queue_push(&ctx->queue, ctx->root_dir);

    double t0 = get_time_sec();

    pthread_t threads[MAX_THREADS];
    int thread_count = (ctx->thread_count > 0 && ctx->thread_count <= MAX_THREADS) ? ctx->thread_count : 8;

    for (int i = 0; i < thread_count; i++) {
        pthread_create(&threads[i], NULL, search_worker, ctx);
    }

    pthread_mutex_lock(&ctx->queue.lock);
    while (!ctx->queue.shutdown || ctx->queue.active_workers > 0) {
        pthread_cond_wait(&ctx->queue.all_done, &ctx->queue.lock);
    }
    pthread_mutex_unlock(&ctx->queue.lock);

    for (int i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }

    ctx->elapsed_sec = get_time_sec() - t0;

    free_visited_inodes(ctx);
    free_queue(&ctx->queue);
    pthread_mutex_destroy(&ctx->stats_lock);
    pthread_mutex_destroy(&ctx->inode_lock);
    pthread_mutex_destroy(&ctx->queue.lock);
    pthread_cond_destroy(&ctx->queue.has_work);
    pthread_cond_destroy(&ctx->queue.all_done);
}

/* Search History Logging */
static void log_history(const char *type, const char *folder, const char *query, int threads, double sec, uint64_t matches) {
    FILE *f = fopen("search_history.txt", "a");
    if (!f) return;
    time_t now = time(NULL);
    struct tm tm_info;
    localtime_r(&now, &tm_info);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &tm_info);

    fprintf(f, "[%s] Type: %-12s | Folder: %-25s | Query: %-15s | Threads: %2d | Time: %7.4fs | Matches: %llu\n",
            time_str, type, folder, query, threads, sec, (unsigned long long)matches);
    fclose(f);
}

static void view_history(void) {
    FILE *f = fopen("search_history.txt", "r");
    if (!f) {
        printf("\n  No search history found yet.\n\n");
        return;
    }

    printf("\n%s==================================================\n", CLR_CYAN);
    printf("              SEARCH HISTORY LOG\n");
    printf("==================================================%s\n\n", CLR_RESET);

    char line[512];
    int count = 0;
    while (fgets(line, sizeof(line), f)) {
        printf("  %s%s%s", CLR_GRAY, line, CLR_RESET);
        count++;
    }
    fclose(f);

    if (count == 0) {
        printf("  (History log is empty)\n");
    }

    printf("\n%sOptions:%s [c] Clear history  |  [Enter] Return to menu: ", CLR_BOLD, CLR_RESET);
    fflush(stdout);

    char buf[16];
    if (fgets(buf, sizeof(buf), stdin)) {
        if (buf[0] == 'c' || buf[0] == 'C') {
            FILE *clear_f = fopen("search_history.txt", "w");
            if (clear_f) {
                fclose(clear_f);
                printf("\n  Search history cleared successfully.\n\n");
            }
        }
    }
}

/* ---------------- Duplicate File Detection ---------------- */
typedef struct dup_file {
    char path[MAX_PATH_LEN];
    uint64_t size;
    uint64_t hash;
    bool has_hash;
    struct dup_file *next;
} dup_file_t;

/* 64-bit FNV-1a Hash */
static uint64_t hash_file_fnv1a(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    uint64_t hash = 0xcbf29ce484222325ULL;
    unsigned char buf[16384];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
        for (size_t i = 0; i < n; i++) {
            hash ^= (uint64_t)buf[i];
            hash *= 0x100000001b3ULL;
        }
    }
    fclose(f);
    return hash;
}

/* Byte-by-byte file content comparison */
static bool compare_bytes(const char *p1, const char *p2) {
    FILE *f1 = fopen(p1, "rb");
    if (!f1) return false;
    FILE *f2 = fopen(p2, "rb");
    if (!f2) { fclose(f1); return false; }
    unsigned char b1[16384], b2[16384];
    bool same = true;
    while (1) {
        size_t n1 = fread(b1, 1, sizeof(b1), f1);
        size_t n2 = fread(b2, 1, sizeof(b2), f2);
        if (n1 != n2) { same = false; break; }
        if (n1 == 0) break;
        if (memcmp(b1, b2, n1) != 0) { same = false; break; }
    }
    fclose(f1);
    fclose(f2);
    return same;
}

static void collect_dup_candidates(const char *dir_path, dup_file_t **list, int *total) {
    DIR *dir = opendir(dir_path);
    if (!dir) return;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0 || entry->d_name[0] == '.') {
            continue;
        }
        char fullpath[MAX_PATH_LEN];
        join_path(fullpath, sizeof(fullpath), dir_path, entry->d_name);

        struct stat st;
        if (lstat(fullpath, &st) != 0) continue;

        /* Skip symlinks to prevent loops and duplicate counting */
        if (S_ISLNK(st.st_mode)) continue;

        if (S_ISDIR(st.st_mode)) {
            collect_dup_candidates(fullpath, list, total);
        } else if (S_ISREG(st.st_mode) && st.st_size > 0) {
            dup_file_t *item = (dup_file_t *)malloc(sizeof(dup_file_t));
            if (item) {
                snprintf(item->path, sizeof(item->path), "%.4000s", fullpath);
                item->size = (uint64_t)st.st_size;
                item->hash = 0;
                item->has_hash = false;
                item->next = *list;
                *list = item;
                (*total)++;
            }
        }
    }
    closedir(dir);
}

/* Comparator for sorting duplicate candidates by size */
static int compare_dup_size(const void *a, const void *b) {
    const dup_file_t *f1 = *(const dup_file_t * const *)a;
    const dup_file_t *f2 = *(const dup_file_t * const *)b;
    if (f1->size < f2->size) return -1;
    if (f1->size > f2->size) return 1;
    return 0;
}

static void run_duplicate_check(const char *root_dir) {
    printf("\n%sScanning for duplicate files in '%s'...%s\n\n", CLR_YELLOW, root_dir, CLR_RESET);
    double t0 = get_time_sec();
    dup_file_t *head = NULL;
    int count = 0;
    collect_dup_candidates(root_dir, &head, &count);

    if (count < 2) {
        printf("  Not enough files to compare.\n\n");
        while (head) { dup_file_t *t = head->next; free(head); head = t; }
        return;
    }

    dup_file_t **arr = (dup_file_t **)malloc(count * sizeof(dup_file_t *));
    if (!arr) {
        printf("%sError: Memory allocation failed.%s\n\n", CLR_RED, CLR_RESET);
        while (head) { dup_file_t *t = head->next; free(head); head = t; }
        return;
    }

    dup_file_t *curr = head;
    for (int i = 0; i < count && curr; i++, curr = curr->next) {
        arr[i] = curr;
    }

    /* Fast O(N log N) sort by file size */
    qsort(arr, count, sizeof(dup_file_t *), compare_dup_size);

    /* Hash only files that share their exact size with an adjacent neighbor */
    for (int i = 0; i < count; i++) {
        bool size_shared = (i > 0 && arr[i]->size == arr[i - 1]->size) ||
                           (i + 1 < count && arr[i]->size == arr[i + 1]->size);
        if (size_shared && !arr[i]->has_hash) {
            arr[i]->hash = hash_file_fnv1a(arr[i]->path);
            arr[i]->has_hash = true;
        }
    }

    bool *matched = (bool *)calloc(count, sizeof(bool));
    if (!matched) {
        free(arr);
        while (head) { dup_file_t *t = head->next; free(head); head = t; }
        return;
    }

    int group_id = 1;
    int duplicate_copies = 0;
    uint64_t wasted_bytes = 0;

    printf("%s---------------- Duplicate File Sets ----------------%s\n", CLR_GREEN, CLR_RESET);
    for (int i = 0; i < count; i++) {
        if (!arr[i]->has_hash || matched[i]) continue;
        bool group_header = false;

        for (int j = i + 1; j < count; j++) {
            if (arr[j]->size != arr[i]->size) {
                break; /* Size differs, subsequent files in sorted array will also differ */
            }

            if (arr[j]->has_hash && !matched[j] && arr[i]->hash == arr[j]->hash) {
                if (compare_bytes(arr[i]->path, arr[j]->path)) {
                    if (!group_header) {
                        char sz_buf[32];
                        format_bytes(arr[i]->size, sz_buf, sizeof(sz_buf));
                        printf("  %s[Duplicate Set #%d]%s Size: %s%s%s\n",
                               CLR_BOLD, group_id++, CLR_RESET, CLR_YELLOW, sz_buf, CLR_RESET);
                        printf("    Original  : %s\n", arr[i]->path);
                        matched[i] = true;
                        group_header = true;
                    }
                    printf("    Duplicate : %s\n", arr[j]->path);
                    matched[j] = true;
                    duplicate_copies++;
                    wasted_bytes += arr[j]->size;
                }
            }
        }
        if (group_header) {
            printf("\n");
        }
    }

    if (duplicate_copies == 0) {
        printf("  No duplicate files detected.\n");
    }
    printf("%s----------------------------------------------------%s\n\n", CLR_GREEN, CLR_RESET);
    double elapsed = get_time_sec() - t0;

    char wasted_buf[32];
    format_bytes(wasted_bytes, wasted_buf, sizeof(wasted_buf));

    printf("  Files checked    : %d\n", count);
    printf("  Duplicate copies : %s%d extra copies found%s (%d sets)\n",
           CLR_GREEN, duplicate_copies, CLR_RESET, group_id - 1);
    if (wasted_bytes > 0) {
        printf("  Wasted space     : %s%s%s (Reclaimable)\n", CLR_YELLOW, wasted_buf, CLR_RESET);
    }
    printf("  Time elapsed     : %s%.4f s (%.1f ms)%s\n\n", CLR_YELLOW, elapsed, elapsed * 1000.0, CLR_RESET);

    log_history("Duplicates", root_dir, "*", get_cpu_cores(), elapsed, (uint64_t)duplicate_copies);

    free(matched);
    free(arr);
    while (head) {
        dup_file_t *tmp = head->next;
        free(head);
        head = tmp;
    }
}

/* ---------------- Multithreaded Benchmark ---------------- */
static void run_benchmark(const char *root_dir) {
    int test_threads[] = {1, 2, 4, 8, 12};
    int num_tests = 5;
    int cores = get_cpu_cores();
    double baseline = 0.0;

    printf("\n%s==================================================\n", CLR_CYAN);
    printf("        MULTITHREADED BENCHMARK MATRIX\n");
    printf("  Target: %-25.25s  Hardware: %2d Cores\n", root_dir, cores);
    printf("==================================================%s\n\n", CLR_RESET);

    printf("  Threads      Time (sec)      Files Checked      Speedup\n");
    printf("  -------------------------------------------------------\n");

    for (int i = 0; i < num_tests; i++) {
        search_ctx_t ctx;
        memset(&ctx, 0, sizeof(ctx));
        snprintf(ctx.root_dir, sizeof(ctx.root_dir), "%.3500s", root_dir);
        snprintf(ctx.query, sizeof(ctx.query), "*");
        ctx.mode = SEARCH_FILENAME;
        ctx.thread_count = test_threads[i];
        ctx.silent = true;

        run_search(&ctx);

        if (i == 0 || baseline <= 0.0) {
            baseline = ctx.elapsed_sec;
        }

        double speedup = (ctx.elapsed_sec > 0.0) ? (baseline / ctx.elapsed_sec) : 1.0;
        printf("  %-8d     %7.4f s         %-14llu    %s%5.2fx%s\n",
               test_threads[i], ctx.elapsed_sec,
               (unsigned long long)ctx.files_scanned,
               (speedup >= 1.2 ? CLR_GREEN : CLR_RESET), speedup, CLR_RESET);
    }
    printf("  -------------------------------------------------------\n\n");
}

#endif /* SEARCH_H */
