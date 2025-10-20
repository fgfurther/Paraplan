#pragma once
#include "schedule.h"
#include <random>
#include <iostream>
#include <algorithm>
#include <set>

class GeneticAlgorithm {
private:
    std::vector<Schedule> population;
    std::mt19937 gen;

    // Структура для хранения информации о ранге и crowding distance
    struct Individual {
        Schedule schedule;
        int rank;
        double crowding_distance;
        Individual(const Schedule& s) : schedule(s), rank(0), crowding_distance(0.0) {}
    };

    // Проверка доминирования
    bool dominates(const std::vector<double>& a, const std::vector<double>& b) const {
        bool at_least_one_better = false;
        for (size_t i = 0; i < a.size(); ++i) {
            if (a[i] > b[i]) return false; // a не доминирует b, если хуже хотя бы в одной цели
            if (a[i] < b[i]) at_least_one_better = true;
        }
        return at_least_one_better;
    }

    // Сортировка по не доминированию
    void nonDominatedSort(std::vector<Individual>& pop) {
        std::vector<std::set<int>> fronts;
        std::vector<int> domination_count(pop.size(), 0);
        std::vector<std::set<int>> dominated_by(pop.size());

        // Шаг 1: Определяем доминирование
        for (size_t i = 0; i < pop.size(); ++i) {
            auto fitness_i = pop[i].schedule.calculateFitness();
            for (size_t j = 0; j < pop.size(); ++j) {
                if (i == j) continue;
                auto fitness_j = pop[j].schedule.calculateFitness();
                if (dominates(fitness_i, fitness_j)) {
                    dominated_by[i].insert(j);
                }
                else if (dominates(fitness_j, fitness_i)) {
                    domination_count[i]++;
                }
            }
            if (domination_count[i] == 0) {
                pop[i].rank = 0;
                if (fronts.empty()) fronts.push_back({});
                fronts[0].insert(i);
            }
        }

        // Шаг 2: Формируем фронты
        size_t front_idx = 0;
        while (!fronts[front_idx].empty()) {
            std::set<int> next_front;
            for (int i : fronts[front_idx]) {
                for (int j : dominated_by[i]) {
                    domination_count[j]--;
                    if (domination_count[j] == 0) {
                        pop[j].rank = front_idx + 1;
                        next_front.insert(j);
                    }
                }
            }
            fronts.push_back(next_front);
            front_idx++;
        }

        // Шаг 3: Вычисление crowding distance
        for (size_t f = 0; f < fronts.size() - 1; ++f) { // Последний фронт может быть пустым
            std::vector<int> front(fronts[f].begin(), fronts[f].end());
            if (front.size() <= 1) continue;

            for (size_t obj = 0; obj < 3; ++obj) { // Для каждой цели
                std::sort(front.begin(), front.end(), [&](int i, int j) {
                    return pop[i].schedule.calculateFitness()[obj] < pop[j].schedule.calculateFitness()[obj];
                    });

                pop[front.front()].crowding_distance = pop[front.back()].crowding_distance = std::numeric_limits<double>::infinity();
                double max_val = pop[front.back()].schedule.calculateFitness()[obj];
                double min_val = pop[front.front()].schedule.calculateFitness()[obj];
                double range = max_val - min_val;
                if (range == 0) continue;

                for (size_t i = 1; i < front.size() - 1; ++i) {
                    pop[front[i]].crowding_distance += (pop[front[i + 1]].schedule.calculateFitness()[obj] -
                        pop[front[i - 1]].schedule.calculateFitness()[obj]) / range;
                }
            }
        }
    }

    // Турнирный отбор с учётом ранга и crowding distance
    Individual tournamentSelection(const std::vector<Individual>& pop) {
        std::uniform_int_distribution<> dist(0, pop.size() - 1);
        int idx1 = dist(gen), idx2 = dist(gen);
        if (pop[idx1].rank < pop[idx2].rank) return pop[idx1];
        if (pop[idx2].rank < pop[idx1].rank) return pop[idx2];
        return pop[idx1].crowding_distance > pop[idx2].crowding_distance ? pop[idx1] : pop[idx2];
    }

public:
    GeneticAlgorithm() {
        std::random_device rd;
        gen.seed(rd());
        for (int i = 0; i < Config::POPULATION_SIZE; ++i) {
            Schedule s;
            s.initialize(gen);
            population.push_back(s);
        }
    }

    Schedule crossover(const Schedule& p1, const Schedule& p2) {
        Schedule child;
        if (p1.lessons.empty() || p2.lessons.empty()) return child;

        std::uniform_int_distribution<> dist(0, (int)p1.lessons.size() - 1);
        int point = dist(gen);
        child.lessons.reserve(p1.lessons.size());
        for (size_t i = 0; i < p1.lessons.size(); ++i)
            child.lessons.push_back(i < point ? p1.lessons[i] : p2.lessons[i]);
        return child;
    }

    std::vector<Schedule> run() {
        std::vector<Individual> pop;
        for (auto& s : population) pop.emplace_back(s);

        for (int gen_num = 0; gen_num < Config::MAX_GENERATIONS; ++gen_num) {
            // Шаг 1: Сортировка по не доминированию
            nonDominatedSort(pop);

            // Шаг 2: Создание новой популяции
            std::vector<Individual> new_pop;
            auto best_it = std::min_element(pop.begin(), pop.end(),
                [](const Individual& a, const Individual& b) { return a.rank < b.rank; });
            new_pop.push_back(*best_it); // Элитизм

            while (new_pop.size() < Config::POPULATION_SIZE) {
                Individual parent1 = tournamentSelection(pop);
                Individual parent2 = tournamentSelection(pop);
                Schedule child = crossover(parent1.schedule, parent2.schedule);
                if (child.lessons.empty()) {
                    child.initialize(gen);
                }
                child.mutate(gen);
                new_pop.emplace_back(child);
            }

            pop = std::move(new_pop);

            // Печать лучшего фронта
            std::cout << "Generation " << gen_num << ": Pareto front size = ";
            nonDominatedSort(pop);
            int front0_size = 0;
            for (auto& ind : pop) if (ind.rank == 0) front0_size++;
            std::cout << front0_size << std::endl;

            // Проверка на идеальное расписание (все цели = 0)
            bool ideal_schedule = false;
            for (auto& ind : pop) {
                auto fitness = ind.schedule.calculateFitness();
                if (ind.rank == 0 && fitness[0] == 0 && fitness[1] == 0 && fitness[2] == 0) {
                    ideal_schedule = true;
                    break;
                }
            }
            if (ideal_schedule) break;
        }

        // Возвращаем Парето-фронт
        std::vector<Schedule> pareto_front;
        for (auto& ind : pop) {
            if (ind.rank == 0) pareto_front.push_back(ind.schedule);
        }
        return pareto_front;
    }
};