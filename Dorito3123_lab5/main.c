#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/resource.h>
#include "index/index.h"
#include "index/search.h"

TreeType parseType(const char* s) {
    if (strcmp(s, "avl") == 0) return TREE_AVL;
    if (strcmp(s, "rb") == 0) return TREE_RB;
    if (strcmp(s, "btree") == 0) return TREE_BTREE;
    fprintf(stderr, "Unknown type: %s\n", s);
    return TREE_AVL;
}

const char* typeName(TreeType t) {
    switch (t) {
        case TREE_AVL: return "avl";
        case TREE_RB: return "rb";
        case TREE_BTREE: return "btree";
    }
    return "avl";
}

static int parseJsonl(const char* line, int* doc_id,
                      char* title, int tcap, char*** toks, int* ntoks) {
    const char* p = strstr(line, "\"doc_id\"");
    if (!p) return 0;
    p = strchr(p, ':'); if (!p) return 0; p++;
    while (*p == ' ' || *p == '"') p++;
    *doc_id = atoi(p);

    p = strstr(line, "\"title\"");
    if (!p) return 0;
    p = strchr(p, ':'); p++;
    while (*p == ' ') p++;
    if (*p == '"') p++;
    int ti = 0;
    while (*p && *p != '"' && ti < tcap - 1) {
        if (*p == '\\') { p++; if (*p) title[ti++] = *p++; }
        else title[ti++] = *p++;
    }
    title[ti] = '\0';

    p = strstr(line, "\"tokens\"");
    if (!p) { *toks = NULL; *ntoks = 0; return 1; }
    p = strchr(p, '['); if (!p) { *toks = NULL; *ntoks = 0; return 1; }
    p++;

    int cap = 64, n = 0;
    char** arr = malloc(cap * sizeof(char*));
    while (*p && *p != ']') {
        while (*p == ' ' || *p == ',') p++;
        if (*p == '"') {
            p++;
            const char* s = p;
            while (*p && *p != '"') p++;
            int len = (int)(p - s);
            if (*p == '"') p++;
            if (len > 0) {
                if (n == cap) { cap *= 2; arr = realloc(arr, cap * sizeof(char*)); }
                arr[n] = malloc(len + 1);
                memcpy(arr[n], s, len); arr[n][len] = '\0';
                n++;
            }
        } else if (*p) p++;
    }
    *toks = arr; *ntoks = n;
    return 1;
}

static void runIndex(TreeType type, const char* data, const char* idx_path, long limit) {
    FILE* f = fopen(data, "r");
    if (!f) { fprintf(stderr, "Cannot open: %s\n", data); exit(1); }

    Index* idx = createIndex(type);
    char line[1024 * 16], title[256];
    char** toks = NULL;
    int doc_id, ntoks; long count = 0;

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    while (fgets(line, sizeof(line), f)) {
        if (limit > 0 && count >= limit) break;
        line[strcspn(line, "\n")] = '\0';
        if (!parseJsonl(line, &doc_id, title, sizeof(title), &toks, &ntoks)) continue;
        indexDocument(idx, doc_id, title, (const char**)toks, ntoks);
        for (int i = 0; i < ntoks; i++) free(toks[i]);
        free(toks); toks = NULL;
        if (++count % 10000 == 0) fprintf(stderr, "indexed %ld\n", count);
    }
    fclose(f);

    clock_gettime(CLOCK_MONOTONIC, &t1);
    double elapsed = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) / 1e9;

    fprintf(stderr, "total: %ld docs\n", count);
    printf("IndexTime: %.3f s\n", elapsed);

    saveIndex(idx, idx_path);

    struct rusage u; getrusage(RUSAGE_SELF, &u);
#ifdef __APPLE__
    long mem_mb = u.ru_maxrss / (1024 * 1024);
#else
    long mem_mb = u.ru_maxrss / 1024;
#endif
    printf("Memory: %ld MB\n", mem_mb);
    freeIndex(idx);
}

static void runSearch(TreeType type, const char* idx_path, const char* query,
                      int json, int fuzzy, int max_dist, int repeat) {
    Index* idx = loadIndex(idx_path, type);
    if (!idx) { fprintf(stderr, "Failed to load: %s\n", idx_path); exit(1); }

    SearchResults* sr = NULL;
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    for (int r = 0; r < repeat; r++) {
        if (sr) freeSearchResults(sr);
        if (fuzzy)
            sr = fuzzySearch(idx, query, max_dist);
        else
            sr = search(idx, query);
    }

    clock_gettime(CLOCK_MONOTONIC, &t1);

    if (repeat > 1) {
        double total_ms = (t1.tv_sec - t0.tv_sec) * 1000.0 + (t1.tv_nsec - t0.tv_nsec) / 1e6;
        printf("repeat=%d total=%.2f ms avg=%.4f ms/query\n", repeat, total_ms, total_ms / repeat);
    }

    if (json) printResultsJSON(sr);
    else      printResultsText(sr);

    freeSearchResults(sr);
    freeIndex(idx);
}

static const char* BENCH_W1[] = {
    "python", "java", "memory", "error", "string",
    "array", "list", "sort", "hash", "tree",
    "null", "loop", "stack", "queue", "pointer"
};
static const char* BENCH_W2[] = {
    "python list", "memory leak", "sort array", "null pointer",
    "java thread", "hash table", "binary tree", "stack overflow",
    "string split", "loop break"
};
static const char* BENCH_W3[] = {
    "python list sort", "memory leak pointer",
    "java thread lock", "binary tree rotation",
    "null pointer exception", "hash table collision",
    "sort linked list", "stack overflow error",
    "string format python", "loop array index"
};

static void runBench(TreeType type, const char* idx_path, int words) {
    Index* idx = loadIndex(idx_path, type);
    if (!idx) { fprintf(stderr, "Failed to load: %s\n", idx_path); exit(1); }

    const char** queries;
    int nq;
    if (words == 1) { queries = BENCH_W1; nq = 15; }
    else if (words == 2) { queries = BENCH_W2; nq = 10; }
    else { queries = BENCH_W3; nq = 10; }

    int total_runs = 1000;
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    for (int i = 0; i < total_runs; i++) {
        SearchResults* sr = search(idx, queries[i % nq]);
        freeSearchResults(sr);
    }

    clock_gettime(CLOCK_MONOTONIC, &t1);
    double total_ms = (t1.tv_sec - t0.tv_sec) * 1000.0 + (t1.tv_nsec - t0.tv_nsec) / 1e6;

    printf("BenchSearch type=%s words=%d runs=%d total=%.2f ms avg=%.4f ms\n",
           typeName(type), words, total_runs, total_ms, total_ms / total_runs);

    freeIndex(idx);
}

static void usage(const char* prog) {
    fprintf(stderr,
        "usage:\n"
        "  %s index  --type=<avl|rb|btree> [--data=PATH] [--index=PATH] [--limit=N]\n"
        "  %s search --type=<avl|rb|btree> [--index=PATH] [--json] [--fuzzy] [--max-dist=N] [--repeat=N] \"query\"\n"
        "  %s bench  --type=<avl|rb|btree> [--index=PATH] --words=<1|2|3>\n",
        prog, prog, prog);
}

int main(int argc, char* argv[]) {
    if (argc < 3) { usage(argv[0]); return 1; }

    const char* mode = argv[1];
    TreeType type = TREE_AVL;
    const char* data = "data/processed/docs.jsonl";
    char idx[512] = {0};
    int json = 0, fuzzy = 0, max_dist = 2, repeat = 1, words = 1;
    long limit = 0;
    const char* q = NULL;

    for (int i = 2; i < argc; i++) {
        if (strncmp(argv[i], "--type=", 7) == 0) type = parseType(argv[i] + 7);
        else if (strncmp(argv[i], "--data=", 7) == 0) data = argv[i] + 7;
        else if (strncmp(argv[i], "--index=", 8) == 0) strncpy(idx, argv[i] + 8, sizeof(idx) - 1);
        else if (strncmp(argv[i], "--limit=", 8) == 0) limit = atol(argv[i] + 8);
        else if (strncmp(argv[i], "--max-dist=", 11) == 0) max_dist = atoi(argv[i] + 11);
        else if (strncmp(argv[i], "--repeat=", 9) == 0) repeat = atoi(argv[i] + 9);
        else if (strncmp(argv[i], "--words=", 8) == 0) words = atoi(argv[i] + 8);
        else if (strcmp(argv[i], "--json") == 0) json = 1;
        else if (strcmp(argv[i], "--fuzzy") == 0) fuzzy = 1;
        else if (argv[i][0] != '-') q = argv[i];
    }

    if (!idx[0]) snprintf(idx, sizeof(idx), "data/index_%s.txt", typeName(type));

    if (strcmp(mode, "index") == 0) runIndex(type, data, idx, limit);
    else if (strcmp(mode, "search") == 0) {
        if (!q) { fprintf(stderr, "no query\n"); return 1; }
        runSearch(type, idx, q, json, fuzzy, max_dist, repeat);
    }
    else if (strcmp(mode, "bench") == 0) runBench(type, idx, words);
    else { fprintf(stderr, "unknown mode: %s\n", mode); usage(argv[0]); return 1; }

    return 0;
}
