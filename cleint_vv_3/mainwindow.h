#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLineEdit>
#include <QLabel>
#include <QDebug>
#include <QPushButton>
#include <QDialog>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QThread>
#include <QStringList>
#include <QStringListModel>
#include <QAbstractItemView>
#include <QTableWidget>

#include <unistd.h>
#include <string.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <list>
#include <typeinfo>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <thread>

#define PORT 4005
#define IP "10.10.21.128"
#define MAX_BUFF 1024
#define MAX_EPOLL_EVENTS 64
using namespace std;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    int socket;                                                                     // 소켓 fd 생성
    int *sock_p;                                                                    // 소켓 포인터
    string recv_buff;                                                               // 수신용 string 변수
    QString ID,PW;                                                                  // lineedit에 입력한 아이디 비밀번호 저장할 변수
    QString new_name, new_id, new_pw, new_age, new_gender, new_height, new_weight;  // 회원가입시 // lineedit에 입력한 각 정보들을 저장할 변수
    void recv_thread();
    QTableWidgetItem *item1;

protected:
    bool check = false;
    vector<QString> food;
    vector<QString> food_g;

private slots:
    void connect_server();              // 서버랑 커넥트하는 함수
    void Login_btn_clicked();           //로그인 버튼 클릭시 연결되는 함수
    void on_Signup_btn_clicked();       //회원가입 버튼 클릭시 연결되는 함수
    void Sign_up();                     // 회원가입 화면에서 완료버튼을 클릭하면 연결되는 함수
    void ID_check();                    // 아이디 중복확인 버튼을 눌렀을때
    void Cancel_signup();               // 회원가입 취소버튼을 눌렀을때
    void on_pushButton_clicked();

    void Myinfo_btn_clicked();          // 내정보 화면에서 비밀번호를 누르고 클릭하면 연결되는 함수
    void Change_myinfobtn_clicked();    // 변경된 키와 몸무게를 입력한 뒤 버튼을 클릭하면 연결되는 함수
    void Cancel_change_myinfo();        // 내정보 수정을 취소했을때 연결되는 함수
    
    // 클라이언트로부터 받은 데이터를 | 기준으로 잘라주는 함수 
    vector<string> recvdata_split(char *python);
    vector<string> recvdata_split(string);

    void on_pushButton_2_clicked();

    void on_comboBox_2_currentIndexChanged(const QString &arg1);
    void on_tabWidget_currentChanged(int index);

    void on_pushButton_cancel_clicked();

private:
    Ui::MainWindow *ui;
    int eventn;                     // epoll event number
    int epfd;                       // epoll fd
    struct epoll_event events[MAX_EPOLL_EVENTS];
    struct sockaddr_in server_addr;
    QStringListModel *model;

    char send_buff[MAX_BUFF];
};
#endif // MAINWINDOW_H
