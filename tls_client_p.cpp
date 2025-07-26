#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <thread>
#include "auth.hpp" 

using namespace std ;
SSL* ssl ;
string client_name ;

void init_openssl(){
    SSL_load_error_strings() ;
    OpenSSL_add_ssl_algorithms() ;

} 
void cleanup_openssl(){
    EVP_cleanup() ;

} 
SSL_CTX* create_context() {
    const SSL_METHOD* method = TLS_client_method() ; 
    SSL_CTX * ctx = SSL_CTX_new(method) ;
    if(!ctx) {
         cerr << "Unable to create SSL context" << endl;
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    } 
     return ctx ;
} 
void receive_message(){
    char buffer[1024] ;
    while(true) {
        int byte_received = SSL_read(ssl ,  buffer , sizeof(buffer)) ;
        if(byte_received <= 0) {
            int err = SSL_get_error(ssl , byte_received);
            if(err== SSL_ERROR_ZERO_RETURN || err== SSL_ERROR_SYSCALL){
                cout<<" disconnected from server " <<endl ;
            } 
            break ;
        } 
        buffer[byte_received] = '\0' ;
        cout<< buffer <<endl ;
    }
} 
 int main(int argc , char* argv[]){
    if(argc != 2){
          cout << "Usage: " << argv[0] << " <server_ip> <name>" << endl;
          return 1 ;
    } 
    
    string server_ip = argv[1];
    // client_name = argv[2] ;
      string name_of_client ;
 
     load_user() ;
     load_friend() ;
    int choice ;
    bool isLoggedIn = false;
    while(true){
        cout<<"1. Register New Account \n2. Login \n3. Exit \n  Enter Your Choice \n" ;
        cin>>choice ;
        // getline.ignore() ;
        if(choice == 1) {
           register_user()  ;

        }
        else if(choice == 2) {
            if(login(name_of_client )) {
                cout<<endl ;
                isLoggedIn = true ;
                while(true){
                int subchoice ;
                  cout<<"1. Do You Want To Add Friend \n2. Do You Want To See Suggested Friend List \n3. Enter in Chat Section \n\n  Enter Your Choice \n" ;
                  cin>>subchoice ;
                  cout<<endl ;
                  if(subchoice == 1){
                    cout<<" Enter Your Friend's username \n" ;
                    string friendusername ;
                    cin>>friendusername ;
                    add_friend(name_of_client , friendusername ) ;

                  } 
                  else if(subchoice == 2){
                    
                    print_friend_list(name_of_client) ;

                    cout<<endl ;
                    suggest_friend(name_of_client) ;
                  } 
                  else {
                      break;
                  } } 
                
                break ;
            } ;
        } 
        else {
            
            break ;
        }
    }
// 🔐 Block server connect unless logged in
if (!isLoggedIn) {
    cerr << "User not logged in. Exiting...\n";
    return 1;
}
   client_name = name_of_client ;

     WSADATA ws ;
   if(  WSAStartup(MAKEWORD(2 , 2 ) , &ws) != 0){
    cerr<<"ws creation failed " <<endl;
    return 1 ;
   } 
    init_openssl();
    SSL_CTX* ssl_ctx = create_context() ;
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);  
     if (sock == INVALID_SOCKET) {
        cerr << "Socket creation failed" << endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in srv ;
    srv.sin_family = AF_INET ;
    srv.sin_port = htons(8888) ;
   inet_pton(AF_INET , server_ip.c_str() , &srv.sin_addr) ;

   if(connect(sock , (sockaddr*)&srv , sizeof(srv)) == SOCKET_ERROR) {
    cerr<<"Connection Failed  " <<endl ;
    closesocket(sock) ;
    WSACleanup() ;
    return 1 ;
   } 

   ssl = SSL_new(ssl_ctx) ;
   SSL_set_fd(ssl, sock) ;
   if(SSL_connect(ssl) <= 0){
     ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        closesocket(sock);
        WSACleanup();
        return 1;
   } 
    SSL_write(ssl , client_name.c_str() ,  client_name.size()) ;
    cout<<"Connected to server  as " << client_name <<endl ;
    cout<<"Type /pm <name> message  to send private message " <<endl ;
    cout<<"Type /quit to exit "<<endl ; 
     thread receiver(receive_message) ;
     receiver.detach() ;

     string input ;
     while(true) {
        getline(cin , input) ;
        if(input == "/quit") {
 break ;
        }
        SSL_write(ssl ,input.c_str() , input.size() ) ;

     } 
      SSL_shutdown(ssl) ;
      SSL_free(ssl) ;
      closesocket(sock) ;
      SSL_CTX_free(ssl_ctx) ;
      cleanup_openssl() ;
      WSACleanup() ;


 }