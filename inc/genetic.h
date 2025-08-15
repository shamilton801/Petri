#include <vector>

namespace genetic {

template <class T> struct ReadonlySpan;
template <class T> struct Span;
template <class T> struct Stats;
template <class T> struct Strategy;

template <class T> Stats<T> run(Strategy<T>);

template <class T> struct Strategy {
  int num_threads; // Number of worker threads that will be evaluating cell
                   // fitness.
  int batch_size;  // Number of cells a worker thread tries to work on in a row
                   // before accessing/locking the work queue again.
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
  int crossover_children_num;      // Number of children to expect the user to
                                   // produce in the crossover function.
  bool enable_mutation;            // Cells may be mutated
                                   // before fitness evaluation
  float mutation_chance; // Chance to mutate cells before fitness evaluation

  // User defined functions
  T (*make_default_cell)();
  void (*mutate)(T &cell_to_modify);
  void (*crossover)(const ReadonlySpan<T> &parents,
                    const Span<T> &out_children);
  float (*fitness)(const T &cell);
};

template <class T> struct Stats {
  std::vector<T> best_cell;
  std::vector<float> average_fitness;
};

template <class T> struct ReadonlySpan {
  T *_data;
  int len;

  const T &operator[](int i);
};

template <class T> struct Span {
  T *_data;
  int len;

  T &operator[](int i);
};

} // namespace genetic
