#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "btree.h"

static void test_basic(void) {
    BTree* t = createBTree();
    btreeInsert(t, "python", 1, "Python basics");
    btreeInsert(t, "list", 2, "Python list");
    btreeInsert(t, "sort", 3, "Sorting");

    assert(t->size == 3);
    Vector* v = btreeSearch(t, "python");
    assert(v && v->size == 1);
    assert(((PostingEntry*)getVectorItem(v, 0))->doc_id == 1);
    assert(btreeSearch(t, "missing") == NULL);

    freeBTree(t);
    printf("btree basic: пройдено\n");
}

static void test_duplicate(void) {
    BTree* t = createBTree();
    btreeInsert(t, "python", 1, "a");
    btreeInsert(t, "python", 2, "b");
    btreeInsert(t, "python", 3, "c");

    assert(t->size == 1);
    assert(btreeSearch(t, "python")->size == 3);

    freeBTree(t);
    printf("btree duplicate: пройдено\n");
}

static void test_splits(void) {
    BTree* t = createBTree();
    const char* keys[] = {"apple","banana","cherry","date","elderberry",
                          "fig","grape","honeydew","kiwi","lemon"};
    for (int i = 0; i < 10; i++) {
        btreeInsert(t, keys[i], i, "x");
    }

    assert(t->size == 10);

    for (int i = 0; i < 10; i++) {
        assert(btreeSearch(t, keys[i]));
    }

    freeBTree(t);
    printf("btree splits: пройдено\n");
}

static int cnt = 0;
static void countv(const char* k, Vector* p, void* c) {
    (void)k;
    (void)p;
    (void)c;
    cnt++;
}

static void test_traverse(void) {
    BTree* t = createBTree();
    for (int i = 0; i < 20; i++) {
        char k[16]; snprintf(k, sizeof(k), "w%02d", i);
        btreeInsert(t, k, i, "x");
    }

    cnt = 0; btreeTraverse(t, countv, NULL);
    assert(cnt == 20);
    freeBTree(t);
    printf("btree traverse: пройдено\n");
}

int main(void) {
    printf("\nbtree tests\n\n");
    test_basic();
    test_duplicate();
    test_splits();
    test_traverse();
    printf("\nвсе btree tests пройдены\n\n");
    return 0;
}
