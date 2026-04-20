#pragma once

#include "server.hpp"

#include <boost/asio.hpp>
#include <chrono>
#include <functional>
#include <memory>

namespace server_host
{
namespace net = boost::asio;
namespace chrono = std::chrono;
    
class graceful_shutdown
{
    using CompletionHandler = std::function<void()>;
    using ActiveSessionGetter = std::function<std::size_t()>;
    using listener_ptr = std::shared_ptr<listener>;

private:
    
    net::io_context& ioc_;                      // 异步IO上下文
    listener_ptr listener_;                     // 监听器对象的共享指针
    ActiveSessionGetter get_active_sessions_;   // 获取当前活跃会话数的函数对象
    chrono::seconds check_interval_;           // 检查活跃会话的时间间隔
    chrono::seconds force_timeout_;         // 关闭超时时间

    net::signal_set signals_;                   // 信号集
    net::steady_timer check_timer_;             // 检查计时器
    net::steady_timer force_timer_;          // 关闭计时器
    CompletionHandler on_complete_;             // 关闭完成后的回调
    bool shutting_down_;                        // 是否正在关闭

public:
    graceful_shutdown(
        net::io_context& ioc,
        listener_ptr listener_ptr_,
        ActiveSessionGetter get_active_sessions,
        chrono::seconds check_interval = std::chrono::seconds(1),
        chrono::seconds force_timeout = std::chrono::seconds(30)
    );

    graceful_shutdown(const graceful_shutdown&) = delete;
    graceful_shutdown& operator=(const graceful_shutdown&) = delete;
    graceful_shutdown(graceful_shutdown&&) = delete;
    graceful_shutdown& operator=(graceful_shutdown&&) = delete;

    // 启动关闭监听器
    void start_shutdown_listener(CompletionHandler on_complete = {});
    
    // 强制停止服务器
    void force_stop();

private:
    // 信号处理函数
    void on_signal(const boost::system::error_code& ec, int signal_number);
    
    // 强制关闭超时处理函数
    void on_force_timeout();

    // 停机函数
    void do_shutdown();

    // 检查活跃会话
    void check_active_sessions();

};
    
}
