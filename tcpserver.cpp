#include "tcpserver.hpp"

TCPServer::TCPServer(unsigned port, std::function<void(TCPClient&)> callback)
    :port(port), callback(callback)
{
    // �������������� ���������� �������
    int tmp = WSAStartup(MAKEWORD(2, 2), &wsadata);

    // �������� �������� ��� ������� ������
    char sName[128];
    gethostname(sName, sizeof(sName));
    printf("Server host: %s\n", sName);

    if ((S = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
    {
        fprintf(stderr, "Can't create socket\n");
        exit(1);
    }

    // ��������� ��������� �������
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    // ��������� ������ �� ���� ��������� ������� �����������, � ��������� �� localhost
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons((u_short)port);
    // ��������� ����� � �������� ������� ���������� � ������
    if (bind(S, (sockaddr*)&serv_addr, sizeof(serv_addr)) == INVALID_SOCKET)
    {
        fprintf(stderr, "Can't bind\n");
        exit(1);
    }

    // ��������� ����� � ����� ������������� ��������� �����
    // � ������������ ����������� �������� �������� �� ���������� 5
    if (listen(S, 5) == INVALID_SOCKET)
    {
        fprintf(stderr, "Can't listen\n");
        exit(1);
    }
}

TCPServer::~TCPServer()
{
    // ��������� ��������� �����
    closesocket(S);
    // ����������� ������� ���������� �������
    WSACleanup();
}


void TCPServer::start()
{
    printf("Server listen on %s:%d\n", inet_ntoa(serv_addr.sin_addr), ntohs(serv_addr.sin_port));

    while (true)
    {
        sockaddr_in clnt_addr;
        int addrlen = sizeof(clnt_addr);
        memset(&clnt_addr, 0, sizeof(clnt_addr));
        // ��������� ������ � ����� �������� ������� �� ����������.
        // ����� ����������, �.�. ���������� ���������� ������ ��� 
        // ����������� ������� ��� ������ 
        NS = accept(S, (sockaddr*)&clnt_addr, &addrlen);
        //std::count << "HERE" << std::endl;
        if (NS == INVALID_SOCKET)
        {
            fprintf(stderr, "Can't accept connection\n");
            break;
        }
        // �������� ��������� ��������������� ������ NS �
        // ���������� � �������
        addrlen = sizeof(serv_addr);
        getsockname(NS, (sockaddr*)&serv_addr, &addrlen);
        // ������� inet_ntoa ���������� ��������� �� ���������� ������, 
        // ������� ������������ �� � ����� ������ printf �� ���������

        this->print_mutex.lock();
        printf("Accepted connection on %s:%d ", inet_ntoa(serv_addr.sin_addr), ntohs(serv_addr.sin_port));
        printf("from client %s:%d\n", inet_ntoa(clnt_addr.sin_addr), ntohs(clnt_addr.sin_port));
        this->print_mutex.unlock();

        this->clients.push_back(std::make_shared<TCPClient>(NS, clnt_addr));
        std::shared_ptr<std::thread> tr{ std::make_shared<std::thread>(&TCPServer::client_loop, this, *std::prev(this->clients.end())) };
        this->threads.push_back(tr);
        tr->detach();

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void TCPServer::client_loop(std::shared_ptr<TCPClient> client)
{
    while (true)
    {
        bool err;
        std::string data;
        data = client->get_data(err);

        if (err)
            break;

        if (data.length() == 0) continue;

        this->callback(*client);
    }
    // ��������� �������������� �����
    this->print_mutex.lock();
    printf("Client disconnected.\n");
    this->print_mutex.unlock();
}


TCPServer::TCPClient::TCPClient(SOCKET S, sockaddr_in cli_addr)
    : S(S), cli_addr(cli_addr)
{
}

TCPServer::TCPClient::~TCPClient()
{
    closesocket(S);
}

const std::string& TCPServer::TCPClient::get_data(bool& err)
{
    char sReceiveBuffer[buf_size] = { 0 };
    int nReaded = recv(S, sReceiveBuffer, buf_size - 1, 0);
    if (nReaded <= 0)
    {
        err = true;
    }
    else
    {
        err = false;
    }

    sReceiveBuffer[nReaded] = 0;
    // ����������� ������� ������� �����
    for (char* pPtr = sReceiveBuffer; *pPtr != 0; pPtr++)
    {
        if (*pPtr == '\n' || *pPtr == '\r')
        {
            *pPtr = 0;
            break;
        }
    }

    this->data = sReceiveBuffer;
    return this->data;
}

const std::string& TCPServer::TCPClient::get_data()
{
    return this->data;
}

void TCPServer::TCPClient::send_data(const std::string& text)
{
    send(this->S, text.c_str(), text.length(), 0);
}
