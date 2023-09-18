#include "cpu_monitor.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// Funkcja pomocnicza do porównywania dwóch struktur CPUData
bool compare_cpu_data(const CPUData* data1, const CPUData* data2) {
    return strcmp(data1->cpu_name, data2->cpu_name) == 0 &&
           data1->user == data2->user &&
           data1->nice == data2->nice &&
           data1->system == data2->system &&
           data1->idle == data2->idle &&
           data1->iowait == data2->iowait &&
           data1->irq == data2->irq &&
           data1->softirq == data2->softirq &&
           data1->steal == data2->steal &&
           data1->guest == data2->guest &&
           data1->guest_nice == data2->guest_nice;
}

// Test czytania danych z pliku /proc/stat
void test_read_cpu_data() {
    // Tworzenie przykładowego pliku /proc/stat
    FILE* stat_file = fopen("test_stat_file", "w");
    if (stat_file == NULL) {
        perror("Nie można otworzyć pliku test_stat_file");
        return;
    }

    // Wpisanie danych do pliku
    fprintf(stat_file, "cpu 100 200 300 400 500 600 700 800 900 1000\n");
    fclose(stat_file);

    // Wywołanie funkcji do odczytu danych
    read_cpu_data("test_stat_file");

    // Porównanie odczytanych danych
    CPUData expected_data = {
        .cpu_name = "cpu",
        .user = 100,
        .nice = 200,
        .system = 300,
        .idle = 400,
        .iowait = 500,
        .irq = 600,
        .softirq = 700,
        .steal = 800,
        .guest = 900,
        .guest_nice = 1000
    };

    assert(compare_cpu_data(&expected_data, &curr_cpu_data[0]));

    // Czyszczenie po teście
    remove("test_stat_file");
}

// Test obliczania zużycia procesora
void test_calculate_cpu_usage() {
    // Ustawienie danych początkowych
    prev_cpu_data[0].user = 100;
    prev_cpu_data[0].nice = 200;
    prev_cpu_data[0].system = 300;
    prev_cpu_data[0].idle = 400;
    prev_cpu_data[0].iowait = 500;
    prev_cpu_data[0].irq = 600;
    prev_cpu_data[0].softirq = 700;
    prev_cpu_data[0].steal = 800;
    prev_cpu_data[0].guest = 900;
    prev_cpu_data[0].guest_nice = 1000;

    curr_cpu_data[0].user = 200;
    curr_cpu_data[0].nice = 300;
    curr_cpu_data[0].system = 400;
    curr_cpu_data[0].idle = 500;
    curr_cpu_data[0].iowait = 600;
    curr_cpu_data[0].irq = 700;
    curr_cpu_data[0].softirq = 800;
    curr_cpu_data[0].steal = 900;
    curr_cpu_data[0].guest = 1000;
    curr_cpu_data[0].guest_nice = 1100;

    // Wywołanie funkcji do obliczania zużycia
    CPUUsage cpu_usage[MAX_CPUS];
    calculate_cpu_usage(cpu_usage, 1);

    // Porównanie obliczonego zużycia
    assert(strcmp(cpu_usage[0].cpu_name, "cpu") == 0);
    assert(fabs(cpu_usage[0].usage - 37.50) < 0.01); // (1000 - 400) / 1000 * 100
}

int main() {
    test_read_cpu_data();
    test_calculate_cpu_usage();

    printf("Wszystkie testy zakończone.\n");

    return 0;
}
