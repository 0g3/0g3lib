// Copyright Koki Matsumura. All rights reserved.

/**
 * @file slice.c
 * @brief 動的配列ライブラリ。
 */

#include "slice.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 *
 * @param elSize 要素のサイズ。たとえば、int型を格納したいならsizeof(int)。
 * @param cap キャパシティ。要素数で指定する。
 *            キャパシティを大きく設定すれば、その分余剰にヒープ領域を占有することになるが、
 *            SliceAppend時にリアロケートする回数が減るので、一般にはAppend系処理が高速になる。
 * @return
 */
Slice* NewSlice(unsigned int elSize, int cap) {
    Slice* s = malloc(sizeof(Slice));
    if (cap < 0 || s == NULL) {
        return NULL;
    }

    void* head = calloc((unsigned int)cap, elSize);
    if (head == NULL) {
        free(s);
        return NULL;
    }

    s->head = head;
    s->len = 0;
    s->cap = cap;
    s->elSize = elSize;
    return s;
}

void DeleteSlice(Slice* s) {
    free(s->head);
    free(s);
}

/**
 * スライスの要素を参照する。コピーではないので注意。
 * @param s
 * @param i 0始まりのインデックスでマイナス値も使える（-nは末尾からn番目の要素）。
 * @return インデックスの有効範囲から外れたiのときNULLを返す。
 */
void* SliceGet(Slice* s, int i) {
    if (i >= 0) {
        if (i >= s->len) {
            return NULL;
        }
        return s->head + i*s->elSize;
    } else {
        if (s->len+i < 0) {
            return NULL;
        }
        return s->head + (s->len+i)*s->elSize;
    }
}

/**
 * スライスを部分的に指定し、コピーする。
 * @param s
 * @param i1 始点。0始まりのインデックスでマイナス値も使える（-nは末尾からn番目の要素）。
 * @param i2 終点。同i1。
 * @param step イテレートの間隔であり、たとえば2のときは要素をひとつ飛ばしにしていく。<br>
 *             なお、stepがマイナス値のときイテレートが逆になる。<br>
 *             つまり始点から終点の順ではなく、終点から始点の順になる。<br>
 * @return 成功時には新しいスライス、失敗時にはNULLを返す。
 */
Slice* SliceSlice(Slice* s, int i1, int i2, int step) {
    Slice* ret = NULL;

    int i = 0;

    if (i1 >= s->len || i1 < -s->len || i2 > s->len || i2 < -s->len || step == 0) {
        return ret;
    }

    if (i1 < 0) {
        i1 = s->len+i1;
    }
    if (i2 < 0) {
        i2 = s->len+i2+1;
    }

    // 終点は始点より小さくならない
    if (i1 >= i2) {
        return NULL;
    }

    ret = NewSlice(s->elSize, i2-i1);
    if (ret == NULL) {
        return NULL;
    }

    // 値のコピー
    if (step > 0) {
        for (i = i1; i < i2; i+=step) {
            void* dat = SliceGet(s, i);
            Slice* result;
            if (dat == NULL) {
                DeleteSlice(ret);
                return NULL;
            }
            result = SliceAppend(ret, dat);
            if (result == NULL) {
                DeleteSlice(ret);
                return NULL;
            }
        }
    } else {
        for (i = i2-1; i >= i1; i+=step) {
            void* dat = SliceGet(s, i);
            Slice* result;
            if (dat == NULL) {
                DeleteSlice(ret);
                return NULL;
            }
            result = SliceAppend(ret, dat);
            if (result == NULL) {
                DeleteSlice(ret);
                return NULL;
            }
        }
    }
    return ret;
}

/**
 * スライスにマップ関数を適応する。要素ごとになにか処理を行いたいときに使う。
 * @param s
 * @param elSize マップ関数の戻り値の型のサイズ。
 * @param f マップ関数。
 * @return 成功時には新しいスライス、失敗時にはNULLを返す。
 */
Slice* SliceMap(Slice* s, unsigned int elSize, void* (*f)(void*)) {
    int i;
    Slice* ret = NewSlice(elSize, s->len);
    for (i = 0; i < s->len; i++) {
        if (SliceAppend(ret, f(SliceGet(s, i))) == NULL) {
            return NULL;
        }
    }
    return ret;
}

/**
 * スライスにフィルター関数を適応する。特定の条件を満たす値を取り出したいときに使う。
 * @param s
 * @param f フィルター関数。
 * @return 成功時には新しいスライス、失敗時にはNULLを返す。
 */
Slice* SliceFilter(Slice* s, int (*f)(void*)) {
    int i;
    Slice* ret = NewSlice(s->elSize, 0);
    for (i = 0; i < s->len; i++) {
        void *dat = SliceGet(s, i);
        if (f(dat)) {
            if (SliceAppend(ret, dat) == NULL) {
                DeleteSlice(ret);
                return NULL;
            }
        }
    }
    return ret;
}

/**
 * スライスにリデュース関数を適応する。全要素を用いてなにか処理をしたいときに使う。
 * @param s
 * @param f リデュース関数。
 * @return 成功時には新しいスライス、失敗時にはNULLを返す。
 */
void* SliceReduce(Slice* s, void* (*f)(void*, void*)) {
    int i;
    void* ret = SliceGet(s, 0);
    for (i = 1; i < s->len; i++) {
        void* d = SliceGet(s, i);
        ret = f(ret, d);
    }
    return ret;
}

/**
 * スライスの各要素に関数によって渡された処理を適応する。
 * SliceMap関数に似ているが、この関数は新しいスライスを生成しない。
 * @param s
 * @param f 各要素に適応したい処理を記述した関数。
 */
void SliceForEach(Slice* s, void (*f)(void*)) {
    int i;
    for (i = 0; i < s->len; i++) {
        f(SliceGet(s, i));
    }
}

/**
 * スライスに要素を追加する。<br>
 * スライスのcapを超えるとき、(追加しようとしている要素サイズ+現在のスライスサイズ)*2のメモリをリアロケートする。
 * @param s
 * @param dat 追加したいデータ。NewSlice関数で指定したデータ型と異なる場合、スライスは壊れるので注意が必要。
 * @return 成功時には引数sで指定されたスライス、失敗時にはNULLを返す。
 */
Slice* SliceAppend(Slice* s, void* dat) {
    void* el;

    if (s->len == s->cap) {
        void *new = realloc(s->head, (s->cap+s->elSize)*s->elSize*2);
        if (new == NULL) {
            return NULL;
        }
        s->head = new;
        s->cap = (s->cap+1)*2;
    }

    el = s->head + s->len*s->elSize;
    memcpy(el, dat, s->elSize);

    s->len++;

    return s;
}

/**
 * スライスに要素の配列を追加する。
 * @param s
 * @param a 要素の配列。
 * @param cnt 要素のサイズ。
 * @return 成功時には引数sで指定されたスライス、失敗時にはNULLを返す。
 */
Slice* SliceAppendArray(Slice* s, void* a, unsigned int cnt) {
    int i;
    for (i = 0; i < cnt; i++) {
        if (SliceAppend(s, a + (s->elSize)*i) == NULL) {
            return NULL;
        }
    }
    return s;
}

/**
 * スライスにスライスを追加する関数。
 * @param s1 追加されるスライス。
 * @param s2 追加するスライス。
 * @return 成功時には引数s1で指定されたスライス、失敗時にはNULLを返す。
 */
Slice* SliceExtend(Slice* s1, Slice* s2) {
    int i;
    for (i = 0; i < s2->len; i++) {
        if (SliceAppend(s1, SliceGet(s2, i)) == NULL) {
            return NULL;
        }
    }
    return s1;
}

/**
 * スライスを空にする関数。
 * @param s
 * @return 成功時には引数sで指定されたスライス、失敗時にはNULLを返す。
 */
Slice* SliceEmpty(Slice* s) {
    free(s->head);
    void* head = malloc(0);
    if (head == NULL) {
        return NULL;
    }
    s->head = head;
    s->len = s->cap = 0;
    return s;
}
