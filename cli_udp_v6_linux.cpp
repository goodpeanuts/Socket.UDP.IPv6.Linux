#include <iostream>
#include <fstream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>

#define PORT 65035
#define BUFFER_SIZE 1024

using namespace std;

int main() {
    // 建立客户端数据报套接字
    int sock_client = socket(AF_INET6, SOCK_DGRAM, 0);
    if (sock_client == -1) {
        cerr << "套接字创建失败" << endl;
        return 1;
    }

    // 设置服务器地址
    sockaddr_in6 server_addr{};
    server_addr.sin6_family = AF_INET6;
    server_addr.sin6_port = htons(PORT);
    if (inet_pton(AF_INET6, "fd15:4ba5:5a2b:1008:1c65:f815:7213:e616", &(server_addr.sin6_addr)) != 1) {
        cerr << "服务器地址设置失败" << endl;
        close(sock_client);
        return 1;
    }

    // 发送连接请求
    if (sendto(sock_client, "Connect", strlen("Connect"), 0, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1) {
        cerr << "连接请求发送失败" << endl;
        close(sock_client);
        return 1;
    }

    // 接收连接成功消息
    char buffer[BUFFER_SIZE];
    socklen_t addr_len = sizeof(server_addr);
    int size = recvfrom(sock_client, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &server_addr, &addr_len);
    if (size == 0) {
        cout << "连接失败" << endl;
        close(sock_client);
        return 0;
    } else {
        cout << "服务器消息: " << buffer << endl;
    }

    // 发送申请服务端传输的文件的文件名
    string filename;
    cout << "请输入要发送的文件名：";
    cin >> filename;

    size = sendto(sock_client, filename.c_str(), filename.length(), 0, (struct sockaddr *) &server_addr, sizeof(server_addr));
    if (size == -1) {
        cerr << "发送文件名失败，错误代码：" << errno << endl;
        close(sock_client);
        return 1;
    }

    // 接收服务端将发送的文件的大小
    streampos fileSize;
    size = recvfrom(sock_client, (char *) &fileSize, sizeof(fileSize), 0, (struct sockaddr *) &server_addr, &addr_len);
    if (size == 0) {
        cout << "与服务器断开连接" << endl;
        close(sock_client);
        return 0;
    } else {
        cout << "接收到的文件大小为: " << fileSize << " bytes" << endl;
    }

    // 创建文件
    ofstream file("received_" + filename, ios::binary);
    if (!file) {
        cerr << "文件创建失败" << endl;
        close(sock_client);
        return 1;
    }

    // 接收文件内容
    int remainingSize = fileSize;
    while (remainingSize > 0) {
        size = recvfrom(sock_client, buffer, min(remainingSize, BUFFER_SIZE), 0, (struct sockaddr *) &server_addr, &addr_len);
        if (size == 0) {
            cout << "对方已关闭连接" << endl;
            break;
        } else if (size == -1) {
            cerr << "接收文件内容失败，错误代码：" << errno << endl;
            break;
        } else {
            file.write(buffer, size);
            remainingSize -= size;
        }
    }

    file.close();
    cout << "文件接收完成，保存为: received_" << filename << endl;

    // 关闭套接字
    close(sock_client);

    return 0;
}

