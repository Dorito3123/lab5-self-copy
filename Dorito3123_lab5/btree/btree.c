#include <stdlib.h>
#include <string.h>
#include "btree.h"

static BTreeNode* mknode(int leaf) {
    BTreeNode* n = calloc(1, sizeof(BTreeNode));
    n->is_leaf = leaf;
    return n;
}

static void dropNode(BTreeNode* n) {
    if (!n) return;
    for (int i = 0; i < n->n; i++) { free(n->keys[i]); vectorFree(n->postings[i]); }
    if (!n->is_leaf) for (int i = 0; i <= n->n; i++) dropNode(n->children[i]);
    free(n);
}

static int findKey(BTreeNode* n, const char* key) {
    for (int i = 0; i < n->n; i++) if (strcmp(n->keys[i], key) == 0) return i;
    return -1;
}

static int childIdx(BTreeNode* n, const char* key) {
    int i = 0;
    while (i < n->n && strcmp(key, n->keys[i]) > 0) i++;
    return i;
}

static Vector* findIn(BTreeNode* n, const char* key) {
    if (!n) return NULL;
    int i = findKey(n, key);
    if (i >= 0) return n->postings[i];
    if (n->is_leaf) return NULL;
    return findIn(n->children[childIdx(n, key)], key);
}

static void splitChild(BTreeNode* x, int i, BTreeNode* y) {
    int t = BTREE_T;
    BTreeNode* z = mknode(y->is_leaf);
    z->n = t - 1;
    for (int j = 0; j < t - 1; j++) {
        z->keys[j] = y->keys[j + t]; z->postings[j] = y->postings[j + t];
        y->keys[j + t] = NULL; y->postings[j + t] = NULL;
    }
    if (!y->is_leaf) for (int j = 0; j < t; j++) { z->children[j] = y->children[j + t]; y->children[j + t] = NULL; }
    for (int j = x->n - 1; j >= i; j--) {
        x->keys[j + 1] = x->keys[j]; x->postings[j + 1] = x->postings[j];
    }
    for (int j = x->n; j >= i + 1; j--) {
        x->children[j + 1] = x->children[j];
    }
    x->keys[i] = y->keys[t - 1]; x->postings[i] = y->postings[t - 1];
    y->keys[t - 1] = NULL; y->postings[t - 1] = NULL;
    x->children[i + 1] = z;
    x->n++;
    y->n = t - 1;
}

static void insNonFull(BTreeNode* n, const char* key, int doc_id, const char* title) {
    int idx = findKey(n, key);
    if (idx >= 0) { appendPosting(n->postings[idx], doc_id, title); return; }

    if (n->is_leaf) {
        int i = n->n - 1;
        while (i >= 0 && strcmp(key, n->keys[i]) < 0) {
            n->keys[i + 1] = n->keys[i]; n->postings[i + 1] = n->postings[i]; i--;
        }
        n->keys[i + 1]     = strdup(key);
        n->postings[i + 1] = createPostingList();
        appendPosting(n->postings[i + 1], doc_id, title);
        n->n++;
    } else {
        int ci = childIdx(n, key);
        if (n->children[ci]->n == BTREE_MAX_KEYS) {
            splitChild(n, ci, n->children[ci]);
            if (strcmp(key, n->keys[ci]) == 0) {
                appendPosting(n->postings[ci], doc_id, title); return;
            }
            if (strcmp(key, n->keys[ci]) > 0) ci++;
        }
        insNonFull(n->children[ci], key, doc_id, title);
    }
}

static int appendIfExists(BTreeNode* n, const char* key, int doc_id, const char* title) {
    if (!n) return 0;
    int i = findKey(n, key);
    if (i >= 0) { appendPosting(n->postings[i], doc_id, title); return 1; }
    if (n->is_leaf) return 0;
    return appendIfExists(n->children[childIdx(n, key)], key, doc_id, title);
}

static void walkNode(BTreeNode* n,
                     void (*visit)(const char* key, Vector* postings, void* ctx), void* ctx) {
    if (!n) return;
    for (int i = 0; i < n->n; i++) {
        if (!n->is_leaf) walkNode(n->children[i], visit, ctx);
        visit(n->keys[i], n->postings[i], ctx);
    }
    if (!n->is_leaf) walkNode(n->children[n->n], visit, ctx);
}

BTree* createBTree(void) {
    BTree* t = malloc(sizeof(BTree));
    t->root = mknode(1); t->size = 0;
    return t;
}

void freeBTree(BTree* t) { if (t) { dropNode(t->root); free(t); } }

void btreeInsert(BTree* t, const char* key, int doc_id, const char* title) {
    if (appendIfExists(t->root, key, doc_id, title)) return;
    if (t->root->n == BTREE_MAX_KEYS) {
        BTreeNode* s = mknode(0);
        s->children[0] = t->root;
        t->root = s;
        splitChild(s, 0, s->children[0]);
        insNonFull(s, key, doc_id, title);
    } else {
        insNonFull(t->root, key, doc_id, title);
    }
    t->size++;
}

Vector* btreeSearch(const BTree* t, const char* key) { return findIn(t->root, key); }

void btreeTraverse(const BTree* t,
                   void (*visit)(const char* key, Vector* postings, void* ctx), void* ctx) {
    walkNode(t->root, visit, ctx);
}
