#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include "index/index.h"
#include "index/search.h"

TreeType parseType(const char* s) {
    if (strcmp(s, "avl")   == 0) return TREE_AVL;
    if (strcmp(s, "rb")    == 0) return TREE_RB;
    if (strcmp(s, "btree") == 0) return TREE_BTREE;
    fprintf(stderr, "Unknown type: %s\n", s);
    return TREE_AVL;
}

const char* typeName(TreeType t) {
    switch (t) {
        case TREE_AVL:   return "avl";
        case TREE_RB:    return "rb";
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

static void runIndex(TreeType type, const char* data, const char* idx_path) {
    FILE* f = fopen(data, "r");
    if (!f) { fprintf(stderr, "Cannot open: %s\n", data); exit(1); }

    Index* idx = createIndex(type);
    char line[1024 * 16], title[256];
    char** toks = NULL;
    int doc_id, ntoks; long count = 0;

    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = '\0';
        if (!parseJsonl(line, &doc_id, title, sizeof(title), &toks, &ntoks)) continue;
        indexDocument(idx, doc_id, title, (const char**)toks, ntoks);
        for (int i = 0; i < ntoks; i++) free(toks[i]);
        free(toks); toks = NULL;
        if (++count % 10000 == 0) fprintf(stderr, "Indexed %ld\n", count);
    }
    fclose(f);
    fprintf(stderr, "\nTotal: %ld docs\n", count);

    saveIndex(idx, idx_path);
    fprintf(stderr, "Saved: %s\n", idx_path);

    struct rusage u; getrusage(RUSAGE_SELF, &u);
#ifdef __APPLE__
    printf("Memory: %ld MB\n", u.ru_maxrss / (1024 * 1024));
#else
    printf("Memory: %ld MB\n", u.ru_maxrss / 1024);
#endif
    freeIndex(idx);
}

static void runSearch(TreeType type, const char* idx_path, const char* query, int json) {
    Index* idx = loadIndex(idx_path, type);
    if (!idx) { fprintf(stderr, "Failed to load: %s\n", idx_path); exit(1); }
    SearchResults* sr = search(idx, query);
    if (json) printResultsJSON(sr);
    else      printResultsText(sr);
    freeSearchResults(sr);
    freeIndex(idx);
}

static void usage(const char* prog) {
    fprintf(stderr,
        "Usage:\n"
        "  %s index  --type=<avl|rb|btree> [--data=PATH] [--index=PATH]\n"
        "  %s search --type=<avl|rb|btree> [--index=PATH] [--json] \"query\"\n",
        prog, prog);
}

int main(int argc, char* argv[]) {
    if (argc < 3) { usage(argv[0]); return 1; }

    const char* mode = argv[1];
    TreeType type    = TREE_AVL;
    const char* data = "data/processed/docs.jsonl";
    char idx[512]    = {0};
    int json         = 0;
    const char* q    = NULL;

    for (int i = 2; i < argc; i++) {
        if      (strncmp(argv[i], "--type=",  7) == 0) type = parseType(argv[i] + 7);
        else if (strncmp(argv[i], "--data=",  7) == 0) data = argv[i] + 7;
        else if (strncmp(argv[i], "--index=", 8) == 0) strncpy(idx, argv[i] + 8, sizeof(idx) - 1);
        else if (strcmp( argv[i], "--json")   == 0)    json = 1;
        else if (argv[i][0] != '-')                    q = argv[i];
    }

    if (!idx[0]) snprintf(idx, sizeof(idx), "data/index_%s.txt", typeName(type));

    if      (strcmp(mode, "index")  == 0) runIndex(type, data, idx);
    else if (strcmp(mode, "search") == 0) {
        if (!q) { fprintf(stderr, "No query\n"); return 1; }
        runSearch(type, idx, q, json);
    } else { fprintf(stderr, "Unknown mode: %s\n", mode); usage(argv[0]); return 1; }

    return 0;
}
