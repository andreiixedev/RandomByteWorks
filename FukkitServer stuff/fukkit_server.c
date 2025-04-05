#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include <termios.h>
#include <fcntl.h>

// ASCII FukkitServer
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

// Function to check if directory exists, a wait and file
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
    printf("\033[38;5;%dm", color_code);
}

// Function to reset color to default
void reset_color() {
    printf("\033[0m"); 
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

    // Read of "CPU data"
    fp = fopen("/proc/stat", "r");
    if (fp != NULL) {
        fgets(buffer, sizeof(buffer), fp);
        sscanf(buffer, "cpu %llu %llu %llu %llu %llu %llu %llu %llu", &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal); //delulu
        fclose(fp);
    }
    total1 = user + nice + system + idle + iowait + irq + softirq + steal;
    idle1 = idle;

    // Wait for a short moment to calculate Shitt CPU usage
    usleep(100000);

    // Second read of "CPU data"
    fp = fopen("/proc/stat", "r");
    if (fp != NULL) {
        fgets(buffer, sizeof(buffer), fp);
        sscanf(buffer, "cpu %llu %llu %llu %llu %llu %llu %llu %llu", &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal);
        fclose(fp);
    }
    total2 = user + nice + system + idle + iowait + irq + softirq + steal;
    idle2 = idle;

    // Calculate CPU usage again
    double cpu_usage = 100.0 * (1.0 - (double)(idle2 - idle1) / (total2 - total1));

    // Read RAM usage (4gb lol)
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

// Animated aschii
void ui_loop() {
    int color_code = 0;
    while (1) {
        time_t now = time(NULL);
        struct tm *local = localtime(&now);
        int remaining_hours = (3 - local->tm_hour + 24) % 24;
        int remaining_minutes = 59 - local->tm_min;
        int remaining_seconds = 59 - local->tm_sec;

        // check if 3AM
        if (local->tm_hour == 3 && local->tm_min == 0 && local->tm_sec == 0) {
            printf("[INFO] It's 3 AM! Resetting sessions...\n");
            reset_sessions();
            sleep(60); // wait a minute do not mess 
        }

        printf("\033[H\033[J");
        apply_color(color_code); 
        printf("%s", fukkit_ascii); // Display ASCII Art
        reset_color(); 

        printf("The servers will reset at 03:00 AM\n");
        printf("Time remaining: %02d:%02d:%02d\n", remaining_hours, remaining_minutes, remaining_seconds);

        // Real time stuff
        get_system_usage();
        display_session_status();

        // Options to make easy
        printf("[1] to reset sessions.\n");
        printf("[2] to reboot.\n");
        printf("[3] to exit.\n");

        // Code-Server
        if (check_session_exists(CODE_SERVER_SESSION)) {
            printf("[4] to stop Code-Server session.\n");
        } else {
            printf("[4] to start Code-Server session.\n");
        }

        // Non-blocking input handling for interactivity
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

                    // Waittttt
                    int tries = 0;
                    while (check_session_exists(MC_SESSION) ||
                            check_session_exists(PLAYIT_SESSION) ||
                            check_session_exists(CODE_SERVER_SESSION)) {
                        printf("[INFO] Waiting for sessions to terminate...\n");
                        sleep(1);
                        if (++tries >= 10) { // timeout 10sec
                            printf("[WARNING] Some sessions are still running. Forcing exit.\n");
                            break;
                        }
                    }

                    printf("[INFO] All sessions terminated. Exiting now.\n");
                    return; // exit from this mess

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

        // Update color
        color_code = (color_code + 1) % 8; // Color range [0, 7]
        sleep(1); // Update 
    }
}

int main() {
    printf("[INFO] Starting the Fukkit server monitor...\n");

    // Checks
    start_session(MC_SESSION, "cd /home/andreiixe/minecraft && ./start.sh");
    start_session(PLAYIT_SESSION, "playit");
    start_session(CODE_SERVER_SESSION, "code-server");

    ui_loop(); // loop this mess
    return 0;
}
