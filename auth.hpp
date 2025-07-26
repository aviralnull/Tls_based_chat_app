#pragma once 
#include<string>
#include<iostream>
#include<string>
#include<fstream>
#include<unordered_map>
#include<sstream>
#include<unordered_set>
using namespace std ;
inline unordered_map<string , unordered_set<string>> friend_list ;
inline unordered_map<size_t, string> reverse_hash_map;
inline  unordered_map<string , unordered_set<string>> graph ;
void load_user() ;
void load_friend() ;
void save_friend() ;
void register_user() ;
bool login(string &s) ;
string get_username(string user) ;
void print_friend_list(const string& username) ;
void add_friend(const string & currusername , const string& friendusername ) ;
void build_graph() ;
void suggest_friend(const string & username) ;
// string get_loggedin_user(const string& user) ;  