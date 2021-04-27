
#ifndef FTPCLIENT_H // include guard
#define FTPCLIENT_H

#include <cstdint>
#include "tcpsocket.h"
#include <iostream>
// #include "FTPClientCLI.h"
#include "cli.h"

class FTPClient;

// using callback = void (CmdLineInterface::*)(std::string);
// typedef void (CmdLineInterface::*callback)(std::string);


    class FTPClient{
       protected:
            TcpSocket socketControl;
            TcpSocket socketData;

            bool connected;
            string hostname;
        
        public:
            FTPClient();
            void connect(const string &hostname, const string &port, CmdLineInterface *callback);
            void send_string_request(TcpSocket &socket, const string &request);
            void set_host_name(string hostname);
            void echo(const string &msg);
            void login(CmdLineInterface *callback);
            void get_list_file();

            bool is_connected();
            // int recvDataBuffer(char* buffer, unsigned int bufLen);
            int recv_data_buffer(TcpSocket &socket, char* buffer, unsigned int bufLen);
            void close();

            string get_host_name();
            string parse_epsv_response();
            /*
                if you want to using callback function
                
                the first:
                    using callback = void (*)([param1],[param2]);
                    void login(callback fnc);
            */
    };

#endif