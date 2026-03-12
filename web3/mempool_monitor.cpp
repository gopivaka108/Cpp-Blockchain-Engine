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
            
            std::string raw_data = beast::buffers_to_string(buffer.data());
            json parsed = json::parse(raw_data);
            
            // 1. Check if the message is a new pending transaction notification
            if (parsed.contains("method") && parsed["method"] == "eth_subscription") {
                std::string tx_hash = parsed["params"]["result"];

                // 2. Build the request to fetch the full transaction details
                json tx_req = {
                    {"jsonrpc", "2.0"},
                    {"id", 2},
                    {"method", "eth_getTransactionByHash"},
                    {"params", {tx_hash}}
                };

                // 3. Send the request
                ws.write(net::buffer(tx_req.dump()));
                
// 4. Read responses until we find the actual transaction details (ignore queued hashes)
                json tx_details;
                while(true) {
                    beast::flat_buffer tx_buffer;
                    ws.read(tx_buffer);
                    tx_details = json::parse(beast::buffers_to_string(tx_buffer.data()));
                    
                    // Break this inner loop only when we see the response to our specific request
                    if (tx_details.contains("id") && tx_details["id"] == 2) {
                        break;
                    }
                }

// 5. Extract data and filter for Uniswap V2 trades
                if (tx_details.contains("result") && !tx_details["result"].is_null() && !tx_details["result"]["to"].is_null()) {
                    
                    std::string to_address = tx_details["result"]["to"];
                    std::string uniswap_v2 = "0x7a250d5630b4cf539739df2c5dacb4c659f2488d";

                    for(auto& c : to_address) { c = tolower(c); }

                    // Heartbeat: print a dot for every single transaction scanned
                    std::cout << "." << std::flush; 

if (to_address == uniswap_v2) {
                        // Safely handle gas price conversion
                        std::string gas_hex = "0x0";
                        if (!tx_details["result"]["gasPrice"].is_null()) {
                            gas_hex = tx_details["result"]["gasPrice"];
                        }
                        
                        uint64_t gas_decimal = 0;
                        if(gas_hex.length() > 2) {
                            gas_decimal = std::stoull(gas_hex.substr(2), nullptr, 16);
                        }

                        // Extract the raw input data (the trade payload)
                        std::string input_data = "None";
                        if (!tx_details["result"]["input"].is_null()) {
                            input_data = tx_details["result"]["input"];
                        }

                        std::cout << "\n\n[!] UNISWAP TRADE DETECTED [!]\n";
                        std::cout << "    Hash:        " << tx_hash << "\n";
                        std::cout << "    Gas (Wei):   " << gas_decimal << "\n";
                        // Print the first 10 characters (the function selector)
                        std::cout << "    Method ID:   " << input_data.substr(0, 10) << "\n";
                        std::cout << "--------------------------------------\n";
                    }
                }
            }
        }
    }
    catch(std::exception const& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}