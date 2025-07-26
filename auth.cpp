#include "auth.hpp"
#include<algorithm>
#include <functional>
using namespace std ;
 unordered_map<string , string> users ;
 string simpleHash(string pass){
    hash<string> hasher ;
    return to_string(hasher(pass)) ;
 }
void load_user() {
    ifstream file("users.txt") ;
  
    string line ;
    while(getline(file , line)) {
      stringstream ss(line) ;
      string user , pass ;
       getline(ss , user , '|') ;
       getline(ss , pass)  ;
       users[user] = pass ;
    }
} 

void load_friend(){
    ifstream file("friends.txt");
    string line ;
    while(getline(file , line)){ 
        size_t colon_pos = line.find(':') ;
        if(colon_pos == string::npos) continue ;
        string user = line.substr(0 , colon_pos) ;
        string friend_list_txt = line.substr(colon_pos+1) ;

        stringstream ss(friend_list_txt) ;
        string hashed_friend ;
        while(getline(ss , hashed_friend , ',')){
            friend_list[user].insert(hashed_friend) ;
        }


    } 
    file.close() ;
    ifstream hin("hashed_map.txt");
    size_t hash ;
    string username ;
    while(hin>>hash>>username) {
         reverse_hash_map[hash] = username;
    }
    hin.close() ;
}

void save_friend(){
    ofstream file("friends.txt") ;
    for(auto &pair : friend_list){
        file <<pair.first <<":" ;
        bool first = true ;
        for(const auto& hashed_friend: pair.second ){
            if(!first) file<<",";
            file<<hashed_friend ;
            first = false ;
        }
        file<<"\n" ;
    } 
    ofstream hout("hashed_map.txt") ;
    for(auto & p : reverse_hash_map){
        hout<<p.first <<" "<<p.second <<"\n" ;
    }
}
void save_user(const string& user , const string& pass){
    ofstream file("users.txt" , ios::app) ;
    file<<user<<"|"<<pass<<"\n" ;
} 


string get_username(string user){
    return user ;
}

void add_friend(const string &currusername, const string &friendusername) {
    if (currusername == friendusername) {
        cout << "It's your own username.\n";
        return;
    }

    hash<string> hasher;
    size_t hashed_friend = hasher(friendusername);
    size_t hashed_curr = hasher(currusername);

    // Optional: Store reverse hash to username
    reverse_hash_map[hashed_friend] = friendusername;
    reverse_hash_map[hashed_curr] = currusername;

    auto& curr_friends = friend_list[currusername];
    if (curr_friends.find(to_string(hashed_friend)) != curr_friends.end()) {
        cout << friendusername << " is already in your friend list.\n";
        return;
    }

    // Add friend hashes
    curr_friends.insert(to_string(hashed_friend));
    friend_list[friendusername].insert(to_string(hashed_curr));
     save_friend();
    cout << "Friend added successfully.\n";
}

bool login(string & name ){
    string username , pass ;
    cout<<"Enter Your Username : "; cin>>username ;
    cout<<endl ;
    cout<<"Enter Your Password : " ; cin>>pass ;
    if(users.count(username) && users[username] == simpleHash(pass)){
        cout << "Login Successful!\n";
      name = username ;

     
        return true ;

    } 
    else {
        cout<<"Invalid Username or Password "<<endl ;  
        return false ;
    }
}

void register_user(){
    string user , pass ;
   cout<<"Choose Username : " ;
   cin>>user ;
   cout<<endl ;
   cout<<"Choose Password : " ;
   cin>>pass ;
   if(users.count(user)){
    cout<<" This Username already exist"<<endl ;
   }  
   else {
    users[user] = simpleHash(pass) ;
    save_user(user ,users[user]) ;
      cout << "Registered Successfully!\n";


   }
}

void print_friend_list(const string& username) {
    auto& st = friend_list[username];
    if (st.empty()) {
        cout << "No friends found.\n";
        return;
    }
    cout << "Friend list for " << username << ":\n";
    for (const string& h : st) {
        size_t hash_value = stoull(h);
        if (reverse_hash_map.count(hash_value)) {
            cout << "  " << reverse_hash_map[hash_value] << endl;
        } else {
            cout << "  (Unknown Friend Hash: " << h << ")\n";
        }
    }
}




void build_graph(){
    graph.clear() ;
    for(auto & [user , friends]: friend_list){
        for(auto & hashed_friend : friends){
            string friend_user = reverse_hash_map[stoull(hashed_friend)] ;
            graph[user].insert(friend_user) ;
            graph[friend_user].insert(user) ;
        }
    }
}

void suggest_friend(const string & username){
    if(graph.empty()) build_graph() ;
    unordered_set<string> direct_friend = graph[username] ;
    unordered_map<string , int> suggestion_count ;
    for(auto & friend_user : graph[username]){
        for(auto & fof: graph[friend_user]){
            if(fof == username) continue ;
            if(direct_friend.count(fof)) continue;
            suggestion_count[fof]++ ;
        }
    }

    vector<pair<string , int>> suggestions(suggestion_count.begin() , suggestion_count.end()) ;
    sort(suggestions.begin() , suggestions.end() , []( auto a , auto b){
        return a.second > b.second ;

    }) ;
    if(suggestions.empty()){
        cout<<"No Suggested Friend Found "<<endl ;

    } else {
        cout<<"suggested friend for "<< username<< endl ;
        cout<<"User ------->  Their Mutuals with Other Friends \n" ;
        for(auto [user , mutuals] : suggestions){
            cout<< user <<" ---> "<< mutuals <<endl ;
        }
    }


}