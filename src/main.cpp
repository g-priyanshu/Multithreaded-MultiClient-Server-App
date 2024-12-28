
#include <boost/asio.hpp>
#include <boost/beast/websocket.hpp>
#include <iostream>
#include <thread>
#include <map>
#include <mutex>
#include <boost/beast/core.hpp>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace asio = boost::asio;
using tcp = asio::ip::tcp;
using namespace std;

map<string, websocket::stream<tcp::socket> *> clients;
mutex client_mutex;

string to_lowercase(const string &s)
{
    string lower_s = s;
    transform(lower_s.begin(), lower_s.end(), lower_s.begin(), ::tolower);
    return lower_s;
}

void log_event(const string &msg)
{
    time_t now = time(0);
    string time_str = ctime(&now);
    time_str.pop_back();
    cout << "[" << time_str << "] " << msg << endl;
}

pair<string, string> parse_message(const string &message)
{
    int space_pos = message.find(' ');
    if (message[0] == '@' && space_pos != string::npos)
    {
        string target = message.substr(1, space_pos - 1);
        string act_msg = message.substr(space_pos + 1);
        return {target, act_msg};
    }
    return {"", message};
}

class WebSocketSession
{
public:
    explicit WebSocketSession(tcp::socket socket) : ws_(std::move(socket)) {}

    void start()
    {
        ws_.accept();

        string client_name;
        beast::flat_buffer buffer;
        ws_.read(buffer);
        client_name = beast::buffers_to_string(buffer.data());
        client_name = to_lowercase(client_name);

        {
            lock_guard<mutex> lock(client_mutex);
            if (clients.count(client_name))
            {
                ws_.write(asio::buffer("Name already taken. Disconnecting..."));
                log_event("Client tried to connect with duplicate name: " + client_name);
                return;
            }
            clients[client_name] = &ws_;
        }

        log_event("Client connected with name: " + client_name);
        ws_.write(asio::buffer("Welcome to the chat, " + client_name + "!"));

        while (true)
        {
            buffer.clear();
            ws_.read(buffer);
            string client_msg = beast::buffers_to_string(buffer.data());

            if (client_msg == "exit")
            {
                log_event("Client '" + client_name + "' disconnected");
                break;
            }

            auto [target_name, act_msg] = parse_message(client_msg);

            if (to_lowercase(target_name) == "all")
            {
                broadcast(client_name, act_msg);
            }
            else if (!target_name.empty())
            {
                sendDataToTarget(target_name, act_msg, client_name);
            }
            else
            {
                ws_.write(asio::buffer("Invalid target. Use @<receiver name> <your message>"));
            }
        }

        {
            lock_guard<mutex> lock(client_mutex);
            clients.erase(client_name);
        }
    }

private:
    websocket::stream<tcp::socket> ws_;

    void sendDataToTarget(const string &targetname, const string &message, const string &sender)
    {
        lock_guard<mutex> lock(client_mutex);
        auto it = clients.find(to_lowercase(targetname));
        if (it != clients.end() && it->second)
        {
            log_event("Sending private message to " + targetname);
            it->second->write(asio::buffer("@" + sender + " sent: " + message));
        }
        else
        {
            log_event("Target client '" + targetname + "' not found or disconnected.");
            auto sender_it = clients.find(to_lowercase(sender));
            if (sender_it != clients.end() && sender_it->second)
            {
                sender_it->second->write(asio::buffer("Error: Target client '" + targetname + "' not found."));
            }
        }
    }

    void broadcast(const string &sender, const string &message)
    {
        lock_guard<mutex> lock(client_mutex);
        for (const auto &client : clients)
        {
            if (client.first != to_lowercase(sender))
            {
                client.second->write(asio::buffer(sender + " (broadcast): " + message));
            }
        }
    }
};

class WebSocketServer
{
public:
    WebSocketServer(short port) : acceptor_(ioc_, tcp::endpoint(tcp::v4(), port)) {}

    void start()
    {
        log_event("Server started on port 8080");

        while (true)
        {
            tcp::socket socket(ioc_);
            acceptor_.accept(socket);
            log_event("Client connected from IP: " + socket.remote_endpoint().address().to_string() +
                      ":" + to_string(socket.remote_endpoint().port()));

            WebSocketSession *session = new WebSocketSession(std::move(socket));
            std::thread(&WebSocketSession::start, session).detach(); // Start session in its own thread
        }
    }

private:
    asio::io_context ioc_;
    tcp::acceptor acceptor_;
};

int main()
{
    try
    {
        WebSocketServer server(8080);
        server.start();
    }
    catch (const std::exception &e)
    {
        log_event("Server error: " + string(e.what()));
    }

    return 0;
}
