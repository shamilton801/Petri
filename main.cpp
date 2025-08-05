#include <algorithm>
#include <cassert>
#include <cfloat>
#include <cstdlib>
#include <iostream>
#include <vector>

#define MUTATION_CHANCE 1.0

float norm_rand() { return (float)rand() / RAND_MAX; }

enum class ConstraintType {
  PRODUCT = 0,
  SUM = 1,
  INDEX_EQ = 2,
};

struct Constraint {
  ConstraintType type;
  int optional_i;
  float value;
};
static std::vector<Constraint> constraints;

struct Cell {
  int n;
  float *params;
};

Cell make_cell(int num_params) {
  Cell res = {num_params, (float *)malloc(num_params * sizeof(float))};
  for (int i = 0; i < num_params; i++) {
    res.params[i] = norm_rand() * 100.0f;
  }
  return res;
}

float get_cell_err(const Cell &a) {
  float total_diff = 0;
  for (auto c : constraints) {
    switch (c.type) {
    case ConstraintType::SUM: {
      float sum = 0;
      for (int i = 0; i < a.n; i++) {
        sum += a.params[i];
      }
      total_diff += abs(c.value - sum);
      break;
    }
    case ConstraintType::PRODUCT: {
      float prod = 1;
      for (int i = 0; i < a.n; i++) {
        prod *= a.params[i];
      }
      total_diff += abs(c.value - prod);
      break;
    }
    case ConstraintType::INDEX_EQ: {
      assert(c.optional_i < a.n);
      total_diff += abs(c.value - a.params[c.optional_i]);
      break;
    }
    }
  }
  return total_diff;
}

bool operator<(const Cell &a, const Cell &b) {
  assert(a.n == b.n);
  return get_cell_err(a) < get_cell_err(b);
}

void combine_cells(const Cell &a, const Cell &b, Cell *child) {
  bool a_first = norm_rand() > 0.5f;
  for (int i = 0; i < a.n; i++) {
	float offset = norm_rand();
	float roll = norm_rand();
    if (a_first) {
      child->params[i] = (i < a.n / 2 ? a.params[i] : b.params[i]) + (roll > 0.5 ? offset : -offset);
    } else {
      child->params[i] = (i < a.n / 2 ? b.params[i] : a.params[i]) + (roll > 0.5 ? offset : -offset);
    }
  }
  if (norm_rand() < MUTATION_CHANCE) {
    float r = norm_rand();
    child->params[(int)r * (a.n-1)] = r * FLT_MAX;
  }
}

int main(int argc, char **argv) {
  int num_params, num_cells, num_generations, num_constraints = 0;
  std::cin >> num_params >> num_cells >> num_generations >> num_constraints;

  std::cout << num_params << " " << num_cells << " " << num_generations << " "
            << num_constraints << std::endl;

  for (int i = 0; i < num_constraints; i++) {
    int type_in, optional_i = 0;
    float value;
    std::cin >> type_in >> value;
    ConstraintType type = static_cast<ConstraintType>(type_in);
    if (type == ConstraintType::INDEX_EQ) {
      std::cin >> optional_i;
    }
    constraints.push_back({type, optional_i, value});
  }

  std::vector<Cell> cells;
  for (int i = 0; i < num_cells; i++) {
    cells.push_back(make_cell(num_params));
  }

  for (int i = 0; i < num_generations; i++) {
    std::sort(cells.begin(), cells.end());
    for (int j = 0; j < num_cells / 2; j++) {
      combine_cells(cells[j], cells[j + 1], &cells[num_cells / 2 + j]);
    }
    if (i % 1000 == 0) {
      std::cout << i << "\t" << get_cell_err(cells[0])+get_cell_err(cells[1])+get_cell_err(cells[2]) << std::endl;
    }
  }
  std::cout << "Final Answer: ";
  for (int i = 0; i < cells[0].n; i++) {
	std::cout << cells[0].params[i] << " ";
  }
  std::cout << std::endl;
}
