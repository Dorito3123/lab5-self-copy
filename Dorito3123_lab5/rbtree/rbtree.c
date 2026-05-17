#include <stdlib.h>
#include <string.h>
#include "rbtree.h"

static RBNode* mknode(RBTree* t, const char* key, int doc_id, const char* title) {
    RBNode* n = malloc(sizeof(RBNode));
    n->key = strdup(key);
    n->color = RB_RED;
    n->postings = createPostingList();
    n->left = n->right = n->parent = t->nil;
    appendPosting(n->postings, doc_id, title);
    return n;
}

static void rotL(RBTree* t, RBNode* x) {
    RBNode* y = x->right;
    x->right = y->left;
    if (y->left != t->nil) {
        y->left->parent = x;
    }
    y->parent = x->parent;
    if (x->parent == t->nil) {
        t->root = y;
    } else if (x == x->parent->left) {
        x->parent->left  = y;
    } else {
        x->parent->right = y;
    }
    y->left = x;
    x->parent = y;
}

static void rotR(RBTree* t, RBNode* y) {
    RBNode* x = y->left;
    y->left = x->right;
    if (x->right != t->nil) {
        x->right->parent = y;
    }
    x->parent = y->parent;
    if (y->parent == t->nil) {
        t->root = x;
    } else if (y == y->parent->right) {
        y->parent->right = x;
    }
    else {
        y->parent->left = x;
    }
    x->right = y;
    y->parent = x;
}

static void fixup(RBTree* t, RBNode* z) {
    while (z->parent->color == RB_RED) {
        if (z->parent == z->parent->parent->left) {
            RBNode* u = z->parent->parent->right;
            if (u->color == RB_RED) {
                z->parent->color = u->color = RB_BLACK;
                z->parent->parent->color = RB_RED;
                z = z->parent->parent;
            } else {
                if (z == z->parent->right) {
                    z = z->parent;
                    rotL(t, z);
                }
                z->parent->color = RB_BLACK;
                z->parent->parent->color = RB_RED;
                rotR(t, z->parent->parent);
            }
        } else {
            RBNode* u = z->parent->parent->left;
            if (u->color == RB_RED) {
                z->parent->color = u->color = RB_BLACK;
                z->parent->parent->color = RB_RED;
                z = z->parent->parent;
            } else {
                if (z == z->parent->left) {
                    z = z->parent; rotR(t, z);
                }
                z->parent->color = RB_BLACK;
                z->parent->parent->color = RB_RED;
                rotL(t, z->parent->parent);
            }
        }
    }
    t->root->color = RB_BLACK;
}

static void dropNodes(RBTree* t, RBNode* n) {
    if (n == t->nil) return;

    dropNodes(t, n->left);
    dropNodes(t, n->right);
    free(n->key); 
    vectorFree(n->postings);
    free(n);
}

RBTree* createRBTree(void) {
    RBTree* t = malloc(sizeof(RBTree));
    RBNode* nil = malloc(sizeof(RBNode));
    nil->color = RB_BLACK;
    nil->key = NULL;
    nil->postings = NULL;
    nil->left = nil->right = nil->parent = nil;
    t->nil = nil;
    t->root = nil;
    t->size = 0;

    return t;
}

void freeRBTree(RBTree* t) {
    if (!t) return;

    dropNodes(t, t->root);
    free(t->nil);
    free(t);
}

void rbInsert(RBTree* t, const char* key, int doc_id, const char* title) {
    RBNode* cur = t->root;
    while (cur != t->nil) {
        int c = strcmp(key, cur->key);
        if (c == 0) {
            appendPosting(cur->postings, doc_id, title);
            return;
        }
        cur = c < 0 ? cur->left : cur->right;
    }

    RBNode* z = mknode(t, key, doc_id, title);
    RBNode* y = t->nil, *x = t->root;
    while (x != t->nil) {
        y = x; x = strcmp(z->key, x->key) < 0 ? x->left : x->right;
    }
    z->parent = y;
    if (y == t->nil) {
        t->root  = z;
    }
    else if (strcmp(z->key, y->key) < 0) {
        y->left  = z;
    }
    else {
        y->right = z;
    }

    fixup(t, z);
    t->size++;
}

Vector* rbSearch(const RBTree* t, const char* key) {
    RBNode* cur = t->root;
    while (cur != t->nil) {
        int c = strcmp(key, cur->key);
        if (c == 0) {
            return cur->postings;
        }
        cur = c < 0 ? cur->left : cur->right;
    }

    return NULL;
}

static void walkNodes(RBTree* t, RBNode* n, void (*visit)(const char* key, Vector* postings, void* ctx), void* ctx) {
    if (n == t->nil) return;

    walkNodes(t, n->left, visit, ctx);
    visit(n->key, n->postings, ctx);
    walkNodes(t, n->right, visit, ctx);
}

void rbTraverse(const RBTree* t, void (*visit)(const char* key, Vector* postings, void* ctx), void* ctx) {
    walkNodes((RBTree*)t, t->root, visit, ctx);
}
