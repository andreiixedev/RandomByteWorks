#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include <termios.h>
#include <fcntl.h>

// ASCII Art with colors
char *fukkit_ascii = 
  "  ______     _    _    _ _                                  \n"
  " |  ____|   | |  | |  (_) |                                 \n"
  " | |__ _   _| | _| | ___| |_   ___  ___ _ ____   _____ _ __ \n"
  " |  __| | | | |/ / |/ / | __| / __|/ _ \\ '__\\ \\ / / _ \\ '__| \n"
  " | |  | |_| |   <|   <| | |_  \\__ \\  __/ |   \\ V /  __/ |   \n"
  " |_|   \\__,_|_|\\_\\_|\\_\\_|__| |___/\\___|_|    \\_/ \\___|_|   \n"
  "                                                             \n";

// Define constants with absolute paths
#define MC_DIR "/home/andreiixe/minecraft"
#define MC_SESSION "minecraftserver"
#define PLAYIT_SESSION "playit"
#define CODE_SERVER_SESSION "code-server"

// Function to check if a file or directory exists
int check_file_exists(const char *path) {
    struct stat buffer;
    return (stat(path, &buffer) == 0);
}

// Function to check if a TMUX session exists
int check_session_exists(const char *session) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "tmux has-session -t %s 2>/dev/null", session);
    return (system(cmd) == 0); // Returns 0 if session exists
}

// Function to get input without blocking
int kbhit(void) {
    struct termios oldt, newt;
    int ch;
    int oldf;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);    // Disable buffering and echo
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    if(ch != EOF) {
        ungetc(ch, stdin);
        return 1;
    }

    return 0;
}

// Function to apply color changes dynamically
void apply_color(int color_code) {
    printf("\033[38;5;%dm", color_code); // Change text color
}

// Function to reset color to default
void reset_color() {
    printf("\033[0m"); // Reset to default color
}

void start_session(const char *session, const char *command) {
    if (!check_session_exists(session)) {
        char cmd[256];
        printf("[INFO] Starting TMUX session for %s...\n", session);
        snprintf(cmd, sizeof(cmd), "tmux new-session -d -s %s \"%s\"", session, command);
        int status = system(cmd);
        if (status != 0) {
            printf("[ERROR] Failed to start session %s. Error code: %d\n", session, status);
        }
    } else {
        printf("[INFO] Session %s is already running. Skipping...\n", session);
    }
}

void stop_session(const char *session) {
    if (check_session_exists(session)) {
        char cmd[256];
        printf("[INFO] Stopping TMUX session for %s...\n", session);
        snprintf(cmd, sizeof(cmd), "tmux kill-session -t %s 2>/dev/null", session);
        system(cmd);
    } else {
        printf("[INFO] Session %s is not active. Skipping...\n", session);
    }
}

void reset_sessions() {
    printf("[INFO] Resetting sessions...\n");

    if (!check_file_exists(MC_DIR)) {
        printf("[ERROR] Directory %s does not exist! Check the location.\n", MC_DIR);
        return;
    }

    stop_session(MC_SESSION);
    stop_session(PLAYIT_SESSION);
    stop_session(CODE_SERVER_SESSION);
    sleep(5);

    start_session(MC_SESSION, "cd /home/andreiixe/minecraft && ./start.sh");
    start_session(PLAYIT_SESSION, "playit");
    sleep(5);
}

void get_system_usage() {
    FILE *fp;
    char buffer[128];
    unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;
    unsigned long long total1, total2, idle1, idle2;

    // First read of CPU data
    fp = fopen("/proc/stat", "r");
    if (fp != NULL) {
        fgets(buffer, sizeof(buffer), fp);
        sscanf(buffer, "cpu %llu %llu %llu %llu %llu %llu %llu %llu", &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal);
        fclose(fp);
    }
    total1 = user + nice + system + idle + iowait + irq + softirq + steal;
    idle1 = idle;

    // Wait for a short moment to calculate CPU usage difference
    usleep(100000); // Sleep for 100 milliseconds

    // Second read of CPU data
    fp = fopen("/proc/stat", "r");
    if (fp != NULL) {
        fgets(buffer, sizeof(buffer), fp);
        sscanf(buffer, "cpu %llu %llu %llu %llu %llu %llu %llu %llu", &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal);
        fclose(fp);
    }
    total2 = user + nice + system + idle + iowait + irq + softirq + steal;
    idle2 = idle;

    // Calculate CPU usage
    double cpu_usage = 100.0 * (1.0 - (double)(idle2 - idle1) / (total2 - total1));

    // Read RAM usage
    unsigned long mem_total = 0, mem_free = 0;
    fp = fopen("/proc/meminfo", "r");
    if (fp != NULL) {
        while (fgets(buffer, sizeof(buffer), fp)) {
            sscanf(buffer, "MemTotal: %lu kB", &mem_total);
            sscanf(buffer, "MemFree: %lu kB", &mem_free);
        }
        fclose(fp);
    }
    double ram_usage = 100.0 * (1.0 - (double)mem_free / mem_total);

    // Display system usage
    printf("-------------------------------\n");
    printf("CPU Usage: %.2f%%\n", cpu_usage);
    printf("RAM Usage: %.2f%%\n", ram_usage);
    printf("-------------------------------\n");
}

void display_session_status() {
    printf("[SESSION STATUS]\n");
    if (check_session_exists(MC_SESSION)) {
        printf("Minecraft Server: RUNNING ✅\n");
    } else {
        printf("Minecraft Server: STOPPED ❌\n");
    }

    if (check_session_exists(PLAYIT_SESSION)) {
        printf("Playit: RUNNING ✅\n");
    } else {
        printf("Playit: STOPPED ❌\n");
    }

    if (check_session_exists(CODE_SERVER_SESSION)) {
        printf("Code-Server: RUNNING\n");
    } else {
        printf("Code-Server: STOPPED\n");
    }
    printf("-------------------------------\n");
}

void ui_loop() {
    int color_code = 0;
    int reset_done_today = 0;

    while (1) {
        printf("\033[H\033[J"); // Clear the screen
        apply_color(color_code);
        printf("%s", fukkit_ascii);
        reset_color();

        get_system_usage();
        display_session_status();

        // Timpul până la ora 3:00
        time_t now = time(NULL);
        struct tm *current_time = localtime(&now);

        // Creează un timp pentru ora 3:00 AM a zilei curente
        struct tm reset_time = *current_time;
        reset_time.tm_hour = 3;
        reset_time.tm_min = 0;
        reset_time.tm_sec = 0;
        time_t reset_epoch = mktime(&reset_time);

        // Dacă a trecut de 3 dimineața, setăm ținta pentru a doua zi
        if (difftime(now, reset_epoch) >= 0) {
            reset_time.tm_mday += 1;
            reset_epoch = mktime(&reset_time);
            reset_done_today = 0; // Resetăm flagul pentru o nouă zi
        }

        // Afișează timpul rămas până la 3:00 AM
        int seconds_left = (int)difftime(reset_epoch, now);
        int hrs = seconds_left / 3600;
        int mins = (seconds_left % 3600) / 60;
        int secs = seconds_left % 60;

        printf("[AUTO RESET] Time until 3:00 AM reset: %02d:%02d:%02d\n", hrs, mins, secs);

        // Declanșează reset dacă e ora 3 fix și nu a fost deja făcut
        if (hrs == 0 && mins == 0 && secs <= 5 && !reset_done_today) {
            printf("[AUTO RESET] It's 3:00 AM! Resetting sessions...\n");
            reset_sessions();
            reset_done_today = 1;
            sleep(5); // Pauză să nu repornească iar în același minut
        }

        // Opțiuni
        printf("[1] to reset sessions.\n");
        printf("[2] to reboot.\n");
        printf("[3] to exit.\n");

        if (check_session_exists(CODE_SERVER_SESSION)) {
            printf("[4] to stop Code-Server session.\n");
        } else {
            printf("[4] to start Code-Server session.\n");
        }

        if (kbhit()) {
            int key = getchar();
            switch (key) {
                case '1':
                    reset_sessions();
                    break;
                case '2':
                    printf("[INFO] Rebooting system...\n");
                    system("sudo reboot");
                    break;
                case '3':
                    printf("[INFO] Exiting program... Waiting for sessions to stop.\n");

                    stop_session(MC_SESSION);
                    stop_session(PLAYIT_SESSION);
                    stop_session(CODE_SERVER_SESSION);

                    int tries = 0;
                    while (check_session_exists(MC_SESSION) ||
                           check_session_exists(PLAYIT_SESSION) ||
                           check_session_exists(CODE_SERVER_SESSION)) {
                        printf("[INFO] Waiting for sessions to terminate...\n");
                        sleep(1);
                        if (++tries >= 10) {
                            printf("[WARNING] Some sessions are still running. Forcing exit.\n");
                            break;
                        }
                    }

                    printf("[INFO] All sessions terminated. Exiting now.\n");
                    return;

                case '4':
                    if (check_session_exists(CODE_SERVER_SESSION)) {
                        stop_session(CODE_SERVER_SESSION);
                    } else {
                        start_session(CODE_SERVER_SESSION, "code-server");
                    }
                    break;
                default:
                    break;
            }
        }

        color_code = (color_code + 1) % 8;
        sleep(1);
    }
}

int main() {
    printf("[INFO] Starting the Fukkit server monitor...\n");

    // Initial check and starting sessions
    start_session(MC_SESSION, "cd /home/andreiixe/minecraft && ./start.sh");
    start_session(PLAYIT_SESSION, "playit");
    start_session(CODE_SERVER_SESSION, "code-server");

    ui_loop(); // Start the UI loop
    return 0;
}