if(TARGET atomics)
  add_test(class/atomics ${SHM_TEST_CMD_LIST} class/atomics -c 4)
endif()
add_test(class/rwlock ${SHM_TEST_CMD_LIST} class/rwlock -c 4)
add_test(class/lifo ${SHM_TEST_CMD_LIST} class/lifo -c 4)
add_test(class/list ${SHM_TEST_CMD_LIST} class/list -c 4)
add_test(class/hash ${SHM_TEST_CMD_LIST} class/hash -\# 65536 -r 4 -n)
add_test(class/future ${SHM_TEST_CMD_LIST} class/future -c 4)
add_test(class/future_datacopy ${SHM_TEST_CMD_LIST} class/future_datacopy)

if(TARGET atomics_inline)
  add_test(class/atomics:inline ${SHM_TEST_CMD_LIST} class/atomics_inline -c 4)
endif()
add_test(class/rwlock:inline ${SHM_TEST_CMD_LIST} class/rwlock_inline -c 4)
add_test(class/lifo:inline ${SHM_TEST_CMD_LIST} class/lifo_inline -c 4)
add_test(class/list:inline ${SHM_TEST_CMD_LIST} class/list_inline -c 4)
add_test(class/hash:inline ${SHM_TEST_CMD_LIST} class/hash_inline -\# 65536 -r 4 -n)
