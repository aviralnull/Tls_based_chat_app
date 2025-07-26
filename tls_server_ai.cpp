#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <thread>
#include <mutex>
#include<algorithm>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "libssl.lib")
#pragma comment(lib, "libcrypto.lib")

using namespace std;

mutex clients_mutex;

struct ClientInfo {
    SSL* ssl;
    SOCKET socket;
    string name;
    sockaddr_in address;
};

vector<ClientInfo> clients;
SSL_CTX* ssl_ctx;

inline void init_openssl() {
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
}

void cleanup_openssl() {
    EVP_cleanup();
}

SSL_CTX* create_context() {
    const SSL_METHOD* method = TLS_server_method();
    SSL_CTX* ctx = SSL_CTX_new(method);
    if (!ctx) {
        cerr << "Unable to create SSL context" << endl;
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
    return ctx;
}

void configure_context(SSL_CTX* ctx) {
    SSL_CTX_set_ecdh_auto(ctx, 1);

    if (SSL_CTX_use_certificate_file(ctx, "server.crt", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, "server.key", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
}

void broadcast_message(const string& message, SSL* sender = nullptr) {
    lock_guard<mutex> guard(clients_mutex);
    for (const auto& client : clients) {
        if (client.ssl != sender) {
            SSL_write(client.ssl, message.c_str(), message.size());
        }
    }
}

void handle_client(SSL* ssl, SOCKET client_socket) {
    char buffer[1024];
    while (true) {
        int bytes_received = SSL_read(ssl, buffer, sizeof(buffer));
        if (bytes_received <= 0) {
            int err = SSL_get_error(ssl, bytes_received);
            if (err == SSL_ERROR_ZERO_RETURN || err == SSL_ERROR_SYSCALL) {
                // Client disconnected
                lock_guard<mutex> guard(clients_mutex);
                auto it = find_if(clients.begin(), clients.end(), 
                    [ssl](const ClientInfo& ci) { return ci.ssl == ssl; });
                if (it != clients.end()) {
                    cout << "Client " << it->name << " disconnected" << endl;
                    closesocket(it->socket);
                    SSL_free(it->ssl);
                    clients.erase(it);
                }
            }
            break;
        }

        buffer[bytes_received] = '\0';
        string message(buffer);

        // Broadcast or process private message
        if (message.substr(0, 4) == "/pm ") {
            // Handle private message
            size_t space_pos = message.find(' ', 4);
            if (space_pos != string::npos) {
                string recipient = message.substr(4, space_pos - 4);
                string pm_content = message.substr(space_pos + 1);

                lock_guard<mutex> guard(clients_mutex);
                for (const auto& client : clients) {
                    if (client.name == recipient) {
                        string sender_name;
                        auto sender_it = find_if(clients.begin(), clients.end(), 
                            [ssl](const ClientInfo& ci) { return ci.ssl == ssl; });
                        if (sender_it != clients.end()) {
                            sender_name = sender_it->name;
                        }
                        string pm = "[PM from " + sender_name + "]: " + pm_content;
                        
                        SSL_write(client.ssl, pm.c_str(), pm.size());
                        break;
                    }
                }
            }
        } 
        else {
            // Broadcast message
            string sender_name;
            {
                lock_guard<mutex> guard(clients_mutex);
                auto it = find_if(clients.begin(), clients.end(), 
                    [ssl](const ClientInfo& ci) { return ci.ssl == ssl; });
                if (it != clients.end()) {
                    sender_name = it->name;
                }
            }
            string broadcast_msg = sender_name + ": " + message;
            broadcast_message(broadcast_msg, ssl);
        }
    }
}

int main() {
    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "WSAStartup failed" << endl;
        return 1;
    }

    // Initialize OpenSSL
    init_openssl();
    ssl_ctx = create_context();
    configure_context(ssl_ctx);

    // Create server socket
    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_socket == INVALID_SOCKET) {
        cerr << "Socket creation failed" << endl;
        WSACleanup();
        return 1;
    }

    // Bind socket
    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8888);

    if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        cerr << "Bind failed" << endl;
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    // Listen
    if (listen(server_socket, SOMAXCONN) == SOCKET_ERROR) {
        cerr << "Listen failed" << endl;
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    cout << "Server started on port 8888" << endl;

    while (true) {
        // Accept new connection
        sockaddr_in client_addr;
        int client_addr_len = sizeof(client_addr);
        SOCKET client_socket = accept(server_socket, (sockaddr*)&client_addr, &client_addr_len);
        if (client_socket == INVALID_SOCKET) {
            cerr << "Accept failed" << endl;
            continue;
        }

        // Create SSL connection
        SSL* ssl = SSL_new(ssl_ctx);
        SSL_set_fd(ssl, client_socket);

        if (SSL_accept(ssl) <= 0) {
            ERR_print_errors_fp(stderr);
            SSL_free(ssl);
            closesocket(client_socket);
            continue;
        }

        // Get client name
        char name_buffer[256];
        int name_len = SSL_read(ssl, name_buffer, sizeof(name_buffer));
        if (name_len <= 0) {
            SSL_free(ssl);
            closesocket(client_socket);
            continue;
        }
        name_buffer[name_len] = '\0';

        // Add client to list
        ClientInfo client_info;
        client_info.ssl = ssl;
        client_info.socket = client_socket;
        client_info.name = string(name_buffer);
        client_info.address = client_addr;

        {
            lock_guard<mutex> guard(clients_mutex);
            clients.push_back(client_info);
        }

        cout << "New client connected: " << client_info.name << endl;

        // Start client handler thread
        thread(handle_client, ssl, client_socket).detach();
    }

    // Cleanup
    closesocket(server_socket);
    SSL_CTX_free(ssl_ctx);
    cleanup_openssl();
    WSACleanup();

    return 0;
}