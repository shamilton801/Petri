#include "genetic.h"
#include "pthread.h"
#include <optional>
#include <variant>
#include <vector>

#define NUM_QUEUE_RETRIES 10

using namespace std;

// std::visit/std::variant overload pattern
// See:
// https://www.modernescpp.com/index.php/visiting-a-std-variant-with-the-overload-pattern/
// You don't have to understand this, just use it :)
template <typename... Ts> struct overload : Ts... {
  using Ts::operator()...;
};
template <class... Ts> overload(Ts...) -> overload<Ts...>;

namespace genetic {

template <class T> struct CellEntry {
  float score;
  T cell;
};

template <class T> struct CrossoverJob {
  const ReadonlySpan<T> &parents;
  const Span<T> &children_out;
};

template <class T> struct FitnessJob {
  const T &cell;
  float &result_out;
};

template <class T> struct WorkQueue {
  variant<CrossoverJob<T>, FitnessJob<T>> *jobs;
  int len;
  int read_i;
  int write_i;
  bool done_writing;

  pthread_mutex_t data_mutex;
  pthread_mutex_t gen_complete_mutex;
  pthread_mutex_t jobs_available_mutex;

  pthread_cond_t gen_complete_cond;
  pthread_cond_t jobs_available_cond;
};

template <class T> WorkQueue<T> make_work_queue(int len) {
  return {.jobs = (variant<FitnessJob<T>, CrossoverJob<T>> *)malloc(
              sizeof(variant<FitnessJob<T>, CrossoverJob<T>>) * len),
          .len = len,
          .read_i = 0,
          .write_i = 0,
          .done_writing = false,
          .data_mutex = PTHREAD_MUTEX_INITIALIZER,
          .gen_complete_mutex = PTHREAD_MUTEX_INITIALIZER,
          .jobs_available_mutex = PTHREAD_MUTEX_INITIALIZER,
          .gen_complete_cond = PTHREAD_COND_INITIALIZER,
          .jobs_available_cond = PTHREAD_COND_INITIALIZER};
}

template <class T> struct JobBatch {
  ReadonlySpan<variant<CrossoverJob<T>, FitnessJob<T>>> jobs;
  bool gen_complete;
};

template <class T>
optional<JobBatch<T>> get_job_batch(WorkQueue<T> &queue, int batch_size,
                                    bool *stop_flag) {
  while (true) {
    for (int i = 0; i < NUM_QUEUE_RETRIES; i++) {
      if (queue.read_i < queue.write_i &&
          pthread_mutex_trylock(&queue.data_mutex)) {
        JobBatch<T> res;
        res.jobs._data = &queue._jobs[queue.read_i];
        int span_size = min(batch_size, queue.write_i - queue.read_i);
        res.jobs.len = span_size;

        queue.read_i += span_size;
        res.gen_complete = queue.done_writing && queue.read_i == queue.write_i;

        pthread_mutex_unlock(&queue.data_mutex);
        return res;
      }
    }
    pthread_mutex_lock(&queue.jobs_available_mutex);
    pthread_cond_wait(queue.jobs_available_cond, &queue.jobs_available_mutex);
    if (stop_flag)
      return {};
  }
}

template <class T> struct WorkerThreadArgs {
  Strategy<T> &strat;
  WorkQueue<T> &queue;
  bool *stop_flag;
};

template <class T> void *worker(void *args) {
  WorkerThreadArgs<T> *work_args = (WorkerThreadArgs<T> *)args;
  Strategy<T> &strat = work_args->strat;
  WorkQueue<T> &queue = work_args->queue;
  bool *stop_flag = work_args->stop_flag;

  auto JobDispatcher = overload{
      [strat](FitnessJob<T> fj) { fj.result_out = strat.fitness(fj.cell); },
      [strat](CrossoverJob<T> cj) {
        strat.crossover(cj.parents, cj.children_out);
      },
  };

  while (true) {
    auto batch = get_job_batch(queue, strat.batch_size, stop_flag);
    if (!batch || *stop_flag)
      return NULL;

    // Do the actual work
    for (int i = 0; i < batch->jobs.len; i++) {
      visit(JobDispatcher, batch->jobs[i]);
    }

    if (batch->gen_complete) {
      pthread_cond_signal(&queue.gen_complete_cond, &queue.gen_complete_mutex);
    }
  }
}

template <class T> Stats<T> run(Strategy<T> strat) {
  Stats<T> stats;
  WorkQueue<T> queue = make_work_queue<T>(strat.num_cells);

  vector<CellEntry<T>> cells_a, cells_b;
  for (int i = 0; i < strat.num_cells; i++) {
    T cell = strat.make_default_cell();
    cells_a.push_back({0, cell, true});
    cells_b.push_back({0, cell, true});
  }

  std::vector<CellEntry<T>> &cur_cells = cells_a;
  std::vector<CellEntry<T>> &next_cells = cells_b;

  bool stop_flag = false;
  WorkerThreadArgs<T> args = {
      .strat = strat, .queue = queue, .stop_flag = &stop_flag};

  // spawn worker threads
  pthread_t threads[strat.num_threads];
  for (int i = 0; i < strat.num_threads; i++) {
    pthread_create(&threads[i], NULL, worker<T>, (void *)args);
  }

  for (int i = 0; i < strat.num_generations; i++) {
    // generate fitness jobs
    // wait for fitness jobs to complete
    // sort cells on performance
    // generate crossover jobs
    cur_cells = cur_cells == cells_a ? cells_b : cells_a;
    next_cells = cur_cells == cells_a ? cells_b : cells_a;
  }

  // stop worker threads
  stop_flag = true;
  pthread_cond_broadcast(queue.jobs_available_cond);
  for (int i = 0; i < strat.num_threads; i++) {
    pthread_join(threads[i], NULL);
  }

}

} // namespace genetic
