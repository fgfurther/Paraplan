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

        // ИЗМЕНЕНО: 4 пары в день вместо 2
        const int LESSONS_PER_DAY = Config::LESSONS_PER_GROUP_PER_DAY;

        for (int g = 0; g < Config::NUM_GROUPS; ++g) {
            for (int d = 0; d < Config::NUM_DAYS; ++d) {
                for (int i = 0; i < LESSONS_PER_DAY; ++i) {
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

        // === Жёсткие конфликты ===
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

        // === Окна ===
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

        // === Баланс нагрузки по группам ===
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

        // === Нехватка мест ===
        for (const auto& l : lessons) {
            if (Config::group_sizes[l.group] > l.room_capacity) {
                capacity_conflicts++;
            }
        }

        // === Неподходящий тип аудитории ===
        for (const auto& l : lessons) {
            if ((l.type == "Лекция" && l.room_type != "Лекционная") ||
                (l.type == "Практика" && l.room_type != "Компьютерная") ||
                (l.type == "Лабораторная" && l.room_type != "Лабораторная")) {
                type_conflicts++;
            }
        }

        // === Нагрузка преподавателей (отклонение от часов) === ПОЛНОСТЬЮ ПЕРЕПИСАНО
        std::vector<int> actual_load(Config::NUM_TEACHERS, 0);
        for (const auto& l : lessons) actual_load[l.teacher]++;

        static const std::vector<int> target_pairs = Config::get_target_pairs();
        for (int t = 0; t < Config::NUM_TEACHERS; ++t) {
            int diff = std::abs(actual_load[t] - target_pairs[t]);
            soft_teacher_load += diff;
        }

        // === Предпочтения по дням ===
        for (const auto& l : lessons) {
            const auto& pref = Config::teacher_preferred_days;
            if (pref.count(l.teacher)) {
                const auto& days = pref.at(l.teacher);
                if (std::find(days.begin(), days.end(), l.day) == days.end()) {
                    soft_teacher_pref++;
                }
            }
        }

        // === Нормировка ===
        double max_conflicts = lessons.size() * (lessons.size() - 1) / 2.0;
        double max_gaps = Config::NUM_GROUPS * Config::NUM_DAYS * (Config::SLOTS_PER_DAY - 1);
        double max_balance = Config::NUM_GROUPS * (std::pow(Config::LESSONS_PER_GROUP_PER_DAY * Config::NUM_DAYS - Config::LESSONS_PER_GROUP_PER_DAY, 2) +
            (Config::NUM_DAYS - 1) * std::pow(Config::LESSONS_PER_GROUP_PER_DAY, 2));
        double max_capacity_conflicts = lessons.size();
        double max_type_conflicts = lessons.size();
        double max_teacher_load = 0.0;
        for (int t = 0; t < Config::NUM_TEACHERS; ++t) {
            int diff = std::max(0, static_cast<int>(lessons.size()) - target_pairs[t]);
            max_teacher_load += diff;
        }
        double max_teacher_pref = lessons.size();

        hard_conflicts = max_conflicts > 0 ? hard_conflicts / max_conflicts : 0.0;
        soft_gaps = max_gaps > 0 ? soft_gaps / max_gaps : 0.0;
        soft_balance = max_balance > 0 ? soft_balance / max_balance : 0.0;
        capacity_conflicts = max_capacity_conflicts > 0 ? capacity_conflicts / max_capacity_conflicts : 0.0;
        type_conflicts = max_type_conflicts > 0 ? type_conflicts / max_type_conflicts : 0.0;
        soft_teacher_load = max_teacher_load > 0 ? soft_teacher_load / max_teacher_load : 0.0;
        soft_teacher_pref = max_teacher_pref > 0 ? soft_teacher_pref / max_teacher_pref : 0.0;

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

        // Текущая нагрузка
        std::vector<int> current_load(Config::NUM_TEACHERS, 0);
        for (const auto& l : lessons) current_load[l.teacher]++;

        static const std::vector<int> target_pairs = Config::get_target_pairs();

        for (auto& l : lessons) {
            if (prob(gen) < Config::MUTATION_RATE) {
                int old_teacher = l.teacher;

                // Умная мутация: 70% шанс выбрать недогруженного преподавателя
                if (prob(gen) < 0.7) {
                    std::vector<int> underloaded;
                    for (int t = 0; t < Config::NUM_TEACHERS; ++t) {
                        if (current_load[t] < target_pairs[t]) {
                            underloaded.push_back(t);
                        }
                    }
                    if (!underloaded.empty()) {
                        std::uniform_int_distribution<> dist(0, underloaded.size() - 1);
                        l.teacher = underloaded[dist(gen)];
                    }
                }

                // Обновляем предмет под нового преподавателя
                auto& pairs = Config::teacher_subject_pairs;
                std::vector<int> valid_subjects;
                for (const auto& p : pairs)
                    if (p.first == l.teacher) valid_subjects.push_back(p.second);
                if (!valid_subjects.empty()) {
                    std::uniform_int_distribution<> dist(0, valid_subjects.size() - 1);
                    l.subject = valid_subjects[dist(gen)];
                }

                // День с учётом предпочтений
                const auto& pref = Config::teacher_preferred_days;
                if (pref.count(l.teacher) && prob(gen) < 0.7) {
                    const auto& days = pref.at(l.teacher);
                    std::uniform_int_distribution<> d(0, days.size() - 1);
                    l.day = days[d(gen)];
                }
                else {
                    l.day = day_dist(gen);
                }

                l.slot = slot_dist(gen);
                int room = room_dist(gen);
                l.room = room;
                l.room_capacity = Config::room_capacities[room];
                l.room_type = Config::room_types[room];
            }
        }
    }
};