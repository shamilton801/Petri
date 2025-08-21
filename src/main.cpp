#include <cassert>
#include <cstdint>
#include <cstdlib>
#include "genetic.h"
#include "rand.h"

using namespace genetic;

const int len = 10;
const float max_float = 9999.9f;
static uint64_t seed = 12;
static float num_mutate_chance = 0.5;
static int num_parents = 2;
static int num_children = 2;


static int target_sum = 200;
static int target_product = 300;

Array<float> make_new_arr() {
    Array<float> arr = { (float*)malloc(sizeof(float)*len), len };
    for (int i = 0; i < arr.len; i++) {
        arr[i] = norm_rand(seed) * max_float;
    }
    return arr;
}

void mutate(Array<float> &arr_to_mutate) {
    for (int i = 0; i < len; i++) {
        if (norm_rand(seed) < num_mutate_chance) {
            arr_to_mutate[i] = norm_rand(seed) * max_float;
        }
    }
}

void crossover(const Array<Array<float>*> parents, const Array<Array<float> *> out_children) {
    for (int i = 0; i < len; i++) {
        (*out_children._data[0])[i] = i < len/2 ? (*parents._data[0])[i] : (*parents._data[1])[i];
        (*out_children._data[1])[i] = i < len/2 ? (*parents._data[1])[i] : (*parents._data[0])[i];
    }
}

// norm_rand can go negative. fix in genetic.cpp
// child stride doesn't make sense. Should always skip over child num

float fitness(const Array<float> &cell) {
    float sum = 0;
    float product = 1;
    for (int i = 0; i < cell.len; i++) {
        sum += cell._data[i];
        product *= cell._data[i];
    }
    return abs(sum - target_sum) + abs(product - target_product);
}

int main(int argc, char **argv) {
    Strategy<Array<float>> strat {
        .num_threads = 1,
        .batch_size  = 1,
        .num_cells   = 10,
        .num_generations = 10,
        .test_all = true,
        .test_chance = 0.0, // doesn't matter
        .enable_crossover = true,
        .enable_crossover_mutation = true,
        .crossover_mutation_chance = 0.6f,
        .crossover_parent_num = 2,
        .crossover_parent_stride = 1,
        .crossover_children_num = 2,
        .enable_mutation = true,
        .mutation_chance = 0.8,
        .rand_seed = seed,
        .higher_fitness_is_better = false,
        .make_default_cell=make_new_arr,
        .mutate=mutate,
        .crossover=crossover,
        .fitness=fitness
    };

    auto res = run(strat);
}
