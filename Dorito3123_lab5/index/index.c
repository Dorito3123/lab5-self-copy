#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "index.h"
#include "../avl/avl.h"
#include "../rbtree/rbtree.h"
#include "../btree/btree.h"

Index* createIndex(TreeType type) {
    Index* idx = malloc(sizeof(Index));
    idx->type  = type;
    switch (type) {
        case TREE_AVL:   idx->tree = createAVLTree();  break;
        case TREE_RB:    idx->tree = createRBTree();   break;
        case TREE_BTREE: idx->tree = createBTree();    break;
    }
    return idx;
}

void insertTerm(Index* idx, const char* term, int doc_id, const char* title) {
    switch (idx->type) {
        case TREE_AVL:   avlInsert(  (AVLTree*)idx->tree, term, doc_id, title); break;
        case TREE_RB:    rbInsert(   (RBTree*) idx->tree, term, doc_id, title); break;
        case TREE_BTREE: btreeInsert((BTree*)  idx->tree, term, doc_id, title); break;
    }
}

Vector* lookupTerm(const Index* idx, const char* term) {
    switch (idx->type) {
        case TREE_AVL:   return avlSearch(  (const AVLTree*)idx->tree, term);
        case TREE_RB:    return rbSearch(   (const RBTree*) idx->tree, term);
        case TREE_BTREE: return btreeSearch((const BTree*)  idx->tree, term);
    }
    return NULL;
}

void indexDocument(Index* idx, int doc_id, const char* title,
                   const char** tokens, int n_tokens) {
    for (int i = 0; i < n_tokens; i++) {
        int dup = 0;
        for (int j = 0; j < i && !dup; j++)
            dup = strcmp(tokens[j], tokens[i]) == 0;
        if (!dup) insertTerm(idx, tokens[i], doc_id, title);
    }
}

void traverseIndex(const Index* idx,
                   void (*visit)(const char* key, Vector* postings, void* ctx), void* ctx) {
    switch (idx->type) {
        case TREE_AVL:   avlTraverse(  (const AVLTree*)idx->tree, visit, ctx); break;
        case TREE_RB:    rbTraverse(   (const RBTree*) idx->tree, visit, ctx); break;
        case TREE_BTREE: btreeTraverse((const BTree*)  idx->tree, visit, ctx); break;
    }
}

static void saveVisitor(const char* key, Vector* postings, void* ctx) {
    FILE* f = (FILE*)ctx;
    for (size_t i = 0; i < postings->size; i++) {
        PostingEntry* e = getVectorItem(postings, i);
        char title[MAX_TITLE_LEN];
        strncpy(title, e->title, MAX_TITLE_LEN - 1);
        title[MAX_TITLE_LEN - 1] = '\0';
        for (char* p = title; *p; p++) if (*p == '\t') *p = ' ';
        fprintf(f, "%s\t%d\t%s\n", key, e->doc_id, title);
    }
}

void saveIndex(const Index* idx, const char* path) {
    FILE* f = fopen(path, "w");
    if (!f) { fprintf(stderr, "Cannot write index: %s\n", path); return; }
    traverseIndex(idx, saveVisitor, f);
    fclose(f);
}

Index* loadIndex(const char* path, TreeType type) {
    FILE* f = fopen(path, "r");
    if (!f) { fprintf(stderr, "Cannot open index: %s\n", path); return NULL; }
    Index* idx = createIndex(type);
    char line[MAX_TITLE_LEN + 512];
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = '\0';
        char* key   = strtok(line, "\t");
        char* id_s  = strtok(NULL, "\t");
        char* title = strtok(NULL, "\t");
        if (!key || !id_s || !title) continue;
        insertTerm(idx, key, atoi(id_s), title);
    }
    fclose(f);
    return idx;
}

void freeIndex(Index* idx) {
    if (!idx) return;
    switch (idx->type) {
        case TREE_AVL:   freeAVLTree((AVLTree*)idx->tree); break;
        case TREE_RB:    freeRBTree( (RBTree*) idx->tree); break;
        case TREE_BTREE: freeBTree(  (BTree*)  idx->tree); break;
    }
    free(idx);
}
