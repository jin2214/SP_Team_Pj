#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <fcntl.h>
#include <time.h>

#define PORT 8080
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

//명령어 이름 설정
char quit[]="!quit\n";
char search[] = "!search\n";
char showall[] = "!showall\n";

int client_sockets[MAX_CLIENTS]; //10명 까지 들어올 수 있음, 소켓 디스크립터 배열
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; //쓰레드 간에 공유 데이터 동기화를 위해 사용, mutex초기화
int fd = 0;

//명령어 !search 구현
void *search_func(void *arg) {
    int client_socket = *(int *)arg;
    char buf[BUFFER_SIZE];
    char line[BUFFER_SIZE];
    char keyword[BUFFER_SIZE];
    int len;
    char start[] = "\n*********************search*********************\n\n";
    char end[] = "\n************************************************\n\n";

    if (recv(client_socket, keyword, sizeof(keyword), 0) > 0){ // 클라이언트에게 키워드 받음
        printf("\nSearching for keyword: %s", keyword); //서버 터미널에 출력 
        keyword[strcspn(keyword, "\n")] = '\0';  // 개행 문자 제거

        // 파일 포인터를 처음으로 이동
        lseek(fd, 0, SEEK_SET);
        send(client_socket, start, strlen(start), 0); // start 메세지 전송
        // 한 줄씩 파일을 읽으며 검색
        int line_index = 0;  // line 버퍼의 인덱스
        while ((len = read(fd, buf, sizeof(buf))) > 0) {
            for (int i = 0; i < len; i++) {
                if (buf[i] == '\n' || line_index == sizeof(line) - 1) { // buf에서 한 줄이 끝나거나 line 버퍼가 꽉차면
                    line[line_index] = '\0';  // 한 줄의 끝을 문자열로 처리
                    if (strstr(line, keyword) != NULL) { //키워드가 포함된 줄이라면
                        printf("Line found: %s\n", line); //서버 터미널에 출력
                        sprintf(line, "%s\n", line); //line 버퍼에 저장(\n 추가)
                        send(client_socket, line, strlen(line), 0); //클라이언트에게 line 전송
                    }
                    line_index = 0;  // 다음 줄 처리를 위해 초기화
                    memset(line, 0, sizeof(line)); //line 버퍼 초기화

                } else { //buf에서 한 줄이 끝나지 않았다면
                    line[line_index++] = buf[i]; //line 버퍼에 buf저장(한 글자씩 저장)
                }
            }
        }
        printf("\n");
        send(client_socket, end, strlen(end), 0); // end 메세지 전송
        // 파일 포인터를 다시 파일 끝으로 이동
        lseek(fd, 0, SEEK_END);
    }
    return NULL;
}

//명령어 !showall 구현
void *showall_func(void *arg) {
    int client_socket = *(int *)arg;
    char buf[BUFFER_SIZE];
    int len = 0;
    char start[] = "\n********************show all********************\n\n";
    char end[] = "\n************************************************\n\n";
    lseek(fd, 0, SEEK_SET);
    send(client_socket, start, strlen(start), 0); // start 메세지 전송
    while ((len = read(fd, buf, sizeof(buf))) > 0) {
        send(client_socket, buf, len, 0); //읽어온 log 전송
    }
    send(client_socket, end, strlen(end), 0); // end 메세지 전송
    lseek(fd, 0, SEEK_END);
}

void *handle_client(void *arg) {
    //인자는 클라이언트 소켓 디스크립터 주소
    int client_socket = *(int *)arg;
    char buffer[BUFFER_SIZE];
    int bytes_read;

    while ((bytes_read = recv(client_socket, buffer, sizeof(buffer), 0)) > 0) {
        //클라이언트 소켓으로부터 send된 데이터를 수신, 성공 시 읽은 바이트 수를 리턴, 데이터 들어올 때까지 대기(블로킹모드)
        //클라이언트가 연결을 종료하면 0이 되고 while문 빠져나감

        //클라이언트에게 받은 메세지가 quit명령어라면
        if(!strcmp(buffer, quit)){
            //while문 밖으로 빠져나감 -> 연결 종료
            break;
        }

        //클라이언트에게 받은 메세지가 search명령어라면
        else if(!strcmp(buffer, search)){
            pthread_t search_thread;
            pthread_create(&search_thread, NULL, search_func,  (void *)&client_socket); //search_func 함수를 독립적으로 사용하는 쓰레드 생성, 클라이언트소켓 전달
            pthread_join(search_thread, NULL); //!search 명령어 끝날때까지 대기
        }

        //클라이언트에게 받은 메세지가 showall명령어라면
        else if(!strcmp(buffer, showall)){
            pthread_t showall_thread;
            pthread_create(&showall_thread, NULL, showall_func, (void *)&client_socket); //showall_func 함수를 독립적으로 사용하는 쓰레드 생성, 클라이언트소켓 전달
            pthread_join(showall_thread, NULL); //!showall 명령어 끝날때까지 대기
        }

        else {
            buffer[bytes_read] = '\0';
            printf("%s", buffer); // 서버 터미널에 출력

            pthread_mutex_lock(&mutex);
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] != 0 && client_sockets[i] != client_socket) {
                    //클라이언트 소켓이 할당되어 있는 만큼, 자기 자신 클라이언트에게는 전송안함
                    send(client_sockets[i], buffer, strlen(buffer), 0); //메세지를 다른 클라이언트에게 전송
                }
            }
            pthread_mutex_unlock(&mutex);
            time_t t = time(NULL);
            struct tm tm = *localtime(&t);
            char chat[BUFFER_SIZE]; //채팅 로그를 저장할 문자열
            int chat_len; 
            chat_len = sprintf(chat, "(%04d-%02d-%02d %02d:%02d:%02d) %s",tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, buffer);
            //chat에 날짜, 시간, 이름, 내용 순으로 저장
            write(fd, chat, chat_len);
            memset(chat, 0, sizeof(chat));
        }
        memset(buffer, 0, sizeof(buffer));
    }

    pthread_mutex_lock(&mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] == client_socket) {
            client_sockets[i] = 0; //연결이 종료된 클라이언트 소켓의 배열을 0으로 바꿈
            printf("A client is disconnected\n");
            break;
        }
    }
    pthread_mutex_unlock(&mutex);

    close(client_socket);
    return NULL;
}

int main() {
    int server_socket, new_socket; //서버 소켓, 클라이언트 소켓 디스크립터
    struct sockaddr_in server_addr, client_addr; //소켓을 바인드 할 때 특성으로 넣을 구조체(?), 소켓의 주소 정보가 저장되는 구조체
    socklen_t addr_len = sizeof(client_addr); //위 구조체 크기

    if((fd = open("log.txt", O_RDWR|O_APPEND, 0666)) == -1) { //채팅을 저장할 "log.txt" 열기
        if((fd = open("log.txt", O_RDWR|O_CREAT, 0666)) == -1) { //"log.txt" 없으면 생성
            perror("LOG file open failed");
            exit(EXIT_FAILURE);
        }
    }

    server_socket = socket(AF_INET, SOCK_STREAM, 0); //서버 소켓 만들기, IPv4 프로토콜, TCP방식
    if (server_socket == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET; //주소체계 설정, IPv4 프로토콜 사용하겠다는 뜻
    server_addr.sin_addr.s_addr = INADDR_ANY; //소켓을 바인딩(특정 IP주소, 포트 번호에 연결), 모든 주소에 바인딩할 수 있도록 함
    server_addr.sin_port = htons(PORT); //서버가 사용할 포트 결정, 바이트 순서를 네트워크 통신에 맞게 변경하는 함수

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, MAX_CLIENTS) == -1) { //서버 소켓을 수신 대기 상태로 전환, backlog는 대기열의 최대크기
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    while (1) {
        new_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len); //서버 소켓 대기열에 연결 요청이 있으면 새로운 소켓으로 할당(클라이언트 요청)
        //대기열에 연결 요청이 없는 경우 block모드에서 프로세스 대기
        if (new_socket == -1) {
            perror("Accept failed");
            continue;
        }

        pthread_mutex_lock(&mutex); //공유자원을 동기화 하기 위해 mutex를 잠금
        int i;
        for (i = 0; i < MAX_CLIENTS; i++) {
            if (client_sockets[i] == 0) {
                client_sockets[i] = new_socket;
                break; //0번째 클라이언트 소켓부터 비어있으면 소켓 디스크립터를 할당 후 break
            }
        }
        pthread_mutex_unlock(&mutex); //mutex 잠금해제

        if (i == MAX_CLIENTS) { 
            //클라이언트가 최대가 된 경우 연결 거부, 소켓 제거
            printf("Maximum clients connected. Connection refused.\n");
            close(new_socket);
        } else {
            //각 클라이언트 소켓 디스크립터에 쓰레드를 할당
            pthread_t thread;
            pthread_create(&thread, NULL, handle_client, (void *)&new_socket); //handle_client 함수를 독립적으로 사용하는 쓰레드 생성, 함수에 전달할 인자 설정
            pthread_detach(thread); //쓰레드를 독립적으로 실행되도록 함, 종료 시 리소스 자동 반환
            printf("New client connected.\n");
        }
    }
    close(fd);
    close(server_socket);
    return 0;
}
