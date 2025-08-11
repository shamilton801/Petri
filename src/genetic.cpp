#include "genetic.h"
#include "pthread.h"
#include <queue>
#include <vector>

namespace genetic {

template <class T> struct CellEntry {
  float score;
  T cell;
  bool stale;
};

template <class T> struct WorkEntry {
  const CellEntry<T> &cur;
  float &score;
};

template <class T> struct WorkQueue {
  std::vector<WorkEntry<T>> jobs;
  int i;
};

static pthread_mutex_t data_mutex = PTHREAD_MUTEX_INITIALIZER;

static pthread_mutex_t ready_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t ready_cond = PTHREAD_COND_INITIALIZER;

static pthread_mutex_t gen_complete_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t gen_complete_cond = PTHREAD_COND_INITIALIZER;

static pthread_mutex_t run_complete_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t run_complete_cond = PTHREAD_COND_INITIALIZER;

/* Thoughts on this approach
 * The ideal implementation of a worker thread has them operating at maximum
 * load with as little synchronization overhead as possible. i.e. The ideal
 * worker thread
 *     1. Never waits for new work
 *     2. Never spends time synchronizing with other worker threads
 *
 * Never is impossible, but we want to get as close as we can.
 *
 * There are two extreme situations to consider
 *     1. Fitness functions with highly variable computation times
 *     2. Fitness functions with identical computation times.
 *
 * Most applications that use this library will fall into the second
 * category.
 *
 * In the highly-variable computation time case, it's useful for worker threads
 * to operate on 1 work entry at a time. Imagine a scenario with 2 threads, each
 * of which claims half the work to do. If thread A completes all of its work
 * quickly, it goes to sleep while thread B slogs away on its harder-to-compute
 * fitness jobs. However, if both threads only claim 1 work entry at a time,
 * thread A can immediately claim new jobs after it completes its current one.
 * Thread B can toil away, but little time is lost since thread A remains
 * productive.
 *
 * In the highly consistent computation time case, it's ideal for each
 * thread to claim an equal share of the jobs (as this minimizes time spent
 * synchronizing access to the job pool). Give each thread its set of work once
 * and let them have at it instead of each thread constantly locking/waiting
 * on the job queue.
 *
 * I take a hybrid approach. Users can specify a "batch size". Worker threads
 * will bite off jobs in chunks and complete them before locking
 * the job pool to grab another chunk. The user should choose a batch size close
 * to 1 if their fitness function compute time is highly variable and closer to
 * num_cells / num_threads if computation time is consistent. Users should
 * experiment with a batch size that works well for their problem.
 *
 * Worth mentioning this avoiding synchronization is irrelevant once computation
 * time >>> synchronization time.
 *
 * There might be room for dynamic batch size modification, but I don't expect
 * to pursue this feature until the library is more mature (and I've run out of
 * cooler things to do).
 *
 */
template <class T>
void worker(std::queue<WorkEntry<T>> &fitness_queue, int batch_size,
            int num_retries) {
  int retries = 0;
  std::vector<WorkEntry<T>> batch;
  bool gen_is_finished;
  while (true) {
    gen_is_finished = false;
    if (pthread_mutex_trylock(&data_mutex)) {
      retries = 0;
      for (int i = 0; i < batch_size; i++) {
        if (fitness_queue.empty()) {
          gen_is_finished = true;
          break;
        }
        batch.push_back(fitness_queue.front());
        fitness_queue.pop();
      }
      pthread_mutex_unlock(&data_mutex);
    } else {
      retries++;
    }

    if (gen_is_finished) {
      pthread_cond_signal(&gen_complete_cond, &gen_complete_mutex);
    }

    if (retries > num_retries) {
      pthread_mutex_lock(&ready_mutex);
      pthread_cond_wait(&ready_cond, &ready_mutex);
      retries = 0;
    }
  }
  pthread_mutex_lock(&data_mutex);
}

// Definitions
template <class T> Stats<T> run(Strategy<T> strat) {
  Stats<T> stats;

  std::queue<WorkEntry<T>> fitness_queue;
  std::vector<CellEntry<T>> cells_a, cells_b;
  for (int i = 0; i < strat.num_cells; i++) {
    T cell = strat.make_default_cell();
    cells_a.push_back({0, cell, true});
    cells_b.push_back({0, cell, true});
  }

  std::vector<CellEntry<T>> &cur_cells = cells_a;
  std::vector<CellEntry<T>> &next_cells = cells_b;

  for (int i = 0; i < strat.num_generations; i++) {

    cur_cells = cur_cells == cells_a ? cells_b : cells_a;
    next_cells = cur_cells == cells_a ? cells_b : cells_a;
  }
}

} // namespace genetic
