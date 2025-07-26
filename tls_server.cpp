#include <iostream>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <winsock2.h>
// #include <openssl/applink.c>


#pragma comment(lib, "ws2_32.lib")

void init_openssl() {
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
        std::cerr << "Unable to create SSL context\n";
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
    return ctx;
}

void configure_context(SSL_CTX* ctx) {
    // Load cert and private key
    if (SSL_CTX_use_certificate_file(ctx, "server.crt", SSL_FILETYPE_PEM) <= 0 ||
        SSL_CTX_use_PrivateKey_file(ctx, "server.key", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
}

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2,2), &wsaData);

    init_openssl();
    SSL_CTX* ctx = create_context();
    configure_context(ctx);

    SOCKET server_fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(4433);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_fd, (sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 1);

    std::cout << "TLS server listening on port 4433...\n";

    while (true) {
        SOCKET client_fd = accept(server_fd, nullptr, nullptr);
        SSL* ssl = SSL_new(ctx);
        SSL_set_fd(ssl, (int)client_fd);

        if (SSL_accept(ssl) <= 0) {
            ERR_print_errors_fp(stderr);
        } else {
            char buffer[1024] = {0};
            SSL_read(ssl, buffer, sizeof(buffer));
            std::cout << "Received: " << buffer << std::endl;

            const char reply[] = "Hello from TLS server!";
            SSL_write(ssl, reply, strlen(reply));
        }

        SSL_shutdown(ssl);
        SSL_free(ssl);
        closesocket(client_fd);
    }

    closesocket(server_fd);
    SSL_CTX_free(ctx);
    cleanup_openssl();
    WSACleanup();
    return 0;
}


//  g++ tls_server.o applink.o -o tls_server.exe `                           
// >>   -L"C:/msys64/mingw64/bin" `
// >>   -L"C:/Users/harsh/Downloads/boost_1_88_0/boost_1_88_0/stage/lib" `
// >>   -lssl -lcrypto -lws2_32 -lwsock32 `
// >>   -lboost_system-mgw15-mt-x64-1_88