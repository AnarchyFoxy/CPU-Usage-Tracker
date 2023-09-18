#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <csignal>
#include <cstring>
#include <unistd.h>
#include <iomanip>

#define MAX_CPUS 32
#define BUFFER_SIZE 256
#define LOG_FILE_PATH "cpu_usage.log"

using namespace std;

struct CPUData {
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
};

struct CPUUsage {
    char cpu_name[16];
    double usage;
};

CPUData prev_cpu_data[MAX_CPUS];
CPUData curr_cpu_data[MAX_CPUS];
int num_cpus;
mutex data_mutex;
condition_variable data_ready;
atomic<bool> program_running(true);
FILE* log_file = nullptr;
mutex log_mutex;

void read_cpu_data() {
    ifstream stat_file("/proc/stat");
    if (!stat_file.is_open()) {
        perror("Nie można otworzyć pliku /proc/stat");
        exit(EXIT_FAILURE);
    }

    string line;
    while (getline(stat_file, line)) {
        if (line.compare(0, 3, "cpu") == 0) {
            CPUData data;
            int cpu_id;
            istringstream ss(line);
            ss >> data.cpu_name >> data.user >> data.nice >> data.system >> data.idle
               >> data.iowait >> data.irq >> data.softirq >> data.steal >> data.guest >> data.guest_nice;

            if (sscanf(data.cpu_name, "cpu%d", &cpu_id) == 1) {
                memcpy(&curr_cpu_data[cpu_id], &data, sizeof(CPUData));
            }
        }
    }
}

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
        snprintf(cpu_usage[i].cpu_name, sizeof(cpu_usage[i].cpu_name), "%s", curr_cpu_data[i].cpu_name);
        cpu_usage[i].usage = usage;
    }
}

void reader_thread() {
    while (program_running) {
        read_cpu_data();
        data_ready.notify_all();
        this_thread::sleep_for(chrono::seconds(1));
    }
}

void analyzer_thread() {
    CPUUsage cpu_usage[MAX_CPUS];
    while (program_running) {
        unique_lock<mutex> lock(data_mutex);
        data_ready.wait(lock);
        calculate_cpu_usage(cpu_usage);
    }
}

void printer_thread() {
    CPUUsage cpu_usage[MAX_CPUS];
    while (program_running) {
        unique_lock<mutex> lock(data_mutex);
        data_ready.wait(lock);
        calculate_cpu_usage(cpu_usage);

        for (int i = 0; i < num_cpus; i++) {
            cout << "CPU " << cpu_usage[i].cpu_name << ": " << fixed << setprecision(2) << cpu_usage[i].usage << "%" << endl;
        }
        cout << endl;
    }
}

void watchdog_thread() {
    while (program_running) {
        this_thread::sleep_for(chrono::seconds(2));
        data_ready.notify_all();
    }
}

void logger_thread() {
    while (program_running) {
        if (log_file != nullptr) {
            lock_guard<mutex> log_lock(log_mutex);
            fprintf(log_file, "Log message\n");
            fflush(log_file);
        }
        this_thread::sleep_for(chrono::seconds(1));
    }
}

void sigterm_handler(int signum) {
    program_running = false;
    data_ready.notify_all();
}

int main() {
    signal(SIGTERM, sigterm_handler);

    log_file = fopen(LOG_FILE_PATH, "w");
    if (log_file == nullptr) {
        perror("Nie można otworzyć pliku logu");
        exit(EXIT_FAILURE);
    }

    num_cpus = sysconf(_SC_NPROCESSORS_ONLN);

    thread reader(reader_thread);
    thread analyzer(analyzer_thread);
    thread printer(printer_thread);
    thread watchdog(watchdog_thread);
    thread logger(logger_thread);

    reader.join();
    analyzer.join();
    printer.join();
    watchdog.join();
    logger.join();

    if (log_file != nullptr) {
        fclose(log_file);
    }

    return 0;
}
