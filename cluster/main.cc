#include <seastar/core/reactor.hh>
#include <seastar/core/app-template.hh>
#include <seastar/core/temporary_buffer.hh>
#include <seastar/core/distributed.hh>
#include <vector>
#include <iostream>
#include <boost/algorithm/string.hpp>
#include <chrono>

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

uint16_t port;
std::string my_addr;
std::vector<std::string> addresses;

std::string serialize(std::vector<std::string> vec, char split = '_') {
    std::string result = "";
    for (int i = 0; i < vec.size(); i++) {
        result.append(vec[i]+split);
    }
    return result;
}

std::vector<std::string> deserialize(std::string msg, char split = '_') {
    std::vector<std::string> vec;
    boost::split(vec, msg, [split](char c){return c == split;});
    return vec;
}
long timestamp() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

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
        //std::cout<<"Stop connection" << std::endl;
        return make_ready_future<>();
    }

    future<> connect(ipv4_addr server_addr) {

        socket_address local = socket_address(::sockaddr_in{AF_INET, INADDR_ANY, {0}});
        engine().net().connect(make_ipv4_address(server_addr), local, transport::TCP).then([this] (connected_socket fd) {
            auto conn = new connection(std::move(fd));
            std::string msg = "connect_" + std::to_string(timestamp()) + '_' + my_addr + "_" +std::to_string(port);
            conn->write(msg).then_wrapped([conn] (auto&& f) {
                conn->read().then_wrapped([conn] (auto&& f) {
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

                std::vector<std::string> result = deserialize(cmd);

                std::cout<< result[0] << std::endl;

                if (result[0] == "connect") {                    
                    std::string tmp_addr = result[1] + ":" +result[2];
                    addresses.push_back(tmp_addr);

                    std::cout << "New node: " << result[2] << ':' <<result[3] << std::endl;

                    std::string curr_nodes = "nodes_" + std::to_string(timestamp()) + '_' serialize(addresses);

                    std::cout << "Send: " << curr_nodes << std::endl;

                    return _write_buf.write(curr_nodes).then([this] {
                        return _write_buf.flush();
                    }).then([this] {
                        return this->read();
                    });
                }
                
                if (result[0] == "nodes") {                    
                    std::cout << "Get nodes: " << cmd << std::endl;
                    addresses = result;
                    addresses.erase(addresses.begin());
                    std::cout << addresses[0] << std::endl;;

                }

                // return _write_buf.write(cmd).then([this] {
                //     return _write_buf.flush();
                // }).then([this] {
                //     return this->read();
                // });
                return this->read();
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
        ("my_addr", bpo::value<std::string>()->required(), "My address")
        ("addr", bpo::value<std::string>()->default_value(""), "Server address");
    return app.run_deprecated(ac, av, [&] {
        auto&& config = app.configuration();
        port = config["port"].as<uint16_t>();
        enable_tcp = config["tcp"].as<std::string>() == "yes";
        auto addr = config["addr"].as<std::string>();
        my_addr = config["my_addr"].as<std::string>();
        if (!enable_tcp) {
            fprint(std::cerr, "Error: no protocols enabled. Use \"--tcp yes\" to enable\n");
            return engine().exit(1);
        }

        addresses.push_back(my_addr + ':' + std::to_string(port));

        auto server = new distributed<tcp_server>;
        server->start().then([server = std::move(server), port, addr] () mutable {
            engine().at_exit([server] {
                return server->stop();
            });
            if (addr.length() > 0) {
                server->invoke_on(0, &tcp_server::connect, ipv4_addr{addr});
            }
            server->invoke_on_all(&tcp_server::listen, ipv4_addr{port});
        }).then([port] {
            std::cout << "Seastar TCP server listening on port " << port << " ...\n";
        });
    });
}