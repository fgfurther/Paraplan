#pragma once
#include <string>
#include <vector>
#include <unordered_map>

namespace Config {
    // Основные параметры
    const int NUM_GROUPS = 3;
    const int NUM_TEACHERS = 5;
    const int NUM_ROOMS = 6;
    const int NUM_SUBJECTS = 5;
    const int SLOTS_PER_DAY = 6;
    const int NUM_DAYS = 6; // включая субботу
    const int POPULATION_SIZE = 10;
    const int MAX_GENERATIONS = 500;
    const double MUTATION_RATE = 0.1;

    // Весовые коэффициенты для фитнеса
    struct FitnessWeights {
        double hard_conflict = 0.7;         // За каждый конфликт преподавателя / комнаты / группы
        double soft_gap = 0.02;               // За окна в расписании
        double soft_balance = 0.02;           // За неравномерное распределение
        double capacity_conflict = 0.02;     // За нехватку мест
        double type_conflict = 0.02;         // За неподходящий класс аудитории
        double teacher_load = 0.2;           // За отклонение от желаемой нагрузки
        double teacher_pref = 0.02;           // За занятия в нежелательные дни
    };

    inline FitnessWeights weights;

    // Тестовые данные
    inline std::vector<std::string> days = { "Понедельник", "Вторник", "Среда", "Четверг", "Пятница", "Суббота" };
    inline std::vector<std::string> groups = { "КИ22-03Б", "КИ21-03Б", "КИ22-04Б" };
    inline std::vector<std::string> teachers = { "Иванов", "Смирнов", "Петров", "Соболев", "Едреев" };
    inline std::vector<std::string> subjects = { "Математика", "Физика", "Химия", "Биология", "История" };
    inline std::vector<std::string> rooms = { "101", "102", "103", "104", "105", "106" };

    // === Размеры групп ===
    inline std::vector<int> group_sizes = { 25, 30, 20 };

    // === Параметры аудиторий ===
    inline std::vector<int> room_capacities = { 30, 40, 20, 25, 50, 15 };
    inline std::vector<std::string> room_types = {
        "Лекционная", "Компьютерная", "Лабораторная",
        "Лекционная", "Компьютерная", "Лабораторная"
    };

    // Допустимые связи (преподаватель–предмет)
    inline std::vector<std::pair<int, int>> teacher_subject_pairs = {
        {0, 0}, {0, 1}, {1, 1}, {1, 2}, {2, 2}, {2, 3},
        {3, 3}, {3, 4}, {4, 0}, {4, 4}
    };

    // === Предпочтения преподавателей ===
    // Желаемая недельная нагрузка (в количестве пар)
    inline std::unordered_map<int, int> teacher_desired_load = {
        {0, 8}, {1, 10}, {2, 12}, {3, 8}, {4, 6}
    };

    // Предпочтительные дни недели (0=Пн, 1=Вт, 2=Ср, 3=Чт, 4=Пт, 5=Сб)
    inline std::unordered_map<int, std::vector<int>> teacher_preferred_days = {
        {0, {0, 1, 2, 3}},       // Иванов: Пн–Чт
        {1, {0, 1, 2, 3, 4}},    // Смирнов: Пн–Пт
        {2, {1, 2, 3}},          // Петров: Вт–Чт
        {3, {0, 1, 2, 3, 4}},    // Соболев: Пн–Пт
        {4, {0, 1, 2}}           // Едреев: Пн–Ср
    };
}
