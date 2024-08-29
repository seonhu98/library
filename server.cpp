#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <mariadb/conncpp.hpp>

#define BUF_SIZE 100
#define MAX_CLNT 256
using namespace std;

void *handle_clnt(void *arg);
void send_msg(const char *msg, int len);
void error_handling(const char *msg);
void addUser(std::unique_ptr<sql::Connection> &conn, std::string name, std::string state, std::string ID, std::string PW, std::string phone, std::string times);
int Userlogin(std::unique_ptr<sql::Connection> &conn, string user_id, string user_pw);
void recomm_book(int clnt_sock, std::unique_ptr<sql::Connection> &conn);
void showBook(std::unique_ptr<sql::Connection> &conn);
void book_rent(std::unique_ptr<sql::Connection> &conn);
void Userjoin(std::unique_ptr<sql::Connection> &conn, string id, string pw, string name, string address, string phonenumm, int clnt_sock);
void book_return(std::unique_ptr<sql::Connection> &conn, string user_id, int clnt_sock);

int clnt_cnt = 0;
int clnt_socks[MAX_CLNT];
pthread_mutex_t mutx;

int main(int argc, char *argv[])
{
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_adr, clnt_adr;
    socklen_t clnt_adr_sz;
    pthread_t t_id;
    if (argc != 2)
    {
        std::cout << "Usage : " << argv[0] << "<port>\n";
        exit(1);
    }

    pthread_mutex_init(&mutx, NULL);
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
        error_handling("bind() error");
    if (listen(serv_sock, 5) == -1)
        error_handling("listen() error");

    while (1)
    {
        clnt_adr_sz = sizeof(clnt_adr);
        clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_adr, &clnt_adr_sz);

        pthread_mutex_lock(&mutx);
        clnt_socks[clnt_cnt++] = clnt_sock;
        pthread_mutex_unlock(&mutx);

        pthread_create(&t_id, NULL, handle_clnt, (void *)&clnt_sock);
        pthread_detach(t_id);
        std::cout << ("Connected client IP: %s \n", inet_ntoa(clnt_adr.sin_addr));
    }
    close(serv_sock);
    return 0;
}

void send_msg(int clnt_sock, const char *msg, int len)
{
    int i;
    pthread_mutex_lock(&mutx);
    write(clnt_sock, msg, len);
    pthread_mutex_unlock(&mutx);
}

void showBook(int clnt_sock, std::unique_ptr<sql::Connection> &conn) // 도서 검색
{
    try
    {
        char mmsg[100];
        std::cout << mmsg;
        std::string search_book = " ";
        string set;
        string blank = " \n";
        string smasg;
        int nnum;
        read(clnt_sock, mmsg, sizeof(mmsg));
        search_book = mmsg;

        std::unique_ptr<sql::Statement> stmnt(conn->createStatement());
        std::cout << mmsg << endl;
        sql::ResultSet *res = stmnt->executeQuery("SELECT * FROM BOOK WHERE BOOK_NAME LIKE '%" + search_book + "%';");
        if (mmsg == search_book)
        {
            while (res->next())
            {
                std::cout << "책번호 : " << res->getInt(1) << std::endl;
                std::cout << "책이름 : " << res->getString(5) << std::endl;
                std::cout << "작가 : " << res->getString(6) << std::endl;
                std::cout << "출판사 :  " << res->getString(7) << std::endl;
                std::cout << "현재 보유량 : " << res->getInt(11) << std::endl;

                smasg = "책번호 : " + res->getString(1) + blank;
                smasg += "책이름 : " + res->getString(5) + blank;
                smasg += "작가 : " + res->getString(6) + blank;
                smasg += "출판사 :  " + res->getString(7) + blank;
                smasg += "현재 보유량 : " + res->getString(11) + "\n" + "\n";

                const char *msg = smasg.c_str();
                int count = 0;
                int wcount = 0;
                while (count < smasg.length())
                {
                    count += write(clnt_sock, msg, strlen(msg));
                    wcount++;
                }
            }
        }
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "찾는 도서가 없습니다" << e.what() << std::endl;
    }
}

void error_handling(const char *msg)
{
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}

void addUser(std::unique_ptr<sql::Connection> &conn, std::string name, std::string state, std::string ID, std::string PW, std::string phone, std::string times)
{
    try
    {
        std::unique_ptr<sql::PreparedStatement> stmnt(conn->prepareStatement("insert into USER values (default, ?, ?, ?, ?, ?, ?, DATE_ADD(?, INTERVAL 6 DAY), '0', '0', 'B')"));
        stmnt->setString(1, name);
        stmnt->setString(2, state);
        stmnt->setString(3, ID);
        stmnt->setString(4, PW);
        stmnt->setString(5, phone);
        stmnt->setString(6, times);
        stmnt->setString(7, times);
        stmnt->executeQuery();
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "Error inserting new task: " << e.what() << std::endl;
    }
}

void Userjoin(std::unique_ptr<sql::Connection> &conn, string id, string pw, string name, string address, string phonenum)
{
    time_t now = time(0);
    tm *settime = localtime(&now);
    char times[20];
    strftime(times, sizeof(times), "%Y/%m/%d %H:%M:%S", settime);
    addUser(conn, name, address, id, pw, phonenum, times);
}

void Userlogin(std::unique_ptr<sql::Connection> &conn, string user_id, string user_pw, int clnt_sock)
{
    try
    {
        std::unique_ptr<sql::Statement> stmnt(conn->createStatement());
        sql::ResultSet *res = stmnt->executeQuery("select * from USER");
        while (res->next())
        {
            if (res->getString(4) == user_id && res->getString(5) == user_pw)
            {
                string set;
                set = res->getString(2);
                const char *showname = set.c_str();
                write(clnt_sock, showname, strlen(showname));
                std::cout << set << " 님이 접속하셨습니다." << endl;
            }
        }
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "Error inserting new task: " << e.what() << std::endl;
    }
}

void recomm_book(std::unique_ptr<sql::Connection> &conn, int clnt_sock) // 추천도서
{
    int randnum;
    srand(time(NULL));
    int randlist[30];
    try
    {
        std::unique_ptr<sql::Statement> stmnt(conn->createStatement());
        sql::ResultSet *res = stmnt->executeQuery("select * from BOOK");
        while (res->next())
        {
            for (int count = 0; count < 30; count++)
            {
                randnum = random() % 76509 + 1;
                randlist[count] = randnum;
                if (res->getInt(1) == randlist[count])
                {
                    string book;
                    book = "도서명: " + res->getString(5) + "\n";
                    const char *recomm_book = book.c_str();
                    write(clnt_sock, recomm_book, strlen(recomm_book));
                    string author;
                    author = "지은이: " + res->getString(6) + "\n\n";
                    const char *recomm_author = author.c_str();
                    write(clnt_sock, recomm_author, strlen(recomm_author));
                }
            }
        }
        std::cout << std::endl;
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "Error select task: " << e.what() << std::endl;
    }
}

void book_rent(std::unique_ptr<sql::Connection> &conn, string user_id, int clnt_sock)
{
    char msg[BUF_SIZE];
    string rent_user;
    time_t now = time(0);
    tm *settime = localtime(&now);
    char times[20];
    strftime(times, sizeof(times), "%Y%m%d", settime);
    string rent_date = times;
    try
    {
        int k = 0;
        int num;
        string book_name;
        string book_no;
        string query;
        string query2;

        read(clnt_sock, msg, sizeof(msg));
        string set_num(msg);
        num = std::stoi(set_num);

        cout << "들어가기 전" << endl;
        if (!strcmp(msg, "1\n"))
        {
            cout << "1번 들어옴" << endl;

            read(clnt_sock, msg, sizeof(msg)); // 책 이름
            cout << msg << endl;
            book_name = msg;
            memset(msg, 0, sizeof(msg));
            string save_book = book_name;
            query = "SELECT BOOK_NO, BOOK_NAME, POS FROM BOOK WHERE BOOK_NAME = '" + book_name + "';";
            sql::Statement *stmnt(conn->createStatement());
            sql::ResultSet *res6 = stmnt->executeQuery(query);
            int k = 0;
            while (res6->next())
            {
                ++k;
            }
            string book_cnt = to_string(k);
            const char *book_count = book_cnt.c_str();
            write(clnt_sock, book_count, strlen(book_count)); // 제발 되라

            query = "SELECT BOOK_NO, BOOK_NAME, POS FROM BOOK WHERE BOOK_NAME = '" + save_book + "';";
            sql::Statement *stmnt2(conn->createStatement());
            sql::ResultSet *res7 = stmnt2->executeQuery(query);
            int h = 0;
            while (res7->next())
            {
                cout << "책 번호: " << res7->getInt(1) << endl;
                book_no = res7->getString(1) + "\n";
                const char *showno = book_no.c_str();
                write(clnt_sock, showno, strlen(showno));
                h++;
                cout << "k: " << k << endl;
                cout << "h: " << h << endl;

                if (h == k)
                {
                    cout << "안녕" << endl;
                    read(clnt_sock, msg, sizeof(msg)); // 책 번호 입력받음
                    cout << "받은 책 번호 " << msg << endl;
                    msg[strcspn(msg, "\n")] = '\0';
                    string kia(msg);
                    query = "SELECT BOOK_NO, BOOK_NAME, POS FROM BOOK WHERE BOOK_NO = " + kia + ";";
                }
            }
        }
        if (!strcmp(msg, "2\n"))
        {
            read(clnt_sock, msg, sizeof(msg)); // 책 번호
            cout << msg << endl;
            book_no = msg;
            memset(msg, 0, sizeof(msg));
            string save_bo = book_no;
            query = "SELECT BOOK_NO, BOOK_NAME, POS FROM BOOK WHERE BOOK_NO = '" + book_no + "';";
            sql::Statement *stmnt(conn->createStatement());
            sql::ResultSet *res6 = stmnt->executeQuery(query);
        }
        sql::Statement *stmnt(conn->createStatement());
        sql::ResultSet *res = stmnt->executeQuery(query);
        while (res->next())
        {
            if (res->getInt(3) == 1)
            {
                memset(msg, 0, sizeof(msg));
                strcpy(msg, "A\n"); // 가능 구분
                write(clnt_sock, msg, strlen(msg));
                if (msg == "1\n")
                {
                    cout << "if문 진입" << endl;
                    cout << num << endl;
                    if (num == 1)
                    {
                        query = "UPDATE BOOK SET POS = 0 WHERE BOOK_NAME = '" + book_name + "';";
                    }
                    else
                    {
                        query = "UPDATE BOOK SET POS = 0 WHERE BOOK_NO = " + book_no + ";";
                    }
                    sql::Statement *stmnt(conn->createStatement());
                    sql::ResultSet *res1 = stmnt->executeQuery(query);
                    cout << "대여완료" << endl;

                    std::unique_ptr<sql::Statement> stmnt2(conn->createStatement());
                    sql::ResultSet *res2 = stmnt2->executeQuery("select * from USER");
                    while (res2->next())
                    {
                        if (res2->getString(4) == user_id)
                        {
                            rent_user = res2->getString(2);
                            cout << rent_user << " 님이 빌림" << endl;
                            cout << endl;
                            if (res2->getString(11) == "A")
                            {
                                std::unique_ptr<sql::PreparedStatement> stmnt(conn->prepareStatement("insert into HISTORY values (default, ?, ?, ?, DATE_ADD(?, INTERVAL 7 DAY), 0, DATE_ADD(?, INTERVAL 21 DAY), 0)"));
                                stmnt->setString(1, rent_date);
                                stmnt->setString(2, rent_user);
                                stmnt->setString(3, book_name);
                                stmnt->setString(4, rent_date);
                                stmnt->setString(5, rent_date);
                                stmnt->executeQuery();
                            }
                            if (res2->getString(11) == "B")
                            {
                                std::unique_ptr<sql::PreparedStatement> stmnt(conn->prepareStatement("insert into HISTORY values (default, ?, ?, ?, 0, DATE_ADD(?, INTERVAL 5 DAY), 0, DATE_ADD(?, INTERVAL 19 DAY))"));
                                stmnt->setString(1, rent_date);
                                stmnt->setString(2, rent_user);
                                stmnt->setString(3, book_name);
                                stmnt->setString(4, rent_date);
                                stmnt->setString(5, rent_date);
                                stmnt->executeQuery();
                            }
                        }
                    }
                }
                else
                {

                    break;
                }
            }
            else
            {
                memset(msg, 0, sizeof(msg));
                strcpy(msg, "B\n"); // 가능 구분
                write(clnt_sock, msg, strlen(msg));

                cout << "대여 가능 여부: 불가능\n";
                memset(msg, 0, sizeof(msg));
                strcpy(msg, "대여 불가\n");
                write(clnt_sock, msg, strlen(msg));
                cout << "-------------------\n\n";
                break;
            }
        }
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "Error selecting tasks: " << e.what() << std::endl;
    }
}

void GradeUpdate(std::unique_ptr<sql::Connection> &conn, string user_id)
{
    time_t now = time(0);
    tm *settime = localtime(&now);
    char times[20];
    strftime(times, sizeof(times), "%Y%m%d", settime);
    string user_name;
    string six_month_date;
    string now_date = times;    // 현재날짜
    string last_date;           // LAST_A , LAST_B
    string rebook_date = times; // 반납일
    string black_date;          // 블랙 풀리는 날
    int rental;                 // 반납 완료된 책 개수
    int rebook;                 // 연체됐던 책 개수
    try
    {
        std::unique_ptr<sql::Statement> stmnt(conn->createStatement());
        std::unique_ptr<sql::Statement> stmnt1(conn->createStatement());
        std::unique_ptr<sql::Statement> stmnt2(conn->createStatement());
        std::unique_ptr<sql::Statement> stmnt3(conn->createStatement());
        std::unique_ptr<sql::Statement> stmntb(conn->createStatement());

        sql::ResultSet *res = stmnt->executeQuery("select * from USER");
        sql::ResultSet *res1 = stmnt1->executeQuery("select date_format(LAST_A, '%Y%m%d') as LAST_A from HISTORY");
        sql::ResultSet *res2 = stmnt2->executeQuery("select date_format(LAST_B, '%Y%m%d') as LAST_B from HISTORY");
        while (res->next())
        {
            if (res->getString(4) == user_id)
            {
                user_name = res->getString(2);
                rental = res->getInt(9);
                rebook = res->getInt(10);
                sql::ResultSet *res3 = stmnt3->executeQuery("select date_format(SIX_MONTH_DATE, '%Y%m%d') as SIX_MONTH_DATE from USER where USER_ID = '" + user_id + "';");
                sql::ResultSet *resb = stmntb->executeQuery("select date_format(BLACK_DATE, '%Y%m%d') as BLACK_DATE from BLACK_USER where C_ID = '" + user_id + "';");

                if (res->getString(11) == "A")
                {
                    if (res1->next())
                    {
                        last_date = res1->getString(1);
                    }
                }
                if (res->getString(11) == "B")
                {
                    if (res3->next())
                    {
                        six_month_date = res3->getString(1);
                    }
                    if (res2->next())
                    {
                        last_date = res2->getString(1);
                        if (stoi(six_month_date) <= stoi(now_date) && (rental - rebook) >= 10)
                        {

                            std::unique_ptr<sql::PreparedStatement> stmnt(conn->prepareStatement("update USER set GRADE = ? where USER_ID = ?"));
                            stmnt->setString(1, "A");
                            stmnt->setString(2, user_id);
                            stmnt->executeUpdate();
                        }
                        if ((stoi(now_date) - stoi(last_date)) >= 14 && (rebook - rental) >= 3)
                        {
                            std::unique_ptr<sql::PreparedStatement> stmnt(conn->prepareStatement("update USER set GRADE = ? where USER_ID = ?"));
                            stmnt->setString(1, "C");
                            stmnt->setString(2, user_id);
                            stmnt->executeUpdate();

                            std::unique_ptr<sql::PreparedStatement> abcd(conn->prepareStatement("insert into BLACK_USER values (default, ?, ?, ?, DATE_ADD(?, INTERVAL 30 DAY))"));
                            abcd->setString(1, user_name);
                            abcd->setString(2, user_id);
                            abcd->setString(3, rebook_date);
                            abcd->setString(4, rebook_date);
                            abcd->executeUpdate();
                        }
                    }
                }
                if (res->getString(11) == "C")
                {
                    if (resb->next())
                    {
                        black_date = resb->getString(1);
                        if (stoi(black_date) <= stoi(now_date))
                        {
                            std::unique_ptr<sql::PreparedStatement> stmnt(conn->prepareStatement("update USER set GRADE = ?,RENTAL = ?, REBOOK = ? where USER_ID = ?"));
                            stmnt->setString(1, "B");
                            stmnt->setString(2, "0");
                            stmnt->setString(3, "0");
                            stmnt->setString(4, user_id);
                            stmnt->executeUpdate();

                            std::unique_ptr<sql::PreparedStatement> abcd(conn->prepareStatement("delete from BLACK_USER where C_ID = ?"));
                            abcd->setString(1, user_id);
                            abcd->executeUpdate();

                            std::unique_ptr<sql::PreparedStatement> setblack1(conn->prepareStatement("set @count=0"));
                            std::unique_ptr<sql::PreparedStatement> setblack2(conn->prepareStatement("set C_NO=@count:=@count+1"));
                            std::unique_ptr<sql::PreparedStatement> setblack3(conn->prepareStatement("alter table BLACO_USER auto_increment = 1"));
                            setblack1->executeUpdate();
                            setblack2->executeUpdate();
                            setblack3->executeUpdate();
                        }
                    }
                }
            }
        }
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "Error selecting tasks: " << e.what() << std::endl;
    }
}

void book_return(std::unique_ptr<sql::Connection> &conn, string user_id, int clnt_sock)
{
    char msg[BUF_SIZE];
    string rent_user;
    time_t now = time(0);
    tm *settime = localtime(&now);
    char times[20];
    strftime(times, sizeof(times), "%Y%m%d", settime);
    string rent_date = times;
    try
    {
        int k = 0;
        int num;
        string book_name;
        string book_no;
        string query;
        string query2;
        read(clnt_sock, msg, sizeof(msg));
        string set_num(msg);
        num = std::stoi(set_num);

        if (!strcmp(msg, "1\n"))
        {
            read(clnt_sock, msg, sizeof(msg)); // 책 이름
            cout << msg << endl;
            book_name = msg;
            memset(msg, 0, sizeof(msg));
            string save_book = book_name;
            query = "SELECT BOOK_NO, BOOK_NAME, POS FROM BOOK WHERE BOOK_NAME = '" + book_name + "';";
            sql::Statement *stmnt(conn->createStatement());
            sql::ResultSet *res6 = stmnt->executeQuery(query);
            int k = 0;
            while (res6->next())
            {
                ++k;
            }
            string book_cnt = to_string(k);
            const char *book_count = book_cnt.c_str();
            write(clnt_sock, book_count, strlen(book_count)); // 제발 되라

            query = "SELECT BOOK_NO, BOOK_NAME, POS FROM BOOK WHERE BOOK_NAME = '" + save_book + "';";
            sql::Statement *stmnt2(conn->createStatement());
            sql::ResultSet *res7 = stmnt2->executeQuery(query);
            int h = 0;

            while (res7->next())
            {
                cout << "책 번호: " << res7->getInt(1) << endl;
                book_no = res7->getString(1) + "\n";
                const char *showno = book_no.c_str();
                write(clnt_sock, showno, strlen(showno));

                h++;
                cout << "k: " << k << endl;
                cout << "h: " << h << endl;
                if (h == k)
                {
                    cout << "안녕" << endl;
                    read(clnt_sock, msg, sizeof(msg)); // 책 번호 입력받음
                    cout << "받은 책 번호 " << msg << endl;
                    msg[strcspn(msg, "\n")] = '\0';
                    string kia(msg);
                    query = "SELECT BOOK_NO, BOOK_NAME, POS FROM BOOK WHERE BOOK_NO = " + kia + ";";
                }
            }
        }
        if (!strcmp(msg, "2\n"))
        {
            read(clnt_sock, msg, sizeof(msg)); // 책 번호
            cout << msg << endl;
            book_no = msg;
            memset(msg, 0, sizeof(msg));
            string save_bo = book_no;
            query = "SELECT BOOK_NO, BOOK_NAME, POS FROM BOOK WHERE BOOK_NO = '" + book_no + "';";
            sql::Statement *stmnt(conn->createStatement());
            sql::ResultSet *res6 = stmnt->executeQuery(query);
        }
        sql::Statement *stmnt(conn->createStatement());
        sql::ResultSet *res = stmnt->executeQuery(query);
        while (res->next())
        {
            if (res->getInt(3) == 0)
            {
                cout << "들어오냐" << endl;
                memset(msg, 0, sizeof(msg));
                strcpy(msg, "A\n"); // 가능 구분
                write(clnt_sock, msg, strlen(msg));
                if (msg == "1\n")
                {
                    cout << "if문 진입" << endl;
                    query = "UPDATE BOOK SET POS = 1 WHERE BOOK_NO = " + book_no + ";";

                    sql::Statement *stmnt(conn->createStatement());
                    sql::ResultSet *res1 = stmnt->executeQuery(query);
                    cout << "반납완료" << endl;
                }
                else
                {
                    break;
                }
            }
            else
            {
                memset(msg, 0, sizeof(msg));
                strcpy(msg, "B\n"); // 가능 구분
                write(clnt_sock, msg, strlen(msg));

                cout << "대여 상태: 반납 완료\n";
                memset(msg, 0, sizeof(msg));
                strcpy(msg, "반납 완료\n");
                write(clnt_sock, msg, strlen(msg));
                cout << "-------------------\n\n";
                break;
            }
        }
        GradeUpdate(conn, user_id);
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "Error selecting tasks: " << e.what() << std::endl;
    }
}

void *handle_clnt(void *arg)
{
    int clnt_sock = *((int *)arg);
    int str_len = 0, i;
    char msg[BUF_SIZE];
    string user_id;
    string user_pw;
    int flag = 0;
    const char *q_msg;
    string name, address, id, pw, phonenum;

    sql::Driver *driver = sql::mariadb::get_driver_instance();
    sql::SQLString url("jdbc:mariadb://10.10.21.227:3306/LIB");
    sql::Properties properties({{"user", "LIB"}, {"password", "1"}});
    std::unique_ptr<sql::Connection> conn(driver->connect(url, properties));

    while ((str_len = read(clnt_sock, msg, sizeof(msg) - 2)) != 0)
    {
        std::cout << msg;
        if (!strcmp(msg, "1\n"))
        {
            int ID_read = read(clnt_sock, msg, sizeof(msg)); // 아이디 READ
            std::string user_id(msg);

            int PW_read = read(clnt_sock, msg, sizeof(msg)); // 패스워드 READ
            std::string user_pw(msg);
            memset(msg, 0, BUF_SIZE);

            Userlogin(conn, user_id, user_pw, clnt_sock);
            int recomend = read(clnt_sock, msg, sizeof(msg));
            if (!strcmp(msg, "1\n"))
            {
                recomm_book(conn, clnt_sock);
            }
            else if (!strcmp(msg, "2\n"))
            {
                book_rent(conn, user_id, clnt_sock);
            }
        }

        if (!strcmp(msg, "2\n"))
        {
            cout << "2번 선택" << endl;
            for (int i = 0; i < 5; i++)
            {
                str_len = read(clnt_sock, msg, sizeof(msg));
                msg[str_len] = '\0';

                if (i == 0)
                {
                    id = string(msg);
                }
                else if (i == 1)
                {
                    pw = string(msg);
                }
                else if (i == 2)
                {
                    name = string(msg);
                }
                else if (i == 3)
                {
                    address = string(msg);
                }
                else if (i == 4)
                {
                    phonenum = string(msg);
                }
            }

            std::unique_ptr<sql::Statement> stmnt(conn->createStatement());
            sql::ResultSet *res = stmnt->executeQuery("select * from USER");

            while (res->next())
            {
                if (id == res->getString(4))
                {
                    string check;
                    check = "1";
                    const char *reid = check.c_str();
                    write(clnt_sock, reid, strlen(reid));

                    for (int i = 0; i < 5; i++)
                    {
                        str_len = read(clnt_sock, msg, sizeof(msg));
                        msg[str_len] = '\0';

                        if (i == 0)
                        {
                            id = string(msg);
                        }
                        else if (i == 1)
                        {
                            pw = string(msg);
                        }
                        else if (i == 2)
                        {
                            name = string(msg);
                        }
                        else if (i == 3)
                        {
                            address = string(msg);
                        }
                        else if (i == 4)
                        {
                            phonenum = string(msg);
                        }
                    }
                }
            }
            Userjoin(conn, id, pw, name, address, phonenum);
        }
        if (!strcmp(msg, "3\n"))
        {
            showBook(clnt_sock, conn);
        }
        memset(msg, 0, sizeof(msg));
    }

    pthread_mutex_lock(&mutx);
    for (i = 0; i < clnt_cnt; i++)
    {
        if (clnt_sock == clnt_socks[i])
        {
            while (i++ < clnt_cnt - 1)
                clnt_socks[i] = clnt_socks[i + 1];
            break;
        }
    }
    clnt_cnt--;

    pthread_mutex_unlock(&mutx);

    close(clnt_sock);
    return NULL;
}
