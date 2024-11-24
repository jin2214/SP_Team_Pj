#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8080
#define BUFFER_SIZE 1024

//명령어 이름 설정
char quit[] = "!quit\n";
char help[] = "!help\n";
char search[] = "!search\n";
char showall[] = "!showall\n";

//!help 명령어시 출력되는 메세지
char manual[] = "\n*********************manual*********************\n\n\
 !help -> Show command manual.\n\
 !quit -> Close chatting application.\n\
 !search -> Search old chat history.\n\
 !showall -> Show all old chat history\n\
\n************************************************\n\n";

void *receive_messages(void *arg) {
    int socket = *(int *)arg;
    char buffer[BUFFER_SIZE];
    int bytes_read;

    while ((bytes_read = recv(socket, buffer, sizeof(buffer), 0)) > 0) {
        //서버에서 온 데이터를 자기 자신의 터미널에 출력
        buffer[bytes_read] = '\0';
        printf("%s", buffer);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    int client_socket;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    char chat[BUFFER_SIZE]; //이름과 내용을 함께 전송할 문자열

    client_socket = socket(AF_INET, SOCK_STREAM, 0); //클라이언트 소켓 생성
    if (client_socket == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); //로컬 호스트의 IP주소로 연결하겠다는 뜻, 다른 컴퓨터라면 서버 컴퓨터의 IP주소를 입력해야 함

    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        //클라이언트 소켓에서 서버 소켓으로 연결 요청을 보냄 -> 서버의 listen중인 소켓의 대기열로 들어가는 듯 함
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    printf("Connected to server. Type messages and press Enter to send.\n");

    pthread_t thread;
    pthread_create(&thread, NULL, receive_messages, (void *)&client_socket); //receive_message함수를 수행하는 쓰레드 생성, 인자로 클라이언트 소켓 주소 전달

    while (fgets(buffer, sizeof(buffer), stdin) != NULL) {
        //표준입력으로 메세지를 입력받음, 이후 입력 대기상태

        //입력받은 메세지가 quit 명령어인지 구분
        if(!strcmp(buffer, quit)) {
            //quit 명령어라면 quit명령어만 서버로 전송
            send(client_socket, buffer, strlen(buffer), 0);
            break; //while문 탈출 -> 연결 종료
        }

        //입력받은 메세지가 help 명령어라면 manual 출력
        else if(!strcmp(buffer, help)) {
            printf(manual);
        }

        //입력받은 메세지가 search 명령어인지 구분
        else if(!strcmp(buffer, search)) {
            //search 명령어라면 search명령어 서버로 전송
            send(client_socket, buffer, strlen(buffer), 0);
            memset(buffer, 0, sizeof(buffer)); //buffer 초기화

            //search 할 키워드 입력 받음
            printf("Enter keyword to search: ");
            fgets(buffer, sizeof(buffer), stdin);
            //키워드 서버로 전송
            send(client_socket, buffer, strlen(buffer), 0);
        }

        //입력받은 메세지가 showall 명령어인지 구분
        else if(!strcmp(buffer, showall)) {
            //showall 명령어라면 showall명령어만 서버로 전송
            send(client_socket, buffer, strlen(buffer), 0);
        }

        //입력된 메세지가 명령어가 아니면
        else{
            sprintf(chat, "[%s] %s", argv[1], buffer); //chat 문자열에 [이름] 내용 저장
            send(client_socket, chat, strlen(chat), 0); //입력받은 메세지를 자기 자신 소켓으로 전달
        }
        memset(buffer, 0, sizeof(buffer)); //buffer 초기화
        memset(chat, 0, sizeof(chat)); //chat 초기화
    }

    close(client_socket);
    return 0;
}