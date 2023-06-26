#include "rwlock.h"

void rwlock_ctor(RwLock *rwlock) {
  pthread_mutex_init(&rwlock->mutex, NULL);
  pthread_cond_init(&rwlock->read_cond, NULL);
  pthread_cond_init(&rwlock->write_cond, NULL);
  rwlock->reader_count = 0;
  rwlock->writer_count = 0;
}

void rwlock_dtor(RwLock *rwlock) {
  pthread_mutex_destroy(&rwlock->mutex);
  pthread_cond_destroy(&rwlock->read_cond);
  pthread_cond_destroy(&rwlock->write_cond);
}

void rwlock_read_lock(RwLock *rwlock) {
  pthread_mutex_lock(&rwlock->mutex);

  while (rwlock->writer_count > 0)
    pthread_cond_wait(&rwlock->read_cond, &rwlock->mutex);
  ++rwlock->reader_count;

  pthread_mutex_unlock(&rwlock->mutex);
}

void rwlock_read_unlock(RwLock *rwlock) {
  pthread_mutex_lock(&rwlock->mutex);

  --rwlock->reader_count;
  if (rwlock->reader_count == 0)
    pthread_cond_signal(&rwlock->write_cond);

  pthread_mutex_unlock(&rwlock->mutex);
}

void rwlock_write_lock(RwLock *rwlock) {
  pthread_mutex_lock(&rwlock->mutex);

  while (rwlock->writer_count > 0 || rwlock->reader_count > 0)
    pthread_cond_wait(&rwlock->write_cond, &rwlock->mutex);
  ++rwlock->writer_count;

  pthread_mutex_unlock(&rwlock->mutex);
}

void rwlock_write_unlock(RwLock *rwlock) {
  pthread_mutex_lock(&rwlock->mutex);

  --rwlock->writer_count;
  if (rwlock->writer_count == 0) {
    pthread_cond_signal(&rwlock->write_cond);
    pthread_cond_broadcast(&rwlock->read_cond);
  }

  pthread_mutex_unlock(&rwlock->mutex);
}
