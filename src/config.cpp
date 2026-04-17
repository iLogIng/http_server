#include "../includes/config.hpp"

#include "../includes/logger.hpp"

const std::string& server_config::configuration::
address() const
{ return config_vals_.address_; }

const unsigned short& server_config::configuration::
port() const
{ return config_vals_.port_; }

const std::string& server_config::configuration::
doc_root() const
{ return config_vals_.doc_root_; }

const unsigned int& server_config::configuration::
threads() const
{ return config_vals_.threads_; }

const unsigned int& server_config::configuration::
timeout_seconds() const
{ return config_vals_.timeout_seconds_; }

const size_t& server_config::configuration::
max_body_size() const
{ return config_vals_.max_body_size_; }

bool
server_config::
valid_address(std::string &addr)
{
    return !addr.empty();
}

bool
server_config::
valid_port(uint64_t port)
{
    return port > 0 && port <= 65535;
}

bool
server_config::
valid_doc_root(std::string &path)
{
    return !path.empty();
}

bool
server_config::
valid_threads(uint64_t count)
{
    return count > 0 && count <= boost::thread::hardware_concurrency() * 2;
}

bool
server_config::
valid_timeout_seconds(uint64_t timeout_second)
{
    return timeout_second > 0;
}

bool
server_config::
valid_max_body_size(uint64_t max_body_size)
{
    return max_body_size > 0;
}

// 采用直接覆盖方案进行命令行参数的传入
void server_config::configuration::
apply_command_line(int argc, char *argv[])
{
    prog_opts::options_description desc("Allowed options");
    desc.add_options()
        ("help,h",              "Show help message")
        ("address,a", prog_opts::value<std::string>(), "Server address")
        ("port,p", prog_opts::value<unsigned short>(), "Server port")
        ("doc_root,r", prog_opts::value<std::string>(), "Document root")
        ("threads,t", prog_opts::value<unsigned int>(), "Number of threads")
        ("timeout_seconds,s", prog_opts::value<unsigned int>(), "Timeout in seconds")
        ("max_body_size,b", prog_opts::value<size_t>(), "Maximum body size");

    prog_opts::variables_map vm;
    try {
        prog_opts::store(prog_opts::parse_command_line(argc, argv, desc), vm);
        prog_opts::notify(vm);
    } catch (const prog_opts::error& e) {
        LOG_ERROR << "Error parsing command line: " << e.what();
        throw;
    }

    if(vm.count("help")) {
        std::cout << desc << std::endl;
        LOG_FATAL << "Help requested, exiting.";
        std::exit(EXIT_SUCCESS);
    }
    
    if(vm.count("address"))
        this->config_vals_.address_ = vm["address"].as<std::string>();
    if(vm.count("port"))
        this->config_vals_.port_ = vm["port"].as<unsigned short>();
    if(vm.count("doc_root"))
        this->config_vals_.doc_root_ = vm["doc_root"].as<std::string>();
    if(vm.count("threads"))
        this->config_vals_.threads_ = vm["threads"].as<unsigned int>();
    if(vm.count("timeout_seconds"))
        this->config_vals_.timeout_seconds_ = vm["timeout_seconds"].as<unsigned int>();
    if(vm.count("max_body_size"))
        this->config_vals_.max_body_size_ = vm["max_body_size"].as<size_t>();

}

void
server_config::configuration::
apply_json_config(std::string path)
{
    std::ifstream ifs(path);
    if (!ifs) {
        LOG_ERROR << "Failed to open config file: " << path;
        throw std::runtime_error("Failed to open config file");
    }
    
    std::stringstream buffer;
    buffer << ifs.rdbuf();
    auto json_values = json::parse(buffer.str()).as_object();

    auto load_string_type_val =
    [&](const char *key) -> std::string {
        if (json_values.contains(key) && json_values.at(key).is_string()) {
            return std::string(json_values.at(key).as_string());
        }
    };

#if 0
    auto load_uint64_type_val =
    [&](const char *key) -> uint64_t {
        if (json_values.contains(key) && json_values.at(key).is_number()) {
            return json_values.at(key).as_uint64();
        }
    };
#endif

    this->config_vals_.address_ = load_string_type_val("address");
    this->config_vals_.doc_root_ = load_string_type_val("doc_root");

    if (json_values.contains("port") && json_values.at("port").is_number()) {
        auto port_val = json_values.at("port").as_int64();
        if(valid_port(port_val)) {
            this->config_vals_.port_ = static_cast<unsigned short>(port_val);
        }
        else {
            LOG_WARNING << "Invalid port in JSON:" << port_val;
        }
    }

    if (json_values.contains("threads") && json_values.at("threads").is_number()) {
        auto thrd_val = json_values.at("threads").as_int64();
        if(valid_threads(thrd_val)) {
            this->config_vals_.threads_ = static_cast<unsigned int>(thrd_val);
        }
        else {
            LOG_WARNING << "Invalid threads count in JSON:" << thrd_val;
        }
    }

    if (json_values.contains("timeout_seconds") && json_values.at("timeout_seconds").is_number()) {
        auto time_val = json_values.at("timeout_seconds").as_int64();
        if(valid_timeout_seconds(time_val)) {
            this->config_vals_.timeout_seconds_ = static_cast<unsigned int>(time_val);
        }
        else {
            LOG_WARNING << "Invalid timeout seconds in JSON:" << time_val;
        }
    }
    
    if (json_values.contains("max_body_size") && json_values.at("max_body_size").is_number()) {
        auto size_val = json_values.at("max_body_size").as_int64();
        if(valid_max_body_size(size_val)) {
            this->config_vals_.max_body_size_ = static_cast<size_t>(size_val);
        }
        else {
            LOG_WARNING << "Invalid max body size in JSON:" << size_val;
        }
    }

}

// 采用优先级覆盖的方法处理传入/配置的参数
//      我认为关于json的解析可以承接该部分的设计
// p1. command line
// p2. json config
// p3. default value
// 所以最终的配置可能是以上三种杂合的结果，需要编写配置回显功能
server_config::configuration::
configuration(int argc, char *argv[])
{
    this->config_vals_ = config_values{};
    try {
        apply_json_config("config.json");
    }
    catch(std::exception &e) {
        LOG_WARNING << "Faild to load JSON config: " << e.what() << ". Using defaults and command line.";
    }
    apply_command_line(argc, argv);
    this->dump();
}

void
server_config::configuration::
dump() const {
    LOG_INFO << "Address:         " << config_vals_.address_;
    LOG_INFO << "Port:            " << config_vals_.port_;
    LOG_INFO << "Document Root:   " << config_vals_.doc_root_;
    LOG_INFO << "Threads:         " << config_vals_.threads_;
    LOG_INFO << "Timeout (sec):   " << config_vals_.timeout_seconds_;
    LOG_INFO << "Max Body Size:   " << config_vals_.max_body_size_ << " bytes";
}


