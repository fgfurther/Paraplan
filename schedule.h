#pragma once
#include "lesson.h"
#include "config.h"
#include <vector>
#include <algorithm>
#include <cmath>
#include <iostream>

class Schedule {
public:
    std::vector<Lesson> lessons;

    void initialize(std::mt19937& gen) {
        lessons.clear();
        std::uniform_int_distribution<> room_dist(0, Config::NUM_ROOMS - 1);
        std::uniform_int_distribution<> slot_dist(0, Config::SLOTS_PER_DAY - 1);
        std::uniform_int_distribution<> day_dist(0, Config::NUM_DAYS - 1);
        std::uniform_int_distribution<> type_dist(0, 2);

        for (int g = 0; g < Config::NUM_GROUPS; ++g) {
            for (int d = 0; d < Config::NUM_DAYS; ++d) {
                for (int i = 0; i < 2; ++i) {
                    Lesson l;
                    l.group = g;
                    l.day = d;
                    l.slot = slot_dist(gen);
                    int room = room_dist(gen);
                    l.room = room;
                    l.room_capacity = Config::room_capacities[room];
                    l.room_type = Config::room_types[room];
                    l.type = (type_dist(gen) == 0 ? "Лекция" : (type_dist(gen) == 1 ? "Практика" : "Лабораторная"));

                    // Подбираем корректную пару преподаватель-предмет
                    auto& pairs = Config::teacher_subject_pairs;
                    std::uniform_int_distribution<> pair_dist(0, (int)pairs.size() - 1);
                    auto p = pairs[pair_dist(gen)];
                    l.teacher = p.first;
                    l.subject = p.second;

                    lessons.push_back(l);
                }
            }
        }
    }

    std::vector<double> calculateFitness(int generation = 0, int maxGenerations = 1) const {
        double hard_conflicts = 0.0;
        double soft_gaps = 0.0;
        double soft_balance = 0.0;
        double capacity_conflicts = 0.0;
        double type_conflicts = 0.0;
        double soft_teacher_load = 0.0;
        double soft_teacher_pref = 0.0;

        // === Проверка жёстких конфликтов (группы, преподаватели, аудитории) ===
        for (size_t i = 0; i < lessons.size(); ++i) {
            for (size_t j = i + 1; j < lessons.size(); ++j) {
                const Lesson& a = lessons[i];
                const Lesson& b = lessons[j];

                if (a.day == b.day && a.slot == b.slot) {
                    if (a.group == b.group) hard_conflicts++;
                    if (a.teacher == b.teacher) hard_conflicts++;
                    if (a.room == b.room) hard_conflicts++;
                }
            }
        }

        // === Проверка конфликтов по вместимости и типу аудитории ===
        for (const auto& l : lessons) {
            if (Config::group_sizes[l.group] > l.room_capacity)
                capacity_conflicts++;

            if ((l.type == "Лекция" && l.room_type != "Лекционная") ||
                (l.type == "Практика" && l.room_type != "Компьютерная") ||
                (l.type == "Лабораторная" && l.room_type != "Лабораторная"))
                type_conflicts++;
        }

        // === Мягкие: окна у групп ===
        for (int g = 0; g < Config::NUM_GROUPS; ++g) {
            for (int d = 0; d < Config::NUM_DAYS; ++d) {
                std::vector<int> slots;
                for (auto& l : lessons)
                    if (l.group == g && l.day == d)
                        slots.push_back(l.slot);

                if (slots.size() > 1) {
                    std::sort(slots.begin(), slots.end());
                    for (size_t i = 1; i < slots.size(); ++i)
                        soft_gaps += (slots[i] - slots[i - 1] - 1);
                }
            }
        }

        // Мягкие: баланс нагрузки (равномерность по дням)
        for (int g = 0; g < Config::NUM_GROUPS; ++g) {
            std::vector<int> per_day(Config::NUM_DAYS, 0);
            for (auto& l : lessons)
                if (l.group == g)
                    per_day[l.day]++;

            double mean = 0.0;
            for (auto v : per_day) mean += v;
            mean /= Config::NUM_DAYS;

            for (auto v : per_day)
                soft_balance += std::pow(v - mean, 2);
        }

        // Мягкие: загрузка преподавателей
        std::unordered_map<int, int> teacher_load;
        for (const auto& l : lessons)
            teacher_load[l.teacher]++;

        for (const auto& [teacher, load] : teacher_load) {
            if (Config::teacher_desired_load.count(teacher)) {
                double desired = Config::teacher_desired_load.at(teacher);
                soft_teacher_load += std::pow(load - desired, 2);
            }
        }

        // === Мягкие: предпочтения по дням ===
        for (const auto& l : lessons) {
            if (Config::teacher_preferred_days.count(l.teacher)) {
                const auto& prefs = Config::teacher_preferred_days.at(l.teacher);
                if (std::find(prefs.begin(), prefs.end(), l.day) == prefs.end()) {
                    soft_teacher_pref += 1.0; // штраф за каждый урок в нежелательный день
                }
            }
        }

        // Нормировка
        double max_conflicts = lessons.size() * (lessons.size() - 1) / 2.0;
        double max_gaps = Config::NUM_GROUPS * Config::NUM_DAYS * (Config::SLOTS_PER_DAY - 1);
        double max_balance = Config::NUM_GROUPS * std::pow(Config::NUM_DAYS, 2);
        double max_capacity_conflicts = lessons.size();
        double max_type_conflicts = lessons.size();
        double max_teacher_load = Config::NUM_TEACHERS * std::pow(Config::NUM_DAYS * Config::SLOTS_PER_DAY, 2);
        double max_teacher_pref = lessons.size();

        hard_conflicts = max_conflicts > 0 ? hard_conflicts / max_conflicts : 0.0;
        soft_gaps = max_gaps > 0 ? soft_gaps / max_gaps : 0.0;
        soft_balance = max_balance > 0 ? soft_balance / max_balance : 0.0;
        capacity_conflicts = max_capacity_conflicts > 0 ? capacity_conflicts / max_capacity_conflicts : 0.0;
        type_conflicts = max_type_conflicts > 0 ? type_conflicts / max_type_conflicts : 0.0;
        soft_teacher_load = max_teacher_load > 0 ? soft_teacher_load / max_teacher_load : 0.0;
        soft_teacher_pref = max_teacher_pref > 0 ? soft_teacher_pref / max_teacher_pref : 0.0;

        // === Возврат нормированных целей ===
        return {
            hard_conflicts,
            soft_gaps,
            soft_balance,
            capacity_conflicts,
            type_conflicts,
            soft_teacher_load,
            soft_teacher_pref
        };
    }

    void mutate(std::mt19937& gen) {
        std::uniform_real_distribution<> prob(0, 1);
        std::uniform_int_distribution<> room_dist(0, Config::NUM_ROOMS - 1);
        std::uniform_int_distribution<> slot_dist(0, Config::SLOTS_PER_DAY - 1);
        std::uniform_int_distribution<> day_dist(0, Config::NUM_DAYS - 1);

        for (auto& l : lessons) {
            if (prob(gen) < Config::MUTATION_RATE) {
                int teacher_id = l.teacher;

                // Получаем предпочтения преподавателя
                const auto& pref_days = Config::teacher_preferred_days[teacher_id];

                // --- День недели ---
                if (!pref_days.empty() && prob(gen) < 0.7) {
                    // 70% шанс, что выберем предпочитаемый день
                    std::uniform_int_distribution<> day_pref_dist(0, (int)pref_days.size() - 1);
                    l.day = pref_days[day_pref_dist(gen)];
                } else {
                    // 30% шанс выбрать случайный день
                    l.day = day_dist(gen);
                }

                // --- Аудитория ---
                int room = room_dist(gen);
                l.room = room;
                l.room_capacity = Config::room_capacities[room];
                l.room_type = Config::room_types[room];
            }
        }
    }
};
