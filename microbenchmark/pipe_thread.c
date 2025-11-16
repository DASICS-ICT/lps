#define _GNU_SOURCE
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <stdint.h>

// 线程间通信结构
typedef struct {
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  int data;
  int ready;
} thread_comm_t;

// 全局通信结构
thread_comm_t parent_to_child;
thread_comm_t child_to_parent;
int iterations;

// 获取纳秒级时间戳
static inline uint64_t get_timestamp_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

// 发送数据到另一个线程
static void send_data(thread_comm_t *comm, int data) {
  pthread_mutex_lock(&comm->mutex);
  comm->data = data;
  comm->ready = 1;
  pthread_cond_signal(&comm->cond);
  pthread_mutex_unlock(&comm->mutex);
}

// 从另一个线程接收数据
static int receive_data(thread_comm_t *comm) {
  pthread_mutex_lock(&comm->mutex);
  while (!comm->ready) {
      pthread_cond_wait(&comm->cond, &comm->mutex);
  }
  int data = comm->data;
  comm->ready = 0;
  pthread_mutex_unlock(&comm->mutex);
  return data;
}

// 线程版本的pingpong函数
static void* child_thread_func(void* arg) {
  for (int i = 0; i < iterations; i++) {
      // 从父线程接收数据
      int data = receive_data(&parent_to_child);
      
      // 发送数据回父线程
      send_data(&child_to_parent, data);
  }
  return NULL;
}

// 初始化通信结构
static void init_comm(thread_comm_t *comm) {
  pthread_mutex_init(&comm->mutex, NULL);
  pthread_cond_init(&comm->cond, NULL);
  comm->data = 0;
  comm->ready = 0;
}

// 清理通信结构
static void cleanup_comm(thread_comm_t *comm) {
  pthread_mutex_destroy(&comm->mutex);
  pthread_cond_destroy(&comm->cond);
}

int main() {
  // 初始化通信结构
  init_comm(&parent_to_child);
  init_comm(&child_to_parent);
  
  iterations = 10000; // 增加迭代次数以获得更准确的测量
  
  pthread_t child_thread;
  
  // 创建子线程
  if (pthread_create(&child_thread, NULL, child_thread_func, NULL) != 0) {
      perror("pthread_create");
      return 1;
  }
  
  // 开始测量时间
  const long long unsigned t1 = get_timestamp_ns();
  
  // 父线程执行pingpong
  for (int i = 0; i < iterations; i++) {
      // 发送数据到子线程
      send_data(&parent_to_child, 'z');
      
      // 从子线程接收数据
      receive_data(&child_to_parent);
  }
  
  // 等待子线程结束
  pthread_join(child_thread, NULL);
  
  const long long unsigned elapsed = get_timestamp_ns() - t1;
  
  printf("%.2fμs/thread_ctxswitch\n", elapsed / (double) (iterations * 2.0 * 1000));
  
  // 清理资源
  cleanup_comm(&parent_to_child);
  cleanup_comm(&child_to_parent);
  
  return 0;
}