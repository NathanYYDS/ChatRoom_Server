/*
*┌────────────────────────────────────────────────┐
*│　描    述：网络聊天室服务端
*│　作    者：Nathan_Guozi
*│　版    本：1.3
*│　创建时间：2024年2月3日
*└────────────────────────────────────────────────┘
*/


#define _CRT_SECURE_NO_WARNINGS
//安全警告
#pragma warning(disable : 4996)
//
#include <iostream>
//标准输入输出流头文件
#include <vector>
//向量容器
#include <winsock2.h>
//Winsock2网络编程
#include <ws2tcpip.h>
//网络地址转换
#include <thread>
//线程处理头文件
#include <time.h>
#include "include/mysql.h"
//mysql头文件
#include <string.h>
#pragma comment(lib, "ws2_32.lib")//链接Winsock2库
#pragma comment(lib, "lib/libmysql.lib") // MySQL库

using namespace std;

const int PORT = 9999;
const int MAX_CLIENTS = 10;
//最大客户端数
vector<SOCKET> clients;//客户端SOCKET向量





// 初始化mysql连接环境
MYSQL* mysql = mysql_init(NULL);

//登录检查
bool loginserver(const std::string& username, const std::string& password) {
    const char* sql = "select * from users";

    // 执行这个sql语句
    int ret = mysql_query(mysql, sql);
    //也可mysql_query(mysql, "select * from users");

    //检查
    if (ret != 0)
    {
        printf("mysql_query() a失败了, 原因: %s\n", mysql_error(mysql));
        return -1;
    }

    // 取出用户表
    MYSQL_RES* res = mysql_store_result(mysql);
    if (res == NULL)
    {
        printf("mysql_store_result() 失败了, 原因: %s\n", mysql_error(mysql));
        return -1;
    }

    // 得到结果集中的列数
    int num = mysql_num_fields(res);

    //获取列名
    MYSQL_FIELD* fields = mysql_fetch_fields(res);

    //遍历每一行并与输入比较
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)) != NULL)
    {
        for (int i = 0; i < num; i = i + 2)
        {
            //std::string 类型可以通过 c_str() 方法返回一个指向其内部 const char* 缓冲区的指针
            if (strcmp(username.c_str(), row[i]) == 0 && strcmp(password.c_str(), row[i + 1]) == 0)
            {
                return 1;
            }
        }
    }
    mysql_free_result(res);
    return 0;
}

//注册检查
bool registercheck(const std::string& username) {
    const char* sql = "select * from users";

    // 执行这个sql语句
    int ret = mysql_query(mysql, sql);
    //也可mysql_query(mysql, "select * from users");

    //检查
    if (ret != 0)
    {
        printf("mysql_query() a失败了, 原因: %s\n", mysql_error(mysql));
        return 0;
    }

    // 取出用户表
    MYSQL_RES* res = mysql_store_result(mysql);
    if (res == NULL)
    {
        printf("mysql_store_result() 失败了, 原因: %s\n", mysql_error(mysql));
        return 0;
    }

    // 得到结果集中的列数
    int num = mysql_num_fields(res);

    //获取列名
    MYSQL_FIELD* fields = mysql_fetch_fields(res);

    //遍历每一行并与输入比较
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)) != NULL)
    {
        for (int i = 0; i < num; i = i + 2)
        {
            //std::string 类型可以通过 c_str() 方法返回一个指向其内部 const char* 缓冲区的指针
            if (strcmp(username.c_str(), row[i]) == 0)
            {
                return 1;
            }
        }
    }
    mysql_free_result(res);
    return 0;
}

void handleLogin(SOCKET clientSocket, const char* inputusername, const char* inputpassword) {
    if (loginserver(inputusername, inputpassword)) {
        // 登录成功，向客户端发送确认消息
        send(clientSocket, "LOGIN_SUCCESS", strlen("LOGIN_SUCCESS"), 0);
    }
    else {
        // 登录失败，向客户端发送失败消息
        send(clientSocket, "LOGIN_FAILED", strlen("LOGIN_FAILED"), 0);
    }
}

// 注册处理
void handleRegister(SOCKET clientSocket, const char* newUsername, const char* newPassword)
{
    // 编写SQL语句进行插入操作
    char insertQuery[1000];
    sprintf(insertQuery, "INSERT INTO users(username, password) VALUES ('%s', '%s')", newUsername, newPassword);

    // 执行注册操作

    if (registercheck(newUsername) == 1)
    {
        send(clientSocket, "REGISTER_FAILED:username is haved", strlen("REGISTER_FAILED:username is haved"), 0);
    }
    else if (mysql_query(mysql, insertQuery) == 0)
    {
        // 注册成功，向客户端发送确认消息
        send(clientSocket, "REGISTER_SUCCESS", strlen("REGISTER_SUCCESS"), 0);
    }
    else
    {
        // 注册失败原因，可能是因为用户名已存在等，向客户端发送失败消息
        char REGISTER_FAILED_REASON[1000];
        sprintf_s(REGISTER_FAILED_REASON, "REGISTER_FAILED:%s", mysql_error(mysql));

        printf("注册失败原因：%s\n", REGISTER_FAILED_REASON);

        send(clientSocket, REGISTER_FAILED_REASON, strlen(REGISTER_FAILED_REASON), 0);
    }
}


void handleClient(SOCKET clientSocket)//处理客户端连接
{
    cout << "有人连接了" << endl;
    char buffer[102400];
    char name[99];
    char message[102416];
    std::string input;
    char* action = NULL;
    char* username = NULL;
    char* password = NULL;
    //int nameReceived = recv(clientSocket, name, sizeof(name), 0);
    //cout << '<' << name << '>' << "进来了" << endl;
//接受客户端消息
    while (true)
    {
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        //溢出漏洞---------------------------------溢出漏洞----------------------------------溢出漏洞
        if (bytesReceived > 0)
        {
            buffer[bytesReceived] = '\0';
            action = strtok(buffer, "|");
            //LOGIN|user|pass
            if (strcmp(action, "LOGIN") == 0)
            {

                username = strtok(NULL, "|");
                strcpy(name, username);
                password = strtok(NULL, "|");
                handleLogin(clientSocket, username, password);
                continue;
            }
            if (strcmp(action, "REGISTER") == 0)
            {
                username = strtok(NULL, "|");
                password = strtok(NULL, "|");
                handleRegister(clientSocket, username, password);
                continue;
            }

        }
        //溢出漏洞---------------------------------溢出漏洞----------------------------------溢出漏洞
        if (bytesReceived <= 0)
            //接受失败或连接关闭
        {
            closesocket(clientSocket);
            //cout << '<' << name << '>' << "退出了" << endl;
            cout << "有人退出了" << endl;
            for (auto it = clients.begin(); it != clients.end(); ++it)
            {
                if (*it == clientSocket)
                {
                    clients.erase(it);
                    break;
                }
            }
            return;
            //结束当前线程
        }
        //else if (buffer[0] == 'I' && buffer[1] == 'I' && buffer[2] == 'D' && buffer[3] == 'D' && buffer[4] == ':')
        //    //检验
        //{
        //    sscanf(buffer, "IIDD:%s", name);
        //    cout << name << endl;
        //    continue;
        //}
        //向所有客户端发送信息

        for (SOCKET client : clients)
        {
            sprintf(message, "<%s>: %s", name, buffer);
            //sprintf(message, "%s", buffer);
            //cout << message << endl;
            //if (client != clientSocket)
            //{
            send(client, message, strlen(message), 0);
            //}
        }
        cout << message << endl;//在服务器上打印

        // input.assign(buffer, bytesReceived);

    }
}

int main()
{
    if (mysql == NULL)
    {
        printf("mysql_init() error\n");
        return -1;
    }

    // 连接数据库服务器 
    mysql = mysql_real_connect(mysql, "IP地址", "MySQL用户", "MySQL密码",
        "clientroom_test", 0, NULL, 0);
    if (mysql == NULL)
    {
        printf("mysql_real_connect() error\n");
        return -1;
    }

    printf("mysql api使用的默认编码: %s\n", mysql_character_set_name(mysql));

    // 设置编码为utf8
    mysql_set_character_set(mysql, "utf8");

    printf("mysql api使用的修改之后的编码: %s\n", mysql_character_set_name(mysql));

    printf("连接数据库服务器成功\n");


    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        cerr << "WSAStartup failed." << endl;
        return 1;
    }

    //创建服务端socket
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET)
    {
        cerr << "Error creating server socket." << endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(PORT);

    //绑定socket到地址和端口
    if (bind(serverSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR)
    {
        cerr << "Error binding to port " << PORT << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    //开始监听
    if (listen(serverSocket, MAX_CLIENTS) == SOCKET_ERROR)
    {
        cerr << "Error listening on port " << PORT << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    cout << "Server started. Listening on port " << PORT << endl;

    //等待并处理客户端连接
    while (true)
    {
        sockaddr_in clientAddr;
        int clientAddrSize = sizeof(clientAddr);
        SOCKET clientSocket = accept(serverSocket, reinterpret_cast<sockaddr*>(&clientAddr), &clientAddrSize);
        if (clientSocket == INVALID_SOCKET)
        {
            cerr << "Error accepting client connection." << endl;
            continue;
        }
        clients.push_back(clientSocket);
        thread(handleClient, clientSocket).detach();
    }

    //清理
    closesocket(serverSocket);
    WSACleanup();
    return 0;
}