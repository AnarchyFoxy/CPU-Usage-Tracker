#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/sysctl.h>
#include <mach/mach.h>

#define MAX_CPUS 32
#define BUFFER_SIZE 256
#define LOG_FILE_PATH "cpu_usage.log"

// Struktura przechowująca dane o zużyciu procesora
typedef struct {
    char cpu_name[16];
    unsigned long long user;
    unsigned long long nice;
    unsigned long long system;
    unsigned long long idle;
    unsigned long long iowait;
    unsigned long long irq;
    unsigned long long softirq;
    unsigned long long steal;
    unsigned long long guest;
    unsigned long long guest_nice;
} CPUData;

// Struktura przechowująca dane o zużyciu procesora w %
typedef struct {
    char cpu_name[16];
    double usage;
} CPUUsage;

// Globalne zmienne
CPUData prev_cpu_data[MAX_CPUS];
CPUData curr_cpu_data[MAX_CPUS];
int num_cpus;
pthread_mutex_t data_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t data_ready = PTHREAD_COND_INITIALIZER;
bool program_running = true;
FILE* log_file = NULL;
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

// Funkcja odczytu danych o zużyciu procesora
void read_cpu_data() {
    mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;
    host_cpu_load_info_data_t load_info;
    kern_return_t status = host_statistics(mach_host_self(), HOST_CPU_LOAD_INFO, (host_info_t)&load_info, &count);

    if (status != KERN_SUCCESS) {
        perror("Błąd przy pobieraniu informacji o CPU");
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFER_SIZE];
    snprintf(buffer, BUFFER_SIZE, "cpu  %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
             load_info.cpu_ticks[CPU_STATE_USER],
             load_info.cpu_ticks[CPU_STATE_NICE],
             load_info.cpu_ticks[CPU_STATE_SYSTEM],
             load_info.cpu_ticks[CPU_STATE_IDLE],
             load_info.cpu_ticks[CPU_STATE_IO_WAIT],
             load_info.cpu_ticks[CPU_STATE_IRQ],
             load_info.cpu_ticks[CPU_STATE_SOFT_IRQ],
             load_info.cpu_ticks[CPU_STATE_IDLE],
             load_info.cpu_ticks[CPU_STATE_IRQ],
             load_info.cpu_ticks[CPU_STATE_NICE]);

    char* buffer_ptr = buffer;
    CPUData data;

    if (strncmp(buffer_ptr, "cpu", 3) == 0) {
        int cpu_id;
        sscanf(buffer_ptr, "%s %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
            data.cpu_name, &data.user, &data.nice, &data.system,
            &data.idle, &data.iowait, &data.irq, &data.softirq,
            &data.steal, &data.guest, &data.guest_nice);

        if (sscanf(data.cpu_name, "cpu%d", &cpu_id) == 1) {
            memcpy(&curr_cpu_data[cpu_id], &data, sizeof(CPUData));
        }
    }
}

// Funkcja obliczająca zużycie procesora w %
void calculate_cpu_usage(CPUUsage* cpu_usage) {
    for (int i = 0; i < num_cpus; i++) {
        unsigned long long prev_total = prev_cpu_data[i].user + prev_cpu_data[i].nice +
                                        prev_cpu_data[i].system + prev_cpu_data[i].idle +
                                        prev_cpu_data[i].iowait + prev_cpu_data[i].irq +
                                        prev_cpu_data[i].softirq + prev_cpu_data[i].steal;
        unsigned long long curr_total = curr_cpu_data[i].user + curr_cpu_data[i].nice +
                                        curr_cpu_data[i].system + curr_cpu_data[i].idle +
                                        curr_cpu_data[i].iowait + curr_cpu_data[i].irq +
                                        curr_cpu_data[i].softirq + curr_cpu_data[i].steal;
        unsigned long long total_diff = curr_total - prev_total;
        unsigned long long idle_diff = curr_cpu_data[i].idle - prev_cpu_data[i].idle;
        double usage = ((double)(total_diff - idle_diff) / total_diff) * 100.0;
        snprintf(cpu_usage[i].cpu_name, sizeof(cpu_usage[i].cpu_name), "CPU%d", i); // Użyj indeksu jako nazwy CPU
        cpu_usage[i].usage = usage;
    }
}

// Funkcja wątku Reader
void* reader_thread(void* arg) {
    while (program_running) {
        read_cpu_data();
        pthread_cond_signal(&data_ready);
        sleep(1);
    }
    return NULL;
}

// Funkcja wątku Analyzer
void* analyzer_thread(void* arg) {
    CPUUsage cpu_usage[MAX_CPUS];
    while (program_running) {
        pthread_mutex_lock(&data_mutex);
        pthread_cond_wait(&data_ready, &data_mutex);
        calculate_cpu_usage(cpu_usage);
        pthread_mutex_unlock(&data_mutex);
    }
    return NULL;
}

// Funkcja wątku Printer
void* printer_thread(void* arg) {
    CPUUsage cpu_usage[MAX_CPUS];
    while (program_running) {
        pthread_mutex_lock(&data_mutex);
        pthread_cond_wait(&data_ready, &data_mutex);
        calculate_cpu_usage(cpu_usage);

        // Wydrukuj zużycie procesora
        for (int i = 0; i < num_cpus; i++) {
            printf("%s: %.2lf%%\n", cpu_usage[i].cpu_name, cpu_usage[i].usage);
        }
        printf("\n");

        pthread_mutex_unlock(&data_mutex);
        sleep(1);
    }
    return NULL;
}

// Funkcja wątku Watchdog
void* watchdog_thread(void* arg) {
    while (program_running) {
        sleep(2);
        pthread_mutex_lock(&data_mutex);
        pthread_cond_signal(&data_ready);
        pthread_mutex_unlock(&data_mutex);
    }
    return NULL;
}

// Funkcja wątku Logger
void* logger_thread(void* arg) {
    while (program_running) {
        // Tutaj można zaimplementować zapisywanie logów do pliku w sposób zsynchronizowany
        if (log_file != NULL) {
            pthread_mutex_lock(&log_mutex);
            fprintf(log_file, "Log message\n");
            fflush(log_file);
            pthread_mutex_unlock(&log_mutex);
        }
        sleep(1);
    }
    return NULL;
}

// Obsługa sygnału SIGTERM
void sigterm_handler(int signum) {
    program_running = false;
    pthread_cond_signal(&data_ready);
}

int main() {
    // Zarejestruj obsługę sygnału SIGTERM
    signal(SIGTERM, sigterm_handler);

    // Otwórz plik logu
    log_file = fopen(LOG_FILE_PATH, "w");
    if (log_file == NULL) {
        perror("Nie można otworzyć pliku logu");
        exit(EXIT_FAILURE);
    }

    // Określ liczbę dostępnych rdzeni CPU
    size_t len = sizeof(num_cpus);
    sysctlbyname("hw.logicalcpu", &num_cpus, &len, NULL, 0);

    // Inicjalizacja wątków
    pthread_t reader, analyzer, printer, watchdog, logger;
    pthread_create(&reader, NULL, reader_thread, NULL);
    pthread_create(&analyzer, NULL, analyzer_thread, NULL);
    pthread_create(&printer, NULL, printer_thread, NULL);
    pthread_create(&watchdog, NULL, watchdog_thread, NULL);
    pthread_create(&logger, NULL, logger_thread, NULL);

    // Oczekuj na zakończenie wątków
    pthread_join(reader, NULL);
    pthread_join(analyzer, NULL);
    pthread_join(printer, NULL);
    pthread_join(watchdog, NULL);
    pthread_join(logger, NULL);

    // Zamknij plik logu
    if (log_file != NULL) {
        fclose(log_file);
    }

    // Zakończ program
    return 0;
}
