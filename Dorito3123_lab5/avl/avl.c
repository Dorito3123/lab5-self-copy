#include <stdlib.h>
#include <string.h>
#include "avl.h"

static int height(AVLNode* n) {
    return n ? n->height : 0;
}

static int bf(AVLNode* n) {
    return height(n->left) - height(n->right);
}

static void upd(AVLNode* n) {
    int l = height(n->left), r = height(n->right);
    n->height = 1 + (l > r ? l : r);
}

static AVLNode* rr(AVLNode* y) {
    AVLNode* x = y->left, *t = x->right;
    x->right = y;
    y->left = t;
    upd(y);
    upd(x);

    return x;
}

static AVLNode* rl(AVLNode* x) {
    AVLNode* y = x->right, *t = y->left;
    y->left = x;
    x->right = t;
    upd(x);
    upd(y);

    return y;
}

static AVLNode* makeNode(const char* key, int doc_id, const char* title) {
    AVLNode* n  = malloc(sizeof(AVLNode));
    n->key = strdup(key);
    n->height = 1;
    n->postings = createPostingList();
    n->left = n->right = NULL;
    appendPosting(n->postings, doc_id, title);

    return n;
}

static AVLNode* ins(AVLNode* n, const char* key, int doc_id, const char* title, int* added) {
    if (!n) {
        *added = 1;
        return makeNode(key, doc_id, title);
    }

    int c = strcmp(key, n->key);

    if (c < 0) {
        n->left  = ins(n->left,  key, doc_id, title, added);
    } else if (c > 0) {
        n->right = ins(n->right, key, doc_id, title, added);
    } else {
        appendPosting(n->postings, doc_id, title);
        return n;
    }

    upd(n);
    int b = bf(n);
    if (b >  1 && strcmp(key, n->left->key)  < 0) {
        return rr(n);
    }
    if (b < -1 && strcmp(key, n->right->key) > 0) {
        return rl(n);
    }
    if (b >  1 && strcmp(key, n->left->key)  > 0) {
        n->left  = rl(n->left); 
        return rr(n);
    }
    if (b < -1 && strcmp(key, n->right->key) < 0) {
        n->right = rr(n->right);
        return rl(n);
    }

    return n;
}

static AVLNode* find(AVLNode* n, const char* key) {
    if (!n) return NULL;

    int c = strcmp(key, n->key);

    if (c == 0) return n;

    return find(c < 0 ? n->left : n->right, key);
}

static void walk(AVLNode* n, void (*visit)(const char* key, Vector* postings, void* ctx), void* ctx) {
    if (!n) return;

    walk(n->left, visit, ctx);
    visit(n->key, n->postings, ctx);
    walk(n->right, visit, ctx);
}

static void drop(AVLNode* n) {
    if (!n) return;

    drop(n->left);
    drop(n->right);
    free(n->key);
    vectorFree(n->postings); 
    free(n);
}

AVLTree* createAVLTree(void) {
    AVLTree* t = malloc(sizeof(AVLTree));
    t->root = NULL; t->size = 0;

    return t;
}

void freeAVLTree(AVLTree* t) {
    if (t) {
        drop(t->root);
        free(t);
    }
}

void avlInsert(AVLTree* t, const char* key, int doc_id, const char* title) {
    int added = 0;
    t->root = ins(t->root, key, doc_id, title, &added);
    if (added) {
        t->size++;
    }

}

Vector* avlSearch(const AVLTree* t, const char* key) {
    AVLNode* n = find(t->root, key);

    return n ? n->postings : NULL;
}

void avlTraverse(const AVLTree* t, void (*visit)(const char* key, Vector* postings, void* ctx), void* ctx) {
    walk(t->root, visit, ctx);
}
