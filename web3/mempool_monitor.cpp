#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <nlohmann/json.hpp>
#include <iostream>
#include <string>

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;
using tcp = boost::asio::ip::tcp;
using json = nlohmann::json;

int main() {
    try {
        // Your specific Alchemy connection details
        std::string host = "eth-mainnet.g.alchemy.com";
        std::string port = "443";
        // Using the API key you generated earlier
        std::string path = "/v2/WXVextenuyecpevTAhdpO";

        net::io_context ioc;
        ssl::context ctx{ssl::context::tlsv12_client};

        tcp::resolver resolver{ioc};
        websocket::stream<beast::ssl_stream<tcp::socket>> ws{ioc, ctx};

        auto const results = resolver.resolve(host, port);
        auto ep = net::connect(get_lowest_layer(ws), results);

        if(! SSL_set_tlsext_host_name(ws.next_layer().native_handle(), host.c_str()))
            throw beast::system_error(
                beast::error_code(static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()),
                "Failed to set SNI Hostname");

        ws.next_layer().handshake(ssl::stream_base::client);

        ws.set_option(websocket::stream_base::decorator(
            [](websocket::request_type& req) {
                req.set(http::field::user_agent,
                    std::string(BOOST_BEAST_VERSION_STRING) + " mempool-monitor");
            }));

        ws.handshake(host, path);

        // Tell the Ethereum node to send us every new pending transaction
        json sub_msg = {
            {"jsonrpc", "2.0"},
            {"id", 1},
            {"method", "eth_subscribe"},
            {"params", {"newPendingTransactions"}}
        };
        
        ws.write(net::buffer(sub_msg.dump()));
        std::cout << "[+] Connected to Ethereum Mainnet! Waiting for live transactions...\n";

        // Listen to the live stream infinitely
        while(true) {
            beast::flat_buffer buffer;
            ws.read(buffer);
            
            // Parse the incoming JSON data
            std::string raw_data = beast::buffers_to_string(buffer.data());
            json parsed = json::parse(raw_data);
            
            std::cout << "Live TX Hash Detected: " << parsed.dump() << "\n-------------------\n";
        }
    }
    catch(std::exception const& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}