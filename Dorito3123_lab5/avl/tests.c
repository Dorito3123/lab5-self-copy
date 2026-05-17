#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "avl.h"

static void test_basic(void) {
    AVLTree* t = createAVLTree();
    avlInsert(t, "python", 1, "Python basics");
    avlInsert(t, "list", 2, "Python list");
    avlInsert(t, "sort", 3, "Sorting");

    assert(t->size == 3);
    Vector* v = avlSearch(t, "python");
    assert(v && v->size == 1);
    assert(((PostingEntry*)getVectorItem(v, 0))->doc_id == 1);
    assert(avlSearch(t, "missing") == NULL);

    freeAVLTree(t);
    printf("avl basic: пройдено\n");
}

static void test_duplicate(void) {
    AVLTree* t = createAVLTree();
    avlInsert(t, "python", 1, "a");
    avlInsert(t, "python", 2, "b");
    avlInsert(t, "python", 3, "c");

    assert(t->size == 1);
    assert(avlSearch(t, "python")->size == 3);

    freeAVLTree(t);
    printf("avl duplicate: пройдено\n");
}

static void test_balance(void) {
    AVLTree* t = createAVLTree();
    for (int i = 0; i < 100; i++) {
        char k[16]; snprintf(k, sizeof(k), "k%03d", i);
        avlInsert(t, k, i, "x");
    }

    assert(t->size == 100);

    for (int i = 0; i < 100; i++) {
        char k[16]; snprintf(k, sizeof(k), "k%03d", i);
        assert(avlSearch(t, k));
    }
    freeAVLTree(t);
    printf("avl balance: пройдено\n");
}

static int cnt = 0;
static void countv(const char* k, Vector* p, void* c) {
    (void)k;
    (void)p;
    (void)c;
    cnt++;
}

static void test_traverse(void) {
    AVLTree* t = createAVLTree();
    avlInsert(t, "c", 1, "C"); avlInsert(t, "b", 2, "B"); avlInsert(t, "a", 3, "A");
    cnt = 0; avlTraverse(t, countv, NULL);
    assert(cnt == 3);
    freeAVLTree(t);
    printf("avl traverse: пройдено\n");
}

int main(void) {
    printf("\navl tests\n\n");
    test_basic();
    test_duplicate();
    test_balance();
    test_traverse();
    printf("\nвсе avl tests пройдены\n\n");
    return 0;
}
