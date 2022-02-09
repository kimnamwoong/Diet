#include "mainwindow.h"

#include <QApplication>
#include <thread>

void Recv(int*& sock_p, string& recv_buff)  // 쓰레드 발생할 함수 
{
    while (1){
        char temp[1024] = {};
        memset(temp,0,1024);
        recv(*sock_p,temp,1024,0);
        recv_buff = temp;
    }
}


int main(int argc, char *argv[])
{

    QApplication a(argc, argv);
    MainWindow w;
    thread th = thread(Recv, ref(w.sock_p), ref(w.recv_buff)); // thread 생성 
    w.show();

    return a.exec();
}
