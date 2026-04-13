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
server_config::config_values::
validate() const
{
    if(address_.empty()) {
        LOG_ERROR << "Address cannot be empty";
        return false;
    }
    if (port_ == 0) {
        LOG_ERROR << "Invalid port number: " << port_;
        return false;
    }
    if (threads_ == 0) {
        LOG_ERROR << "Number of threads must be greater than 0";
        return false;
    }
    if (timeout_seconds_ == 0) {
        LOG_ERROR << "Timeout seconds must be greater than 0";
        return false;
    }
    if (max_body_size_ == 0) {
        LOG_ERROR << "Max body size must be greater than 0";
        return false;
    }
    return true;
}

server_config::config_values server_config::configuration::
load_defaults()
{
    return config_values{};
}

server_config::config_values server_config::configuration::
parse_json_config(const std::string& path)
{
    std::ifstream ifs(path);
    if (!ifs) {
        LOG_ERROR << "Failed to open config file: " << path;
        throw std::runtime_error("Failed to open config file");
    }
    
    std::stringstream buffer;
    buffer << ifs.rdbuf();
    auto json_values = json::parse(buffer.str()).as_object();

    // 使用 lambda 封装查找逻辑，避免重复查找和临时对象悬挂
    auto get_string = [&](const char* key, const char* default_val) ->
    std::string {
        if (auto val = json_values.if_contains(key); val && val->is_string())
        {
            return std::string(val->as_string());
        }
        return default_val;
    };

    auto get_uint = [&](const char* key, uint64_t default_val) ->
    uint64_t {
        if (auto val = json_values.if_contains(key); val && val->is_number())
        {
            return val->as_uint64();
        }
        return default_val;
    };

    return {
        /*address*/             get_string("address", "0.0.0.0"),
        /*port*/                static_cast<unsigned short>(get_uint("port", 8080)),
        /*doc_root*/            get_string("doc_root", "."),
        /*threads*/             static_cast<unsigned int>(get_uint("threads", 1)),
        /*timeout_seconds*/     static_cast<unsigned int>(get_uint("timeout_seconds", 30)),
        /*max_body_size*/       static_cast<std::size_t>(get_uint("max_body_size", 1 << 20))
    };
}

server_config::config_values server_config::configuration::
parse_command_line(int argc, char *argv[])
{
    prog_opts::options_description desc("Allowed options");
    desc.add_options()
        ("help,h",              "Show help message")
        ("address,a", prog_opts::value<std::string>()->default_value("0.0.0.0"), "Server address (default: 0.0.0.0)")
        ("port,p", prog_opts::value<unsigned short>()->default_value(8080), "Server port (default: 8080)")
        ("doc_root,r", prog_opts::value<std::string>()->default_value("."), "Document root (default: .)")
        ("threads,t", prog_opts::value<unsigned int>()->default_value(1), "Number of threads (default: 1)")
        ("timeout_seconds,s", prog_opts::value<unsigned int>()->default_value(30), "Timeout in seconds (default: 30)")
        ("max_body_size,b", prog_opts::value<size_t>()->default_value(1048576), "Maximum body size (default: 1048576 (1MB))");

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

    return {
        /*address*/             vm["address"].as<std::string>(),
        /*port*/                static_cast<unsigned short>(vm["port"].as<unsigned short>()),
        /*doc_root*/            vm["doc_root"].as<std::string>(),
        /*threads*/             static_cast<unsigned int>(vm["threads"].as<unsigned int>()),
        /*timeout_seconds*/     static_cast<unsigned int>(vm["timeout_seconds"].as<unsigned int>()),
        /*max_body_size*/       static_cast<std::size_t>(vm["max_body_size"].as<std::size_t>())
    };
}

void server_config::configuration::
merge_configs(
    server_config::config_values& base,
    const server_config::config_values& overrides)
{
    if(!overrides.validate()) {
        LOG_ERROR << "Invalid configuration overrides, skipping this merge step.";
        throw std::runtime_error("Invalid configuration overrides");
        return;
    }
    base.address_ = overrides.address_;
    base.port_ = overrides.port_;
    base.doc_root_ = overrides.doc_root_;
    base.threads_ = overrides.threads_;
    base.timeout_seconds_ = overrides.timeout_seconds_;
    base.max_body_size_ = overrides.max_body_size_;
}

server_config::config_values server_config::configuration::
load_config(int argc, char *argv[])
{
    auto cfg_vals = load_defaults();

    try {
        auto json_config = parse_json_config("config.json");
        merge_configs(cfg_vals, json_config);
    } catch (const std::exception& e) {
        LOG_WARNING << "Failed to load config file: " << e.what() << ". Using defaults.";
    }

    try {
        auto cmd_config = parse_command_line(argc, argv);
        merge_configs(cfg_vals, cmd_config);
    } catch (const std::exception& e) {
        LOG_ERROR << "Failed to parse command line: " << e.what();
        throw std::runtime_error("Failed to parse command line");
    }

    return cfg_vals;
}

server_config::configuration::
configuration(int argc, char *argv[])
{
    this->config_vals_ = load_config(argc, argv);
}


