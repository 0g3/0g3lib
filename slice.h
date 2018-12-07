// Copyright Koki Matsumura. All rights reserved.

#ifndef INC_0G3LIB_SLICE_H
#define INC_0G3LIB_SLICE_H

typedef struct {
    void *head;
    int len;
    int cap;
    unsigned int elSize;
} Slice;

Slice* NewSlice(unsigned int elSize, int cap);
void DeleteSlice(Slice* s);
void* SliceGet(Slice* s, int i);
Slice* SliceSlice(Slice* s, int i1, int i2, int step);
Slice* SliceMap(Slice* s, unsigned int elSize, void* (*f)(void*));
Slice* SliceFilter(Slice* s, int (*f)(void*));
void* SliceReduce(Slice* s, void* (*f)(void*, void*));
void SliceForEach(Slice* s, void (*f)(void*));
Slice* SliceAppend(Slice* s, void* dat);
Slice* SliceAppendArray(Slice* s, void* a, unsigned int cnt);
Slice* SliceExtend(Slice* s1, Slice* s2);
Slice* SliceEmpty(Slice* s);

#endif //INC_0G3LIB_SLICE_H
