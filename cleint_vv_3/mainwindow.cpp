#include "mainwindow.h"
#include "ui_mainwindow.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->PW_le->setEchoMode(QLineEdit::Password);

    // 로그인 화면 백그라운드 이미지 설정
    QPixmap bg("/home/iot/cleint_vv_3/image/logo2.png");    // 이미지 경로!!
    bg = bg.scaled(this->size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    QPalette p(palette());
    p.setBrush(QPalette::Background, bg);
    setAutoFillBackground(false);
    setPalette(p);

    connect_server();

    connect(ui->Login_btn,&QPushButton::clicked,this,&MainWindow::Login_btn_clicked);               // 로그인 버튼 클릭시 연결
    connect(ui->PW_le,&QLineEdit::returnPressed,this,&MainWindow::Login_btn_clicked);               // pw_lineedit입력 후 엔터키를 눌렀을때
    connect(ui->pushButton,&QPushButton::clicked,this,&MainWindow::on_pushButton_clicked);
    connect(ui->pushButton_2,&QPushButton::clicked,this,&MainWindow::on_pushButton_2_clicked);
    connect(ui->pushButton_pw_change,&QPushButton::clicked,this,&MainWindow::Myinfo_btn_clicked);

    ui->lineEdit_pw_change->setEchoMode(QLineEdit::Password);                                       // 내정보에서 비밀번호를 입력했을때 가려서 나오게 하는 거

    connect(ui->lineEdit_pw_change,&QLineEdit::returnPressed,this,&MainWindow::Myinfo_btn_clicked); // myinfo pw 입력 후 엔터키를 눌렀을때
    connect(ui->lineEdit_8,&QLineEdit::returnPressed,this,&MainWindow::Change_myinfobtn_clicked);   // chagne_weight line edit 입력 후 엔터키를 눌렀을때

    ui->pushButton_6->setText("제출");

    connect(ui->pushButton_6,&QPushButton::clicked,this,&MainWindow::Change_myinfobtn_clicked);     // 내 정보화면에서 비밀번호를 누르고 버튼을 클릭했을때
    connect(ui->pushButton_7,&QPushButton::clicked,this,&MainWindow::Cancel_change_myinfo);         // 내 정보 수정 취소버튼을 눌렀을때

    model = new QStringListModel(this); // 이거 없으면 '확인'버튼 한번 누르면 프로그램 종료 됩니다.
}

MainWindow::~MainWindow()
{
    delete ui;
}
void MainWindow::connect_server()   // 서버와 연결하는 함수
{
    socket = ::socket(PF_INET, SOCK_STREAM, 0);
    cout << "init socket : " << socket << endl;
    sock_p = &socket;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(IP);
    server_addr.sin_port = htons(PORT);

    if(::connect(socket,(struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        cout << "연결이 실패했습니다." << endl;
    }
    eventn = epoll_wait(epfd, events, MAX_EPOLL_EVENTS, 1000/*timeout*/);
    for(int i = 0; i < eventn; i++)
    {
        if(events[i].events & EPOLLIN)
        {
            printf("Socket %d connected\n", events[i].data.fd);
        }
    }

}

vector<string> MainWindow::recvdata_split(char *data) // 받은 데이터를 | 기준으로 잘라서 벡터에 저장, 인자값이 char포인터
{
    stringstream ss;
    string temp(data);

    ss.str(temp);
    string one;
    vector<string> input;
    while (getline(ss,one,'|'))
    {
        input.push_back(one);
    }
    return input;
}

vector<string> MainWindow::recvdata_split(string data) // 받은 데이터를 | 기준으로 잘라서 벡터에 저장, 인자값이 string
{
    stringstream ss;
    string temp = data;
    cout << __LINE__ << endl;
    ss.str(temp);
    string one;
    vector<string> input;
    while (getline(ss,one,'|'))
    {
        input.push_back(one);
    }
    return input;
}

void MainWindow::Login_btn_clicked()    //로그인 버튼 클릭시
{
    ID = ui->ID_le->text();
    PW = ui->PW_le->text();
    memset(send_buff,0,sizeof(send_buff));
    QString log_res = "login|"+ ID + "|" + PW;
    strcpy(send_buff,log_res.toStdString().c_str());
    if (send(socket,send_buff,sizeof(send_buff),0) <0)
    {
        qDebug() << "send_error!";
        exit(-1);
    }
    usleep(100000);
    cout << recv_buff << endl;
    vector<string> login_check = recvdata_split(recv_buff);
    cout << login_check[1] << endl;
    if (login_check[1] == "1") // 로그인 성공시
    {
        QMessageBox msg;
        msg.setText("Login Success");
        msg.exec();
        ui->stackedWidget->setCurrentIndex(2);

        // 로그인 후 메인 화면 백그라운드 이미지 설정
        QPixmap bg("/home/iot/cleint_vv_3/image/back.png");
        bg = bg.scaled(this->size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        QPalette p(palette());
        p.setBrush(QPalette::Background, bg);
        setAutoFillBackground(false);
        setPalette(p);

        ui->label_ID->setText(ID+" 님");         // 내정보수정 ID라벨에 내 아이디 표시하기
    }
    else    // 로그인 실패시 실패 메세지박스 출력
    {
        QMessageBox fail_msg;
        fail_msg.setText("Login Fail");
        fail_msg.setIcon(QMessageBox::Warning);
        fail_msg.setWindowTitle("Caution");
        fail_msg.exec();
    }

}
void MainWindow::on_Signup_btn_clicked()    // 회원가입 버튼 클릭시 연결되는 함수
{
    ui->stackedWidget->setCurrentIndex(1);
    QString path = "/home/iot/test4/sign_up.png";
    QPixmap pixmap(path);
    ui->label->setPixmap(pixmap);
    ui->lineEdit_3->setEchoMode(QLineEdit::Password);
    connect(ui->pushButton_3,&QPushButton::clicked,this,&MainWindow::ID_check);         // 아이디 중복 확인 버튼을 눌렀을때
    connect(ui->pushButton_4,&QPushButton::clicked,this,&MainWindow::Sign_up);          // 제출 버튼을 눌렀을때
    connect(ui->pushButton_5,&QPushButton::clicked,this,&MainWindow::Cancel_signup);    // 취소 버튼을 눌렀을때

}
void MainWindow::Sign_up()  // 회원가입 화면에서 제출버튼을 눌렀을때
{
    new_name = ui->lineEdit->text();            //이름
    new_id = ui->lineEdit_2->text();            //아이디
    new_pw = ui->lineEdit_3->text();            //비번
    if(ui->radioButton->isChecked())            //성별 남자 체크시
        new_gender = ui->radioButton->text();
    else
        new_gender = ui->radioButton_2->text(); //성별 여자 체크시
    new_age = ui->lineEdit_6->text();           //나이
    new_height = ui->lineEdit_4->text();        // 키
    new_weight = ui->lineEdit_5->text();        //몸무게

    //확인용 출력문
    qDebug() << new_name
             <<" " << new_id <<" " <<
                new_pw << " " << new_age << " " << new_gender << "" << new_height <<" " << new_weight <<endl;
    // 하나라도 입력을 안할 경우 서버로 send를 안하고 실패 경고 메세지가 뜬다
    if (new_name.isEmpty() or new_id.isEmpty() or new_pw.isEmpty() or new_gender.isEmpty() or new_age.isEmpty() or new_height.isEmpty() or new_weight.isEmpty())    // 오류 있음 고치셈!!
    {
        QMessageBox fail;
        fail.setText("Sign UP Fail");
        fail.setIcon(QMessageBox::Warning);
        fail.setWindowTitle("Caution");
        fail.exec();
    }
    else
    {
        string signup_res = "sign_up|" + new_name.toStdString() + "|" + new_id.toStdString() + "|" + new_pw.toStdString() + "|" + new_age.toStdString() + "|" + new_gender.toStdString() + "|" + new_height.toStdString()
                +"|"+new_weight.toStdString();
        memset(send_buff,0,sizeof(send_buff));
        strcpy(send_buff,signup_res.c_str());
        cout << send_buff << endl;
        if(send(socket,send_buff,sizeof(send_buff),0) <0)
        {
            cout << "send_error!" << endl;
            return;
        }

        QMessageBox success;
        success.setText("제출 완료되었습니다.");
        success.setIcon(QMessageBox::Information);
        success.exec();
        ui->stackedWidget->setCurrentIndex(0);
    }
    // 회원가입시 입력한 정보를 담은 QString 변수들 초기화
    new_name.clear();
    new_id.clear();
    new_pw.clear();
    new_gender.clear();
    new_age.clear();
    new_height.clear();
    new_weight.clear();

    // 입력했던 lineedit 모두 초기화
    ui->lineEdit->setText(" ");
    ui->lineEdit_2->setText(" ");
    ui->lineEdit_3->setText("");
    ui->lineEdit_6->setText(" ");
    ui->lineEdit_4->setText(" ");
    ui->lineEdit_5->setText(" ");

}
void MainWindow::ID_check() // 아이디 중복 버튼을 눌렀을때
{

    string id_send;
    new_id = ui->lineEdit_2->text();
    id_send = "id_check|" + new_id.toStdString();
    memset(send_buff,0,sizeof(send_buff));
    strcpy(send_buff,id_send.c_str());
    if (send(socket,send_buff,sizeof(send_buff),0) < 0)
    {
        cout << "send_error!";
        return;
    }
    cout << "send_buff: " << send_buff << endl;

    usleep(5000);
    cout << recv_buff << endl;
    vector<string> id_check = recvdata_split(recv_buff);
     cout << __LINE__ << endl;
    if (id_check[1]=="1") // 아이디 중복일때
    {
        QMessageBox check_fail;
        check_fail.setText("아이디가 중복됩니다.");
        check_fail.setIcon(QMessageBox::Warning);
        check_fail.setWindowTitle("Caution");
        check_fail.exec();
    }

    else
    {
        QMessageBox check_success;
        check_success.setText("사용가능한 아이디입니다.");
        check_success.setIcon(QMessageBox::Information);
        check_success.exec();
    }

}
void MainWindow::Cancel_signup()    // 회원가입 취소버튼을 눌렀을때
{
    ui->lineEdit->setText("");
    ui->lineEdit_2->setText("");
    ui->lineEdit_3->setText("");
    ui->lineEdit_6->setText("");
    ui->lineEdit_4->setText("");
    ui->lineEdit_5->setText("");
    ui->stackedWidget->setCurrentIndex(0);
}


void MainWindow::on_pushButton_clicked()
{
    QStringList List;
    QString temp = "";
    static int count = 0;

    if(check == true)
    {
        food.clear();
    }
    if(count % 2 == 0)
    {
        food.push_back(ui->comboBox_2->currentText());
        food_g.push_back(ui->lineEdit_gram->text());
        cout << "a.size() : " << food.size() << endl;
        for (unsigned long i = 0; i < food.size(); i++)
        {
            temp += food[i] + "\t\t"+ food_g[i] + "\n";
        }
        List << temp;

        model->setStringList(List);
        ui->listView->setModel(model);
    }
    check = false;
    count++;
}

void MainWindow::on_pushButton_2_clicked()
{
    QStringList List;
    model->setStringList(List);

    memset(send_buff, 0, sizeof(send_buff));
    string log_res;;
    for(unsigned long i = 0; i < food.size(); i++)
    {
        log_res.clear();
        log_res = "eat|";
        log_res.append(ID.toStdString());
        log_res.append("|");
        log_res.append(food[i].toStdString());
        log_res.append("|");
        log_res.append(food_g[i].toStdString());
        log_res.append("|");
        strcpy(send_buff, log_res.c_str());
        cout << send_buff << endl;
        send(socket,send_buff,sizeof(send_buff), 0);

        ui->listView->setModel(model);
        usleep(5000); // 서버로 전송할때에 대기 시간필요
    }

    food.clear();
    check = true;
}

void MainWindow::on_comboBox_2_currentIndexChanged(const QString &)
{
    vector<string> recv_tok;
    recv_tok = recvdata_split(recv_buff);
    QString carbohydrate;
    QString protein;
    QString fat;
    QString salt;
    QString kcal;
    for(unsigned int i = 0; i < 15; i++)
    {
        if(recv_tok[3+6*i] == ui->comboBox_2->currentText().toStdString())
        {
            kcal = QString::fromUtf8(recv_tok[4+6*i].c_str());
            carbohydrate = QString::fromUtf8(recv_tok[5+6*i].c_str());
            protein = QString::fromUtf8(recv_tok[6+6*i].c_str());
            fat = QString::fromUtf8(recv_tok[7+6*i].c_str());
            salt = QString::fromUtf8(recv_tok[8+6*i].c_str());

        }
    }
    cout << recv_tok[5+6*0] << endl;
    cout << carbohydrate.toStdString() << endl;
    ui->label_carbohydrate->setText(carbohydrate);
    ui->label_protein->setText(protein);
    ui->label_fat->setText(fat);
    ui->label_salt->setText(salt);
    ui->label_kcal->setText(kcal);
}

void MainWindow::on_tabWidget_currentChanged(int index) // 탭 클릭시 연결되는 함수
{
    QPixmap walkimg("/home/iot/cleint_vv_3/image/walk.jpg");
    QPixmap walkimg2 = walkimg.scaled(200, 180, Qt::KeepAspectRatio);
    ui->label_walk_img->setPixmap(walkimg2);

    QPixmap runimg("/home/iot/cleint_vv_3/image/running.jpeg");
    QPixmap runimg2 = runimg.scaled(200, 180, Qt::KeepAspectRatio);
    ui->label_run_img->setPixmap(runimg2);

    QPixmap cycleimg("/home/iot/cleint_vv_3/image/cycle.jpg");
    QPixmap img2 = cycleimg.scaled(200, 180, Qt::KeepAspectRatio);
    ui->label_cycling_img->setPixmap(img2);

    cout << index << endl;
    if(index == 1)                                  // 운동탭 클릭시
    {
        memset(send_buff,0,sizeof(send_buff));
        QString log_res = "exercise|"+ ID + "|";
        strcpy(send_buff,log_res.toStdString().c_str());
        if (send(socket,send_buff,sizeof(send_buff),0) <0)
        {
            qDebug() << "send_error!";
            exit(-1);
        }
        vector<string> recv_tok;
        usleep(50000); // 서버에서 값을 받아오기 위한 대기시간
        recv_tok = recvdata_split(recv_buff);
        double exercise = atof(recv_tok[1].c_str());
        ui->label_walk->setText(QString::fromUtf8((to_string(int(exercise/20))+"분 동안 걷기").c_str())); // 섭취한 칼로리 /20 = min
        ui->label_run->setText(QString::fromUtf8((to_string(int(exercise/30))+"분 동안 뛰기").c_str())); // 섭취한 칼로리 /20 = min
        ui->label_cycling->setText(QString::fromUtf8((to_string(int(exercise/40))+"분 동안 자전거 타기").c_str())); // 섭취한 칼로리 /20 = min
    }
    else if(index == 3)                         // 영양정보 탭을 클릭시
    {
        memset(send_buff,0,sizeof(send_buff));
        QString log_res = "my_nutrition|"+ ID + "|";
        strcpy(send_buff,log_res.toStdString().c_str());
        if (send(socket,send_buff,sizeof(send_buff),0) <0)
        {
            qDebug() << "send_error!";
            exit(-1);
        }
        vector<string> recv_tok;
        usleep(50000); // 서버에서 값을 받아오기 위한 대기시간
        recv_tok = recvdata_split(recv_buff);

        ui->tableWidget->setItem(0,0,new QTableWidgetItem(QString::fromUtf8(recv_tok[6].c_str())));
        ui->tableWidget->setItem(0,1,new QTableWidgetItem(QString::fromUtf8(to_string(stoi(recv_tok[7])*6).c_str())));
        ui->tableWidget->setItem(0,2,new QTableWidgetItem(QString::fromUtf8(to_string(stoi(recv_tok[7])).c_str())));
        ui->tableWidget->setItem(0,3,new QTableWidgetItem(QString::fromUtf8(to_string(stoi(recv_tok[7])).c_str())));
        ui->tableWidget->setItem(0,4,new QTableWidgetItem(QString::fromUtf8("3g")));

        ui->tableWidget->setItem(1,0,new QTableWidgetItem(QString::fromUtf8(recv_tok[1].c_str())));
        ui->tableWidget->setItem(1,1,new QTableWidgetItem(QString::fromUtf8(to_string(stoi(recv_tok[2])).c_str())));
        ui->tableWidget->setItem(1,2,new QTableWidgetItem(QString::fromUtf8(to_string(stoi(recv_tok[3])).c_str())));
        ui->tableWidget->setItem(1,3,new QTableWidgetItem(QString::fromUtf8(to_string(stoi(recv_tok[4])).c_str())));
        ui->tableWidget->setItem(1,4,new QTableWidgetItem(QString::fromUtf8(to_string(stoi(recv_tok[5])).c_str())));

    }

}

void MainWindow::on_pushButton_cancel_clicked()
{
    food.clear();
    food_g.clear();
    QString temp = "";
    QStringList List;

    List << temp;

    model->setStringList(List);

    ui->listView->setModel(model);
}

// 내정보 수정과 관련된 함수들 Myinfo_btn_clicked(), Change_myinfobtn_clicked(), Cancel_change_myinfo()
void MainWindow::Myinfo_btn_clicked()
{
    QString Myinfo_PW;          // 내정보 화면에서 비밀번호를 입력한 값을 저장하기 위한 변수
    Myinfo_PW = ui->lineEdit_pw_change->text();
    qDebug() <<"PW" << PW << "Myinfo PW: " <<Myinfo_PW << endl;
    if(PW == Myinfo_PW) // 로그인할때 입력한 비밀번호와 내정보에서 입력한 비밀번호가 같으면
    {
        QMessageBox success;
        success.setText("Success");
        success.setIcon(QMessageBox::Information);
        success.exec();
        ui->stackedWidget_2->setCurrentIndex(1);
        ui->label_13->setText("키");
        ui->label_14->setText("몸무게");
    }
    else
    {
        QMessageBox fail_msg;
        fail_msg.setText("Fail");
        fail_msg.setIcon(QMessageBox::Warning);
        fail_msg.setWindowTitle("Caution");
        fail_msg.exec();
    }

}
void MainWindow::Change_myinfobtn_clicked()
{
    QString change_height,change_weight;
    string myinfo_result;
    change_height = ui->lineEdit_7->text();
    change_weight = ui->lineEdit_8->text();
    qDebug() <<"change_height" << change_height << "change_weight" << change_weight << endl;
    if(change_height.isEmpty() or change_weight.isEmpty())
    {
        QMessageBox fail_msg;
        fail_msg.setText("Fail");
        fail_msg.setIcon(QMessageBox::Warning);
        fail_msg.setWindowTitle("Caution");
        fail_msg.exec();
    }
    else    // 제출 성공시
    {
        myinfo_result = "adjust_info|" + ID.toStdString()+ "|" + change_height.toStdString() + "|" + change_weight.toStdString();
        strcpy(send_buff,myinfo_result.c_str());
        if(send(socket,send_buff,sizeof(send_buff),0) <0)
        {
            cout << "send_buff error"<< endl;
            exit(-1);
        }
        cout << send_buff << endl;
        QMessageBox success;
        success.setText("Success");
        success.setIcon(QMessageBox::Information);
        success.exec();

    }

    ui->lineEdit_7->setText("");
    ui->lineEdit_8->setText("");
    ui->lineEdit_pw_change->setText("");
    ui->stackedWidget_2->setCurrentIndex(0);
}
void MainWindow::Cancel_change_myinfo()
{
    ui->lineEdit_7->setText("");
    ui->lineEdit_8->setText("");
    ui->lineEdit_pw_change->setText("");
    ui->stackedWidget_2->setCurrentIndex(0);
}

