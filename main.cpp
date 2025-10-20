#include "genetic_algorithm.h"
#include <iostream>
#include <windows.h> 

void printSchedule(const Schedule& s, int index) {
    auto fitness = s.calculateFitness();
    std::cout << "\n=== Расписание " << index + 1 << " (Конфликты = " << fitness[0]
        << ", Окна = " << fitness[1] << ", Баланс = " << fitness[2] << ") ===\n";
    for (int g = 0; g < Config::NUM_GROUPS; ++g) {
        std::cout << "\nГруппа: " << Config::groups[g] << "\n";
        for (int d = 0; d < Config::NUM_DAYS; ++d) {
            std::cout << "  " << Config::days[d] << ":\n";
            std::vector<Lesson> day_lessons;
            for (auto& l : s.lessons)
                if (l.group == g && l.day == d)
                    day_lessons.push_back(l);
            std::sort(day_lessons.begin(), day_lessons.end(),
                [](const Lesson& a, const Lesson& b) { return a.slot < b.slot; });

            for (auto& l : day_lessons) {
                std::cout << "    Лента " << l.slot + 1 << ": "
                    << Config::subjects[l.subject] << " ведет "
                    << Config::teachers[l.teacher] << " в аудитории "
                    << Config::rooms[l.room] << " (" << l.type << ")\n";
            }
        }
    }
}

int main() {
    SetConsoleOutputCP(CP_UTF8);
    setlocale(LC_ALL, "ru_RU.UTF-8");
    GeneticAlgorithm ga;
    std::vector<Schedule> pareto_front = ga.run();
    std::cout << "\n=== Найдено " << pareto_front.size() << " Парето-оптимальных расписаний ===\n";
    for (size_t i = 0; i < pareto_front.size(); ++i) {
        printSchedule(pareto_front[i], i);
    }
    return 0;
}
