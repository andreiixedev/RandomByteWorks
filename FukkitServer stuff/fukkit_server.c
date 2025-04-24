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

#define MC_DIR "/home/andreiixe/minecraft"
#define MC_SESSION "minecraftserver"
#define PLAYIT_SESSION "playit"
#define CODE_SERVER_SESSION "code-server"
#define NETWORK_INTERFACE "wls3"  // Ajustează la interfața ta, ex: "eth0" sau "wlan0"

int check_file_exists(const char *path) {
    struct stat buffer;
    return (stat(path, &buffer) == 0);
}

int check_session_exists(const char *session) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "tmux has-session -t %s 2>/dev/null", session);
    return (system(cmd) == 0);
}

int kbhit(void) {
    struct termios oldt, newt;
    int ch, oldf;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);
    if (ch != EOF) { ungetc(ch, stdin); return 1; }
    return 0;
}

void apply_color(int color_code) {
    printf("\033[48;5;%dm\033[38;5;15m", color_code);
}

void reset_color() {
    printf("\033[0m");
}

void start_session(const char *session, const char *command) {
    if (!check_session_exists(session)) {
        char cmd[256];
        printf("[INFO] Starting TMUX session for %s...\n", session);
        snprintf(cmd, sizeof(cmd), "tmux new-session -d -s %s \"%s\"", session, command);
        if (system(cmd) != 0)
            printf("[ERROR] Failed to start session %s\n", session);
    } else {
        printf("[INFO] Session %s already running\n", session);
    }
}

void stop_session(const char *session) {
    if (check_session_exists(session)) {
        char cmd[256];
        printf("[INFO] Stopping TMUX session for %s...\n", session);
        snprintf(cmd, sizeof(cmd), "tmux kill-session -t %s 2>/dev/null", session);
        system(cmd);
    } else {
        printf("[INFO] Session %s not active\n", session);
    }
}

void reset_sessions() {
    printf("[INFO] Resetting all sessions...\n");
    if (!check_file_exists(MC_DIR)) {
        printf("[ERROR] Directory %s does not exist\n", MC_DIR);
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

void execute_command_from_file() {
    if (!check_file_exists("command_input.txt")) return;
    FILE *f = fopen("command_input.txt","r");
    if (!f) { printf("[ERROR] Cannot open command_input.txt\n"); return; }
    char cmd[10];
    if (fgets(cmd, sizeof(cmd), f)) {
        printf("[INFO] Running command: %s", cmd);
        switch (cmd[0]) {
            case '1': reset_sessions(); break;
            case '2': system("sudo reboot"); break;
            case '3': printf("Exiting...\n"); exit(0);
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
                printf("[ERROR] Invalid command\n");
        }
    }
    fclose(f);
    remove("command_input.txt");
}

// Verifică dacă rețeaua e online
int is_online() {
    return (system("ping -c1 -W1 8.8.8.8 > /dev/null 2>&1") == 0);
}

// Reset al interfeței doar când e offline și la 20s între resetări
void reset_network_interface() {
    static time_t last_network_reset = 0;
    time_t now = time(NULL);

    if (!is_online() && now - last_network_reset >= 20) {
        last_network_reset = now;
        printf("[INFO] Network is offline, resetting %s...\n", NETWORK_INTERFACE);

        char cmd[256];
        snprintf(cmd, sizeof(cmd), "sudo ip link set %s down", NETWORK_INTERFACE);
        system(cmd);
        sleep(10);
        snprintf(cmd, sizeof(cmd), "sudo ip link set %s up", NETWORK_INTERFACE);
        system(cmd);

        printf("[INFO] Network interface %s has been reset.\n", NETWORK_INTERFACE);
    }
}

void ui_loop() {
    while (1) {
        printf("\033[H\033[J");          // clear screen
        printf("%s", fukkit_ascii);
        reset_color();

        printf("╔═══════════════[ FUKKIT SERVER DASHBOARD ]═══════════════╗\n");
        printf("║  [1] Reset all sessions                                 ║\n");
        printf("║  [2] Reboot system                                      ║\n");
        printf("║  [3] Exit                                               ║\n");

        if (check_session_exists(CODE_SERVER_SESSION)) {
            printf("║  [4] Code-Server: Running                               ║\n");
            printf("║      ┌─ Last Output ─────────────────────────────────┐  ║\n");
            system("tmux capture-pane -pt code-server -S -10 | tail -n 10");
            printf("║      └───────────────────────────────────────────────┘  ║\n");
        } else {
            printf("║  [4] Code-Server: Not Running                          ║\n");
        }

        printf("║  [5] Reset Minecraft server                             ║\n");
        if (check_session_exists(MC_SESSION)) {
            printf("║      Status: Running                                    ║\n");
            printf("║      ┌─ Last Output ─────────────────────────────────┐  ║\n");
            system("tmux capture-pane -pt minecraftserver -S -10 | tail -n 10");
            printf("║      └───────────────────────────────────────────────┘  ║\n");
        } else {
            printf("║      Status: Not Running                               ║\n");
        }

        printf("║  [6] Reset Playit                                       ║\n");
        if (check_session_exists(PLAYIT_SESSION)) {
            printf("║      Status: Running                                    ║\n");
        } else {
            printf("║      Status: Not Running                               ║\n");
        }

        printf("║  Network Status: ");
        if (is_online()) {
            printf("\033[32mONLINE\033[0m                                 ║\n");
        } else {
            printf("\033[31mOFFLINE\033[0m                                ║\n");
            reset_network_interface();
        }

        printf("╚═════════════════════════════════════════════════════════╝\n");

        execute_command_from_file();

        if (kbhit()) {
            int key = getchar();
            switch (key) {
                case '1': reset_sessions(); break;
                case '2': system("sudo reboot"); break;
                case '3':
                    stop_session(CODE_SERVER_SESSION);
                    stop_session(MC_SESSION);
                    stop_session(PLAYIT_SESSION);
                    printf("[INFO] Exiting...\n");
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
            }
        }

        sleep(1);
    }
}

int main() {
    ui_loop();
    return 0;
}