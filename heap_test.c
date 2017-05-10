#include "heap.h" 

#include <stdlib.h>
#include <string.h> 
#include <time.h> 
#include <assert.h> 
#include <stdio.h> 

typedef struct IdxInt {
    int val;
    size_t idx;
} IdxInt;

static void print_int_heap(Heap *H)
{
    size_t i;
    for (i = 0; i < H->size; i++) {
        printf("%d ", *(int*)H->A[i]);
    }
    printf("\n");
}

static int print_IdxInt_heap(Heap *H)
{
    size_t i;
    int passed = 1;
    for (i = 0; i < H->size; i++) {
        printf("x->val: %2d x->idx: %2lu idx: %2lu",
                ((IdxInt*)H->A[i])->val,
                ((IdxInt*)H->A[i])->idx,
                i);
        printf("\n");
        passed &= (((IdxInt*)H->A[i])->idx == i);
    }
    printf("\n");
    return passed;
}

/* index of parent */
#define PARENT(i) (((size_t)i + 1)/2 - 1)

/* index of left child */
#define LEFT(i) (2*(size_t)i+1)

/* index of right child */
#define RIGHT(i) (LEFT(i)+1) 

/* This comparison function makes a min heap where equal elements are allowed */
static int int_cmp(void *a, void *b)
{
    return (*((int*)a) >= *((int*)b));
}

/* This set index function does nothing as we are not interested in this
 * information */
static void set_idx_ignore(void *a, size_t idx)
{
    return;
}

int IdxInt_cmp(void *a, void *b)
{
    IdxInt *a_, *b_;
    a_ = (IdxInt*)a;
    b_ = (IdxInt*)b;
    return (a_->val >= b_->val);
}

void IdxInt_set_idx(void *a, size_t idx)
{
    ((IdxInt*)a)->idx = idx;
}

static int chk_is_heap(Heap *H, size_t i, int cmp(void*,void*))
{
    int passed = 1;
    if (LEFT(i) < H->size) {
        passed &= cmp(H->A[LEFT(i)],H->A[i]);
        passed &= chk_is_heap(H,LEFT(i),cmp);
    }
    if (RIGHT(i) < H->size) {
        passed &= cmp(H->A[RIGHT(i)],H->A[i]);
        passed &= chk_is_heap(H,RIGHT(i),cmp);
    }
    return passed;
}

static int chk_is_sorted(int *A, size_t size, int cmp(void*,void*))
{
    int passed = 1;
    if (size <= 1) {
        return 1;
    }
    while (size-- > 1) {
        passed &= cmp((void*)&A[size],(void*)&A[size-1]);
    }
    return passed;
}

static int IdxInt_chk_is_sorted(IdxInt *A, size_t size, int cmp(void*,void*))
{
    int passed = 1;
    if (size <= 1) {
        return 1;
    }
    while (size-- > 1) {
        passed &= cmp((void*)&A[size],(void*)&A[size-1]);
    }
    return passed;
}

#define ARRAY_LEN 20

int Heap_min_heap_test_ignore_idx(void)
{

    int A[ARRAY_LEN];
    int *pA[ARRAY_LEN];
    int B[ARRAY_LEN];
    size_t i;
    int passed = 1;
    srandom(time(NULL));
    for (i = 0; i < ARRAY_LEN; i++) {
        A[i] = random() % 100;
        pA[i] = &A[i];
    }
    Heap *H;
    HEAP_ALLOC(H, ARRAY_LEN, int_cmp, set_idx_ignore);
    Heap_make_heap(H, (void**)pA, ARRAY_LEN);
//    print_int_heap(H);
    passed &= chk_is_heap(H, 0, int_cmp);
    assert(passed);
    memset(B, 0, sizeof(int)*ARRAY_LEN);
    i = 0;
    while (H->size) {
        int *key;
        Heap_pop(H, (void*)&key);
        B[i++] = *key;
    }
    passed &= chk_is_sorted(B,ARRAY_LEN,int_cmp);
    assert(passed);
    for (i = 0; i < ARRAY_LEN; i++) {
        HeapErr err;
        err = Heap_push(H, (void*)pA[i]);
        if (err) {
            break;
        }
    }
    passed &= chk_is_heap(H, 0,int_cmp);
    assert(passed);
    return passed;
}

int Heap_min_heap_test_with_idx(void)
{

    IdxInt A[ARRAY_LEN];
    IdxInt *pA[ARRAY_LEN];
    IdxInt B[ARRAY_LEN];
    size_t i;
    int passed = 1;
    srandom(time(NULL));
    for (i = 0; i < ARRAY_LEN; i++) {
        A[i].val = random() % 100;
        A[i].idx = i; /* Index must be provided when using Heap_make_heap */
        pA[i] = &A[i];
    }
    Heap *H;
    HEAP_ALLOC(H, ARRAY_LEN, IdxInt_cmp, IdxInt_set_idx);
    Heap_make_heap(H, (void**)pA, ARRAY_LEN);
    passed &= print_IdxInt_heap(H);
    passed &= chk_is_heap(H, 0, IdxInt_cmp);
    assert(passed);
    memset(B, 0, sizeof(IdxInt)*ARRAY_LEN);
    i = 0;
    while (H->size) {
        IdxInt *key;
        Heap_pop(H, (void*)&key);
        B[i++] = *key;
    }
    IdxInt_chk_is_sorted(B,ARRAY_LEN,IdxInt_cmp);
    assert(passed);
    for (i = 0; i < ARRAY_LEN; i++) {
        HeapErr err;
        /* Scramble items stored indices to show that Heap_push puts them in
         * properly. */
        pA[i]->idx = random();
        err = Heap_push(H, (void*)pA[i]);
        if (err) {
            break;
        }
    }
    passed &= chk_is_heap(H, 0,IdxInt_cmp);
    passed &= print_IdxInt_heap(H);
    assert(passed);
    return passed;
}

int main(void)
{
    int passed = 1;
    passed &= Heap_min_heap_test_ignore_idx();
    passed &= Heap_min_heap_test_with_idx();
    return -1 * (!passed);
}
    


