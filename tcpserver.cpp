#include "tcpserver.hpp"

TCPServer::TCPServer(unsigned port, std::function<void(TCPClient&)> callback)
    :port(port), callback(callback)
{
    // Инициализируем библиотеку сокетов
    int tmp = WSAStartup(MAKEWORD(2, 2), &wsadata);

    // Пытаемся получить имя текущей машины
    char sName[128];
    gethostname(sName, sizeof(sName));
    printf("Server host: %s\n", sName);

    if ((S = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
    {
        fprintf(stderr, "Can't create socket\n");
        exit(1);
    }

    // Заполняем структуру адресов
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    // Разрешаем работу на всех доступных сетевых интерфейсах, в частности на localhost
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons((u_short)port);
    // Связываем сокет с заданным сетевым интерфесом и портом
    if (bind(S, (sockaddr*)&serv_addr, sizeof(serv_addr)) == INVALID_SOCKET)
    {
        fprintf(stderr, "Can't bind\n");
        exit(1);
    }

    // Переводим сокет в режим прослушивания заданного порта
    // с максимальным количеством ожидания запросов на соединение 5
    if (listen(S, 5) == INVALID_SOCKET)
    {
        fprintf(stderr, "Can't listen\n");
        exit(1);
    }
}

TCPServer::~TCPServer()
{
    // Закрываем серверный сокет
    closesocket(S);
    // освобождаем ресурсы библиотеки сокетов
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
        // Переводим сервис в режим ожидания запроса на соединение.
        // Вызов синхронный, т.е. возвращает управление только при 
        // подключении клиента или ошибке 
        NS = accept(S, (sockaddr*)&clnt_addr, &addrlen);
        //std::count << "HERE" << std::endl;
        if (NS == INVALID_SOCKET)
        {
            fprintf(stderr, "Can't accept connection\n");
            break;
        }
        // Получаем параметры присоединенного сокета NS и
        // информацию о клиенте
        addrlen = sizeof(serv_addr);
        getsockname(NS, (sockaddr*)&serv_addr, &addrlen);
        // Функция inet_ntoa возвращает указатель на глобальный буффер, 
        // поэтому использовать ее в одном вызове printf не получится

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
    // закрываем присоединенный сокет
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
    // Отбрасываем символы превода строк
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
