#include "../includes/graceful_shutdown.hpp"

server_host::graceful_shutdown::
graceful_shutdown(
    net::io_context& ioc,
    listener_ptr listener_ptr_,
    ActiveSessionGetter get_active_sessions,
    chrono::seconds check_interval,
    chrono::seconds force_timeout
)
    : ioc_(ioc)
    , listener_(std::move(listener_ptr_))
    , get_active_sessions_(std::move(get_active_sessions))
    , check_interval_(check_interval)
    , force_timeout_(force_timeout)
    , signals_(ioc, SIGINT, SIGTERM)
    , check_timer_(ioc)
    , force_timer_(ioc)
    , shutting_down_(false)
{}

void
server_host::graceful_shutdown::
start_shutdown_listener(CompletionHandler on_complete)
{
    on_complete_ = std::move(on_complete);
    signals_.async_wait(
        [this](const boost::system::error_code& ec, int signal_number) {
            return on_signal(ec, signal_number);
        }
    );
    LOG_INFO << "Graceful shutdown listener started, Waiting SIGINT or SIGTERM to stop.";
}

void
server_host::graceful_shutdown::
on_signal(const boost::system::error_code& ec, int signal_number)
{
    if (ec) {
        if (ec == net::error::operation_aborted) {
            LOG_INFO << "Signal Handler cancelled due to shutdown.";
            return;
        }
        if (ec) {
            LOG_ERROR << "Signal Handler Error: " << ec.message();
            return;
        }
        return;
    }
    LOG_INFO << "Received signal " << signal_number << ", shutting down the server...";

    do_shutdown();
}

void
server_host::graceful_shutdown::
do_shutdown()
{
    if (shutting_down_) {
        return;
    }
    shutting_down_ = true;

    if (listener_) {
        listener_->stop();
    }
    
    check_active_sessions();

    if (force_timeout_ > chrono::seconds(0)) {
        force_timer_.expires_after(force_timeout_);
        force_timer_.async_wait(
            [this](const boost::system::error_code& ec) {
                if (ec) {
                    if (ec == net::error::operation_aborted) {
                        LOG_INFO << "Force shutdown timer cancelled due to shutdown completion.";
                        return;
                    }
                    LOG_ERROR << "Force shutdown timer error: " << ec.message();
                    return;
                }
                LOG_WARNING << "Force shutdown timeout reached, stopping the server immediately.";
                ioc_.stop();
            }
        );
    }
}

void
server_host::graceful_shutdown::
check_active_sessions()
{
    std::size_t count_active = get_active_sessions_();
    if (count_active == 0) {
        LOG_INFO << "All sessions were closed, stopping the server...";
        force_timer_.cancel();
        if (on_complete_) {
            on_complete_();
        }
        return;
    }

    LOG_INFO    << "Waiting for "
                << count_active
                << " active sessions to close...";
    check_timer_.expires_after(check_interval_);
    check_timer_.async_wait(
        [this](const boost::system::error_code& ec) {
            if (ec != net::error::operation_aborted) {
                check_active_sessions();
            }
        }
    );
}

void
server_host::graceful_shutdown::
on_force_timeout()
{
    LOG_WARNING << "Force shutdown timeout reached, stopping the server immediately.";
    force_timer_.cancel();
    check_timer_.cancel();
    if (on_complete_) {
        on_complete_();
    }
}

void
server_host::graceful_shutdown::
force_stop()
{
    LOG_WARNING << "Force shutdown invoked, stopping the server immediately.";
    signals_.cancel();
    force_timer_.cancel();
    check_timer_.cancel();
    if (on_complete_) {
        on_complete_();
    }
}
