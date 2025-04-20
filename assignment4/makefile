CFLAGS = -g -Wall -Wextra -Wpedantic -Wno-unused-parameter

CC= clang
.PHONY: all
all: test_assign4

test_assign4: test_assign4_1.c storage_mgr.c dberror.c buffer_mgr.c buffer_mgr_stat.c btree_mgr_helper.c expr.c record_mgr.c rm_serializer.c btree_mgr.c
	$(CC) $(CFLAGS) -o test_assign4 test_assign4_1.c storage_mgr.c dberror.c buffer_mgr.c buffer_mgr_stat.c btree_mgr_helper.c expr.c record_mgr.c rm_serializer.c btree_mgr.c

.PHONY: clean
clean:
	rm -f test_assign4 *.o result.txt testidx

run:
	./test_assign4