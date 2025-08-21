#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <variant>
#include <vector>

#include "genetic.h"
#include "pthread.h"
#include "rand.h"

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

template <class T> struct cell_entry {
  float score;
  T *cell;
  bool stale;
};

template <class T> struct crossover_job {
  Array<cell_entry<T> *> &parents;
  Array<cell_entry<T> *> &children_out;
};

template <class T> struct fitness_job {
  cell_entry<T> *cell_entry;
};

template <class T> struct mutate_job {
  cell_entry<T> *cell_entry;
};

template <class T> struct work_queue {
  variant<crossover_job<T>, fitness_job<T>, mutate_job<T>> *jobs;
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

template <class T> work_queue<T> make_work_queue(int len) {
  return {.jobs = (variant<fitness_job<T>, crossover_job<T>> *)malloc(
              sizeof(variant<fitness_job<T>, crossover_job<T>>) * len),
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

template <class T> struct job_batch {
  Array<variant<crossover_job<T>, fitness_job<T>>> jobs;
  bool gen_complete;
};

template <class T>
optional<job_batch<T>> get_job_batch(work_queue<T> &queue, int batch_size,
                                     bool *stop_flag) {
  while (true) {
    for (int i = 0; i < NUM_QUEUE_RETRIES; i++) {
      if (queue.read_i < queue.write_i &&
          pthread_mutex_trylock(&queue.data_mutex)) {
        job_batch<T> res;
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

template <class T> struct worker_thread_args {
  Strategy<T> &strat;
  work_queue<T> &queue;
  bool *stop_flag;
};

template <class T> void *worker(void *args) {
  worker_thread_args<T> *work_args = (worker_thread_args<T> *)args;
  Strategy<T> &strat = work_args->strat;
  work_queue<T> &queue = work_args->queue;
  bool *stop_flag = work_args->stop_flag;

  auto job_dispatcher = overload{
      [strat](mutate_job<T> mj) {
        strat.mutate(*mj.cell_entry->cell);
        mj.cell_entry->stale = true;
      },
      [strat](fitness_job<T> fj) {
        fj.cell_entry->score = strat.fitness(*fj.cell_entry->cell);
        fj.cell_entry->stale = false;
      },
      [strat](crossover_job<T> cj) {
        Array<T *> parent_cells, child_cells;
        parent_cells = {(T **)malloc(sizeof(T *) * cj.parents.len),
                        cj.parents.len};
        child_cells = {(T **)malloc(sizeof(T *) * cj.children_out.len),
                       cj.children_out.len};
        for (int i = 0; i < cj.parents.len; i++) {
          parent_cells[i] = cj.parents[i].cell;
        }
        for (int i = 0; i < cj.children_out.len; i++) {
          child_cells[i] = cj.children_out[i].cell;
          cj.children_out[i].stale = true;
        }
        strat.crossover(parent_cells, child_cells);
      },
  };

  while (true) {
    auto batch = get_job_batch(queue, strat.batch_size, stop_flag);
    if (!batch || *stop_flag)
      return NULL;

    // Do the actual work
    for (int i = 0; i < batch->jobs.len; i++) {
      visit(job_dispatcher, batch->jobs[i]);
    }

    if (batch->gen_complete) {
      pthread_cond_signal(&queue.gen_complete_cond, &queue.gen_complete_mutex);
    }
  }
}

template <class T> Stats<T> run(Strategy<T> strat) {
  Stats<T> stats;

  // The work queue is what all the worker threads will checking
  // for jobs
  work_queue<T> queue = make_work_queue<T>(strat.num_cells);

  // The actual cells. Woo!
  T cells[strat.num_cells];

  // Using a vector so I can use the make_heap, push_heap, etc.
  vector<cell_entry<T>> cell_queue;
  for (int i = 0; i < strat.num_cells; i++) {
    cells[i] = strat.make_default_cell();
    cell_queue.push_back({0, &cells[i], true});
  }

  bool stop_flag = false;
  worker_thread_args<T> args = {
      .strat = strat, .queue = queue, .stop_flag = &stop_flag};

  // spawn worker threads
  pthread_t threads[strat.num_threads];
  for (int i = 0; i < strat.num_threads; i++) {
    pthread_create(&threads[i], NULL, worker<T>, (void *)args);
  }

  uint64_t rand_state = strat.rand_seed;

  for (int i = 0; i < strat.num_generations; i++) {
    // Mutate some random cells in the population
    for (int i = 0; i < cell_queue.size(); i++) {
      if (abs(norm_rand(rand_state)) < strat.mutation_chance) {
        queue.jobs[queue.write_i] = mutate_job<T>{&cell_queue[i]};
        queue.write_i++;
      }
    }
    pthread_cond_broadcast(&queue.jobs_available_cond);

    // Potential issue here where mutations aren't done computing and fitness
    // jobs begin. maybe need to gate this.

    // Generate fitness jobs
    for (int i = 0; i < cell_queue.size(); i++) {
      if (cell_queue[i].stale &&
          (strat.test_all || abs(norm_rand(rand_state)) < strat.test_chance)) {
        queue.jobs[queue.write_i] = fitness_job<T>{&cell_queue[i]};
        queue.write_i++;
      }
      pthread_cond_broadcast(&queue.jobs_available_cond);
    }
    queue.done_writing = true;

    // wait for fitness jobs to complete
    pthread_mutex_lock(&queue.gen_complete_mutex);

    // Before going to sleep, do a quick check to see if the fitness jobs are
    // already complete.
    pthread_mutex_lock(&queue.data_mutex);
    bool already_complete = queue.read_i != queue.write_i;
    pthread_mutex_unlock(&queue.data_mutex);
    if (already_complete) {
      pthread_mutex_unlock(&queue.gen_complete_mutex);
    } else {
      pthread_cond_wait(&queue.gen_complete_cond, &queue.gen_complete_mutex);
    }

    // Sort cells on performance
    std::sort(cell_queue.begin(), cell_queue.end(),
              [strat](cell_entry<T> a, cell_entry<T> b) {
                return strat.higher_fitness_is_better ? a > b : a < b;
              });

    printf("Top Score: %f\n", cell_queue[0].score);

    if (!strat.enable_crossover)
      continue;

    // generate crossover jobs
    // dear god. forgive me father
    queue.write_i = 0;
    queue.read_i = 0;
    int count = 0;
    int n_par = strat.crossover_parent_num;
    int n_child = strat.crossover_children_num;
    int child_i = cell_queue.size() - 1;
    int par_i = 0;
    while (child_i - par_i <= n_par + n_child) {
      Array<cell_entry<T> *> parents = {
          (cell_entry<T> **)malloc(sizeof(cell_entry<T> *) * n_par), n_par};
      Array<cell_entry<T> *> children = {
          (cell_entry<T> **)malloc(sizeof(cell_entry<T> *) * n_child), n_child};

      for (; par_i < par_i + n_par; par_i++) {
        parents[i] = cell_queue[par_i];
      }

      for (; child_i > child_i - n_child; child_i--) {
        children[i] = cell_queue[child_i];
      }

      queue.jobs[queue.write_i] = crossover_job<T>{parents, children};
      par_i += strat.crossover_parent_stride;
      child_i += strat.crossover_children_stride;
    }
  }

  // stop worker threads
  stop_flag = true;
  pthread_cond_broadcast(&queue.jobs_available_cond);
  for (int i = 0; i < strat.num_threads; i++) {
    pthread_join(threads[i], NULL);
  }
}

template <class T> T &Array<T>::operator[](int i) {
  return _data[i];
}

} // namespace genetic
