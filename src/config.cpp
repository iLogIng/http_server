#include "../includes/config.hpp"

#include "../includes/logger.hpp"

#include <fstream>

const std::string&
server_config::configuration::
address() const
{ return config_vals_.address_; }

unsigned short
server_config::configuration::
port() const
{ return config_vals_.port_; }

const std::string&
server_config::configuration::
doc_root() const
{ return config_vals_.doc_root_; }

const std::string&
server_config::configuration::
log_file() const
{ return config_vals_.log_file_; }

unsigned int
server_config::configuration::
threads() const
{ return config_vals_.threads_; }

unsigned int
server_config::configuration::
timeout_seconds() const
{ return config_vals_.timeout_seconds_; }

size_t
server_config::configuration::
max_body_size() const
{ return config_vals_.max_body_size_; }

size_t
server_config::configuration::
max_connections() const
{ return config_vals_.max_connections_;}

size_t
server_config::configuration::
max_cache_entries() const
{ return config_vals_.max_cache_entries_;}

bool
server_config::
valid_address(const std::string &addr)
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
valid_doc_root(const std::string &path)
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

bool
server_config::
valid_max_connections(uint64_t max_connections)
{
    return max_connections > 0;
}

bool
server_config::
valid_max_cache_entries(uint64_t max_cache_entries)
{
    return max_cache_entries < (static_cast<uint64_t>(1) << 31);
}

// 采用直接覆盖方案进行命令行参数的传入
void server_config::configuration::
apply_command_line(int argc, char *argv[])
{
    prog_opts::options_description desc("Allowed options");
    desc.add_options()
        ("help,h",              "Show help message")
        ("config,c", prog_opts::value<std::string>(), "Path to JSON config file")
        ("address,a", prog_opts::value<std::string>(), "Server address")
        ("port,p", prog_opts::value<unsigned short>(), "Server port")
        ("doc_root,r", prog_opts::value<std::string>(), "Document root")
        ("log_file,l", prog_opts::value<std::string>(), "Log file path")
        ("threads,t", prog_opts::value<unsigned int>(), "Number of threads")
        ("timeout_seconds,s", prog_opts::value<unsigned int>(), "Timeout in seconds")
        ("max_body_size,b", prog_opts::value<size_t>(), "Maximum body size")
        ("max_connections,n", prog_opts::value<size_t>(), "Max connections")
        ("max_cache_entries, e", prog_opts::value<size_t>(), "Max Cache Entries");

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
    
    if(vm.count("config"))
        this->config_vals_.config_path_ = vm["config"].as<std::string>();

    if(vm.count("address"))
        this->config_vals_.address_ = vm["address"].as<std::string>();
    if(vm.count("port"))
        this->config_vals_.port_ = vm["port"].as<unsigned short>();
    if(vm.count("doc_root"))
        this->config_vals_.doc_root_ = vm["doc_root"].as<std::string>();
    if(vm.count("log_file"))
        this->config_vals_.log_file_ = vm["log_file"].as<std::string>();
    if(vm.count("threads"))
        this->config_vals_.threads_ = vm["threads"].as<unsigned int>();
    if(vm.count("timeout_seconds"))
        this->config_vals_.timeout_seconds_ = vm["timeout_seconds"].as<unsigned int>();
    if(vm.count("max_body_size"))
        this->config_vals_.max_body_size_ = vm["max_body_size"].as<size_t>();
    if(vm.count("max_connections"))
        this->config_vals_.max_connections_ = vm["max_connections"].as<size_t>();
    if(vm.count("max_cache_entries"))
        this->config_vals_.max_cache_entries_ = vm["max_cache_entries"].as<size_t>();

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

    if (json_values.contains("address") && json_values.at("address").is_string()) {
        this->config_vals_.address_ = std::string(json_values.at("address").as_string());
    }
    else {
        LOG_WARNING << "Address Not Found or Invalid in JSON config.";
    }
    if(json_values.contains("doc_root") && json_values.at("doc_root").is_string()) {
        this->config_vals_.doc_root_ = std::string(json_values.at("doc_root").as_string());
    }
    else {
        LOG_WARNING << "Document Root Not Found or Invalid in JSON config.";
    }
    if (json_values.contains("log_file") && json_values.at("log_file").is_string()) {
        this->config_vals_.log_file_ = std::string(json_values.at("log_file").as_string());
    }

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

    if (json_values.contains("max_connections") && json_values.at("max_connections").is_number()) {
        auto size_val = json_values.at("max_connections").as_int64();
        if(valid_max_connections(size_val)) {
            this->config_vals_.max_connections_ = static_cast<size_t>(size_val);
        }
        else {
            LOG_WARNING << "Invalid max connections in JSON:" << size_val;
        }
    }

    if (json_values.contains("max_cache_entries") && json_values.at("max_cache_entries").is_number()) {
        auto size_val = json_values.at("max_cache_entries").as_int64();
        if(valid_max_cache_entries(size_val)) {
            this->config_vals_.max_cache_entries_ = static_cast<size_t>(size_val);
        }
        else {
            LOG_WARNING << "Invalid max cache entries in JSON:" << size_val;
        }
    }
}

// 配置加载：命令行 > JSON > 默认值
server_config::configuration::
configuration(int argc, char *argv[])
{
    this->config_vals_ = config_values{};

    // 预解析 --config，确定配置文件路径
    bool config_explicit = false;
    {
        prog_opts::options_description cfg_opt("Configuration file option");
        cfg_opt.add_options()
            ("config,c", prog_opts::value<std::string>(), "Path to JSON config file");
        prog_opts::variables_map vm;
        try {
            prog_opts::store(
                prog_opts::command_line_parser(argc, argv)
                    .options(cfg_opt).allow_unregistered().run(),
                vm);
            prog_opts::notify(vm);
        } catch (const prog_opts::error&) { }
        if (vm.count("config")) {
            this->config_vals_.config_path_ = vm["config"].as<std::string>();
            config_explicit = true;
        }
    }

    // 加载 JSON 配置
    try {
        apply_json_config(this->config_vals_.config_path_);
    }
    catch(std::exception &e) {
        if (config_explicit) {
            LOG_FATAL << "Failed to load specified config file '"
                      << this->config_vals_.config_path_ << "': " << e.what();
            std::exit(EXIT_FAILURE);
        }
        LOG_WARNING << "Failed to load default config file '"
                    << this->config_vals_.config_path_ << "': " << e.what()
                    << ". Using defaults and command line.";
    }

    // 命令行参数覆盖 JSON
    apply_command_line(argc, argv);
    this->dump();
}

void
server_config::configuration::
dump() const {
    LOG_INFO << "Config File:           " << config_vals_.config_path_;
    LOG_INFO << "Address:               " << config_vals_.address_;
    LOG_INFO << "Port:                  " << config_vals_.port_;
    LOG_INFO << "Document Root:         " << config_vals_.doc_root_;
    LOG_INFO << "Log File:              " << config_vals_.log_file_;
    LOG_INFO << "Threads:               " << config_vals_.threads_;
    LOG_INFO << "Timeout (sec):         " << config_vals_.timeout_seconds_;
    LOG_INFO << "Max Body Size:         " << config_vals_.max_body_size_ << " bytes";
    LOG_INFO << "Max Connections:       " << config_vals_.max_connections_;
    LOG_INFO << "Max Cache Entries:     " << config_vals_.max_cache_entries_;
}


