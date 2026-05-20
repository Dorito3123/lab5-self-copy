#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "search.h"
#include "../levenstein/levenshtein.h"

static char** tokenize(const char* query, int* n) {
    char* buf = strdup(query);
    for (char* p = buf; *p; p++) *p = (char)tolower((unsigned char)*p);
    for (char* p = buf; *p; p++) if (!isalpha((unsigned char)*p)) *p = ' ';

    int cap = 16; *n = 0;
    char** toks = malloc(cap * sizeof(char*));
    char* save = NULL, *tok = strtok_r(buf, " ", &save);
    while (tok) {
        if (strlen(tok) > 2) {
            if (*n == cap) { cap *= 2; toks = realloc(toks, cap * sizeof(char*)); }
            toks[(*n)++] = strdup(tok);
        }
        tok = strtok_r(NULL, " ", &save);
    }
    free(buf);
    return toks;
}

Vector* intersectPostings(Vector** lists, int n) {
    Vector* res = createPostingList();
    if (!lists || n == 0) return res;

    int base = 0;
    for (int i = 1; i < n; i++)
        if (lists[i] && (!lists[base] || lists[i]->size < lists[base]->size)) base = i;
    if (!lists[base]) return res;

    for (size_t i = 0; i < lists[base]->size; i++) {
        PostingEntry* e = getVectorItem(lists[base], i);
        int ok = 1;
        for (int j = 0; j < n && ok; j++) {
            if (j == base) continue;
            if (!lists[j]) { ok = 0; break; }
            int found = 0;
            for (size_t k = 0; k < lists[j]->size && !found; k++)
                found = ((PostingEntry*)getVectorItem(lists[j], k))->doc_id == e->doc_id;
            if (!found) ok = 0;
        }
        if (ok) appendPosting(res, e->doc_id, e->title);
    }
    return res;
}

SearchResults* search(Index* idx, const char* query) {
    SearchResults* sr = malloc(sizeof(SearchResults));
    sr->results = createVector(sizeof(SearchResult));
    sr->total = 0; sr->time_ms = 0.0;

    int n = 0;
    char** toks = tokenize(query, &n);
    if (n == 0) { free(toks); return sr; }

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    Vector** lists = malloc(n * sizeof(Vector*));
    for (int i = 0; i < n; i++) lists[i] = lookupTerm(idx, toks[i]);
    Vector* hits = intersectPostings(lists, n);
    free(lists);

    clock_gettime(CLOCK_MONOTONIC, &t1);
    sr->time_ms = (t1.tv_sec - t0.tv_sec) * 1000.0 + (t1.tv_nsec - t0.tv_nsec) / 1e6;
    sr->total = (int)hits->size;

    int limit = hits->size < 10 ? (int)hits->size : 10;
    for (int i = 0; i < limit; i++) {
        PostingEntry* e = getVectorItem(hits, i);
        SearchResult r; r.doc_id = e->doc_id; r.score = n;
        strncpy(r.title, e->title, MAX_TITLE_LEN - 1); r.title[MAX_TITLE_LEN - 1] = '\0';
        appendVectorItem(sr->results, &r);
    }
    vectorFree(hits);
    for (int i = 0; i < n; i++) free(toks[i]);
    free(toks);
    return sr;
}

void printResultsText(const SearchResults* sr) {
    printf("Время: %.1f мс | Найдено: %d документов\n\n", sr->time_ms, sr->total);
    for (size_t i = 0; i < sr->results->size; i++) {
        SearchResult* r = getVectorItem(sr->results, i);
        printf("%2zu. [id=%d] %s\n", i + 1, r->doc_id, r->title);
    }
}

void printResultsJSON(const SearchResults* sr) {
    printf("{\"total\":%d,\"time_ms\":%.2f,\"results\":[", sr->total, sr->time_ms);
    for (size_t i = 0; i < sr->results->size; i++) {
        SearchResult* r = getVectorItem(sr->results, i);
        if (i) printf(",");
        printf("{\"doc_id\":%d,\"title\":\"", r->doc_id);
        for (const char* p = r->title; *p; p++) {
            if (*p == '"') printf("\\\"");
            else if (*p == '\\') printf("\\\\");
            else putchar(*p);
        }
        printf("\",\"score\":%d}", r->score);
    }
    printf("]}\n");
}

void freeSearchResults(SearchResults* sr) {
    if (!sr) return;
    vectorFree(sr->results);
    free(sr);
}

typedef struct {
    const char* term;
    int max_dist;
    Vector* out;
} FuzzyCtx;

static void fuzzyVis(const char* key, Vector* postings, void* ctx) {
    FuzzyCtx* fc = (FuzzyCtx*)ctx;
    int d = levenshteinDistance(fc->term, key);
    if (d >= 0 && d <= fc->max_dist) {
        FuzzyCandidate c;
        strncpy(c.term, key, 255); c.term[255] = '\0';
        c.distance = d;
        c.postings = postings;
        appendVectorItem(fc->out, &c);
    }
}

Vector* fuzzyFindCandidates(Index* idx, const char* term, int max_distance) {
    Vector* out = createVector(sizeof(FuzzyCandidate));
    FuzzyCtx ctx; ctx.term = term; ctx.max_dist = max_distance; ctx.out = out;
    traverseIndex(idx, fuzzyVis, &ctx);
    return out;
}

typedef struct {
    int doc_id;
    char title[MAX_TITLE_LEN];
    int matched;
    int total_dist;
} DocAcc;

static int cmpDocAcc(const void* a, const void* b) {
    const DocAcc* da = (const DocAcc*)a;
    const DocAcc* db = (const DocAcc*)b;
    int sa = da->matched * 10 - (da->matched ? da->total_dist / da->matched : 0);
    int sb = db->matched * 10 - (db->matched ? db->total_dist / db->matched : 0);
    return sb - sa;
}

SearchResults* fuzzySearch(Index* idx, const char* query, int max_distance) {
    SearchResults* sr = malloc(sizeof(SearchResults));
    sr->results = createVector(sizeof(SearchResult));
    sr->total = 0; sr->time_ms = 0.0;

    int n = 0;
    char** toks = tokenize(query, &n);
    if (n == 0) { free(toks); return sr; }

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    int acc_n = 0, acc_cap = 256;
    DocAcc* acc = malloc(acc_cap * sizeof(DocAcc));

    for (int ti = 0; ti < n; ti++) {
        Vector* cands = fuzzyFindCandidates(idx, toks[ti], max_distance);

        int seen_n = 0, seen_cap = 64;
        int* seen = malloc(seen_cap * sizeof(int));

        for (size_t ci = 0; ci < cands->size; ci++) {
            FuzzyCandidate* c = (FuzzyCandidate*)getVectorItem(cands, ci);
            for (size_t pi = 0; pi < c->postings->size; pi++) {
                PostingEntry* e = (PostingEntry*)getVectorItem(c->postings, pi);

                int already = 0;
                for (int si = 0; si < seen_n && !already; si++)
                    already = (seen[si] == e->doc_id);
                if (already) continue;

                if (seen_n == seen_cap) {
                    seen_cap *= 2;
                    seen = realloc(seen, seen_cap * sizeof(int));
                }
                seen[seen_n++] = e->doc_id;

                int di = -1;
                for (int j = 0; j < acc_n; j++)
                    if (acc[j].doc_id == e->doc_id) { di = j; break; }
                if (di < 0) {
                    if (acc_n == acc_cap) {
                        acc_cap *= 2;
                        acc = realloc(acc, acc_cap * sizeof(DocAcc));
                    }
                    di = acc_n++;
                    acc[di].doc_id = e->doc_id;
                    strncpy(acc[di].title, e->title, MAX_TITLE_LEN - 1);
                    acc[di].title[MAX_TITLE_LEN - 1] = '\0';
                    acc[di].matched = 0; acc[di].total_dist = 0;
                }
                acc[di].matched++;
                acc[di].total_dist += c->distance;
            }
        }
        free(seen);
        vectorFree(cands);
    }

    clock_gettime(CLOCK_MONOTONIC, &t1);
    sr->time_ms = (t1.tv_sec - t0.tv_sec) * 1000.0 + (t1.tv_nsec - t0.tv_nsec) / 1e6;
    sr->total = acc_n;

    qsort(acc, acc_n, sizeof(DocAcc), cmpDocAcc);

    int limit = acc_n < 10 ? acc_n : 10;
    for (int i = 0; i < limit; i++) {
        SearchResult r;
        r.doc_id = acc[i].doc_id;
        r.score = acc[i].matched * 10 - (acc[i].matched ? acc[i].total_dist / acc[i].matched : 0);
        strncpy(r.title, acc[i].title, MAX_TITLE_LEN - 1);
        r.title[MAX_TITLE_LEN - 1] = '\0';
        appendVectorItem(sr->results, &r);
    }
    free(acc);

    for (int i = 0; i < n; i++) free(toks[i]);
    free(toks);
    return sr;
}
