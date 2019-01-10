#include <seastar/core/reactor.hh>
#include <seastar/core/app-template.hh>
#include <seastar/core/temporary_buffer.hh>
#include <seastar/core/distributed.hh>
#include <vector>
#include <iostream>

using namespace seastar;
static std::string str_ping{"ping"};
static std::string str_pong{"pong"};
static std::string str_unknow{"unknow cmd"};
static int tx_msg_total_size = 100 * 1024 * 1024;
static int tx_msg_size = 4 * 1024;
static int tx_msg_nr = tx_msg_total_size / tx_msg_size;
static int rx_msg_size = 4 * 1024;
static std::string str_txbuf(tx_msg_size, 'X');
static bool enable_tcp = false;

struct claster_node {
    server_socket socket;
    ipv4_addr address;
};

class tcp_server {
    std::vector<server_socket> _tcp_listeners;
    std::vector<claster_node> nodes;
public:
    future<> listen(ipv4_addr addr) {
        if (enable_tcp) {
            //std::cout<< "Enable TCP" << std::endl;
            listen_options lo;
            lo.proto = transport::TCP;
            lo.reuse_address = true;
            server_socket new_socket = engine().listen(make_ipv4_address(addr), lo);
            _tcp_listeners.push_back(std::move(new_socket)); // aaaaa ewentualnie &
            claster_node single_node;
            single_node.socket = std::move(new_socket);
            single_node.address = addr;
            nodes.push_back(std::move(single_node));
            do_accepts(_tcp_listeners);
        }
        return make_ready_future<>();
    }

    future<> stop() {
        std::cout<<"Stop connection" << std::endl;
        return make_ready_future<>();
    }

    future<> connect(ipv4_addr server_addr) {

        socket_address local = socket_address(::sockaddr_in{AF_INET, INADDR_ANY, {0}});
        engine().net().connect(make_ipv4_address(server_addr), local, transport::TCP).then([this] (connected_socket fd) {
            auto conn = new connection(std::move(fd));
            conn->write("connect").then_wrapped([conn] (auto&& f) {
                std::cout<<"closing"<<std::endl;
                conn->read_once().then_wrapped([conn] (auto&& f) {
                    delete conn;
                    std::cout<<"conn closed"<<std::endl;
                    try {
                        f.get();
                    } catch (std::exception& ex) {
                        fprint(std::cerr, "request error: %s\n", ex.what());
                    }
                });
            });
        });

        return make_ready_future();
    }

    void do_accepts(std::vector<server_socket>& listeners) {
        int which = listeners.size() - 1;
        //std::cout<< "Which: " << which << std::endl;
        listeners[which].accept().then([this, &listeners] (connected_socket fd, socket_address addr) mutable {
            std::cout<< "Accept connection: " << addr << std::endl;
            auto conn = new connection(*this, std::move(fd), addr);
            conn->read().then_wrapped([conn] (auto&& f) {
                try {
                    f.get();
                } catch (std::exception& ex) {
                    std::cout << "request error " << ex.what() << "\n";
                }
            });
            do_accepts(listeners);
        }).then_wrapped([] (auto&& f) {
            try {
                f.get();
            } catch (std::exception& ex) {
                std::cout << "accept failed: " << ex.what() << "\n";
            }
        });
    }
    class connection {
        connected_socket _fd;
        socket_address _addr;
        input_stream<char> _read_buf;
        output_stream<char> _write_buf;

    public:
        connection(tcp_server& server, connected_socket&& fd, socket_address addr)
            : _fd(std::move(fd))
            , _addr(addr)
            , _read_buf(_fd.input())
            , _write_buf(_fd.output()) {}

        connection(connected_socket&& fd)
            : _fd(std::move(fd))
            , _read_buf(_fd.input())
            , _write_buf(_fd.output()) {}

        future<> read() {
            if (_read_buf.eof()) {
                return make_ready_future();
            }
            // size_t n = 4;
            return _read_buf.read().then([this] (temporary_buffer<char> buf) {
                if (buf.size() == 0) {
                    return make_ready_future();
                }
                auto cmd = std::string(buf.get(), buf.size());
                std::cout<<cmd << std::endl;
                return _write_buf.write(cmd).then([this] {
                    return _write_buf.flush();
                }).then([this] {
                    return this->read();
                });
            });
        }

        future<> read_once() {
            if (_read_buf.eof()) {
                return make_ready_future();
            }
            // size_t n = 4;
            return _read_buf.read().then([this] (temporary_buffer<char> buf) {
                if (buf.size() == 0) {
                    return make_ready_future();
                }
                auto cmd = std::string(buf.get(), buf.size());
                std::cout<<cmd << std::endl;
                // return _write_buf.write(cmd).then([this] {
                //     return _write_buf.flush();
                // }).then([this] {
                //     return this->make_ready_future();
                // });
                return make_ready_future();
            });
        }

        future<> write(std::string msg) {
            return _write_buf.write(msg).then([this] {
                return _write_buf.flush();
            }).then([this] {
                return make_ready_future<>();
            });
        }
    };
};

namespace bpo = boost::program_options;

int main(int ac, char** av) {
    app_template app;
    app.add_options()
        ("port", bpo::value<uint16_t>()->default_value(10000), "TCP server port")
        ("tcp", bpo::value<std::string>()->default_value("yes"), "tcp listen")
        ("addr", bpo::value<std::string>()->default_value(""), "Server address");
    return app.run_deprecated(ac, av, [&] {
        std::cout<<"AAAA"<<std::endl;
        auto&& config = app.configuration();
        uint16_t port = config["port"].as<uint16_t>();
        enable_tcp = config["tcp"].as<std::string>() == "yes";
        auto addr = config["addr"].as<std::string>();
        if (!enable_tcp) {
            fprint(std::cerr, "Error: no protocols enabled. Use \"--tcp yes\" to enable\n");
            return engine().exit(1);
        }

        auto server = new distributed<tcp_server>;
        server->start().then([server = std::move(server), port, addr] () mutable {
            engine().at_exit([server] {
                return server->stop();
            });
            if (addr.length() > 0) {
                server->invoke_on(1, &tcp_server::connect, ipv4_addr{addr});
            }
            server->invoke_on_all(&tcp_server::listen, ipv4_addr{port});
        }).then([port] {
            std::cout << "Seastar TCP server listening on port " << port << " ...\n";
        });
    });
}