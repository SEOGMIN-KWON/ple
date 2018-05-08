output/txn_test: runtime/runtime.c include/common.h include/lwt.h \
 include/runtime.h include/tls.h runtime/rlocal.h include/tls.h \
 include/mem.h
output/txn_test: runtime/rcu.c include/common.h include/lwt.h \
 runtime/rlocal.h include/runtime.h include/tls.h include/tls.h \
 include/lwt.h include/mem.h include/rcu.h
output/txn_test: runtime/lwt.c include/common.h include/lwt.h \
 runtime/rlocal.h include/runtime.h include/tls.h include/tls.h \
 include/lwt.h include/mem.h
output/txn_test: runtime/mem.c include/common.h include/lwt.h \
 runtime/rlocal.h include/runtime.h include/tls.h include/tls.h \
 include/lwt.h
output/txn_test: runtime/random.c include/common.h include/lwt.h \
 include/runtime.h include/tls.h
output/txn_test: datatype/nstring.c include/common.h include/lwt.h \
 include/nstring.h include/common.h include/datatype.h include/murmur.h \
 include/mem.h
output/txn_test: map/map.c include/common.h include/lwt.h include/map.h \
 include/datatype.h include/mem.h
output/txn_test: map/list.c include/common.h include/lwt.h include/list.h \
 include/map.h include/datatype.h include/mem.h include/rcu.h
output/txn_test: map/skiplist.c include/common.h include/lwt.h \
 include/skiplist.h include/map.h include/datatype.h include/runtime.h \
 include/tls.h include/mem.h include/rcu.h
output/txn_test: map/hashtable.c include/common.h include/lwt.h \
 include/murmur.h include/mem.h include/rcu.h include/hashtable.h \
 include/map.h include/datatype.h
output/txn_test: test/txn_test.c test/CuTest.h include/common.h \
 include/lwt.h include/runtime.h include/tls.h include/txn.h \
 include/map.h include/datatype.h include/map.h include/hashtable.h
output/txn_test: test/CuTest.c test/CuTest.h
output/txn_test: txn/txn.c include/common.h include/lwt.h include/txn.h \
 include/map.h include/datatype.h include/mem.h include/rcu.h \
 include/lwt.h include/skiplist.h
