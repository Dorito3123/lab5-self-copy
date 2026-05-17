#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "generic.h"

// Вспомогательная функция для изменения размера
static bool needToResize(Vector *vector, bool *increase)
{
    if (!vector || !increase) return false; // O(1)
    if (vector->size == vector->capacity) { // O(1)
        *increase = true; // O(1)
        return true; // O(1)
    }
    if (vector->capacity > MIN_SIZE && vector->size <= vector->capacity / 4) { // O(1)
        *increase = false; // O(1)
        return true; // O(1)
    }
    return false; // O(1)
}
/*
    O(1)
    Θ(1)
    Ω(1)
*/

// Определяем увеличивать размер или уменьшать
static int resize(Vector *vector, bool increase)
{
    if (!vector) return -1; // O(1)
    size_t new_cap;
    if (increase) { // O(1)
        new_cap = (vector->capacity == 0 ? MIN_SIZE : vector->capacity * 2); // O(1)
    }
    else {
        new_cap = vector->capacity / 2; // O(1)
        if (new_cap < MIN_SIZE) new_cap = MIN_SIZE; // O(1)
        if (new_cap < vector->size) new_cap = vector->size; // O(1)
    }
    if (new_cap == vector->capacity) return 0; // O(1)
    void *new_data = realloc(vector->data, new_cap * vector->elem_size); // O(N)
    if (!new_data && new_cap > 0) { // O(1)
        return -1; // O(1)
    }
    vector->data = new_data; // O(1)
    vector->capacity = new_cap; // O(1)
    return 0; // O(1)
}
/*
    O(N) из-за realloc
    Θ(N) Не применима однозначно (зависит от фрагментации памяти), но принято считать O(N) для операции ресайза.
    Ω(1)
*/
Vector *createVector(size_t elem_size)
{
    if (elem_size == 0) return NULL;
    Vector *v = (Vector *)malloc(sizeof(Vector));
    if (!v) return NULL;
    v->elem_size = elem_size;
    v->size = 0;
    v->capacity = MIN_SIZE;
    v->data = (v->capacity > 0) ? malloc(v->capacity * v->elem_size) : NULL;
    if (v->capacity > 0 && !v->data) {
        free(v);
        return NULL;
    }
    return v;
}

int appendVectorItem(Vector *vector, void *el)
{
    if (!vector || !el) return -1; // O(1)
    bool inc = false; // O(1)
    if (needToResize(vector, &inc)) { // O(1)
        if (resize(vector, inc) != 0) return -1; //  O(N), O(1) 
    }
    void *dst = (char *)vector->data + vector->size * vector->elem_size; // O(1)
    memcpy(dst, el, vector->elem_size); // O(1)
    vector->size += 1; // O(1)
    return 0; // O(1)
}
/*
    O(N) — когда массив переполнен и срабатывает resize.
    Θ(1)
    Ω(1) 
*/

void *getVectorItem(Vector *vector, size_t index)
{
    if (!vector || index >= vector->size) return NULL; // O(1)
    return (char *)vector->data + index * vector->elem_size; // O(1)
}
/*
    O(1)
    Θ(1)
    Ω(1) 
*/

int setVectorItem(Vector *vector, size_t index, void *value)
{
    if (!vector || !value || index >= vector->size) return -1; // O(1)
    void *dst = (char *)vector->data + index * vector->elem_size; // O(1)
    memcpy(dst, value, vector->elem_size); // O(1)
    return 0; // O(1)
}
/*
    O(1)
    Θ(1)
    Ω(1) 
*/

void *popVectorItem(Vector *vector, size_t index)
{
    if (!vector || vector->size == 0 || index >= vector->size) return NULL; // O(1)
    void *copy = malloc(vector->elem_size); // O(1)
    if (!copy) return NULL; // O(1)
    void *src = (char *)vector->data + index * vector->elem_size; // O(1)
    memcpy(copy, src, vector->elem_size); // O(1)
    size_t tail_count = vector->size - index - 1; //O(1)
    if (tail_count > 0) { // O(1)
        void *from = (char *)vector->data + (index + 1) * vector->elem_size; // O(1)
        memmove(src, from, tail_count * vector->elem_size); // O(N - I) — сдвиг ВСЕХ элементов, идущих после удаляемого
    }
    vector->size -= 1; // O(1)
    bool inc = false; // O(1)
    if (needToResize(vector, &inc) && !inc) { // O(1)
        (void)resize(vector, false); // O(N)
    }
    return copy; // O(1)
}
/*
    O(1) — элемент не найден, приходится просмотреть весь массив
    Θ(N)
    Ω(N) 
*/
long int findVectorItem(Vector *vector, void *value, EqualsFunc cmp)
{
    if (!vector || vector->size == 0 || !value) return -1; //O(1)
    for (size_t i = 0; i < vector->size; ++i) { //O(N)
        void *elem = (char *)vector->data + i * vector->elem_size; //O(1)
        int equal = 0; //O(1)
        if (cmp) { // O(1)
            equal = (cmp(value, elem) != 0); // O(1)
        } 
        else {
            equal = (memcmp(value, elem, vector->elem_size) == 0); // O(1)
        }
        if (equal) return (long int)i; // O(1)
    }
    return -1; // O(1) 
}
/*
    O(N) — элемент не найден, приходится просмотреть весь массив
    Θ(N)
    Ω(1) 
*/

int vectorFree(Vector *vector)
{
    if (!vector) return -1;
    free(vector->data);
    free(vector);
    return 0;
}