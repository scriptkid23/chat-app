#include "FTPClient.h"

FTPClient::FTPClient()
{
    connected = false;
    isLogin = false;
};

void FTPClient::connect(const string &hostname, const string &port, CmdLineInterface *callback)
{
  
        char buffer[256];

        socketControl.connect(hostname, port);
        Response res = get_receive_socket_control();
        set_host_name(hostname);

        callback->set_hostname(hostname);
        callback->setCmdPrompt(callback->getPrompt());

        this->connected = true;

}
void FTPClient::close(CmdLineInterface *callback)
{
    try
    {
        socketControl.close();
    }
    catch (SocketException &e)
    {
        cerr << "ERROR: " << e.what() << endl;
    }
    this->connected = false;
    this->isLogin = false;
    callback->set_hostname("");
    callback->set_user("");
    callback->setCmdPrompt("62pm2@spirity> ");
}
bool FTPClient::is_connected()
{
    return this->connected;
}
bool FTPClient::is_login()
{
    return this->isLogin;
}

void FTPClient::set_host_name(string hostname)
{
    this->hostname = hostname;
}

string FTPClient::get_host_name()
{
    return this->hostname;
}

void FTPClient::login(const string &username, const string &password, CmdLineInterface *callback)
{

    if (!is_connected())
        throw SocketException("You should connect!");

    std::string request;
    Response res;

    request = "USER " + username + "\r\n";

    socketControl.send(request);

    res = get_receive_socket_control();


    if (res.getCode() == "331")
    {
        request = "PASS " + password + "\r\n";

        socketControl.send(request);
        res = get_receive_socket_control();
        if (res.getCode() == "230")
        {
            callback->set_user(username);
            callback->set_current_working_directory(get_present_working_directory());
            isLogin = true;
            callback->setCmdPrompt(callback->getPrompt());
        }
        else
        {
            callback->set_user("");
            callback->set_current_working_directory("");
            callback->setCmdPrompt(callback->getPrompt());

            isLogin = false;
            throw CustomizeException(res.toString());
        }
    }
    else
    {
        throw CustomizeException(res.toString());
    }
}
string FTPClient::parse_epsv_response()
{
    socketControl.send("EPSV\r\n");

    Response res = get_receive_socket_control();

    std::regex rx(R"([[:digit:]]+)");
    std::smatch m;
    std::string str = res.getMessage();
    regex_search(str, m, rx);

    return m[0];
}
string FTPClient::get_receive_socket_data()
{
    int bytes;
    char buffer[255];
    string result;
    while (true)
    {
        if ((bytes = socketData.recv(buffer, 255)) == -1)
        {
            std::cout << "recv error: " << strerror(errno) << std::endl;
            exit(1);
        }
        if (bytes == 0)
        {
            break;
        }
        for (int i = 0; i < bytes; i++)
        {
            result += buffer[i];
        }
    }
    return result;
}
Response FTPClient::get_receive_socket_control()
{

    char buffer[500];
    int bytes = socketControl.recv(buffer, 500);
    buffer[bytes] = 0;

    return Extensions::convert_buffer_to_response(buffer);
};
string FTPClient::get_list_file()
{
    if (!is_connected() && !is_login())
        throw SocketException("You should connect and login!");

    Response res;

    string port = parse_epsv_response();

    socketData.connect(hostname, port);

    socketControl.send("NLST\r\n");

    // Beacause response of NLST return 2 response
    get_receive_socket_control();
    get_receive_socket_control();

    // get result from socket data
    string response = get_receive_socket_data();

    // close socket data
    socketData.close();
    return response;
}
string FTPClient::get_present_working_directory()
{
    if (!is_connected() && !is_login())
        throw SocketException("You should connect and login!");

    socketControl.send("PWD\r\n");
    string response = get_receive_socket_control().getMessage();
    string result = "";
    for (int i = response.find_first_of('"') + 1; i < response.find_last_of('"'); i++)
    {
        result += response[i];
    }
    return result;
};
string FTPClient::get_directory()
{
    if (!is_connected() && !is_login())
        throw SocketException("You should connect and login!");

    string port = parse_epsv_response();

    socketData.connect(hostname, port);

    socketControl.send("LIST\r\n");

    get_receive_socket_control();
    get_receive_socket_control();
    string res = get_receive_socket_data();

    socketData.close();
    return res;
}
void FTPClient::download(const string &filename)
{
    if (!is_connected() && !is_login())
        throw SocketException("You should connect and login!");

    Response res;

    FILE *file;

    string src = "./download/" + filename;

    char *source = const_cast<char *>((src).c_str());
    file = fopen(source, "wb");
    if (!file)
    {
        throw CustomizeException("You should create folder 'download'");
    }

    string port = parse_epsv_response();

    socketData.connect(hostname, port);

    socketControl.send("TYPE I\r\n");
    res = get_receive_socket_control();
    if (res.getCode() != "200")
        throw CustomizeException(res.toString());

    string request = "RETR " + filename + "\r\n";
    socketControl.send(request);

    res = get_receive_socket_control();
    if (res.getCode() == "550")
    {

        socketData.close();
        throw CustomizeException(res.toString());
    }
    get_receive_socket_control();

    SetConsoleTextAttribute(console, COLOR_PRIMARY);
    cout << "INFO: Download File: " << filename << " ..." << endl;
    SetConsoleTextAttribute(console, COLOR_DEFAULT);

    int bytes;
    char buffer[4096];
    while (true)
    {
        if ((bytes = socketData.recv(buffer, 4096)) == -1)
        {
            std::cout << "recv error: " << strerror(errno) << std::endl;
        }
        if (bytes == 0)
        {
            SetConsoleTextAttribute(console, COLOR_PRIMARY);
            std::cout << "INFO: Download File succeeded." << std::endl;
            SetConsoleTextAttribute(console, COLOR_DEFAULT);
            break;
        }
        fwrite(buffer, bytes, 1, file);
    }
    fclose(file);
    socketData.close();
}
void FTPClient::upload(const string &source)
{
    if (!is_connected() && !is_login())
        throw SocketException("You should connect and login!");

    Response res;
    std::ifstream file(source, std::ios::in | std::ios::binary | std::ios::ate);

    if (file)
    {
        long length = file.tellg();
        file.seekg(0, file.beg);

        string port = parse_epsv_response();

        socketData.connect(hostname, port);

        socketControl.send("TYPE I\r\n");
        res = get_receive_socket_control();
        if (res.getCode() != "200")
            throw CustomizeException(res.toString());

        string filename = Extensions::get_file_name(source);
        string request = "STOR " + filename + "\r\n";

        socketControl.send(request);
        res = get_receive_socket_control();

        if (res.getCode() != "150")
        {    
            socketData.close();
            throw CustomizeException(res.toString());
        }

        SetConsoleTextAttribute(console, COLOR_PRIMARY);
        std::cout << "INFO: Sending File : " << source << " ..." << std::endl;
        SetConsoleTextAttribute(console, COLOR_DEFAULT);

        string data;
        double c_length = length;

        while (length > 0)
        {
            int read_sz = 2048 < length ? 2048 : length;
            char buffer[2048 + 1];

            file.read(buffer, read_sz);
            data.assign(buffer, read_sz);

            socketData.send(data);
            length -= read_sz;
        }
        socketData.close();
        res = get_receive_socket_control();
        if (res.getCode() == "226")
        {
            SetConsoleTextAttribute(console, COLOR_PRIMARY);
            cout << "INFO: Upload file succeeded." << endl;
            SetConsoleTextAttribute(console, COLOR_DEFAULT);
        }
        else
        {
            SetConsoleTextAttribute(console, COLOR_ERROR);
            cout << "INFO: Upload file failed." << endl;
            SetConsoleTextAttribute(console, COLOR_DEFAULT);
        }
        file.close();
    }
    else
    {
        throw CustomizeException("Doesn't exist. Please check the filename.");
    }
}
void FTPClient::change_current_working_directory(const string &directory, CmdLineInterface *callback)
{
    //TODO: code;
    socketControl.send("CWD " + directory + "\r\n");
    Response res = get_receive_socket_control();

    if (res.getCode() == "550")
    {
        throw CustomizeException(res.toString());
    }
    callback->set_current_working_directory(get_present_working_directory());
    callback->setCmdPrompt(callback->getPrompt());
}
void FTPClient::delete_directory(const string &directory)
{
    //TODO: code;
    socketControl.send("XRMD " + directory + "\r\n");
    Response res = get_receive_socket_control();
    if (res.getCode() != "250")
    {
        throw CustomizeException(res.toString());
    }
}
void FTPClient::create_directory(const string &directory)
{
    //TODO: code;
    socketControl.send("XMKD " + directory + "\r\n");
    Response res = get_receive_socket_control();
    if (res.getCode() != "257")
    {
        throw CustomizeException(res.toString());
    }
}
void FTPClient::delete_file(const string &directory)
{
    //TODO: code;
    socketControl.send("DELE " + directory + "\r\n");
    Response res = get_receive_socket_control();
    if (res.getCode() != "250")
    {
        throw CustomizeException(res.toString());
    }
}
void FTPClient::rename_directory_or_file(const string &src, const string &dest)
{
    //TODO: code;

    socketControl.send("RNFR " + src + "\r\n");
    Response res = get_receive_socket_control();
    if (res.getCode() != "350")
    {
        throw CustomizeException(res.toString());
    }
    socketControl.send("RNTO " + dest + "\r\n");
    res = get_receive_socket_control();
    if (res.getCode() != "250")
    {
        throw CustomizeException(res.toString());
    }
}
