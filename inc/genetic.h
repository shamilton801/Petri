#include <vector>

namespace genetic {

template <class T> struct Array;
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
  bool test_all; // Sets whether or not every cell's fitness is evaluated every
                 // generation
  float test_chance; // Chance to test any given cell's fitness. Relevant only
                     // if test_all is false.
  bool enable_crossover; // Cells that score well in the evaluation stage
                         // produce children that replace low-scoring cells
  bool enable_crossover_mutation;  // Mutations can occur after crossover
  float crossover_mutation_chance; // Chance to mutate a child cell
  int crossover_parent_num;        // Number of unique high-scoring parents in a
                                   // crossover call.
  int crossover_parent_stride; // Number of parents to skip over when moving to
                               // the next set of parents. A stride of 1 would
                               // produce maximum overlap because the set of
                               // parents would only change by one every
                               // crossover.
  int crossover_children_num;  // Number of children to expect the user to
                               // produce in the crossover function.
  bool enable_mutation;          // Cells may be mutated
                                 // before fitness evaluation
  float mutation_chance; // Chance for any given cell to be mutated cells during
                         // the mutation
  uint64_t rand_seed;
  bool higher_fitness_is_better; // Sets whether or not to consider higher
                                 // fitness values better or worse. Set this to
                                 // false if fitness is an error function.

  // User defined functions
  T (*make_default_cell)();
  void (*mutate)(T &cell_to_modify);
  void (*crossover)(const Array<T *> parents, const Array<T *> out_children);
  float (*fitness)(const T &cell);
};

template <class T> struct Stats {
  std::vector<T> best_cell;
  std::vector<float> best_cell_fitness;
};

template <class T> struct Array {
  T *_data;
  int len;

  T &operator[](int i);
};

} // namespace genetic
