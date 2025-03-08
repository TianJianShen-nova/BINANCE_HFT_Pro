#include <websocketpp/config/asio_client.hpp> 
#include <websocketpp/client.hpp>
#include <iomanip>
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <nlohmann/json.hpp> 
#include <mutex>
#include <string>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/crypto.h> // 添加此头文件
#include <iomanip> 
#include <curl/curl.h>
#include <ctime>
#include <sys/time.h>

using namespace std::chrono;
// 使用 nlohmann::json 命名空间
using json = nlohmann::json;

typedef websocketpp::client<websocketpp::config::asio_tls_client> client; // 使用 TLS 客户端配置

class HQ_FX{
    public:
        HQ_FX();
        std::string listenKey;
        void yanchang();
    private:
        void run();
        static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* data);
};

HQ_FX::HQ_FX(){
    HQ_FX::run();
}


size_t HQ_FX::WriteCallback(void* contents, size_t size, size_t nmemb, std::string* data) {
    data->append((char*)contents, size * nmemb);
    return size * nmemb;
}

void HQ_FX::run() {
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "Failed to initialize cURL" << std::endl;
        return;
    }

    std::string url = "https://fapi.binance.com/fapi/v1/listenKey";
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); 
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    // 添加自定义 Headers
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "X-MBX-APIKEY:xxxxxxxxxxxxxxxxxxxxxxxxxxx");//APIKEY
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "");
    // 设置响应数据回调
    std::string response_data;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, HQ_FX::WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);

    // 执行请求
    //std::cout<<HQ_FX::symbol<<std::endl;
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::cerr << "cURL error: " << curl_easy_strerror(res) << std::endl;
    } else {
        //std::cout << "Response: " << response_data << std::endl;
        json obj_arg = json::parse(response_data);
        HQ_FX::listenKey = obj_arg["listenKey"];
    }

    // 清理资源
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    
}

void HQ_FX::yanchang() {
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "Failed to initialize cURL" << std::endl;
        return;
    }

    std::string url = "https://fapi.binance.com/fapi/v1/listenKey";
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); 
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    // 添加自定义 Headers
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "X-MBX-APIKEY:xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");//APIKEY
    headers = curl_slist_append(headers,"Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // 设置自定义请求为 PUT
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");

    // 设置 POST 数据（PUT 请求的数据通过此选项设置）
    json json_;
    json_["listenKey"] = HQ_FX::listenKey;
    std::string json_data = json_.dump();
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, json_data.length());
    // 设置响应数据回调
    std::string response_data;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, HQ_FX::WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);

    // 执行请求
    //std::cout<<HQ_FX::symbol<<std::endl;
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::cerr << "cURL error: " << curl_easy_strerror(res) << std::endl;
    } else {
        std::cout << "Response: " << response_data << std::endl;
        //json obj_arg = json::parse(response_data);
        //HQ_FX::listenKey = obj_arg["listenKey"];
    }

    // 清理资源
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    
}

class Books5{
    public: 
        Books5();
        std::atomic<double> ask1_price;
        std::atomic<double> bid1_price;
        void wait();
    private:
        std::atomic<bool> is_connected; 
        std::atomic<bool> should_reconnect; 
        std::string wss_url;
        client cli;
        void on_message(client* c, websocketpp::connection_hdl hdl, client::message_ptr msg);
        void on_close(client* c, websocketpp::connection_hdl hdl);
        void on_open(client* c, websocketpp::connection_hdl hdl);
        void on_fail(client* c, websocketpp::connection_hdl hdl);
        void run();
        std::thread t;
};

void Books5::wait(){
    // 等待 ASIO 线程结束
    Books5::t.join();
}

Books5::Books5(){
    // 定义并初始化静态成员变量
    Books5::ask1_price.store(0.0, std::memory_order_relaxed);;
    Books5::bid1_price.store(0.0, std::memory_order_relaxed);;
    Books5::is_connected.store(false, std::memory_order_relaxed);; // 标记当前连接状态
    Books5::should_reconnect.store(true, std::memory_order_relaxed);; // 标记是否需要重连
    Books5::wss_url = "wss://fstream.binance.com/ws/xrpusdt@depth5@100ms";
    Books5::run();
}

void Books5::on_message(client* c, websocketpp::connection_hdl hdl, client::message_ptr msg) {
    //std::cout<<msg->get_payload()<<std::endl;
    try {
        // 解析 JSON 字符串
        json obj_arg = json::parse(msg->get_payload());

        if (obj_arg["e"] == "depthUpdate") {
            

            // 获取 asks 数组
            json asks = obj_arg["a"];

            // 获取 asks 数组的第一个元素
            json ask1 = asks[0];

            // 解析 ask 价格
            std::string ask_str = ask1[0].get<std::string>();
            double ask_price = std::stod(ask_str);

            // 获取 bids 数组
            json bids = obj_arg["b"];

            // 获取 bids 数组的第一个元素
            json bid1 = bids[0];

            // 解析 bid 价格
            std::string bid_str = bid1[0].get<std::string>();
            double bid_price = std::stod(bid_str);

            // 更新 Main 类中的静态成员变量
            
            Books5::ask1_price.store(ask_price, std::memory_order_relaxed); // 存储数据
            Books5::bid1_price.store(bid_price, std::memory_order_relaxed); // 存储数据
            /* code */
            //std::cout<<"ask:"<<ask1_price.load(std::memory_order_relaxed)<<"  ";
            //std::cout<<"bid:"<<bid1_price.load(std::memory_order_relaxed)<<std::endl;
        } else {
            // 如果 channel 不是 "books5"，可以在这里处理其他逻辑
            // std::cout << "eee" << std::endl;
        }
    } catch (const std::exception& e) {
        // 捕获并处理 JSON 解析错误
        std::cerr << "JSON parsing error: " << e.what() << std::endl;
        std::cout<<msg->get_payload()<<std::endl;
    }
}

void Books5::on_close(client* c, websocketpp::connection_hdl hdl) {
    std::cout << "OrderBook Connection closed." << std::endl;
    Books5::is_connected = false;

    if (Books5::should_reconnect) {
        std::cout << "OrderBook Attempting to reconnect..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5)); // 等待 5 秒后重连

        websocketpp::lib::error_code ec;
        client::connection_ptr con = c->get_connection(Books5::wss_url, ec);
        if (ec) {
            std::cout << "OrderBook Reconnect error: " << ec.message() << std::endl;
            return;
        }

        // 设置连接超时时间（例如 5 秒）
        con->set_open_handshake_timeout(2000); // 5 秒

        c->connect(con);
    }
}

void Books5::on_open(client* c, websocketpp::connection_hdl hdl) {
    std::cout << "OrderBook Connection opened." << std::endl;
    Books5::is_connected = true;

    // 每次连接成功后发送一条消息
    websocketpp::lib::error_code ec;
    c->send(hdl, "{\"method\":\"SUBSCRIBE\",\"params\": [\"xrpusdt@depth5@100ms\"],\"id\":1}", websocketpp::frame::opcode::text,ec);
    if (ec) {
        std::cout << "OrderBook Send failed: " << ec.message() << std::endl;
    } else {
        std::cout << "OrderBook Sent message: OK"<< std::endl;
    }
}

void Books5::on_fail(client* c, websocketpp::connection_hdl hdl) {
    std::cout << "OrderBook Connection failed or timed out." << std::endl;
    Books5::is_connected = false;

    if (Books5::should_reconnect) {
        std::cout << "OrderBook Attempting to reconnect..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5)); // 等待 5 秒后重连

        websocketpp::lib::error_code ec;
        client::connection_ptr con = c->get_connection(Books5::wss_url, ec);
        if (ec) {
            std::cout << "OrderBook Reconnect error: " << ec.message() << std::endl;
            return;
        }

        // 设置连接超时时间（例如 5 秒）
        con->set_open_handshake_timeout(2000); // 5 秒

        c->connect(con);
    }
}


std::string timestampToString() {
    // 获取当前时间点
    auto now = std::chrono::system_clock::now();
    // 将时间点转换为 time_t
    std::time_t timestamp = std::chrono::system_clock::to_time_t(now);

    // 将 time_t 转换为本地时间的 tm 结构体
    struct tm localTime;
    localtime_r(&timestamp, &localTime);

    // 使用 stringstream 格式化时间
    std::ostringstream oss;
    oss << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");

    return oss.str();
}

void Books5::run() {
    

    // 设置日志级别（可选）
    cli.clear_access_channels(websocketpp::log::alevel::all);
    cli.clear_error_channels(websocketpp::log::elevel::all);

    // 初始化 ASIO
    cli.init_asio();

    // 注册消息回调
    cli.set_message_handler([this](websocketpp::connection_hdl hdl, client::message_ptr msg) {
        Books5::on_message(&cli, hdl, msg);
    });

    // 注册连接关闭回调
    cli.set_close_handler([this](websocketpp::connection_hdl hdl) {
        Books5::on_close(&cli, hdl);
    });

    // 注册连接打开回调
    cli.set_open_handler([this](websocketpp::connection_hdl hdl) {
        Books5::on_open(&cli, hdl);
    });

    // 注册连接失败回调
    cli.set_fail_handler([this](websocketpp::connection_hdl hdl) {
        Books5::on_fail(&cli, hdl);
    });

    // 设置 TLS 初始化回调
    cli.set_tls_init_handler([](websocketpp::connection_hdl) {
        // 创建一个新的 TLS 上下文
        return websocketpp::lib::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::tlsv12);
    });

    // 创建连接
    websocketpp::lib::error_code ec;
    client::connection_ptr con = cli.get_connection(Books5::wss_url, ec);
    if (ec) {
        std::cout << "OrderBook Connection error: " << ec.message() << std::endl;
        return ;
    }

    // 设置连接超时时间（例如 5 秒）
    con->set_open_handshake_timeout(5000); // 5 秒
    // 连接服务器
    cli.connect(con);

    // 启动 ASIO 事件循环（在单独的线程中运行）
    Books5::t = std::thread([this]() {
        cli.run();
    });

    return ;
}

class Order{
    public: 
        Order();
        void post_order_short_sell(std::string px);
        void post_order_short_buy(std::string px);
        void cancel_order(long _id);
        void wait();
    private:
        std::atomic<bool> is_connected; 
        std::atomic<bool> should_reconnect; 
        
        std::string wss_url;
        client cli;
        void on_message(client* c, websocketpp::connection_hdl hdl, client::message_ptr msg);
        void on_close(client* c, websocketpp::connection_hdl hdl);
        void on_open(client* c, websocketpp::connection_hdl hdl);
        void on_fail(client* c, websocketpp::connection_hdl hdl);
        std::string generateSignature(const std::string& data);
        long long getTimestamp();
        std::thread t;
        client::connection_ptr con;
        void print(std::string msg);    
};

void Order::print(std::string msg){
    auto now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);
    
    // 跨平台线程安全处理
    std::tm tm;
    localtime_r(&time, &tm); // Linux/macOS

    std::string a = "buy";
    std::string b = "sell";
    std::string c = "open";
    std::string d = "cancel";
    if(msg.find(c)!=std::string::npos){
        if(msg.find(a)!=std::string::npos){
            std::cout<<"\033[31m"<<std::put_time(&tm, "%Y-%m-%d %H:%M:%S")<<"\t"<<msg<<"\033[0m"<<std::endl;
        }else if(msg.find(b)!=std::string::npos){
            std::cout<<"\033[34m"<<std::put_time(&tm, "%Y-%m-%d %H:%M:%S")<<"\t"<<msg<<"\033[0m"<<std::endl;
        }
    }else if(msg.find(d)!=std::string::npos){
        std::cout<<"\033[32m"<<std::put_time(&tm, "%Y-%m-%d %H:%M:%S")<<"\t"<<msg<<"\033[0m"<<std::endl;
    }
    //std::cout<<std::put_time(&tm, "%Y-%m-%d %H:%M:%S")<<"\t"<<msg<<std::endl;
}

void Order::post_order_short_sell(std::string px){
    long long ts = Order::getTimestamp();
    websocketpp::lib::error_code ec;
    json params;
    params["apiKey"] = "xxxxxxxxxxxxxxxxxxxxx";
    params["side"] = "SELL";
    params["positionSide"] = "SHORT";
    params["symbol"] = "XRPUSDT";
    params["timeInForce"] = "GTC";
    params["type"] = "LIMIT";
    params["timestamp"] = ts;
    params["quantity"] = "2.0";
    params["price"] = px;
    params["selfTradePreventionMode"] = "EXPIRE_TAKER";
    //params["recvWindow"]=50000;
    std::string params_data = "";
    std::vector<std::string> keys;
    for (const auto& [key, val] : params.items()) {
        keys.push_back(key);
    }
    std::sort(keys.begin(), keys.end());
    for (const auto& key : keys){
        std::string k = key;
        auto val = params[key];
        std::string v = "";
        try{
            v=val;
        }catch(std::exception e){
            try{
                long long vv = val;
                v = std::to_string(vv);
            }catch(std::exception e){
                //std::cout<<"E1 ="<<e.what()<<std::endl;
            }
        }
        if(params_data==""){
            params_data = k+"="+v;
        }else{
            params_data = params_data+"&"+k+"="+v;
        }
    }
    params["signature"] = Order::generateSignature(params_data);
    //std::cout<<params_data<<std::endl;
    //std::cout<<params.dump()<<std::endl;
    json send_data;
    send_data["id"] ="NOVA-9999";
    send_data["method"] = "order.place";
    send_data["params"] = params;

    cli.send(Order::con->get_handle(), send_data.dump(), websocketpp::frame::opcode::text,ec);
    if (ec) {
        std::cout << "Order post_order Send failed: " << ec.message() << std::endl;
    } else {
        //std::cout << "post order buy: OK"<< ec.message()<< std::endl;
        Order::print("open short sell "+px);
    }
}

void Order::post_order_short_buy(std::string px){
    long long ts = Order::getTimestamp();
    websocketpp::lib::error_code ec;
    json params;
    params["apiKey"] = "xxxxxxxxxxxxxxxxx";
    params["side"] = "BUY";
    params["positionSide"] = "SHORT";
    params["symbol"] = "XRPUSDT";
    params["timeInForce"] = "GTC";
    params["type"] = "LIMIT";
    params["timestamp"] = ts;
    params["quantity"] = "2.0";
    params["price"] = px;
    params["selfTradePreventionMode"] = "EXPIRE_TAKER";
    //params["recvWindow"]=50000;
    std::string params_data = "";
    std::vector<std::string> keys;
    for (const auto& [key, val] : params.items()) {
        keys.push_back(key);
    }
    std::sort(keys.begin(), keys.end());
    for (const auto& key : keys){
        std::string k = key;
        auto val = params[key];
        std::string v = "";
        try{
            v=val;
        }catch(std::exception e){
            try{
                long long vv = val;
                v = std::to_string(vv);
            }catch(std::exception e){
                //std::cout<<"E1 ="<<e.what()<<std::endl;
            }
        }
        if(params_data==""){
            params_data = k+"="+v;
        }else{
            params_data = params_data+"&"+k+"="+v;
        }
    }
    params["signature"] = Order::generateSignature(params_data);
    //std::cout<<params_data<<std::endl;
    //std::cout<<params.dump()<<std::endl;
    json send_data;
    send_data["id"] ="NOVA-9999";
    send_data["method"] = "order.place";
    send_data["params"] = params;

    cli.send(Order::con->get_handle(), send_data.dump(), websocketpp::frame::opcode::text,ec);
    if (ec) {
        std::cout << "Order post_order Send failed: " << ec.message() << std::endl;
    } else {
        //std::cout << "post order buy: OK"<< ec.message()<< std::endl;
        Order::print("open short buy "+px);
    }
}

void Order::cancel_order(long _id){
    long long ts = Order::getTimestamp();
    websocketpp::lib::error_code ec;
    json params;
    params["apiKey"] = "xxxxxxxxxxxxxxxxxxxxxxxxx";
    params["orderId"] = _id;
    params["symbol"] = "XRPUSDT";
    params["timestamp"] = ts;
    //params["recvWindow"]=50000;
    std::string params_data = "";
    std::vector<std::string> keys;
    for (const auto& [key, val] : params.items()) {
        keys.push_back(key);
    }
    std::sort(keys.begin(), keys.end());
    for (const auto& key : keys){
        std::string k = key;
        auto val = params[key];
        std::string v = "";
        try{
            v=val;
        }catch(std::exception e){
            try{
                long long vv = val;
                v = std::to_string(vv);
            }catch(std::exception e){
                //std::cout<<"E1 ="<<e.what()<<std::endl;
            }
        }
        if(params_data==""){
            params_data = k+"="+v;
        }else{
            params_data = params_data+"&"+k+"="+v;
        }
    }
    params["signature"] = Order::generateSignature(params_data);
    //std::cout<<params_data<<std::endl;
    //std::cout<<params.dump()<<std::endl;
    json send_data;
    send_data["id"] ="NOVA-9999";
    send_data["method"] = "order.cancel";
    send_data["params"] = params;

    cli.send(Order::con->get_handle(), send_data.dump(), websocketpp::frame::opcode::text,ec);
    if (ec) {
        std::cout << "Order post_order Send failed: " << ec.message() << std::endl;
    } else {
        //std::cout << "post order buy: OK"<< ec.message()<< std::endl;
        Order::print("cancel "+std::to_string(_id));
    }
}

// 获取当前时间戳（Unix Epoch 时间戳，单位为秒）
long long Order::getTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    return timestamp;
    // struct timeval tv;
    // gettimeofday(&tv, nullptr); // 获取 UTC 时间（需系统时区已设为 UTC）
    
    // long long ms = tv.tv_sec * 1000LL + tv.tv_usec / 1000;
    // return ms;
}

// 生成签名字符串
std::string Order::generateSignature(const std::string& data) {
    const std::string secretKey = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hashLen;
    HMAC(
        EVP_sha256(), 
        reinterpret_cast<const unsigned char*>(secretKey.c_str()), 
        secretKey.size(),
        reinterpret_cast<const unsigned char*>(data.c_str()), 
        data.size(),
        hash, &hashLen
    );
    
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (int i = 0; i < hashLen; ++i) {
        oss << std::setw(2) << static_cast<int>(hash[i]);
    }
    std::string signature = oss.str();
    //std::cout << "HMAC returned pointer: " << signature << std::endl;
    //OPENSSL_free(mac);
    return signature;
    // // 使用 HMAC SHA256 加密
    // unsigned char hash[EVP_MAX_MD_SIZE];
    // unsigned int hashLen;
    // HMAC(
    //     EVP_sha256(),                          // 使用 SHA256 算法
    //     reinterpret_cast<const unsigned char*>(secretKey.c_str()), secretKey.size(), // SecretKey
    //     reinterpret_cast<const unsigned char*>(data.c_str()), data.size(), // 数据
    //     hash, &hashLen                         // 输出
    // );

    // // 对加密结果进行 Base64 编码
    // return base64Encode(hash, hashLen);
}

Order::Order(){
    Order::is_connected.store(false, std::memory_order_relaxed); // 标记当前连接状态
    Order::should_reconnect.store(true, std::memory_order_relaxed); // 标记是否需要重连
    Order::wss_url = "wss://ws-fapi.binance.com/ws-fapi/v1";
        // 设置日志级别（可选）
    cli.clear_access_channels(websocketpp::log::alevel::all);
    cli.clear_error_channels(websocketpp::log::elevel::all);

    // 初始化 ASIO
    cli.init_asio();

    // 注册消息回调
    cli.set_message_handler([this](websocketpp::connection_hdl hdl, client::message_ptr msg) {
        Order::on_message(&cli, hdl, msg);
    });

    // 注册连接关闭回调
    cli.set_close_handler([this](websocketpp::connection_hdl hdl) {
        Order::on_close(&cli, hdl);
    });

    // 注册连接打开回调
    cli.set_open_handler([this](websocketpp::connection_hdl hdl) {
        Order::on_open(&cli, hdl);
    });

    // 注册连接失败回调
    cli.set_fail_handler([this](websocketpp::connection_hdl hdl) {
        Order::on_fail(&cli, hdl);
    });

    // 设置 TLS 初始化回调
    cli.set_tls_init_handler([](websocketpp::connection_hdl) {
        // 创建一个新的 TLS 上下文
        return websocketpp::lib::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::tlsv12);
    });

    // 创建连接
    websocketpp::lib::error_code ec;
    Order::con = cli.get_connection(Order::wss_url, ec);
    if (ec) {
        std::cout << "Connection error: " << ec.message() << std::endl;
        return ;
    }

    // 设置连接超时时间（例如 5 秒）
    con->set_open_handshake_timeout(5000); // 5 秒
    // 连接服务器
    cli.connect(con);

    // 启动 ASIO 事件循环（在单独的线程中运行）
    Order::t=std::thread([this]() {
        cli.run();
    });
    
}

void Order::wait(){
    Order::t.join();
}

void Order::on_message(client* c, websocketpp::connection_hdl hdl, client::message_ptr msg) {
    try {
        //std::cout<<"Order on_msg "<<msg->get_payload()<<std::endl;
        // 解析 JSON 字符串
        //json obj = json::parse(msg->get_payload());
        
    } catch (const std::exception& e) {
        // 捕获并处理 JSON 解析错误
        //std::cerr << "JSON parsing error: " << e.what() << std::endl;
    }
}

void Order::on_close(client* c, websocketpp::connection_hdl hdl) {
    std::cout << "Order Connection closed." << std::endl;
    Order::is_connected = false;

    if (Order::should_reconnect) {
        std::cout << "Order Attempting to reconnect..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5)); // 等待 5 秒后重连

        websocketpp::lib::error_code ec;
        Order::con = c->get_connection(Order::wss_url, ec);
        if (ec) {
            std::cout << "Order Reconnect error: " << ec.message() << std::endl;
            return;
        }

        // 设置连接超时时间（例如 5 秒）
        con->set_open_handshake_timeout(5000); // 5 秒

        c->connect(con);
    }
}

void Order::on_open(client* c, websocketpp::connection_hdl hdl) {
    std::cout << "Order Connection opened." << std::endl;
    Order::is_connected = true;

    // 每次连接成功后发送一条消息
    
}

void Order::on_fail(client* c, websocketpp::connection_hdl hdl) {
    std::cout << "Order Connection failed or timed out." << std::endl;
    Order::is_connected = false;

    if (Order::should_reconnect) {
        std::cout << "Order Attempting to reconnect..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5)); // 等待 5 秒后重连

        websocketpp::lib::error_code ec;
        Order::con = c->get_connection(Order::wss_url, ec);
        if (ec) {
            std::cout << "Order Reconnect error: " << ec.message() << std::endl;
            return;
        }

        // 设置连接超时时间（例如 5 秒）
        con->set_open_handshake_timeout(5000); // 5 秒

        c->connect(con);
    }
}

class Account{
    public: 
        Account();
        void wait();
        std::atomic<long> order_id;
        std::atomic<double> order_px;
        std::atomic<double> pos_short;
        Order order;
        Books5 bk5;
        std::atomic<bool> is_connected; 
    private:
        
        std::atomic<bool> should_reconnect; 
        HQ_FX hq;
        std::string wss_url;
        client cli;
        void on_message(client* c, websocketpp::connection_hdl hdl, client::message_ptr msg);
        void on_close(client* c, websocketpp::connection_hdl hdl);
        void on_open(client* c, websocketpp::connection_hdl hdl);
        void on_fail(client* c, websocketpp::connection_hdl hdl);
        std::thread t;
        std::thread t_yanchang;
        client::connection_ptr con;  
};

Account::Account(){
    Account::is_connected.store(false, std::memory_order_relaxed); // 标记当前连接状态
    Account::should_reconnect.store(true, std::memory_order_relaxed); // 标记是否需要重连
    Account::pos_short.store(-1.0, std::memory_order_relaxed);
    Account::order_id.store(0, std::memory_order_relaxed);
    Account::order_px.store(0.0, std::memory_order_relaxed);
    
    Account::wss_url = "wss://fstream.binance.com/ws/"+Account::hq.listenKey;
        // 设置日志级别（可选）
    cli.clear_access_channels(websocketpp::log::alevel::all);
    cli.clear_error_channels(websocketpp::log::elevel::all);

    // 初始化 ASIO
    cli.init_asio();

    // 注册消息回调
    cli.set_message_handler([this](websocketpp::connection_hdl hdl, client::message_ptr msg) {
        Account::on_message(&cli, hdl, msg);
    });

    // 注册连接关闭回调
    cli.set_close_handler([this](websocketpp::connection_hdl hdl) {
        Account::on_close(&cli, hdl);
    });

    // 注册连接打开回调
    cli.set_open_handler([this](websocketpp::connection_hdl hdl) {
        Account::on_open(&cli, hdl);
    });

    // 注册连接失败回调
    cli.set_fail_handler([this](websocketpp::connection_hdl hdl) {
        Account::on_fail(&cli, hdl);
    });

    // 设置 TLS 初始化回调
    cli.set_tls_init_handler([](websocketpp::connection_hdl) {
        // 创建一个新的 TLS 上下文
        return websocketpp::lib::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::tlsv12);
    });

    // 创建连接
    websocketpp::lib::error_code ec;
    Account::con = cli.get_connection(Account::wss_url, ec);
    if (ec) {
        std::cout << "Connection error: " << ec.message() << std::endl;
        return ;
    }

    // 设置连接超时时间（例如 5 秒）
    con->set_open_handshake_timeout(5000); // 5 秒
    // 连接服务器
    cli.connect(con);

    // 启动 ASIO 事件循环（在单独的线程中运行）
    Account::t=std::thread([this]() {
        cli.run();
    });
    Account::t_yanchang=std::thread([this]() {
        while (true)
        {
            /* code */
            try{
                std::this_thread::sleep_for(std::chrono::milliseconds(30*60*1000)); // 睡眠 30*60*1000 毫秒
                Account::hq.yanchang();
            }catch(const std::exception& e){
                std::cout<<e.what()<<std::endl;
            }
        }
        
    });
    Account::hq.yanchang();
}

void Account::wait(){
    Account::order.wait();
    Account::bk5.wait();
    Account::t.join();
    Account::t_yanchang.join();
}

void Account::on_message(client* c, websocketpp::connection_hdl hdl, client::message_ptr msg) {
    try {
        //std::cout<<"Account on_msg "<<msg->get_payload()<<std::endl;
        // 解析 JSON 字符串
        json obj = json::parse(msg->get_payload());
        if(obj["e"]=="ACCOUNT_UPDATE"){
            //std::cout<<"ACCOUNT_UPDATE "<<msg->get_payload()<<std::endl;
            json a = obj["a"];
            json p = a["P"];
            for(json pp:p){
                std::string s = pp["s"];
                std::string ps = pp["ps"];
                std::string pa_str = pp["pa"];
                double pa = std::stod(pa_str);
                
                if(s=="XRPUSDT" && ps=="SHORT"){
                    pa = -pa;
                    Account::pos_short.store(pa, std::memory_order_relaxed);
                    //std::cout<<"pos_short "<<pa<<std::endl;
                }
            }
        }
        if(obj["e"]=="ORDER_TRADE_UPDATE"){
            json o = obj["o"];
            std::string X = o["X"];
            std::string S = o["S"];
            std::string ps = o["ps"];
            std::string ap = o["p"];
            long i = o["i"];
            double avg_price = std::stod(ap);
            if(ps=="SHORT"){
                if(S=="SELL"){
                    if(X=="NEW"){
                        Account::order_id.store(i,std::memory_order_relaxed);
                        Account::order_px.store(avg_price,std::memory_order_relaxed);
                    }else if(X=="CANCELED" || X=="EXPIRED"){
                        //std::ostringstream oss;
                        //oss << std::fixed << std::setprecision(4) << Account::bk5.ask1_price.load(std::memory_order_relaxed); // 保留两位小数
                        //Account::order.post_order_short_sell(oss.str());
                        //Account::order_id.store(0,std::memory_order_relaxed);
                        //Account::order_px.store(0.0,std::memory_order_relaxed);
                        Account::order_id.store(0,std::memory_order_relaxed);
                    }else if(X=="FILLED"){
                        double pri = avg_price*(1-0.0001);
                        std::ostringstream oss;
                        oss << std::fixed << std::setprecision(4) << pri; // 保留 位小数
                        Account::order.post_order_short_buy(oss.str());
                        Account::order_id.store(0,std::memory_order_relaxed);
                        //Account::order_px.store(0.0,std::memory_order_relaxed);
                    }
                }else if(S=="BUY"){
                    // if(X=="NEW"){
                    //     Account::order_id.store(i,std::memory_order_relaxed);
                    //     Account::order_px.store(avg_price,std::memory_order_relaxed);
                    // }else if(X=="CANCELED" || X=="EXPIRED"){
                    //     std::ostringstream oss;
                    //     oss << std::fixed << std::setprecision(4) << Account::bk5.bid1_price.load(std::memory_order_relaxed)-0.0000; // 保留两位小数
                    //     Account::order.post_order_short_buy(oss.str());
                    //     //Account::order_id.store(0,std::memory_order_relaxed);
                    //     //Account::order_px.store(0.0,std::memory_order_relaxed);
                    // }else if(X=="FILLED"){
                        
                    //     std::ostringstream oss;
                    //     oss << std::fixed << std::setprecision(4) << Account::bk5.ask1_price.load(std::memory_order_relaxed); // 保留两位小数
                    //     Account::order.post_order_short_sell(oss.str());
                    //     //Account::order_id.store(0,std::memory_order_relaxed);
                    //     //Account::order_px.store(0.0,std::memory_order_relaxed);
                    // }
                }
            }
        }
        
    } catch (const std::exception& e) {
        // 捕获并处理 JSON 解析错误
        //std::cerr << "JSON parsing error: " << e.what() << std::endl;
    }
}

void Account::on_close(client* c, websocketpp::connection_hdl hdl) {
    std::cout << "Account Connection closed." << std::endl;
    Account::is_connected.store(false, std::memory_order_relaxed);

    if (Account::should_reconnect) {
        std::cout << "Account Attempting to reconnect..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5)); // 等待 5 秒后重连

        websocketpp::lib::error_code ec;
        Account::con = c->get_connection(Account::wss_url, ec);
        if (ec) {
            std::cout << "Account Reconnect error: " << ec.message() << std::endl;
            return;
        }

        // 设置连接超时时间（例如 5 秒）
        con->set_open_handshake_timeout(5000); // 5 秒

        c->connect(con);
    }
}

void Account::on_open(client* c, websocketpp::connection_hdl hdl) {
    std::cout << "Account Connection opened." << std::endl;
    Account::is_connected.store(true, std::memory_order_relaxed);

    // 每次连接成功后发送一条消息
    
}

void Account::on_fail(client* c, websocketpp::connection_hdl hdl) {
    std::cout << "Account Connection failed or timed out." << std::endl;
    Account::is_connected.store(false, std::memory_order_relaxed);

    if (Account::should_reconnect) {
        std::cout << "Account Attempting to reconnect..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5)); // 等待 5 秒后重连

        websocketpp::lib::error_code ec;
        Account::con = c->get_connection(Account::wss_url, ec);
        if (ec) {
            std::cout << "Account Reconnect error: " << ec.message() << std::endl;
            return;
        }

        // 设置连接超时时间（例如 5 秒）
        con->set_open_handshake_timeout(5000); // 5 秒

        c->connect(con);
    }
}

int main(){
    Account account;

    std::this_thread::sleep_for(std::chrono::milliseconds(200)); // 睡眠 200 毫秒

    while(true){
        std::this_thread::sleep_for(std::chrono::milliseconds(200)); // 睡眠 毫秒
        if(account.bk5.ask1_price.load(std::memory_order_relaxed)!=0.0 && account.bk5.bid1_price.load(std::memory_order_relaxed)!=0.0&& account.pos_short.load(std::memory_order_relaxed)>-1.0){
            
            
            if(account.order_id.load(std::memory_order_relaxed)==0 && account.pos_short.load(std::memory_order_relaxed)<75.0){
                double ord_px = account.bk5.ask1_price.load(std::memory_order_relaxed);
                std::ostringstream oss;
                oss << std::fixed << std::setprecision(4) << ord_px; // 保留小数
                account.order_px.store(ord_px,std::memory_order_relaxed);
                account.order_id.store(999,std::memory_order_relaxed);
                account.order.post_order_short_sell(oss.str());
                //std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 睡眠 毫秒
            }else if(account.order_id.load(std::memory_order_relaxed)!=0){
                if(account.bk5.ask1_price.load(std::memory_order_relaxed)<account.order_px.load(std::memory_order_relaxed)||account.bk5.ask1_price.load(std::memory_order_relaxed)>account.order_px.load(std::memory_order_relaxed)){
                    account.order.cancel_order(account.order_id.load(std::memory_order_relaxed));
                    //account.order_id.store(0,std::memory_order_relaxed);
                }
            } 
        }
    }
    
    account.wait();
    return 0;
}
