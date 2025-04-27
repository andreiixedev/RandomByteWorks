#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include <termios.h>
#include <fcntl.h>

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
#define PANELBOT_SESSION "panelbot"
#define NETWORK_INTERFACE "wls3"  // Ajustează la interfața ta, ex: "eth0" sau "wlan0"

// Declarațiile funcțiilor
void reset_sessions(void);
void reset_panelbot_session(void);

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

void start_panelbot_session() {
    if (!check_session_exists(PANELBOT_SESSION)) {
        char cmd[512];
        printf("[INFO] Starting PANELBOT session...\n");
        
        // Crează comanda pentru a activa mediul virtual și a porni botul
        snprintf(cmd, sizeof(cmd), "tmux new-session -d -s %s \"source botenv/bin/activate && export DISCORD_BOT_TOKEN=HEHEHE && python3 bot.py\"", PANELBOT_SESSION);
        
        if (system(cmd) != 0) {
            printf("[ERROR] Failed to start PANELBOT session\n");
        }
    } else {
        printf("[INFO] PANELBOT session already running\n");
    }
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
            case '7':
                reset_panelbot_session();  // Reset panelbot
                break;
            default:
                printf("[ERROR] Invalid command\n");
        }
    }
    fclose(f);
    remove("command_input.txt");
}

int is_online() {
    return (system("ping -c1 -W1 8.8.8.8 > /dev/null 2>&1") == 0);
}

void get_cpu_usage() {
    FILE *fp = popen("top -bn1 | grep 'Cpu' | sed \"s/.*, *\\([0-9.]*\\)%* id.*/\\1/\" | awk '{print 100 - $1}'", "r");
    if (fp) {
        char buffer[256];
        if (fgets(buffer, sizeof(buffer), fp)) {
            printf("║  CPU Usage: %.2f%%\n", atof(buffer));
        }
        fclose(fp);
    }
}

void get_memory_usage() {
    FILE *fp = popen("free | grep Mem | awk '{print $3/$2 * 100.0}'", "r");
    if (fp) {
        char buffer[256];
        if (fgets(buffer, sizeof(buffer), fp)) {
            float memory_usage = atof(buffer);
            printf("║  Memory Usage: [");

            int bars = (int)(memory_usage / 5); 
            for (int i = 0; i < 20; i++) {
                if (i < bars) {
                    printf("=");
                } else {
                    printf(" ");
                }
            }

            printf("] %.2f%%\n", memory_usage);
        }
        fclose(fp);
    }
}

void get_ping() {
    FILE *fp = popen("ping -c 1 8.8.8.8 | grep 'time=' | awk -F'=' '{print $4}' | cut -d' ' -f1", "r");
    if (fp) {
        char buffer[256];
        if (fgets(buffer, sizeof(buffer), fp)) {
            printf("║  Ping: %s", buffer);
        }
        fclose(fp);
    }
}


void reset_sessions() {
    stop_session(MC_SESSION);
    stop_session(CODE_SERVER_SESSION);
    stop_session(PLAYIT_SESSION);
    stop_session(PANELBOT_SESSION);
    sleep(1);
    start_session(CODE_SERVER_SESSION, "code-server");
    start_session(MC_SESSION, "cd ~/minecraft && ./start.sh");
    start_session(PLAYIT_SESSION, "playit");
    start_panelbot_session();
}

void reset_panelbot_session() {
    stop_session(PANELBOT_SESSION);
}

void ui_loop() {
    while (1) {
        printf("\033[H\033[J");          // clear screen
        printf("%s", fukkit_ascii);
        reset_color();

        printf("╔═══════════════[ FUKKIT SERVER DASHBOARD ]═══════════════\n");
        printf("║  [1] Reset all sessions                                 \n");
        printf("║  [2] Reboot system                                      \n");
        printf("║  [3] Exit                                               \n");

        if (check_session_exists(CODE_SERVER_SESSION)) {
            printf("║  [4] Code-Server: Running                               \n");
            printf("║      ┌─ Last Output ─────────────────────────────────┐  \n");
            system("tmux capture-pane -pt code-server -S -10 | tail -n 10");
            printf("║      └───────────────────────────────────────────────┘  \n");
        } else {
            printf("║  [4] Code-Server: Not Running                          \n");
        }

        printf("║  [5] Reset Minecraft server                            \n");
        if (check_session_exists(MC_SESSION)) {
            printf("║      Status: Running                                    \n");
            printf("║      ┌─ Last Output ─────────────────────────────────┐  \n");
            system("tmux capture-pane -pt minecraftserver -S -10 | tail -n 10");
            printf("║      └───────────────────────────────────────────────┘  \n");
        } else {
            printf("║      Status: Not Running                               \n");
        }

        printf("║  [6] Reset Playit                                       \n");
        if (check_session_exists(PLAYIT_SESSION)) {
            printf("║      Status: Running                                    \n");
        } else {
            printf("║      Status: Not Running                               \n");
        }

        printf("║  [7] Reset Panelbot: ");
        if (check_session_exists(PANELBOT_SESSION)) {
            printf("Running                                    \n");
        } else {
            printf("Not Running                       \n");
        }

        printf("║\n");
        printf("║\n");
        printf("║ ┌─ System Status ───────────────────────────────┐  \n");
        printf("║  Network Status: ");
        if (is_online()) {
            printf("\033[32mONLINE\033[0m                                \n");
            get_ping();
        } else {
            printf("\033[31mOFFLINE\033[0m                                \n");
        }

        get_cpu_usage();
        get_memory_usage();

        printf("║ └───────────────────────────────────────────────┘  \n");

        printf("║\n");
        printf("║\n");
        printf("╚═════════════════════════════════════════════════════════\n");
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
                    stop_session(PANELBOT_SESSION); 
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
                    start_session(PLAYIT_SESSION, "playit");
                    break;
                case '7':
                    stop_session(PANELBOT_SESSION);
                    sleep(1);
                    start_panelbot_session();
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