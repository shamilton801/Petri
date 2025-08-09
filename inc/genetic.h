#include <vector>

namespace genetic {

template <class T> struct Strategy {
  // The recommended number of threads is <= number of cores on your pc.
  // Set this to -1 use the default value (number of cores - 1)
  int num_threads;     // Number of worker threads that will be evaluating cell fitness
  int num_cells;       // Size of the population pool
  int num_generations; // Number of times (epochs) to run the algorithm
  bool test_all;       // Sets whether or not every cell is tested every generation
  float test_chance;   // Chance to test any given cell's fitness. Relevant only if test_all is false.

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
