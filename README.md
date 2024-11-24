# SystemProgramming_TeamProject

클라이언트 소스코드랑 서버 소스코드입니다.

- 문제점
log.txt 파일이 계속 쌓이는데 어떻게 할 것인가?
각종 명령어 입력 시 segment fault 발생 -> 명령어 개수 다르면 에러메세지 보내야 할 듯
서버 종료는 따로 없음
서버에서 log.txt 파일에 접근할 때 쓰레드 lock 해야하나?
search 할 때 이름도 검색됨 -> 당연한건가?

