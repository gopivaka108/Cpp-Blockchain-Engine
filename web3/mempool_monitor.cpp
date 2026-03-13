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
#include <cctype>

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;
using tcp = boost::asio::ip::tcp;
using json = nlohmann::json;

int main() {
    try {
        std::string host = "eth-mainnet.g.alchemy.com";
        std::string port = "443";
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

        json sub_msg = {
            {"jsonrpc", "2.0"},
            {"id", 1},
            {"method", "eth_subscribe"},
            {"params", {"newPendingTransactions"}}
        };
        
        ws.write(net::buffer(sub_msg.dump()));
        std::cout << "[+] Connected to Ethereum Mainnet! Waiting for live transactions...\n";

        while(true) {
            beast::flat_buffer buffer;
            ws.read(buffer);
            
            std::string raw_data = beast::buffers_to_string(buffer.data());
            json parsed = json::parse(raw_data);
            
            if (parsed.contains("method") && parsed["method"] == "eth_subscription") {
                std::string tx_hash = parsed["params"]["result"];

                json tx_req = {
                    {"jsonrpc", "2.0"},
                    {"id", 2},
                    {"method", "eth_getTransactionByHash"},
                    {"params", {tx_hash}}
                };

                ws.write(net::buffer(tx_req.dump()));
                
                json tx_details;
                while(true) {
                    beast::flat_buffer tx_buffer;
                    ws.read(tx_buffer);
                    tx_details = json::parse(beast::buffers_to_string(tx_buffer.data()));
                    if (tx_details.contains("id") && tx_details["id"] == 2) break;
                }

                if (tx_details.contains("result") && !tx_details["result"].is_null() && !tx_details["result"]["to"].is_null()) {
                    std::string to_address = tx_details["result"]["to"];
                    std::string uniswap_v2 = "0x7a250d5630b4cf539739df2c5dacb4c659f2488d";
                    std::string uniswap_universal = "0x3fc91a3afd70395cd496c647d5a6cc9d4b2b7fad";

                    for(auto& c : to_address) { c = std::tolower(c); }
                    
                    std::cout << "." << std::flush; 

                    if (to_address == uniswap_v2 || to_address == uniswap_universal) {
                        std::string gas_hex = "0x0";
                        if (!tx_details["result"]["gasPrice"].is_null()) {
                            gas_hex = tx_details["result"]["gasPrice"];
                        }
                        
                        uint64_t gas_decimal = 0;
                        if(gas_hex.length() > 2) {
                            gas_decimal = std::stoull(gas_hex.substr(2), nullptr, 16);
                        }

                        std::string input_data = "None";
                        if (!tx_details["result"]["input"].is_null()) {
                            input_data = tx_details["result"]["input"];
                        }

                        std::cout << "\n\n[!] UNISWAP TRADE DETECTED [!]\n";
                        std::cout << "    Hash:        " << tx_hash << "\n";
                        std::cout << "    Gas (Wei):   " << gas_decimal << "\n";
                        std::cout << "    Method ID:   " << input_data.substr(0, 10) << "\n";
                        std::cout << "--------------------------------------\n";
                    }
                }
            }
        }
    }
    catch(std::exception const& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}