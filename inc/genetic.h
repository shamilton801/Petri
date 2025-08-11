#include <vector>
#include <span>

namespace genetic {

template <class T> struct Strategy {
  int num_threads; // Number of worker threads that will be evaluating cell
                   // fitness.
  int num_retries; // Number of times worker threads will try to grab work pool
                   // lock before sleeping
  int batch_size;  // Number of cells a worker thread tries to evaluate in a row
                   // before locking the pool again.
  int num_cells;   // Size of the population pool
  int num_generations; // Number of times (epochs) to run the algorithm
  bool test_all; // Sets whether or not every cell is tested every generation
  float test_chance; // Chance to test any given cell's fitness. Relevant only
                     // if test_all is false.
  bool enable_crossover; // Cells that score well in the evaluation stage
                         // produce children that replace low-scoring cells
  bool enable_crossover_mutation;  // Mutations can occur after crossover
  float crossover_mutation_chance; // Chance to mutate a child cell
  int crossover_parent_num;        // Number of unique high-scoring parents in a
                                   // crossover call.
  int crossover_children_num;      // Number of children produced in a crossover
  bool enable_mutation;  // Cells may be mutated before fitness evaluation
  float mutation_chance; // Chance to mutate cells before fitness evaluation

  // User defined functions
  T (*make_default_cell)();
  void (*mutate)(T &cell);
  void (*crossover)(const std::span<T> &parents, std::span<T> &out_children);
  float (*fitness)(const T &cell);
};

template <class T> struct Stats {
  std::vector<T> best_cell;
  std::vector<float> average_fitness;
};

template <class T> Stats<T> run(Strategy<T>);

} // namespace genetic
