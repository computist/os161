#ifndef _ASST2TEST_H_
#define _ASST2TEST_H_


int test_minimal_acquire_release_acquire_counter(void);

int test_pid_upper_limit_counter(void);

int test_pid_release(void);

int test_pid_upper_limit_queue(void);

int test_minimal_acquire_release_acquire_queue(void);

void release_ids(int from, int to);

int test_pid_upper_limit(void);

int test_pid_in_use(void);

int test_fd_create_destroy(void);

int test_fd_table(void);


int test_file_ops(void);

int test_file_table_copy(void);

int test_synch_hashtable(void);

#endif
