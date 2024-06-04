/*
*����������������������������������������������������������������������������������������������������
*������    �������������ҷ����
*������    �ߣ�Nathan_Guozi
*������    ����1.3
*��������ʱ�䣺2024��2��3��
*����������������������������������������������������������������������������������������������������
*/


#define _CRT_SECURE_NO_WARNINGS
//��ȫ����
#pragma warning(disable : 4996)
//
#include <iostream>
//��׼���������ͷ�ļ�
#include <vector>
//��������
#include <winsock2.h>
//Winsock2������
#include <ws2tcpip.h>
//�����ַת��
#include <thread>
//�̴߳���ͷ�ļ�
#include <time.h>
#include "include/mysql.h"
//mysqlͷ�ļ�
#include <string.h>
#pragma comment(lib, "ws2_32.lib")//����Winsock2��
#pragma comment(lib, "lib/libmysql.lib") // MySQL��

using namespace std;

const int PORT = 9999;
const int MAX_CLIENTS = 10;
//���ͻ�����
vector<SOCKET> clients;//�ͻ���SOCKET����





// ��ʼ��mysql���ӻ���
MYSQL* mysql = mysql_init(NULL);

//��¼���
bool loginserver(const std::string& username, const std::string& password) {
    const char* sql = "select * from users";

    // ִ�����sql���
    int ret = mysql_query(mysql, sql);
    //Ҳ��mysql_query(mysql, "select * from users");

    //���
    if (ret != 0)
    {
        printf("mysql_query() aʧ����, ԭ��: %s\n", mysql_error(mysql));
        return -1;
    }

    // ȡ���û���
    MYSQL_RES* res = mysql_store_result(mysql);
    if (res == NULL)
    {
        printf("mysql_store_result() ʧ����, ԭ��: %s\n", mysql_error(mysql));
        return -1;
    }

    // �õ�������е�����
    int num = mysql_num_fields(res);

    //��ȡ����
    MYSQL_FIELD* fields = mysql_fetch_fields(res);

    //����ÿһ�в�������Ƚ�
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)) != NULL)
    {
        for (int i = 0; i < num; i = i + 2)
        {
            //std::string ���Ϳ���ͨ�� c_str() ��������һ��ָ�����ڲ� const char* ��������ָ��
            if (strcmp(username.c_str(), row[i]) == 0 && strcmp(password.c_str(), row[i + 1]) == 0)
            {
                return 1;
            }
        }
    }
    mysql_free_result(res);
    return 0;
}

//ע����
bool registercheck(const std::string& username) {
    const char* sql = "select * from users";

    // ִ�����sql���
    int ret = mysql_query(mysql, sql);
    //Ҳ��mysql_query(mysql, "select * from users");

    //���
    if (ret != 0)
    {
        printf("mysql_query() aʧ����, ԭ��: %s\n", mysql_error(mysql));
        return 0;
    }

    // ȡ���û���
    MYSQL_RES* res = mysql_store_result(mysql);
    if (res == NULL)
    {
        printf("mysql_store_result() ʧ����, ԭ��: %s\n", mysql_error(mysql));
        return 0;
    }

    // �õ�������е�����
    int num = mysql_num_fields(res);

    //��ȡ����
    MYSQL_FIELD* fields = mysql_fetch_fields(res);

    //����ÿһ�в�������Ƚ�
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)) != NULL)
    {
        for (int i = 0; i < num; i = i + 2)
        {
            //std::string ���Ϳ���ͨ�� c_str() ��������һ��ָ�����ڲ� const char* ��������ָ��
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
        // ��¼�ɹ�����ͻ��˷���ȷ����Ϣ
        send(clientSocket, "LOGIN_SUCCESS", strlen("LOGIN_SUCCESS"), 0);
    }
    else {
        // ��¼ʧ�ܣ���ͻ��˷���ʧ����Ϣ
        send(clientSocket, "LOGIN_FAILED", strlen("LOGIN_FAILED"), 0);
    }
}

// ע�ᴦ��
void handleRegister(SOCKET clientSocket, const char* newUsername, const char* newPassword)
{
    // ��дSQL�����в������
    char insertQuery[1000];
    sprintf(insertQuery, "INSERT INTO users(username, password) VALUES ('%s', '%s')", newUsername, newPassword);

    // ִ��ע�����

    if (registercheck(newUsername) == 1)
    {
        send(clientSocket, "REGISTER_FAILED:username is haved", strlen("REGISTER_FAILED:username is haved"), 0);
    }
    else if (mysql_query(mysql, insertQuery) == 0)
    {
        // ע��ɹ�����ͻ��˷���ȷ����Ϣ
        send(clientSocket, "REGISTER_SUCCESS", strlen("REGISTER_SUCCESS"), 0);
    }
    else
    {
        // ע��ʧ��ԭ�򣬿�������Ϊ�û����Ѵ��ڵȣ���ͻ��˷���ʧ����Ϣ
        char REGISTER_FAILED_REASON[1000];
        sprintf_s(REGISTER_FAILED_REASON, "REGISTER_FAILED:%s", mysql_error(mysql));

        printf("ע��ʧ��ԭ��%s\n", REGISTER_FAILED_REASON);

        send(clientSocket, REGISTER_FAILED_REASON, strlen(REGISTER_FAILED_REASON), 0);
    }
}


void handleClient(SOCKET clientSocket)//����ͻ�������
{
    cout << "����������" << endl;
    char buffer[102400];
    char name[99];
    char message[102416];
    std::string input;
    char* action = NULL;
    char* username = NULL;
    char* password = NULL;
    //int nameReceived = recv(clientSocket, name, sizeof(name), 0);
    //cout << '<' << name << '>' << "������" << endl;
//���ܿͻ�����Ϣ
    while (true)
    {
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        //���©��---------------------------------���©��----------------------------------���©��
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
        //���©��---------------------------------���©��----------------------------------���©��
        if (bytesReceived <= 0)
            //����ʧ�ܻ����ӹر�
        {
            closesocket(clientSocket);
            //cout << '<' << name << '>' << "�˳���" << endl;
            cout << "�����˳���" << endl;
            for (auto it = clients.begin(); it != clients.end(); ++it)
            {
                if (*it == clientSocket)
                {
                    clients.erase(it);
                    break;
                }
            }
            return;
            //������ǰ�߳�
        }
        //else if (buffer[0] == 'I' && buffer[1] == 'I' && buffer[2] == 'D' && buffer[3] == 'D' && buffer[4] == ':')
        //    //����
        //{
        //    sscanf(buffer, "IIDD:%s", name);
        //    cout << name << endl;
        //    continue;
        //}
        //�����пͻ��˷�����Ϣ

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
        cout << message << endl;//�ڷ������ϴ�ӡ

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

    // �������ݿ������ 
    mysql = mysql_real_connect(mysql, "IP��ַ", "MySQL�û�", "MySQL����",
        "clientroom_test", 0, NULL, 0);
    if (mysql == NULL)
    {
        printf("mysql_real_connect() error\n");
        return -1;
    }

    printf("mysql apiʹ�õ�Ĭ�ϱ���: %s\n", mysql_character_set_name(mysql));

    // ���ñ���Ϊutf8
    mysql_set_character_set(mysql, "utf8");

    printf("mysql apiʹ�õ��޸�֮��ı���: %s\n", mysql_character_set_name(mysql));

    printf("�������ݿ�������ɹ�\n");


    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        cerr << "WSAStartup failed." << endl;
        return 1;
    }

    //���������socket
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

    //��socket����ַ�Ͷ˿�
    if (bind(serverSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR)
    {
        cerr << "Error binding to port " << PORT << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    //��ʼ����
    if (listen(serverSocket, MAX_CLIENTS) == SOCKET_ERROR)
    {
        cerr << "Error listening on port " << PORT << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    cout << "Server started. Listening on port " << PORT << endl;

    //�ȴ�������ͻ�������
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

    //����
    closesocket(serverSocket);
    WSACleanup();
    return 0;
}