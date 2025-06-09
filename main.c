#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SIZE 9
#define POP_SIZE 100
#define MAX_GENERATIONS 1000
#define ELITISM 1

typedef enum {
    SELECTION_ROULETTE,
    SELECTION_TOURNAMENT,
    SELECTION_RANDOM
} SelectionType;

typedef enum {
    CROSSOVER_ROW,
    CROSSOVER_UNIFORM,
    CROSSOVER_COLUMN
} CrossoverType;

typedef enum {
    MUTATION_SWAP,
    MUTATION_REPAIR,
    MUTATION_RANDOM_REPLACE
} MutationType;

typedef struct {
    int grid[SIZE][SIZE];
    int fitness;
} Individual;

// Global fixed cells grid (0 if not fixed)
int fixed[SIZE][SIZE];

Individual best_overall;
int best_overall_fitness = 1000000;

// Settings - łatwo zmieniać:
SelectionType selection_type = SELECTION_TOURNAMENT;
CrossoverType crossover_type = CROSSOVER_UNIFORM;
MutationType mutation_type = MUTATION_RANDOM_REPLACE;
int min_mutation_rate = 1;
int max_mutation_rate = 20;

// --- Funkcje pomocnicze ---

int count_conflicts(int grid[SIZE][SIZE]) {
    int conflicts = 0, count[SIZE + 1];

    // Wiersze i kolumny
    for (int i = 0; i < SIZE; i++) {
        memset(count, 0, sizeof(count));
        for (int j = 0; j < SIZE; j++)
            if (grid[i][j]) count[grid[i][j]]++;
        for (int k = 1; k <= SIZE; k++)
            if (count[k] > 1) conflicts += count[k] - 1;

        memset(count, 0, sizeof(count));
        for (int j = 0; j < SIZE; j++)
            if (grid[j][i]) count[grid[j][i]]++;
        for (int k = 1; k <= SIZE; k++)
            if (count[k] > 1) conflicts += count[k] - 1;
    }

    // Bloki 3x3
    for (int r = 0; r < SIZE; r += 3) {
        for (int c = 0; c < SIZE; c += 3) {
            memset(count, 0, sizeof(count));
            for (int i = 0; i < 3; i++)
                for (int j = 0; j < 3; j++)
                    if (grid[r + i][c + j]) count[grid[r + i][c + j]]++;
            for (int k = 1; k <= SIZE; k++)
                if (count[k] > 1) conflicts += count[k] - 1;
        }
    }

    return conflicts;
}

void print_grid(int grid[SIZE][SIZE]) {
    for (int i = 0; i < SIZE; i++) {
        if (i % 3 == 0) printf("+-------+-------+-------+\n");
        for (int j = 0; j < SIZE; j++) {
            if (j % 3 == 0) printf("| ");
            printf("%d ", grid[i][j]);
        }
        printf("|\n");
    }
    printf("+-------+-------+-------+\n");
}

// Generuje osobnika zgodnie z ustalonymi komórkami fixed
void generate_individual(Individual* ind) {
    for (int i = 0; i < SIZE; i++) {
        int used[SIZE + 1] = { 0 };
        // Najpierw zablokowane komórki
        for (int j = 0; j < SIZE; j++) {
            if (fixed[i][j]) {
                ind->grid[i][j] = fixed[i][j];
                used[fixed[i][j]] = 1;
            }
        }
        // Uzupełniamy pozostałe unikalnymi liczbami w wierszu
        for (int j = 0; j < SIZE; j++) {
            if (!fixed[i][j]) {
                int val;
                do {
                    val = rand() % SIZE + 1;
                } while (used[val]);
                ind->grid[i][j] = val;
                used[val] = 1;
            }
        }
    }
    ind->fitness = count_conflicts(ind->grid);
}

// --- Seleksje ---

Individual* roulette_selection(Individual* population) {
    int total = 0;
    for (int i = 0; i < POP_SIZE; i++)
        total += (1000 - population[i].fitness);
    int pick = rand() % total;
    int cumulative = 0;
    for (int i = 0; i < POP_SIZE; i++) {
        cumulative += (1000 - population[i].fitness);
        if (cumulative >= pick) return &population[i];
    }
    return &population[POP_SIZE - 1];
}

Individual* tournament_selection(Individual* population) {
    Individual* best = &population[rand() % POP_SIZE];
    for (int i = 0; i < 2; i++) {
        Individual* challenger = &population[rand() % POP_SIZE];
        if (challenger->fitness < best->fitness)
            best = challenger;
    }
    return best;
}

Individual* random_selection(Individual* population) {
    return &population[rand() % POP_SIZE];
}

// --- Krzyżowania ---

void crossover_row(const Individual* p1, const Individual* p2, Individual* child) {
    int row = rand() % SIZE;
    for (int i = 0; i < SIZE; i++)
        for (int j = 0; j < SIZE; j++)
            child->grid[i][j] = (i < row) ? p1->grid[i][j] : p2->grid[i][j];
}

void crossover_uniform(const Individual* p1, const Individual* p2, Individual* child) {
    for (int i = 0; i < SIZE; i++)
        for (int j = 0; j < SIZE; j++)
            child->grid[i][j] = (rand() % 2) ? p1->grid[i][j] : p2->grid[i][j];
}

void crossover_column(const Individual* p1, const Individual* p2, Individual* child) {
    int col = rand() % SIZE;
    for (int i = 0; i < SIZE; i++)
        for (int j = 0; j < SIZE; j++)
            child->grid[i][j] = (j < col) ? p1->grid[i][j] : p2->grid[i][j];
}

void crossover(const Individual* p1, const Individual* p2, Individual* child) {
    switch (crossover_type) {
        case CROSSOVER_ROW: crossover_row(p1, p2, child); break;
        case CROSSOVER_UNIFORM: crossover_uniform(p1, p2, child); break;
        case CROSSOVER_COLUMN: crossover_column(p1, p2, child); break;
    }
    child->fitness = count_conflicts(child->grid);
}

// --- Mutacje ---

void mutate_swap(Individual* ind, int rate) {
    for (int i = 0; i < SIZE; i++) {
        if (rand() % 100 < rate) {
            int c1 = rand() % SIZE, c2 = rand() % SIZE;
            if (!fixed[i][c1] && !fixed[i][c2]) {
                int tmp = ind->grid[i][c1];
                ind->grid[i][c1] = ind->grid[i][c2];
                ind->grid[i][c2] = tmp;
            }
        }
    }
}

void mutate_repair(Individual* ind, int rate) {
    (void)rate; // Tu rate ignorujemy bo naprawa jest deterministyczna
    for (int i = 0; i < SIZE; i++) {
        int seen[SIZE + 1] = { 0 };
        for (int j = 0; j < SIZE; j++)
            if (fixed[i][j]) seen[ind->grid[i][j]] = 1;
        for (int j = 0; j < SIZE; j++) {
            if (!fixed[i][j] && seen[ind->grid[i][j]]) {
                for (int k = 1; k <= SIZE; k++) {
                    if (!seen[k]) {
                        ind->grid[i][j] = k;
                        seen[k] = 1;
                        break;
                    }
                }
            } else {
                seen[ind->grid[i][j]] = 1;
            }
        }
    }
}

void mutate_random_replace(Individual* ind, int rate) {
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            if (!fixed[i][j] && rand() % 100 < rate) {
                ind->grid[i][j] = rand() % SIZE + 1;
            }
        }
    }
}

void mutate(Individual* ind, int rate) {
    switch (mutation_type) {
        case MUTATION_SWAP: mutate_swap(ind, rate); break;
        case MUTATION_REPAIR: mutate_repair(ind, rate); break;
        case MUTATION_RANDOM_REPLACE: mutate_random_replace(ind, rate); break;
    }
    ind->fitness = count_conflicts(ind->grid);
}

// --- Algorytm genetyczny ---

int genetic_algorithm(int attempt) {
    Individual population[POP_SIZE];
    for (int i = 0; i < POP_SIZE; i++) generate_individual(&population[i]);

    for (int gen = 0; gen < MAX_GENERATIONS; gen++) {
        qsort(population, POP_SIZE, sizeof(Individual),
              (int (*)(const void*, const void*))strcmp); // błąd w oryginale, zmieniłem na poprawną funkcję

        qsort(population, POP_SIZE, sizeof(Individual), (int(*)(const void*,const void*)) [](const void* a, const void* b) -> int {
            const Individual* ia = (const Individual*)a;
            const Individual* ib = (const Individual*)b;
            return ia->fitness - ib->fitness;
        });

        if (population[0].fitness < best_overall_fitness) {
            best_overall = population[0];
            best_overall_fitness = population[0].fitness;
        }

        if (population[0].fitness == 0) {
            printf("\nZnaleziono rozwiazanie w probie #%d, generacja %d:\n", attempt, gen);
            print_grid(population[0].grid);
            return 1;
        }

        if (gen % 100 == 0)
            printf("Proba #%d, generacja %d - fitness: %d\n", attempt, gen, population[0].fitness);

        int total_fitness = 0;
        for (int i = 0; i < POP_SIZE; i++) total_fitness += (1000 - population[i].fitness);

        int mutation_rate = max_mutation_rate - ((max_mutation_rate - min_mutation_rate) * gen / MAX_GENERATIONS);

        Individual new_pop[POP_SIZE];
        if (ELITISM) new_pop[0] = population[0];

        for (int i = ELITISM; i < POP_SIZE; i++) {
            Individual* p1;
            Individual* p2;

            // selekcja
            switch (selection_type) {
                case SELECTION_ROULETTE:
                    p1 = roulette_selection(population);
                    p2 = roulette_selection(population);
                    break;
                case SELECTION_TOURNAMENT:
                    p1 = tournament_selection(population);
                    p2 = tournament_selection(population);
                    break;
                case SELECTION_RANDOM:
                default:
                    p1 = random_selection(population);
                    p2 = random_selection(population);
                    break;
            }

            crossover(p1, p2, &new_pop[i]);
            mutate(&new_pop[i], mutation_rate);
        }
        memcpy(population, new_pop, sizeof(population));
    }

    printf("\nProba #%d nie powiodla sie. Najlepszy wynik (fitness = %d):\n", attempt, best_overall_fitness);
    print_grid(best_overall.grid);
    return 0;
}

// --- Załaduj przykładowe sudoku ---

void load_example() {
    int ex[SIZE][SIZE] = {
        {5,3,0, 0,7,0, 0,0,0},
        {6,0,0, 1,9,5, 0,0,0},
        {0,9,8, 0,0,0, 0,6,0},
        {8,0,0, 0,6,0, 0,0,3},
        {4,0,0, 8,0,3, 0,0,1},
        {7,0,0, 0,2,0, 0,0,6},
        {0,6,0, 0,0,0, 2,8,0},
        {0,0,0, 4,1,9, 0,0,5},
        {0,0,0, 0,8,0, 0,7,9}
    };
    memcpy(fixed, ex, sizeof(fixed));
}

int main() {
    srand(time(NULL));
    load_example();

    printf("Algorytm genetyczny Sudoku\n");
    printf("Wybierz selekcje:\n 0 - Ruletka\n 1 - Turniej\n 2 - Losowa\n");
    scanf("%d", (int*)&selection_type);
    printf("Wybierz krzyżowanie:\n 0 - Po wierszach\n 1 - Uniform\n 2 - Po kolumnach\n");
    scanf("%d", (int*)&crossover_type);
    printf("Wybierz mutację:\n 0 - Zamiana (swap)\n 1 - Naprawa (repair)\n 2 - Losowa zmiana\n");
    scanf("%d", (int*)&mutation_type);

    int max_attempts = 100;
    for (int i = 1; i <= max_attempts; i++) {
        if (genetic_algorithm(i)) break;
    }

    printf("\nNajlepsze znalezione rozwiazanie (fitness = %d):\n", best_overall_fitness);
    print_grid(best_overall.grid);

    return 0;
}