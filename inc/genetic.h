#include <vector>

namespace genetic {

template <class T> struct Strategy {
  int num_threads; // Number of worker threads that will be evaluating cell
                   // fitness.
  int num_retries; // Number of times worker threads will try to grab work pool
                   // lock before sleeping
  int batch_size;  // Number of cells a worker thread tries to evaluate in a row
                   // before locking the pool again. 1 tends to be fine
  int num_cells;   // Size of the population pool
  int num_generations; // Number of times (epochs) to run the algorithm
  bool test_all; // Sets whether or not every cell is tested every generation
  float test_chance; // Chance to test any given cell's fitness. Relevant only
                     // if test_all is false.

  // User defined functions
  T (*make_default_cell)();
  float (*fitness)(const T &cell);
  void (*mutate)(const T &cell, T *out);
  void (*crossover)(const T &a, const T &b, T *out);

  float mutation_chance_per_gen;
};

template <class T> struct Stats {
  std::vector<T> best_cell;
  std::vector<float> average_fitness;
};

template <class T> Stats<T> run(Strategy<T>);

} // namespace genetic
