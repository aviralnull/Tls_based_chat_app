#End To End Encrypted Chat Server 

# Compile Client file to executable file
 g++ tls_client_p.cpp auth.cpp  -o tls_client_p2.exe -lssl -lcrypto -lssl -lcrypto -lws2_32 -lwsock32 



   # Compile Server file to  executable file 
g++ tls_server_p.cpp -o tls_server_p.exe -Llib -Iinclude -lssl -lcrypto  -lws2_32 -lwsock32 


   
# Now  run 
./tls_server_p 
./tls_client_p2

#These are based on my file paths  update this accordingly 
