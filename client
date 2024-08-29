#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define BUF_SIZE 1000
#define NAME_SIZE 20
using namespace std;

void *send_msg(void *arg);
void *recv_msg(void *arg);
void error_handling(const char *msg);

char name[NAME_SIZE] = "[DEFAULT]";
char msg[BUF_SIZE];

int main(int argc, char *argv[])
{
  int sock;
  struct sockaddr_in serv_addr;
  pthread_t snd_thread, rcv_thread;
  void *thread_return;
  if (argc != 4)
  {
    printf("Usage : %s <IP> <port> <name>\n", argv[0]);
    exit(1);
  }

  sprintf(name, "[%s]", argv[3]);
  sock = socket(PF_INET, SOCK_STREAM, 0);

  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
  serv_addr.sin_port = htons(atoi(argv[2]));

  if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
    error_handling("connect() error");

  pthread_create(&snd_thread, NULL, send_msg, (void *)&sock);
  pthread_join(snd_thread, &thread_return);
  close(sock);
  return 0;
}

void *send_msg(void *arg)
{
  int sock = *((int *)arg);
  char name_msg[NAME_SIZE + BUF_SIZE];
  string ligin[3] = {"ID 입력", "PW 입력", "검색할 도서를 입력하세요"};
  string join[6] = {"회원가입 창입니다.", "아이디를 입력하세요.", "비밀번호를 입력하세요.", "이름을 입력하세요.", "주소를 입력하세요.", "전화번호를 입력하세요."};
  pthread_t snd_thread, rcv_thread;
  void *thread_return;

  while (1)
  {
    cout << "1번 : 로그인, 2번 : 회원가입  3 : 도서 검색 : ";
    fgets(msg, BUF_SIZE, stdin);
    if (!strcmp(msg, "q\n") || !strcmp(msg, "Q\n"))
    {
      close(sock);
      exit(0);
    }
    sprintf(name_msg, "%s %s", name, msg);
    write(sock, msg, strlen(msg));
    if (!strcmp(msg, "1\n"))
    {
      cout << "로그인 화면입니다" << endl;
      for (int i = 0; i < 2; i++)
      {
        memset(msg, 0, BUF_SIZE);
        cout << ligin[i] << ": ";
        cin.getline(msg, BUF_SIZE);

        if (write(sock, msg, strlen(msg)) <= 0)
        {
          cout << "Error" << endl;
          break;
        }
      }
      memset(msg, 0, sizeof(msg));
      int well = read(sock, msg, sizeof(msg));
      std::string wcome(msg);
      std::cout << "환영합니다 " << wcome << " 님" << std::endl;
      const char *select;
      std::cout << "1번 : 도서 추천   2번 : 대여   3번 : 반납" << std::endl;
      int num;
      cin >> num;
      getchar();
      string numString = std::to_string(num);
      if (numString == "1")
      {
        cout << "추천도서\n";
        strcpy(msg, "1\n");
        write(sock, msg, strlen(msg));
        pthread_create(&rcv_thread, NULL, recv_msg, (void *)&sock);
        pthread_join(rcv_thread, &thread_return);
        close(sock);
      }
      else if (numString == "2")
      {
        int choice;
        strcpy(msg, "2\n");
        write(sock, msg, strlen(msg)); // 2번 진입
        cout << "어떤 항목으로 검색하시겠습니까?\n1) 책 이름         2) 책 번호\n";
        cin >> choice;
        getchar();
        if (choice == 1)
        {
          strcpy(msg, "1\n");
          write(sock, msg, strlen(msg)); // 위 항목 '책 이름' 번호
          memset(msg, 0, sizeof(msg));
          cout << "책 이름을 입력하시오: ";
          cin.getline(msg, BUF_SIZE);
          write(sock, msg, strlen(msg)); // 찐 책 이름
          memset(msg, 0, sizeof(msg));

          char name_msg1[NAME_SIZE + BUF_SIZE];
          int str_len1;
          str_len1 = read(sock, name_msg1, NAME_SIZE + BUF_SIZE - 1); // k를 받아옴
          fputs(name_msg1, stdout);
          string kia(name_msg1);
          int tiger = std::stoi(kia); // 받아온 char*인 k를 string으로 바꿔주고, string을 int로 바꿔줌

          for (int k = 0; k < tiger; ++k)
          {
            name_msg1[NAME_SIZE + BUF_SIZE] = '\0';
            str_len1 = read(sock, name_msg1, NAME_SIZE + BUF_SIZE - 1);
            fputs(name_msg1, stdout);
          }
          cout << "책 번호를 입력하시오: " << endl;
          memset(msg, 0, sizeof(msg));
          cin.getline(msg, BUF_SIZE);
          write(sock, msg, strlen(msg)); // 책 번호 입력

          memset(msg, 0, sizeof(msg));
          read(sock, msg, sizeof(msg)); // 책 가능 불가능 1번 2번 구분
          cout << msg << endl;
          if (!strcmp(msg, "A\n"))
          {
            cout << "대여 가능" << endl;
            memset(msg, 0, sizeof(msg));
            cout << "-------------------\n"
                 << endl;
            cout << "대여하시겠습니까?\n1) 예     2)아니오\n"
                 << endl;
            memset(msg, 0, sizeof(msg));
            cin.getline(msg, BUF_SIZE);
            write(sock, msg, strlen(msg)); // 대여번호 선택
            cout << "대여완료" << endl;
          }
          if (!strcmp(msg, "B\n"))
          {
            memset(msg, 0, sizeof(msg));
            read(sock, msg, sizeof(msg)); // 책 대여 불가능
          }

          memset(msg, 0, sizeof(msg));
          read(sock, msg, sizeof(msg)); // 책 대여 여부 가능/ 불가능
        }
        else // 책 번호
        {
          strcpy(msg, "2\n");
          write(sock, msg, strlen(msg)); // 위 항목 '책 번호'의 번호
          cout << "책 번호를 입력하시오: ";
          cin.getline(msg, BUF_SIZE);
          write(sock, msg, strlen(msg)); // 찐 책 번호

          memset(msg, 0, sizeof(msg));
          read(sock, msg, sizeof(msg)); // 책 가능 불가능 1번 2번 구분
          if (!strcmp(msg, "A\n"))
          {
            cout << "대여 상태: 대여중" << endl;
            memset(msg, 0, sizeof(msg));
            cout << "-------------------\n"
                 << endl;
            cout << "반납하시겠습니까?\n1) 예     2)아니오\n"
                 << endl;
            memset(msg, 0, sizeof(msg));
            cin.getline(msg, BUF_SIZE);
            write(sock, msg, strlen(msg)); // 대여번호 선택
            cout << "반납완료" << endl;
          }
          if (!strcmp(msg, "B\n"))
          {
            memset(msg, 0, sizeof(msg));
            read(sock, msg, sizeof(msg)); // 책 대여 불가능
          }
        }
      }
      else if (numString == "3")
      {
        int choice;
        strcpy(msg, "3\n");
        write(sock, msg, strlen(msg)); // 3번 진입
        cout << "어떤 항목으로 검색하시겠습니까?\n1) 책 이름         2) 책 번호\n";
        cin >> choice;
        getchar();
        if (choice == 1)
        {
          strcpy(msg, "1\n");
          write(sock, msg, strlen(msg)); // 위 항목 '책 이름' 번호
          memset(msg, 0, sizeof(msg));
          cout << "책 이름을 입력하시오: ";
          cin.getline(msg, BUF_SIZE);
          write(sock, msg, strlen(msg)); // 찐 책 이름
          memset(msg, 0, sizeof(msg));

          char name_msg1[NAME_SIZE + BUF_SIZE];
          int str_len1;
          str_len1 = read(sock, name_msg1, NAME_SIZE + BUF_SIZE - 1); // k를 받아옴
          string kia(name_msg1);
          int tiger = std::stoi(kia); // 받아온 char*인 k를 string으로 바꿔주고, string을 int로 바꿔줌

          for (int k = 0; k < tiger; ++k)
          {
            name_msg1[NAME_SIZE + BUF_SIZE] = '\0';
            str_len1 = read(sock, name_msg1, NAME_SIZE + BUF_SIZE - 1);
            fputs(name_msg1, stdout);
          }
          cout << "책 번호를 입력하시오: " << endl;
          memset(msg, 0, sizeof(msg));
          cin.getline(msg, BUF_SIZE);
          write(sock, msg, strlen(msg)); // 책 번호 입력

          memset(msg, 0, sizeof(msg));
          read(sock, msg, sizeof(msg)); // 책 가능 불가능 1번 2번 구분
          cout << msg << endl;
          if (!strcmp(msg, "A\n"))
          {
            cout << "대여 상태: 대여중" << endl;
            memset(msg, 0, sizeof(msg));
            cout << "-------------------\n"
                 << endl;
            cout << "반납하시겠습니까?\n1) 예     2)아니오\n"
                 << endl;
            memset(msg, 0, sizeof(msg));
            cin.getline(msg, BUF_SIZE);
            cout << msg << endl;
            write(sock, msg, strlen(msg)); // 대여번호 선택
            cout << "보냄" << endl;
            cout << "반납종료" << endl;
          }
          if (!strcmp(msg, "B\n"))
          {
            memset(msg, 0, sizeof(msg));
            read(sock, msg, sizeof(msg)); // 책 대여 불가능
          }

          memset(msg, 0, sizeof(msg));
          read(sock, msg, sizeof(msg)); // 책 대여 여부 가능/ 불가능
        }
        else
        {
          strcpy(msg, "2\n");
          write(sock, msg, strlen(msg)); // 위 항목 '책 번호'의 번호
          cout << "책 번호를 입력하시오: ";
          cin.getline(msg, BUF_SIZE);
          write(sock, msg, strlen(msg)); // 찐 책 번호

          memset(msg, 0, sizeof(msg));
          read(sock, msg, sizeof(msg)); // 책 가능 불가능 1번 2번 구분
          if (!strcmp(msg, "A\n"))
          {
            cout << "대여 상태: 대여중" << endl;
            memset(msg, 0, sizeof(msg));
            cout << "-------------------\n"
                 << endl;
            cout << "반납하시겠습니까?\n1) 예     2)아니오\n"
                 << endl;
            memset(msg, 0, sizeof(msg));
            cin.getline(msg, BUF_SIZE);
            write(sock, msg, strlen(msg)); // 대여번호 선택
            cout << "반납완료" << endl;
          }
          if (!strcmp(msg, "B\n"))
          {
            memset(msg, 0, sizeof(msg));
            read(sock, msg, sizeof(msg)); // 책 대여 불가능
          }
        }
      }
    }

    else if (!strcmp(msg, "2\n"))
    {
      cout << join[0] << endl;
      for (int i = 1; i < 6; i++)
      {
        memset(msg, 0, BUF_SIZE);
        cout << join[i] << ": ";
        cin.getline(msg, BUF_SIZE);

        if (write(sock, msg, strlen(msg)) <= 0)
        {
          cout << "Error" << endl;
          break;
        }
      }
      memset(msg, 0, sizeof(msg));
      char well = read(sock, msg, sizeof(msg));
      if (!strcmp(msg, "1"))
      {
        cout << "중복 아이디입니다 다시 입력해주세요" << endl;
        for (int i = 1; i < 6; i++)
        {
          memset(msg, 0, BUF_SIZE);
          cout << join[i] << ": ";
          cin.getline(msg, BUF_SIZE);

          if (write(sock, msg, strlen(msg)) <= 0)
          {
            cout << "Error" << endl;
            break;
          }
        }
      }
    }

    if (!strcmp(msg, "3\n"))
    {
      char mmsg[500];
      cout << "도서 검색 화면입니다" << endl;
      {
        memset(msg, 0, BUF_SIZE);
        memset(mmsg, 0, BUF_SIZE);
        cout << ligin[2] << ": ";
        int str_len = 0;
        std::string a = " ";
        cin.getline(msg, BUF_SIZE);
        write(sock, msg, sizeof(msg) - 2);
        while (1)
        {
          str_len = read(sock, mmsg, sizeof(mmsg));
          if (str_len == 0)
            break;
          fputs(mmsg, stdout);
          memset(mmsg, 0, BUF_SIZE);
        }
        std::cout << "chek03" << endl;
        std::string test(msg);
      }
    }
  }

  return NULL;
}

void *recv_msg(void *arg) // read thread main
{
  int sock = *((int *)arg);
  char name_msg[NAME_SIZE + BUF_SIZE];
  int str_len;
  while (1)
  {
    str_len = read(sock, name_msg, NAME_SIZE + BUF_SIZE - 1);
    if (str_len == -1)
      return (void *)-1;
    name_msg[str_len] = 0;
    fputs(name_msg, stdout);
  }
  return NULL;
}

void error_handling(const char *msg)
{
  fputs(msg, stderr);
  fputc('\n', stderr);
  exit(1);
}
