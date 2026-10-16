#include "search.h"

static int g_thread_count = 12;

/* Clean section header */
static void print_section(const char *title) {
    printf("\n%s--- %s ---%s\n", CLR_CYAN, title, CLR_RESET);
}

/* Pause before returning to menu, so results stay visible */
static void pause_for_menu(void) {
    printf("\nPress [Enter] to return to menu... ");
    fflush(stdout);
    char buf[128];
    if (!fgets(buf, sizeof(buf), stdin)) {
        /* EOF reached */
    }
}

/* Helper to read string with default fallback */
static void get_input(const char *prompt, char *buffer, size_t buf_len, const char *default_val) {
    if (default_val && default_val[0] != '\0') {
        printf("%s%s [%s]: %s", CLR_BOLD, prompt, default_val, CLR_RESET);
    } else {
        printf("%s%s: %s", CLR_BOLD, prompt, CLR_RESET);
    }
    fflush(stdout);

    char line[MAX_PATH_LEN];
    if (fgets(line, sizeof(line), stdin)) {
        line[strcspn(line, "\r\n")] = '\0';
        if (line[0] != '\0') {
            snprintf(buffer, buf_len, "%s", line);
            return;
        }
    }
    if (default_val) {
        snprintf(buffer, buf_len, "%s", default_val);
    } else {
        buffer[0] = '\0';
    }
}

/* Print Search Summary Card */
static void print_summary(const search_ctx_t *ctx, const char *type_name) {
    int cores = get_cpu_cores();
    double ms = ctx->elapsed_sec * 1000.0;
    double rate = (ctx->elapsed_sec > 0.0001) ? ((double)ctx->files_scanned / ctx->elapsed_sec) : 0.0;

    printf("\n%s---------------- Search Summary ----------------%s\n", CLR_CYAN, CLR_RESET);
    printf("  Files scanned  : %llu\n", (unsigned long long)ctx->files_scanned);
    printf("  Matches found  : %s%llu%s\n", CLR_GREEN, (unsigned long long)ctx->matches_found, CLR_RESET);
    printf("  Threads used   : %s%d active worker threads%s (Hardware Cores: %d)\n",
           CLR_BOLD, ctx->thread_count, CLR_RESET, cores);
    printf("  Time elapsed   : %s%.4f seconds (%.1f ms)%s\n", CLR_YELLOW, ctx->elapsed_sec, ms, CLR_RESET);
    if (rate > 0.0) {
        printf("  Scanning speed : %.1f files/second\n", rate);
    }
    printf("%s------------------------------------------------%s\n", CLR_CYAN, CLR_RESET);

    /* Record to search history */
    log_history(type_name, ctx->root_dir, ctx->query, ctx->thread_count, ctx->elapsed_sec, ctx->matches_found);
}

/* Option 1: Search by Filename */
static void flow_search_filename(void) {
    print_section("1. Search by Filename");

    search_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.mode = SEARCH_FILENAME;
    ctx.thread_count = g_thread_count;

    char raw_dir[MAX_PATH_LEN];
    get_input("Enter folder", raw_dir, sizeof(raw_dir), ".");
    normalize_path(raw_dir, ctx.root_dir, sizeof(ctx.root_dir));

    get_input("Enter filename (or pattern e.g. *.c)", ctx.query, sizeof(ctx.query), "*");
    if (ctx.query[0] == '\0') {
        printf("%sSearch pattern cannot be empty.%s\n", CLR_RED, CLR_RESET);
        pause_for_menu();
        return;
    }

    struct stat st;
    if (stat(ctx.root_dir, &st) != 0 || !S_ISDIR(st.st_mode)) {
        printf("%sError: '%s' is not an accessible directory.%s\n", CLR_RED, ctx.root_dir, CLR_RESET);
        pause_for_menu();
        return;
    }

    printf("\n%sStarting search with %d threads in '%s'...%s\n\n",
           CLR_YELLOW, ctx.thread_count, ctx.root_dir, CLR_RESET);
    run_search(&ctx);
    print_summary(&ctx, "Filename");
    pause_for_menu();
}

/* Option 2: Search Text Inside Files */
static void flow_search_content(void) {
    print_section("2. Search Text Inside Files");

    search_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.mode = SEARCH_CONTENT;
    ctx.thread_count = g_thread_count;

    char raw_dir[MAX_PATH_LEN];
    get_input("Enter folder", raw_dir, sizeof(raw_dir), ".");
    normalize_path(raw_dir, ctx.root_dir, sizeof(ctx.root_dir));

    get_input("Enter text to find", ctx.query, sizeof(ctx.query), "");
    if (ctx.query[0] == '\0') {
        printf("%sSearch string cannot be empty.%s\n", CLR_RED, CLR_RESET);
        pause_for_menu();
        return;
    }

    struct stat st;
    if (stat(ctx.root_dir, &st) != 0 || !S_ISDIR(st.st_mode)) {
        printf("%sError: '%s' is not an accessible directory.%s\n", CLR_RED, ctx.root_dir, CLR_RESET);
        pause_for_menu();
        return;
    }

    printf("\n%sScanning text with %d threads in '%s'...%s\n\n",
           CLR_YELLOW, ctx.thread_count, ctx.root_dir, CLR_RESET);
    run_search(&ctx);
    print_summary(&ctx, "Content");
    pause_for_menu();
}

/* Option 3: Search by File Extension */
static void flow_search_extension(void) {
    print_section("3. Search by File Extension");

    search_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.mode = SEARCH_EXTENSION;
    ctx.thread_count = g_thread_count;

    char raw_dir[MAX_PATH_LEN];
    get_input("Enter folder", raw_dir, sizeof(raw_dir), ".");
    normalize_path(raw_dir, ctx.root_dir, sizeof(ctx.root_dir));

    char ext_buf[64];
    get_input("Enter extension (e.g. .c, .pdf, .txt, .py)", ext_buf, sizeof(ext_buf), ".c");

    if (ext_buf[0] != '.' && ext_buf[0] != '\0') {
        snprintf(ctx.query, sizeof(ctx.query), ".%s", ext_buf);
    } else {
        snprintf(ctx.query, sizeof(ctx.query), "%s", ext_buf);
    }

    struct stat st;
    if (stat(ctx.root_dir, &st) != 0 || !S_ISDIR(st.st_mode)) {
        printf("%sError: '%s' is not an accessible directory.%s\n", CLR_RED, ctx.root_dir, CLR_RESET);
        pause_for_menu();
        return;
    }

    printf("\n%sSearching files with extension '%s' (%d threads)...%s\n\n",
           CLR_YELLOW, ctx.query, ctx.thread_count, CLR_RESET);
    run_search(&ctx);
    print_summary(&ctx, "Extension");
    pause_for_menu();
}

/* Option 4: Search Windows PC */
static void flow_search_windows(void) {
    print_section("4. Search Windows PC");

    char user_dir[MAX_PATH_LEN];
    get_windows_user_path(user_dir, sizeof(user_dir));

    char desktop_dir[MAX_PATH_LEN], downloads_dir[MAX_PATH_LEN], docs_dir[MAX_PATH_LEN];
    snprintf(desktop_dir, sizeof(desktop_dir), "%.3500s/Desktop", user_dir);
    snprintf(downloads_dir, sizeof(downloads_dir), "%.3500s/Downloads", user_dir);
    snprintf(docs_dir, sizeof(docs_dir), "%.3500s/Documents", user_dir);

    printf("  1. Windows Desktop   (%s)\n", desktop_dir);
    printf("  2. Windows Downloads (%s)\n", downloads_dir);
    printf("  3. Windows Documents (%s)\n", docs_dir);
    printf("  4. Entire C: Drive   (/mnt/c)\n");
    printf("  5. Custom Windows Path (e.g., C:\\...)\n");

    char choice_buf[16];
    get_input("Select Windows location [1-5]", choice_buf, sizeof(choice_buf), "1");
    int loc = atoi(choice_buf);

    search_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.thread_count = g_thread_count;

    if (loc == 1) {
        snprintf(ctx.root_dir, sizeof(ctx.root_dir), "%.3500s", desktop_dir);
    } else if (loc == 2) {
        snprintf(ctx.root_dir, sizeof(ctx.root_dir), "%.3500s", downloads_dir);
    } else if (loc == 3) {
        snprintf(ctx.root_dir, sizeof(ctx.root_dir), "%.3500s", docs_dir);
    } else if (loc == 4) {
        snprintf(ctx.root_dir, sizeof(ctx.root_dir), "/mnt/c");
    } else {
        char custom[MAX_PATH_LEN];
        get_input("Enter Windows path (e.g. C:\\Users\\...)", custom, sizeof(custom), "C:\\");
        normalize_path(custom, ctx.root_dir, sizeof(ctx.root_dir));
    }

    struct stat st;
    if (stat(ctx.root_dir, &st) != 0 || !S_ISDIR(st.st_mode)) {
        printf("%sError: '%s' is not accessible.%s\n", CLR_RED, ctx.root_dir, CLR_RESET);
        pause_for_menu();
        return;
    }

    char mode_buf[16];
    get_input("Search type (1: Filename, 2: Text inside files)", mode_buf, sizeof(mode_buf), "1");
    ctx.mode = (atoi(mode_buf) == 2) ? SEARCH_CONTENT : SEARCH_FILENAME;

    get_input("Enter search term", ctx.query, sizeof(ctx.query), "*");
    if (ctx.query[0] == '\0') {
        printf("%sSearch term cannot be empty.%s\n", CLR_RED, CLR_RESET);
        pause_for_menu();
        return;
    }

    printf("\n%sSearching Windows PC '%s' with %d threads...%s\n\n",
           CLR_YELLOW, ctx.root_dir, ctx.thread_count, CLR_RESET);
    run_search(&ctx);
    print_summary(&ctx, "Windows-PC");
    pause_for_menu();
}

/* Option 5: Search by File Size */
static void flow_search_size(void) {
    print_section("5. Search by File Size");

    search_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.thread_count = g_thread_count;

    char raw_dir[MAX_PATH_LEN];
    get_input("Enter folder", raw_dir, sizeof(raw_dir), ".");
    normalize_path(raw_dir, ctx.root_dir, sizeof(ctx.root_dir));

    struct stat st;
    if (stat(ctx.root_dir, &st) != 0 || !S_ISDIR(st.st_mode)) {
        printf("%sError: '%s' is not an accessible directory.%s\n", CLR_RED, ctx.root_dir, CLR_RESET);
        pause_for_menu();
        return;
    }

    printf("  1. Larger than threshold  (e.g., > 10 MB)\n");
    printf("  2. Smaller than threshold (e.g., < 100 KB)\n");

    char comp_buf[16];
    get_input("Choose filter [1-2]", comp_buf, sizeof(comp_buf), "1");
    ctx.mode = (atoi(comp_buf) == 2) ? SEARCH_SIZE_LT : SEARCH_SIZE_GT;

    char size_val_buf[32];
    get_input("Enter size number (e.g. 10, 50, 500)", size_val_buf, sizeof(size_val_buf), "10");
    double val = atof(size_val_buf);
    if (val <= 0.0) val = 1.0;

    printf("  1. MB  |  2. KB  |  3. GB  |  4. Bytes\n");
    char unit_buf[16];
    get_input("Choose unit [1-4]", unit_buf, sizeof(unit_buf), "1");
    int unit = atoi(unit_buf);

    uint64_t multiplier = 1024ULL * 1024ULL; /* default MB */
    if (unit == 2) multiplier = 1024ULL;
    else if (unit == 3) multiplier = 1024ULL * 1024ULL * 1024ULL;
    else if (unit == 4) multiplier = 1ULL;

    ctx.size_threshold_bytes = (uint64_t)(val * (double)multiplier);

    char readable_sz[32];
    format_bytes(ctx.size_threshold_bytes, readable_sz, sizeof(readable_sz));
    snprintf(ctx.query, sizeof(ctx.query), "%s %s", (ctx.mode == SEARCH_SIZE_GT ? ">=" : "<="), readable_sz);

    printf("\n%sScanning for files %s %s in '%s' (%d threads)...%s\n\n",
           CLR_YELLOW, (ctx.mode == SEARCH_SIZE_GT ? "larger than" : "smaller than"),
           readable_sz, ctx.root_dir, ctx.thread_count, CLR_RESET);
    run_search(&ctx);
    print_summary(&ctx, "Size-Filter");
    pause_for_menu();
}

/* Option 6: Search Hidden / Config Files */
static void flow_search_hidden(void) {
    print_section("6. Search Hidden / Config Files");

    search_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.mode = SEARCH_HIDDEN;
    ctx.thread_count = g_thread_count;

    char raw_dir[MAX_PATH_LEN];
    get_input("Enter folder", raw_dir, sizeof(raw_dir), ".");
    normalize_path(raw_dir, ctx.root_dir, sizeof(ctx.root_dir));

    struct stat st;
    if (stat(ctx.root_dir, &st) != 0 || !S_ISDIR(st.st_mode)) {
        printf("%sError: '%s' is not an accessible directory.%s\n", CLR_RED, ctx.root_dir, CLR_RESET);
        pause_for_menu();
        return;
    }

    get_input("Optional name filter (e.g. env, git, or press Enter for all)", ctx.query, sizeof(ctx.query), "");

    printf("\n%sScanning hidden files in '%s' (%d threads)...%s\n\n",
           CLR_YELLOW, ctx.root_dir, ctx.thread_count, CLR_RESET);
    run_search(&ctx);
    print_summary(&ctx, "Hidden-Files");
    pause_for_menu();
}

/* Option 7: Find Duplicate Files */
static void flow_duplicate_files(void) {
    print_section("7. Find Duplicate Files");

    char raw_dir[MAX_PATH_LEN], clean_dir[MAX_PATH_LEN];
    get_input("Enter folder", raw_dir, sizeof(raw_dir), ".");
    normalize_path(raw_dir, clean_dir, sizeof(clean_dir));

    struct stat st;
    if (stat(clean_dir, &st) != 0 || !S_ISDIR(st.st_mode)) {
        printf("%sError: '%s' is not an accessible directory.%s\n", CLR_RED, clean_dir, CLR_RESET);
        pause_for_menu();
        return;
    }

    run_duplicate_check(clean_dir);
    pause_for_menu();
}

/* Option 8: Benchmark Performance */
static void flow_benchmark(void) {
    print_section("8. Benchmark Performance (Threads Speedup)");

    char raw_dir[MAX_PATH_LEN], clean_dir[MAX_PATH_LEN];
    get_input("Enter folder to benchmark", raw_dir, sizeof(raw_dir), ".");
    normalize_path(raw_dir, clean_dir, sizeof(clean_dir));

    struct stat st;
    if (stat(clean_dir, &st) != 0 || !S_ISDIR(st.st_mode)) {
        printf("%sError: '%s' is not an accessible directory.%s\n", CLR_RED, clean_dir, CLR_RESET);
        pause_for_menu();
        return;
    }

    run_benchmark(clean_dir);
    pause_for_menu();
}

/* Option 9: View Search History Log */
static void flow_view_history(void) {
    view_history();
}

/* Print Clean Classic Menu Dashboard */
static void print_dashboard(void) {
    printf("\n%s==================================================\n", CLR_CYAN);
    printf("              PARALLEL FILE SEARCH\n");
    printf("==================================================%s\n", CLR_RESET);
    printf("  1. Search by Filename\n");
    printf("  2. Search Text Inside Files\n");
    printf("  3. Search by File Extension (e.g., .c, .pdf, .txt)\n");
    printf("  4. Search Windows PC (C: Drive / Desktop / Downloads)\n");
    printf("  5. Search by File Size (Larger / Smaller Than)\n");
    printf("  6. Search Hidden / Config Files (.env, dotfiles)\n");
    printf("  7. Find Duplicate Files\n");
    printf("  8. Benchmark Performance (Threads Speedup)\n");
    printf("  9. View Search History Log\n");
    printf("  10. Exit\n");
    printf("%s==================================================%s\n", CLR_CYAN, CLR_RESET);
    printf("Choose an option [1-10]: ");
    fflush(stdout);
}

int main(void) {
    int cores = get_cpu_cores();
    g_thread_count = (cores >= 12) ? 12 : ((cores >= 8) ? 8 : cores);
    char choice_buf[32];

    while (1) {
        print_dashboard();

        if (!fgets(choice_buf, sizeof(choice_buf), stdin)) {
            break;
        }

        int choice = atoi(choice_buf);

        switch (choice) {
            case 1:
                flow_search_filename();
                break;
            case 2:
                flow_search_content();
                break;
            case 3:
                flow_search_extension();
                break;
            case 4:
                flow_search_windows();
                break;
            case 5:
                flow_search_size();
                break;
            case 6:
                flow_search_hidden();
                break;
            case 7:
                flow_duplicate_files();
                break;
            case 8:
                flow_benchmark();
                break;
            case 9:
                flow_view_history();
                break;
            case 10:
                printf("\n%sExiting Parallel File Search. Goodbye!%s\n\n", CLR_GREEN, CLR_RESET);
                return 0;
            default:
                printf("\n%sPlease choose a number between 1 and 10.%s\n", CLR_RED, CLR_RESET);
                break;
        }
    }
    return 0;
}
