#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <iostream>

using boost::asio::ip::tcp;

int main(int argc, char* argv[])
{
    if (argc != 2) {
        std::cerr << "Usage: server <port>\n";
        return 1;
    }

    boost::asio::io_service io_service;

    tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), std::atoi(argv[1])));

    std::cout << "Server started on port " << argv[1] << std::endl;

    for (;;) {
        tcp::socket socket(io_service);
        acceptor.accept(socket);

        boost::asio::streambuf request;
        boost::asio::read_until(socket, request, "\r\n");

        std::istream request_stream(&request);
        std::string request_line;
        std::getline(request_stream, request_line);

        std::cout << "Received request: " << request_line << std::endl;

        std::string method, path, http_version;
        request_stream >> method >> path >> http_version;

        if (method != "GET") {
            boost::asio::streambuf response;
            std::ostream response_stream(&response);
            response_stream << "HTTP/1.1 405 Method Not Allowed\r\n";
            response_stream << "Content-Length: 0\r\n";
            response_stream << "Connection: close\r\n\r\n";
            boost::asio::write(socket, response);
        } else {
            boost::system::error_code ec;
            boost::filesystem::path file_path("files" + path);
            if (!boost::filesystem::exists(file_path)) {
                boost::asio::streambuf response;
                std::ostream response_stream(&response);
                response_stream << "HTTP/1.1 404 Not Found\r\n";
                response_stream << "Content-Length: 0\r\n";
                response_stream << "Connection: close\r\n\r\n";
                boost::asio::write(socket, response);
            } else {
                boost::asio::streambuf response;
                std::ostream response_stream(&response);
                response_stream << "HTTP/1.1 200 OK\r\n";
                response_stream << "Content-Length: " << boost::filesystem::file_size(file_path) << "\r\n";
                response_stream << "Connection: close\r\n\r\n";
                boost::asio::write(socket, response);

                std::ifstream file(file_path.string(), std::ios::binary);
                std::vector<char> buffer(4096);
                while (file.read(buffer.data(), buffer.size())) {
                    boost::asio::write(socket, boost::asio::buffer(buffer));
                }
                if (file.gcount() > 0) {
                    boost::asio::write(socket, boost::asio::buffer(buffer.data(), file.gcount()));
                }
            }
        }
    }
}

