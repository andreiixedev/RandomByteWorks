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

// Function to start a TMUX session
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

// Function to stop a TMUX session
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

// Function to reset all sessions
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

// Function to read the command from "command_input.txt"
void execute_command_from_file() {
    if (check_file_exists("command_input.txt")) {
        FILE *file = fopen("command_input.txt", "r");
        if (file != NULL) {
            char command[10]; // Expecting a single character command (e.g. '4', '3', etc.)
            if (fgets(command, sizeof(command), file) != NULL) {
                printf("[INFO] Executing command: %s\n", command);
                switch (command[0]) {
                    case '1':
                        reset_sessions();
                        break;
                    case '2':
                        system("sudo reboot");
                        break;
                    case '3':
                        printf("Exiting...\n");
                        exit(0);
                    case '4':
                        if (check_session_exists(CODE_SERVER_SESSION))
                            stop_session(CODE_SERVER_SESSION);
                        else
                            start_session(CODE_SERVER_SESSION, "code-server");
                        break;
                    case '5':
                        stop_session(MC_SESSION);
                        sleep(1);
                        start_session(MC_SESSION, "cd ~/minecraft && ./start.sh");
                        break;
                    case '6':
                        stop_session(PLAYIT_SESSION);
                        sleep(1);
                        start_session(PLAYIT_SESSION, "~/playit/playit-linux");
                        break;
                    default:
                        printf("[ERROR] Invalid command in command_input.txt\n");
                }
            }
            fclose(file);

            // Delete the file after executing the command
            if (remove("command_input.txt") == 0) {
                printf("[INFO] command_input.txt deleted after execution.\n");
            } else {
                printf("[ERROR] Failed to delete command_input.txt\n");
            }
        } else {
            printf("[ERROR] Could not open command_input.txt!\n");
        }
    }
}

// UI loop for handling user inputs
// UI loop for handling user inputs
void ui_loop() {
    printf("%s", fukkit_ascii);
    int color_code = 0;
    int reset_done_today = 0;

    while (1) {
        printf("\033[H\033[J"); // Clear the screen
        apply_color(color_code);
        printf("%s", fukkit_ascii);
        reset_color();

        // UI aesthetic starts here
        printf("╔═══════════════[ FUKKIT SERVER DASHBOARD ]═══════════════╗\n");
        printf("║  [1] Reset all sessions                                 ║\n");
        printf("║  [2] Reboot system                                      ║\n");
        printf("║  [3] Exit                                               ║\n");

        // Code-Server Session
        if (check_session_exists(CODE_SERVER_SESSION)) {
            printf("║  [4] Code-Server: Running                                ║\n");
            printf("║      ┌─ Last Output ─────────────────────────────────┐  ║\n");
            system("tmux capture-pane -pt code-server -S -10 | tail -n 10");
            printf("║      └───────────────────────────────────────────────┘  ║\n");
        } else {
            printf("║  [4] Code-Server: Not Running                          ║\n");
        }

        // Minecraft Server Session
        printf("║  [5] Reset Minecraft server                             ║\n");
        if (check_session_exists(MC_SESSION)) {
            printf("║      Status: Running                                    ║\n");
            printf("║      ┌─ Last Output ─────────────────────────────────┐  ║\n");
            system("tmux capture-pane -pt minecraftserver -S -10 | tail -n 10");
            printf("║      └───────────────────────────────────────────────┘  ║\n");
        } else {
            printf("║      Status: Not Running                               ║\n");
        }

        // Playit Session
        printf("║  [6] Reset Playit                                       ║\n");
        if (check_session_exists(PLAYIT_SESSION)) {
            printf("║      Status: Running                                    ║\n");
        } else {
            printf("║      Status: Not Running                               ║\n");
        }

        // Reset countdown
        time_t now = time(NULL);
        struct tm *current_time = localtime(&now);

        struct tm reset_time = *current_time;
        reset_time.tm_hour = 3;
        reset_time.tm_min = 0;
        reset_time.tm_sec = 0;

        time_t reset_epoch = mktime(&reset_time);
        if (difftime(now, reset_epoch) >= 0) {
            reset_time.tm_mday += 1;
            reset_epoch = mktime(&reset_time);
            reset_done_today = 0;
        }

        int seconds_left = (int)difftime(reset_epoch, now);
        int hrs = seconds_left / 3600;
        int mins = (seconds_left % 3600) / 60;
        int secs = seconds_left % 60;

        printf("║  Auto reset at 3:00 AM in: %02d:%02d:%02d                     ║\n", hrs, mins, secs);
        printf("╚═════════════════════════════════════════════════════════╝\n");

        // Execute command from file automatically
        execute_command_from_file();

        // Execute user input commands via keyboard
        if (kbhit()) {
            int key = getchar();
            switch (key) {
                case '1':
                    reset_sessions();
                    break;
                case '2':
                    system("sudo reboot");
                    break;
                case '3':
                    // Stop all sessions before exit
                    printf("[INFO] Stopping all sessions before exit...\n");

                    stop_session(CODE_SERVER_SESSION);
                    stop_session(MC_SESSION);
                    stop_session(PLAYIT_SESSION);

                    // Exit the program after stopping sessions
                    printf("[INFO] Exiting...\n");
                    exit(0);
                    break;
                case '4':
                    if (check_session_exists(CODE_SERVER_SESSION))
                        stop_session(CODE_SERVER_SESSION);
                    else
                        start_session(CODE_SERVER_SESSION, "code-server");
                    break;
                case '5':
                    stop_session(MC_SESSION);
                    sleep(1);
                    start_session(MC_SESSION, "cd ~/minecraft && ./start.sh");
                    break;
                case '6':
                    stop_session(PLAYIT_SESSION);
                    sleep(1);
                    start_session(PLAYIT_SESSION, "~/playit/playit-linux");
                    break;
            }
        }
        sleep(1);
    }
}

int main() {
    ui_loop();
    return 0;
}