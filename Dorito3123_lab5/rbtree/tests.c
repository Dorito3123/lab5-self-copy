#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "rbtree.h"

static void test_basic(void) {
    RBTree* t = createRBTree();
    rbInsert(t, "python", 1, "Python basics");
    rbInsert(t, "list", 2, "Python list");
    rbInsert(t, "sort", 3, "Sorting");

    assert(t->size == 3);
    Vector* v = rbSearch(t, "python");
    assert(v && v->size == 1);
    assert(((PostingEntry*)getVectorItem(v, 0))->doc_id == 1);
    assert(rbSearch(t, "missing") == NULL);

    freeRBTree(t);
    printf("rb basic: пройдено\n");
}

static void test_duplicate(void) {
    RBTree* t = createRBTree();
    rbInsert(t, "python", 1, "a");
    rbInsert(t, "python", 2, "b");
    rbInsert(t, "python", 3, "c");

    assert(t->size == 1);
    assert(rbSearch(t, "python")->size == 3);

    freeRBTree(t);
    printf("rb duplicate: пройдено\n");
}

static void test_many(void) {
    RBTree* t = createRBTree();
    for (int i = 0; i < 200; i++) {
        char k[16]; snprintf(k, sizeof(k), "w%04d", i);
        rbInsert(t, k, i, "x");
    }

    assert(t->size == 200);

    for (int i = 0; i < 200; i++) {
        char k[16]; snprintf(k, sizeof(k), "w%04d", i);
        assert(rbSearch(t, k));
    }
    freeRBTree(t);
    printf("rb many: пройдено\n");
}

static int cnt = 0;

static void countv(const char* k, Vector* p, void* c) {
    (void)k;
    (void)p;
    (void)c; 
    cnt++;
}

static void test_traverse(void) {
    RBTree* t = createRBTree();
    rbInsert(t, "z", 1, "Z"); rbInsert(t, "a", 2, "A"); rbInsert(t, "m", 3, "M");
    cnt = 0; rbTraverse(t, countv, NULL);
    assert(cnt == 3);
    freeRBTree(t);
    printf("rb traverse: пройдено\n");
}

int main(void) {
    printf("\nrb tests\n\n");
    test_basic();
    test_duplicate();
    test_many();
    test_traverse();
    printf("\nвсе rb tests пройдены\n\n");
    return 0;
}
