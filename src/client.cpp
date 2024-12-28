
#include <boost/asio.hpp>
#include <boost/beast/websocket.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <boost/beast/core.hpp>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace asio = boost::asio;
using tcp = asio::ip::tcp;

std::atomic<bool> connected{false};

void read_message(websocket::stream<tcp::socket> &ws)
{
    try
    {
        beast::flat_buffer buffer;
        while (true)
        {
            ws.read(buffer);
            std::string message = beast::buffers_to_string(buffer.data());
            std::cout << "\n"
                      << message << "\n"
                      << std::endl;
            buffer.clear();
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error receiving message: " << e.what() << std::endl;
        connected = false;
    }
}

int main()
{
    try
    {
        asio::io_context ioc;
        tcp::resolver resolver(ioc);
        websocket::stream<tcp::socket> ws(ioc);

        // Resolve and connect to server
        auto const results = resolver.resolve("127.0.0.1", "8080");
        asio::connect(ws.next_layer(), results.begin(), results.end());
        ws.handshake("localhost", "/");

        // Prompt for the name input
        std::string name;
        std::cout << "Enter your name: " << std::endl;
        std::getline(std::cin, name);

        // Send the name to the server
        ws.write(asio::buffer(name));

        // Start receiving messages in a separate thread
        std::thread(read_message, std::ref(ws)).detach();

        connected = true;
        std::string message;
        std::cout << "Enter your message: use <@reciever_name message or @all message for brodcast" << std::endl;
        while (connected)
        {
            std::getline(std::cin, message);
            if (message == "exit")
            {
                connected = false;
                break;
            }

            // Send the message to the server
            ws.write(asio::buffer(message));
        }

        ws.close(websocket::close_code::normal);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}
