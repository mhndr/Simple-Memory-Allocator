cc -shared -o libmemalloc.so -fPIC mem_alloc.c
cc ./lru.c -o lru -L.  -lmemalloc
