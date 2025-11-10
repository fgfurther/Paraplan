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
    const int LESSONS_PER_GROUP_PER_DAY = 4; // ИЗМЕНЕНО: было 2 → теперь 4 пары в день
    const int POPULATION_SIZE = 50;         
    const int MAX_GENERATIONS = 500;       
    const double MUTATION_RATE = 0.15;      

    // Весовые коэффициенты для фитнеса
    struct FitnessWeights {
        double hard_conflict = 0.7;
        double soft_gap = 0.02;
        double soft_balance = 0.02;
        double capacity_conflict = 0.02;
        double type_conflict = 0.02;
        double teacher_load = 0.2;      // Отклонение от целевой нагрузки в часах
        double teacher_pref = 0.02;
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

    // === ТРУДОВАЯ НАГРУЗКА В ЧАСАХ В НЕДЕЛЮ НА 3 ГРУППЫ === ДОБАВЛЕНО
    inline std::vector<int> teacher_semester_hours = { 9, 9, 12, 15, 15 };

    // Предпочтительные дни недели (0=Пн, 1=Вт, 2=Ср, 3=Чт, 4=Пт, 5=Сб)
    inline std::unordered_map<int, std::vector<int>> teacher_preferred_days = {
        {0, {0,1,2,3}}, {1, {0,1,2,3,4}}, {2, {1,2,3}}, {3, {0,1,2,3,4}}, {4, {0,1,2}}
    };

    // === Вспомогательная функция: целевое количество пар === ДОБАВЛЕНО
    inline std::vector<int> get_target_pairs() {
        std::vector<int> target;
        for (int h : teacher_semester_hours) {
            int pairs = static_cast<int>(std::round(h / 1.5));
            target.push_back(pairs);
        }
        return target;
    }

    // УДАЛЕНО: teacher_desired_load — больше не используется
}