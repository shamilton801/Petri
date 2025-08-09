#include "genetic.h"
#include <queue>
#include <vector>
#include <pthread.h>

namespace genetic {

template <class T> struct CellEntry {
  float score;
  T cell;
  bool stale;
};

template <class T> struct WorkEntry {
  const std::vector<CellEntry<T>> &cur;
  std::vector<CellEntry<T>> &next;
  int cur_i;
};

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
