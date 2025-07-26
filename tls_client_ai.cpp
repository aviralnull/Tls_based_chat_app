#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <thread>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "libssl.lib")
#pragma comment(lib, "libcrypto.lib")

using namespace std;

SSL* ssl;
string client_name;

inline void init_openssl() {
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
}

void cleanup_openssl() {
    EVP_cleanup();
}

SSL_CTX* create_context() {
    const SSL_METHOD* method = TLS_client_method();
    SSL_CTX* ctx = SSL_CTX_new(method);
    if (!ctx) {
        cerr << "Unable to create SSL context" << endl;
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
    return ctx;
}

void receive_messages() {
    char buffer[1024];
    while (true) {
        int bytes_received = SSL_read(ssl, buffer, sizeof(buffer));
        if (bytes_received <= 0) {
            int err = SSL_get_error(ssl, bytes_received);
            if (err == SSL_ERROR_ZERO_RETURN || err == SSL_ERROR_SYSCALL) {
                cout << "Disconnected from server" << endl;
            }
            break;
        }

        buffer[bytes_received] = '\0';
        cout << buffer << endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cout << "Usage: " << argv[0] << " <server_ip> <name>" << endl;
        return 1;
    }

    string server_ip = argv[1];
    client_name = argv[2];

    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "WSAStartup failed" << endl;
        return 1;
    }

    // Initialize OpenSSL
    init_openssl();
    SSL_CTX* ssl_ctx = create_context();

    // Create socket
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        cerr << "Socket creation failed" << endl;
        WSACleanup();
        return 1;
    }

    // Connect to server
    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8888);
    inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr);

    if (connect(sock, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        cerr << "Connection failed" << endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    // Create SSL connection
    ssl = SSL_new(ssl_ctx);
    SSL_set_fd(ssl, sock);

    if (SSL_connect(ssl) <= 0) {
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    // Send client name
    SSL_write(ssl, client_name.c_str(), client_name.size());

    cout << "Connected to server as " << client_name << endl;
    cout << "Type /pm <name> <message> to send private message" << endl;
    cout << "Type /quit to exit" << endl;

    // Start receiver thread
    thread receiver(receive_messages);
    receiver.detach();

    // Handle user input
    string input;
    while (true) {
        getline(cin, input);

        if (input == "/quit") {
            break;
        }

        SSL_write(ssl, input.c_str(), input.size());
    }

    // Cleanup
    SSL_shutdown(ssl);
    SSL_free(ssl);
    closesocket(sock);
    SSL_CTX_free(ssl_ctx);
    cleanup_openssl();
    WSACleanup();

    return 0;
}