#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

#define BUFFER_SIZE 1024

void *receive_messages(void *arg);
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
void send_message(const char *message);
void display_manual_with_ui();
void display_with_time(const char *message);

int main(int argc, char *argv[]) {
    int client_socket;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    const char *RESET = "\033[0m";
    const char *RED = "\033[1;31m";

    if (argc != 4) {
        printf("Usage: [filename] [name] [ip address] [port number]\n");
        exit(-1);
    }
    
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[3]));
    server_addr.sin_addr.s_addr = inet_addr(argv[2]);

    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    printf("\033[31mConnected to server. Type messages and press Enter to send.\033[0m\n");

    write(client_socket, argv[1], strlen(argv[1]));

    pthread_t thread;
    pthread_create(&thread, NULL, receive_messages, (void *)&client_socket);

    display_manual_with_ui();
    printf("%s\nEnter a command or message: %s", RED, RESET); 
    while (1) {   
        if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
            size_t len = strlen(buffer);
            buffer[len - 1] = '\0';  // 개행 문자 제거

            if (strcmp(buffer, "!help") == 0) {
                display_manual_with_ui();
                continue;
            }

            if (strcmp(buffer, "!quit") == 0) {
                send(client_socket, buffer, strlen(buffer), 0);
                break;
            }

            send_message(buffer); 
            send(client_socket, buffer, strlen(buffer), 0); 
        }
    }

    close(client_socket);
    return 0;
}

void *receive_messages(void *arg) {
    int socket = *(int *)arg;
    char buffer[BUFFER_SIZE];
    int bytes_read;
    
    while ((bytes_read = recv(socket, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes_read] = '\0';
        const char *CYAN = "\033[1;36m"; 
        const char *RESET = "\033[0m"; 
        printf("%sServer: ", CYAN); 
        display_with_time(buffer);  // 시간 추가
        printf("%s", RESET);  // 색상 초기화
        fflush(stdout);
    }
    return NULL;
}

void send_message(const char *message) {
    const char *RESET = "\033[0m";
    const char *BLUE = "\033[1;34m";  
    printf("%s[Sending]: ", BLUE);
    display_with_time(message);  // 시간 추가
    printf("%s", RESET);
}

// 명령어 및 UI 출력 함수
void display_manual_with_ui() {
    const char *GREEN = "\033[1;32m";
    const char *RESET = "\033[0m";

    printf("\n%s================ Chat Application ================%s\n", GREEN, RESET);
    printf("Available Commands:\n");
    printf("1. !help - Show command manual\n");
    printf("2. !quit - Close the application\n");
    printf("3. !position [POSITION] - Change your position\n");
    printf("4. !info - Show all clients and positions\n");
    printf("5. !search [KEYWORD] - Search old chat history\n");
    printf("6. !showall - Show all chat history\n");
    printf("\n**************************************************\n\n");
}

// 시간 표시를 추가한 메시지 출력 함수
void display_with_time(const char *message) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    printf("[%02d:%02d:%02d] %s", t->tm_hour, t->tm_min, t->tm_sec, message);
}
