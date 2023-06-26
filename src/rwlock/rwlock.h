#pragma once

#ifndef ADRIFT_RWLOCK_RWLOCK_H
#define ADRIFT_RWLOCK_RWLOCK_H

#include <pthread.h>

typedef struct RwLock {
  pthread_mutex_t mutex;
  pthread_cond_t read_cond, write_cond;
  _Atomic int reader_count, writer_count;
} RwLock;

void rwlock_ctor(RwLock *rwlock);

void rwlock_dtor(RwLock *rwlock);

void rwlock_read_lock(RwLock *rwlock);

void rwlock_read_unlock(RwLock *rwlock);

void rwlock_write_lock(RwLock *rwlock);

void rwlock_write_unlock(RwLock *rwlock);

#endif // ADRIFT_RWLOCK_RWLOCK_H
