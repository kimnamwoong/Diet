#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sstream>
#include <thread>
#include <vector>
#include <list>
#include <time.h>
#include "/usr/include/mysql/mysql.h"

#define DB_IP "10.10.21.128"
#define DB_USER "kim"
#define DB_PASS "1234"
#define DB_NAME "Health"

#define LISTEN_Q 15
#define PORT 4005
#define EPOLL_SIZE 1024
#define MAXBUFF 1024

using namespace std;
/* 
클라이언트와 송수신할때 구분하는 코드 
login : login
id_check : 아이디체크
sign_up : 회원가입
food_info : 음식조회
food_add : 음식추가 (생각중)
adjust_info : 내 정보 수정 (몸무게,키)
*/

string mytime();                          // 현재 시간을 구하는 함수 
vector<string> recvdata_split(char *cpp) //클라이언트로부터 받은 데이터(char형)를 | 기준으로 잘라서 벡터에 저장
{
    stringstream ss;
    string temp(cpp);
    
    ss.str(temp);
    string one;
    vector<string> input;
    while (getline(ss,one,'|'))
    {
        input.push_back(one);
    }
    return input;
}

class Server
{
public:
    Server();
    void start();
    void Select_foodinfo(string , double *);

private:
    void init();
    void db_init();
    void close_server();
    int client_number(int);
    int send_msg(int clientfd);
    const void add_fd(int, int, bool);

    struct sockaddr_in server_addr;
    int listener;               // server socket
    int epoll_fd;               
    list<int> client_list;      //save clientfd

    MYSQL conn;
    MYSQL *connection = NULL; // Mysql Handle.  MYSQL *connection = NULL, conn; 으로 해도 같은 결과!
    MYSQL_RES *sql_result;    // Query 결과를 담는 구조체 포인터
    MYSQL_ROW sql_row;        // Query 결과로 나온 행 정보를 담는 구조체
    int query_state;           // Query 성공 여부를 받는 변수
    char query[255];          // Query 문자열 변수
};

Server::Server()
{
    // 서버 주소 정보 초기화 
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family    = AF_INET;
    server_addr.sin_port        = htons(PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);    // INADDR_ANY : 사용가능한 랜카드 IP 사용
    
    //서버fd 및 epollfd초기화
    listener = 0;
    epoll_fd = 0;
    
    mysql_init(&conn);  // mysql connect 초기화
}
void Server::db_init()
{
    connection = mysql_real_connect(&conn,DB_IP, DB_USER, DB_PASS, DB_NAME, 3306, (char *)NULL, 0);  // db connect
    // 연결에 실패했을때
    if (connection == NULL){
        fprintf(stderr, "Mysql connection error : %s", mysql_error(&conn));
        return;
    }
    cout << "DB connected" << endl; 
}

void Server::init()
{
    // 소켓 fd 생성 
    listener = socket(PF_INET,SOCK_STREAM,0);
    cout << "Server Connect....." << endl;
    if(listener == -1)
    {
        // socket error!! 
        perror("socket Error()\n"); 
        exit(-1);
    }
    int nServSockopt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &nServSockopt, sizeof(nServSockopt));
    if( bind(listener, (struct sockaddr*)&server_addr, sizeof(server_addr) ) < 0 )
    {
        // bind error!
        perror("bind Error()\n");
        exit(-1);
    }
    int ret = listen( listener, 5 );
    if( ret < 0 )
    {
        // listen error!
        perror("Listen Error");
        exit(-1);
    }
    // epoll fd 생성 
    epoll_fd = epoll_create(EPOLL_SIZE);
    if (epoll_fd < 0)
    {
        // epoll create  error
        perror("epfd Error");
        exit(-1);
    }
    
    add_fd(epoll_fd, listener, true);
}

// epollfd 새 fd 등록
// enable_et 인자는 Edge Trigger 모드 활성화 여부. True : 활성화 / False : Level Trigger
const void Server::add_fd(int epoll_fd, int listener, bool enable_et)
{
    struct epoll_event ev;

    // 입력한 인자를 epoll_event 구조체에 기입 후 epoll 에 추가
    ev.data.fd = listener;
    ev.events = EPOLLIN;

    if(enable_et)
    {
        ev.events = EPOLLIN | EPOLLET;  // EPOLLIN : 수신할 데이터가 존재하는 상황, EPOLLET : 이벤트의 감지를 엣지 트리거 방식으로 동작(epoll_ctl에만 사용)
    }

    epoll_ctl( epoll_fd, EPOLL_CTL_ADD, listener, &ev );

    // 소켓 비 차단 모드 설정
    // 소켓 I / O 완료 여부 관계 없이 즉시 반환 / 함수가 있는 쓰레드는 계속 실행
    fcntl( listener, F_SETFL, fcntl( listener, F_GETFD, 0 ) | O_NONBLOCK );
    
}
void Server::start()
{
    static struct epoll_event events[EPOLL_SIZE];
    db_init();
    init();

    while(true)
    { 
        int epoll_event_count = epoll_wait(epoll_fd, events, EPOLL_SIZE, -1);
        if (epoll_event_count < 0)
        {
            perror("epoll failed ");
            break;
        }
        for (int i = 0; i < epoll_event_count; ++i)
        {
            int sockfd = events[i].data.fd;
            if (sockfd == listener) // client 연결 처리 
            {
                struct sockaddr_in client_address;
                socklen_t client_addrLength = sizeof(struct sockaddr_in);

                int clientfd = accept(listener, (struct sockaddr *)&client_address, &client_addrLength);

                client_list.push_back(clientfd);

                int client_num = client_number(clientfd);

                cout << "클라이언트 연결 IP : " << inet_ntoa(client_address.sin_addr) << ", clientfd = " << clientfd << ", client No. " << client_num << endl;

                add_fd(epoll_fd, clientfd, true);

                cout << "New clientfd added " << clientfd << " to epoll " << endl;
                cout << "현재 " << client_list.size() << "명 연결되었습니다! " << endl << endl;

                client_num++;
            }
            else
            {
                int ret = send_msg(sockfd);
                if (ret < 0)
                {
                    perror("Error()");
                    close_server();
                    exit(-1);
                }
            }
        }
    }
    close_server();
}
// 실제 클라이언트와 데이터 송수신 하는부분
int Server::send_msg(int clientfd)
{
    char recv_buff[1024];
    char send_buff[1024];
    memset(recv_buff, 0, sizeof(recv_buff));
    memset(send_buff, 0, sizeof(send_buff));

    int clnt_num = client_number(clientfd);

    cout << "Receive from (client_num: " << clnt_num << ", socket_num:" << clientfd << " )" << endl;
    int len = recv(clientfd, recv_buff, sizeof(recv_buff), 0);
    if (len == 0)
    {
        cout << "client disconnected " << clientfd << endl;

        shutdown(clientfd, SHUT_RDWR);
        close(clientfd);

        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, clientfd, NULL);
        client_list.remove(clientfd);

        clnt_num--;
        return 1;
    }
    cout << "recv_buff : " << recv_buff << endl;
    
    vector<string> recv_tok;    
    recv_tok = recvdata_split(recv_buff);                                                                   // 클라이언트로부터 온 데이터를 쪼개서 담을 벡터 
    cout << recv_tok[0] << endl;
    if(recv_tok[0]== "login")                                                                               //로그인을 할 때
    {
        sprintf(query,"SELECT m_id, m_pw FROM Health.member_info WHERE m_id = '%s' AND m_pw = '%s'",recv_tok[1].c_str(),recv_tok[2].c_str());
        
        query_state = mysql_query(connection,query);
        if(query_state !=0)     //쿼리오류일 때
        {
            fprintf(stderr, "Mysql query error: %s", mysql_error(&conn));
            exit(1);
        }
        sql_result = mysql_store_result( connection );                                                      // 데이터 받아오기
        int rows = mysql_num_rows( sql_result );                                                            // 테이블 로우 수
        cout << "row: " << rows << endl;                                                                    //클라이언트가 입력한 id와 패스워드가 db에 있으면 1 없으면 0
        int login_rows = rows;                                                                              // member_info 테이블에서 나온 rows 저장 
        
        if (rows==1)                                                                                         // 로그인 성공시
        {
            sprintf(query, "SELECT f_name,f_kcal,f_carbohydrate,f_protein,f_fat,f_salt FROM food_info");     //음식정보 가져오기
            query_state = mysql_query(connection,query);
            stringstream ss;
            if(query_state !=0)     //쿼리오류일 때
            {
                fprintf(stderr, "Mysql query error: %s", mysql_error(&conn));
                exit(1);
            }
            sql_result = mysql_store_result( connection );      
            int rows = mysql_num_rows( sql_result );        
            int column = mysql_num_fields(sql_result);      
            char send_Row[1024] = {};                                                                       // 클라이언트로 send할 문자열 선언 
            string temp;
            temp = "food_info_all|" + to_string(login_rows) + "|";
            strcat(send_Row, temp.c_str());                                                                 // 맨앞에 붙여줄 코드명 추가 
            while((sql_row = mysql_fetch_row(sql_result)) != NULL)                                         // 결과 send_Row에 저장
            {
                for(int i=0; i<column; i++)
                {          
                    strcat(send_Row, sql_row[i]);
                    strcat(send_Row, "|");
                }
                cout << endl;
            }
            cout << "send_Row: " << send_Row << endl;
            send(clientfd, send_Row, sizeof(send_Row), 0);                                              
            cout << send_buff << endl;
            mysql_free_result(sql_result);                                                              // 결과 비우기
        }
        else                                                                                            // 로그인 실패시 
        {
            string login_result = recv_tok[0]+ "|" + to_string(rows) + "|";
            memset(send_buff,0,sizeof(send_buff));
            strcpy(send_buff,login_result.c_str());
            if (send(clientfd,send_buff,sizeof(send_buff),0) <0 )
            {
                cout << "send_error";
                return -1;
            }
            cout << send_buff << endl;
        }
    }   
    else if(recv_tok[0]== "sign_up")                                                                    //회원가입을 할 때
    {   
        double day_kcal;
        if (recv_tok[5]=="남")
        {
            day_kcal = (stof(recv_tok[6])/100) * (stof(recv_tok[6])/100) * 22 * 35;      //1일 권장칼로리 = 키(m) x 키(m) x (남:22,여:21) x 35
        }
        else if (recv_tok[5]=="여")
        {
            day_kcal = (stof(recv_tok[6])/100) * (stof(recv_tok[6])/100) * 21 * 35;      //1일 권장칼로리 = 키(m) x 키(m) x (남:22,여:21) x 35
        }

        double BMI = stof(recv_tok[7]) / (stof(recv_tok[6])/100 * stof(recv_tok[6])/100);       //BMI(kg/m^2) = 몸무게 / 키의제곱
        sprintf(query,"INSERT INTO Health.member_info(m_name,m_id,m_pw,m_age,m_gender,m_height,m_weight,m_day_kcal,BMI) VALUES('%s','%s','%s',%d,'%s',%d,%d,%.2lf,%.2lf)",recv_tok[1].c_str(),recv_tok[2].c_str(),recv_tok[3].c_str(),atoi(recv_tok[4].c_str()),recv_tok[5].c_str(),atoi(recv_tok[6].c_str()),atoi(recv_tok[7].c_str()),day_kcal,BMI);
        //들어갈 목록 순서: 이름,아이디,비밀번호,나이,성별,키,몸무게 - 순서 확인하고 변수입력하기
        cout << query << endl;
        query_state = mysql_query(connection,query);
        if(query_state !=0)
        {
            fprintf(stderr, "Mysql query error: %s", mysql_error(&conn));
            exit(1);
        }
    }
    else if(recv_tok[0]=="id_check")        //중복체크
    {
        sprintf(query,"SELECT m_id FROM Health.member_info WHERE m_id = '%s'",recv_tok[1].c_str());
        
        query_state = mysql_query(connection,query);
        if(query_state !=0)
        {
            fprintf(stderr, "Mysql query error: %s", mysql_error(&conn));
            exit(1);
        }
        sql_result = mysql_store_result( connection );  
        int rows = mysql_num_rows( sql_result );        
        cout << "row: " << rows << endl;                 //클라이언트가 입력한 아이디가 db에 있으면 1 없으면 0이 출력된다.
        string id_check_result = recv_tok[0]+ "|" + to_string(rows) + "|"; 
        cout << id_check_result << endl;
        memset(send_buff, 0, sizeof(send_buff));
        strcpy(send_buff, id_check_result.c_str());
        send(clientfd, send_buff, sizeof(send_buff), 0);
        cout << "send_buff: " << send_buff << endl;
    }

    else if (recv_tok[0]=="food_info")      // 음식에 대한 정보를 조회할 때
    {
        stringstream ss;
        sprintf(query,"SELECT f_kcal, f_carbohydrate, f_protein,f_fat, f_salt FROM Health.food_info WHERE f_name = '%s'", recv_tok[1].c_str());
        query_state = mysql_query(connection,query);
        if(query_state !=0)
        {
            fprintf(stderr, "Mysql query error: %s", mysql_error(&conn));
            exit(1);
        }
        sql_result = mysql_store_result( connection );  
        int rows = mysql_num_rows( sql_result );        
        int column = mysql_num_fields(sql_result);      
        while ((sql_row = mysql_fetch_row(sql_result)) != NULL)
        {
            for (int i = 0; i < column; i++)
            {
                ss << sql_row[i] << "|";
            }
        }

        string food_info = recv_tok[0] + "$" + ss.str() + "$";  //code + $ + 내용물(|구분되어 있음) + $로 마지막임을 표시
        cout << food_info << endl;
        memset(send_buff, 0, sizeof(send_buff));
        strcpy(send_buff, food_info.c_str());
        send(clientfd, send_buff, sizeof(send_buff), 0);
        cout << "send_buff: " << send_buff << endl;
    }
    else if (recv_tok[0]=="eat")        //먹은 음식을 추가했을 때
    {
        string current_time = mytime();
        cout << "current_time.c_str() : " << current_time.c_str() <<  endl;
        sprintf(query, "SELECT day_kcal, day_carbohydrate, day_protein, day_fat, day_salt  FROM Health.day_kcal WHERE m_id='%s' and date = '%s'",recv_tok[1].c_str(), current_time.c_str()); //아이디는 로그인할때 따로 변수에 저장해서 여기에 넣어줘야한다. 시간도 추가해야함
        
        query_state = mysql_query(connection,query);
        if(query_state !=0)
        {
            fprintf(stderr, "Mysql query error: %s", mysql_error(&conn));
            exit(1);
        }
        sql_result = mysql_store_result( connection );      // 데이터 받아오기
        int rows = mysql_num_rows( sql_result );            // 테이블 로우 수
        int column = mysql_num_fields(sql_result);          //테이블 컬럼 수
        double temp1[5] = {0};                              // day_kcal tbl에 저장된 기존 칼로리 및 영양성분 데이터 

        while ((sql_row = mysql_fetch_row(sql_result)) != NULL)
        {
            for (int i=0; i<column; i++)
            {
                temp1[i] = atof(sql_row[i]);                //칼로리,탄수화물,단백질,지방,나트륨 순서대로 나옴
                cout <<"temp1 : " << temp1[i] << endl;
            }
        }
        if (rows == 1)                                      //DB안에 정보가 있으면 날짜에 해당하는곳에 update
        {
            double add_cal[5] = {0};
            double temp2[5];                                //칼로리 및 영양소 저장 배열 
            Select_foodinfo(recv_tok[2],temp2);             // food_info에 클라이언트가 보낸 음식에 대한 칼로리 및 영양소 정보를 temp2에 저장 
            string daytime = mytime();
            sprintf(query, "SELECT f_kcal, f_carbohydrate, f_protein, f_fat, f_salt FROM Health.food_info WHERE f_name ='%s'",recv_tok[2].c_str()); //아이디는 로그인할때 따로 변수에 저장해서 여기에 넣어줘야한다. 시간도 추가해야함
            
            query_state = mysql_query(connection,query);
            if(query_state !=0)
            {
                fprintf(stderr, "Mysql query error: %s", mysql_error(&conn));
                exit(1);
            }
            sql_result = mysql_store_result( connection );  //데이터 받아오기
            int rows = mysql_num_rows( sql_result );        //테이블 로우 수
            int column = mysql_num_fields(sql_result);      //테이블 컬럼 수
            double data_food_info[5];                       //음식에 대한 정보를 집어넣는 변수설정

            while ((sql_row = mysql_fetch_row(sql_result)) != NULL)
            {
                for (int i=0; i<column; i++)
                {
                    data_food_info[i] = atof(sql_row[i]);   //더블형태로 각 정보를 저장
                }
            }
            double gram_data[5] = {0};
            cout << recv_tok[3] << endl;
            for(int i = 0; i < column; i++)
            {
                gram_data[i] = data_food_info[i] / 100 * stof(recv_tok[3]); // 100g당 영양소 / 100 * gram량
                cout << "gram_data "<< i + 1 << " : " << gram_data[i] << endl;

            }

            for(int i=0;i<5;i++)                            // 기존에 저장된 칼로리 정보에 새로 추가한 음식에 대한 칼로리 정보를 더해줌.
            {
                add_cal[i] = temp1[i] + gram_data[i];
                cout << add_cal[i] << endl;
            }

            sprintf(query, "UPDATE Health.day_kcal SET day_kcal = %lf,day_carbohydrate = %lf, day_protein = %lf, day_fat = %lf, day_salt = %lf WHERE m_id='%s'",add_cal[0],add_cal[1],add_cal[2],add_cal[3],add_cal[4],recv_tok[1].c_str());
            query_state = mysql_query(connection,query);
            if(query_state !=0)
            {
                fprintf(stderr, "Mysql query error: %s", mysql_error(&conn));
                exit(1);
            }
        }
        else if (rows == 0)                             //DB안에 정보가 없으면 insert
        {
            double temp2[5];
            Select_foodinfo(recv_tok[2],temp2);         // food_info에 클라이언트가 보낸 음식에 대한 칼로리 및 영양소 정보를 temp2에 저장 
            // 100g당 영양소/100 *recv_tok[3]
            double data_temp = 100 / 100 * stof(recv_tok[3]); // 0218
            cout << data_temp << endl;
            for(int i=0;i<5;i++)
            {
                cout <<"temp2: " <<temp2[i] << endl;
            }
            string current_time = mytime();
            sprintf(query, "INSERT INTO Health.day_kcal(date,m_id, day_kcal, day_carbohydrate,day_protein,day_fat,day_salt) VALUES('%s','%s','%lf','%lf','%lf','%lf','%lf')",current_time.c_str(),recv_tok[1].c_str(),temp2[0],temp2[1],temp2[2],temp2[3],temp2[4]);//ID는 클라이언트에서 받아올것 recv_tok에서
            query_state = mysql_query(connection,query);
            if(query_state !=0)
            {
                fprintf(stderr, "Mysql query error: %s", mysql_error(&conn));
                exit(1);
            }
        }

    }
    else if (recv_tok[0]=="exercise")
    {
        stringstream ss;

        sprintf(query, "SELECT d.day_kcal,d.day_carbohydrate,d.day_protein,d.day_fat,d.day_salt, m.BMI,m.m_day_kcal FROM member_info m join day_kcal d on m.m_id = d.m_id where m.m_id = '%s'", recv_tok[1].c_str());
        query_state = mysql_query(connection,query);
        if(query_state !=0)
        {
            fprintf(stderr, "Mysql query error: %s", mysql_error(&conn));
            exit(1);
        }
        sql_result = mysql_store_result( connection );  // 데이터 받아오기
        int rows = mysql_num_rows( sql_result );        // 테이블 로우 수
        int column = mysql_num_fields(sql_result);      //테이블 컬럼 수

        while ((sql_row = mysql_fetch_row(sql_result)) != NULL)
        {
            for (int i = 0; i < column; i++)
            {
                ss << sql_row[i] << "|";
            }
        }

        string food_info = recv_tok[0] + "|" + ss.str() + "|";  //code + $ + 내용물(|구분되어 있음) + $로 마지막임을 표시
        cout << food_info << endl;
        memset(send_buff, 0, sizeof(send_buff));
        strcpy(send_buff, food_info.c_str());
        send(clientfd, send_buff, sizeof(send_buff), 0);
        cout << "send_buff: " << send_buff << endl;
    }
    else if (recv_tok[0]=="adjust_info")                        //내 정보 수정시
    {
        //1아이디,2키,3몸무게,4성별
        stringstream ss;
        sprintf(query, "SELECT m_gender FROM Health.member_info WHERE m_id = '%s'", recv_tok[1].c_str());  //아이디는 로그인할때 따로 변수에 저장해서 여기에 넣어줘야한다.
        query_state = mysql_query(connection,query);
        if(query_state !=0)
        {
            fprintf(stderr, "Mysql query error: %s", mysql_error(&conn));
            exit(1);
        }
        sql_result = mysql_store_result( connection );  // 데이터 받아오기
        int rows = mysql_num_rows( sql_result );        // 테이블 로우 수
        int column = mysql_num_fields(sql_result);      //테이블 컬럼 수

        while ((sql_row = mysql_fetch_row(sql_result)) != NULL)
        {
            for (int i = 0; i < column; i++)
            {
                ss << sql_row[i];
            }
        }

        double day_kcal;
        if (ss.str() == "남")
        {
            day_kcal = (stof(recv_tok[2])/100) * (stof(recv_tok[2])/100) * 22 * 35;             //1일 권장칼로리 = 키(m) x 키(m) x (남:22,여:21) x 35
        }
        else if (ss.str() == "여")
        {
            day_kcal = (stof(recv_tok[2])/100) * (stof(recv_tok[2])/100) * 21 * 35;             //1일 권장칼로리 = 키(m) x 키(m) x (남:22,여:21) x 35
        }
        double BMI = stof(recv_tok[3]) / (stof(recv_tok[2])/100 * stof(recv_tok[2])/100);       //BMI(kg/m^2) = 몸무게 / 키의제곱

        sprintf(query,"UPDATE Health.member_info SET m_height = %d,m_weight = %d , m_day_kcal = %.2lf, BMI=%.2lf WHERE m_id = '%s'",stoi(recv_tok[2]),stoi(recv_tok[3]), day_kcal, BMI,recv_tok[1].c_str());
        query_state = mysql_query(connection,query);
        if(query_state !=0)
        {
            fprintf(stderr, "Mysql query error: %s", mysql_error(&conn));
            exit(1);
        }
      
    }

    else if(recv_tok[0]=="my_nutrition")            //내가 오늘 얼마나 먹었는지 확인할 때
    {
        sprintf(query,"SELECT d.day_kcal,d.day_carbohydrate,d.day_protein,d.day_fat,d.day_salt,m.m_day_kcal,m_weight FROM member_info m join day_kcal d on m.m_id = d.m_id where m.m_id = '%s'",recv_tok[1].c_str());
        
        query_state = mysql_query(connection,query);
        if(query_state !=0)
        {
            fprintf(stderr, "Mysql query error: %s", mysql_error(&conn));
            exit(1);
        }
        sql_result = mysql_store_result( connection );  // 데이터 받아오기
        int rows = mysql_num_rows( sql_result );        // 테이블 로우 수
        int column = mysql_num_fields(sql_result);      //테이블 컬럼 수
        stringstream ss;
        while ((sql_row = mysql_fetch_row(sql_result)) != NULL)
        {
            for (int i = 0; i < column; i++)
            {
                ss << sql_row[i]<< "|";
            }
        }

        string food_info = recv_tok[0] + "|" + ss.str() + "|";  //code + $ + 내용물(|구분되어 있음) + $로 마지막임을 표시
        cout << food_info << endl;
        memset(send_buff, 0, sizeof(send_buff));
        strcpy(send_buff, food_info.c_str());
        send(clientfd, send_buff, sizeof(send_buff), 0);
        cout << "send_buff: " << send_buff << endl;
    }
    return len;
}
void Server::Select_foodinfo(string f_name, double *cal)
{
    sprintf(query, "SELECT f_kcal,f_carbohydrate,f_protein,f_fat,f_salt FROM Health.food_info WHERE f_name = '%s'", f_name.c_str()); //사용자가 추가한 음식에 대한 영양분 가져오기
                                                //recv_tok[2] 는 gram수
    query_state = mysql_query(connection, query);
    if (query_state != 0)
    {
        fprintf(stderr, "Mysql query error: %s", mysql_error(&conn));
        exit(1);
    }
    sql_result = mysql_store_result(connection); //데이터 받아오기
    int rows = mysql_num_rows(sql_result);       // 테이블 로우 수
    cout << "row: " << rows << endl;             //클라이언트가 입력한 음식이 db에 있으면 1 없으면 0
    int column = mysql_num_fields(sql_result);   //테이블 컬럼 수                     
    while ((sql_row = mysql_fetch_row(sql_result)) != NULL)
    {
        for (int i = 0; i < column; i++)
        {                               //칼로리 및 영양소 저장 배열
            cal[i] = atof(sql_row[i]); //칼로리,탄수화물,단백질,지방,나트륨 순서대로 저장
        }
    }
}
int Server::client_number(int socket)
{
    int i = 0;
    for(list<int>::iterator it = client_list.begin(); it !=client_list.end(); ++it)
    {
        if(*it == socket)
        {
            return i;
        }
        i++;
    }
    return -1;
}
void Server::close_server()
{
    close(listener);
    close(epoll_fd);
}

string mytime()
{
    struct tm curr_tm;
    time_t curr_time = time(nullptr);

    localtime_r(&curr_time, &curr_tm);

    int curr_year = curr_tm.tm_year + 1900;
    int curr_month = curr_tm.tm_mon + 1;
    int curr_day = curr_tm.tm_mday;

    char mytime[128];
    memset(mytime, 0x00, sizeof(mytime));
    
    sprintf(mytime,"%d-%02d-%02d",curr_year, curr_month, curr_day);
    cout << mytime;
    string temp(mytime);

    return temp;
}

int main(void)
{
    Server server;
    server.start();
    return 0;
}