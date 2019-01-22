// async_tcp_echo_server.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2017 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <boost/asio.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/algorithm/string.hpp>
#include <pthread.h>
#include "../map/NvmHashMap.hpp"

// struct root
// {
//     pmem::obj::persistent_ptr<NvmHashMap<int, int> > pmap;
// };

// pmem::obj::persistent_ptr<root> root_ptr;

using boost::asio::ip::tcp;
namespace bpo = boost::program_options;

bpo::options_description desc("Allowed options");
bpo::variables_map vm;
boost::asio::io_context io_context;

class session;

class node
{
public:
  session *_session;
  std::string addr;
  std::string port;
  node(session *s, std::string a, std::string p)
  {
    this->_session = s;
    this->addr = a;
    this->port = p;
  }
  node(std::string a, std::string p)
  {
    this->addr = a;
    this->port = p;
    this->_session = nullptr;
  }
  node()
  {
    this->_session = nullptr;
  }
};

std::unordered_map<std::string, node> nodes_map;

std::string serialize(std::vector<std::string> vec, char split = '_')
{
  std::string result = "";
  for (int i = 0; i < vec.size() - 1; i++)
  {
    result.append(vec[i] + split);
  }
  result.append(vec[vec.size() - 1]);
  return result;
}

std::string serialize(std::unordered_map<std::string, node> map, char split = '_')
{
  std::string result = "";
  for (const auto &[key, value] : map)
  {
    result.append(value.addr + split + value.port + split);
  }
  result = result.substr(0, result.length() - 1);
  return result;
}

std::vector<std::string> deserialize(std::string msg, char split = '_')
{
  std::vector<std::string> vec;
  boost::split(vec, msg, [split](char c) { return c == split; });
  return vec;
}

long timestamp()
{
  return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

class session
    : public std::enable_shared_from_this<session>
{
public:
  session(tcp::socket socket)
      : socket_(std::move(socket))
  {
  }

  void start()
  {
    do_read();
  }

  void connect(std::string addr, std::string port)
  {
    tcp::resolver resolver(io_context);
    boost::asio::connect(socket_, resolver.resolve(addr, port));
    write("connect_" + std::to_string(timestamp()) + "_" + vm["my_addr"].as<std::string>() + "_" + vm["port"].as<std::string>());
    node n = node(this, addr, port);
    nodes_map[addr + ":" + port] = n;
    do_read();
  }

  void do_read()
  {
    auto self(shared_from_this());
    socket_.async_read_some(boost::asio::buffer(data_, max_length),
    [this, self](boost::system::error_code ec, std::size_t length) {
      if (!ec)
      {
        std::vector<std::string> cmd = deserialize(std::string(data_));
        if (cmd[0] == "connect")
        {
          std::cout << "New node: " << cmd[2] << ":" << cmd[3] << std::endl;
          node n = node(this, cmd[2], cmd[3]);
          nodes_map[cmd[2] + ":" + cmd[3]] = n;
          write("nodes_" + std::to_string(timestamp()) + "_" + serialize(nodes_map));
        }
        if (cmd[0] == "nodes")
        {
          std::cout << "Get nodes: " << data_ << std::endl;
          for (int i = 2; i < cmd.size(); i+=2)
          {
            if (nodes_map.find(cmd[i] + ":" + cmd[i+1]) == nodes_map.end())
            {
              tcp::socket sock(io_context);
              std::make_shared<session>(std::move(sock))->connect(cmd[i], cmd[i+1]);
            }
          }
        }
        if (cmd[0] == "test") {
          std::cout<<"test"<<std::endl;
        }
        //do_write(length);
      }
      memset(data_, 0, max_length);
      do_read();
    });
  }

  void do_write(std::size_t length)
  {
    auto self(shared_from_this());
    boost::asio::async_write(socket_, boost::asio::buffer(data_, length),
    [this, self](boost::system::error_code ec, std::size_t /*length*/) {
      if (!ec)
      {
        //do_read();
      }
    });
  }

  void write(std::string msg)
  {
    auto self(shared_from_this());
    boost::asio::async_write(socket_, boost::asio::buffer(msg.c_str(), msg.length()),
    [this, self](boost::system::error_code ec, std::size_t /*length*/) {
      if (!ec)
      {
      }
    });
  }

  tcp::socket socket_;
  enum
  {
    max_length = 1024
  };
  char data_[max_length];
};

class server
{
public:
  server(boost::asio::io_context &io_context, short port)
      : acceptor_(io_context, tcp::endpoint(tcp::v4(), port))
  {
    do_accept();
  }

private:
  void do_accept()
  {
    acceptor_.async_accept(
        [this](boost::system::error_code ec, tcp::socket socket) {
          if (!ec)
          {
            std::make_shared<session>(std::move(socket))->start();
          }

          do_accept();
        });
  }

  tcp::acceptor acceptor_;
};

void *keyboard(void *arg) {
  std::string command;
  while(true) {
    std::getline(std::cin, command);
    std::vector<std::string> cmd = deserialize(command, ' ');
    if (cmd[0] == "test" && cmd.size() >= 3) {
      std::cout<<"test on "<<cmd[1]<<":"<<cmd[2]<<std::endl;
      nodes_map[cmd[1] + ":" + cmd[2]]._session->write("test");
    }
    if (cmd[0] == "ls") {
      std::string map = serialize(nodes_map);
      std::cout<<map<<std::endl;
    }
    if (cmd[0] == "q") {
      break;
    }
  }
}

int main(int argc, char *argv[])
{
  // pmem::obj::pool<root> pop;
  // pop = pmem::obj::pool<root>::create("file.txt", "", PMEMOBJ_MIN_POOL, 0777);

  // root_ptr = pop.root();

  // pmem::obj::transaction::run(pop, [&] {
  //     root_ptr->pmap = pmem::obj::make_persistent<NvmHashMap<int, int> >(8);
  // });
  // root_ptr->pmap->insertNew(1, 42);
  // std::cout << root_ptr->pmap->get(1) << std::endl;

  desc.add_options()
    ("port", bpo::value<std::string>()->default_value("10000"),"TCP server port")
    ("my_addr", bpo::value<std::string>()->required(), "My address")
    ("server", bpo::value<std::string>()->default_value(""), "Server address")
    ("server_port", bpo::value<std::string>()->default_value(""), "Server port");

  bpo::store(bpo::parse_command_line(argc, argv, desc), vm);
  bpo::notify(vm);

  pthread_t keyboard_thread;
	pthread_create(&keyboard_thread, NULL, keyboard, NULL);

  std::cout << "TCP server listen on port: " << vm["port"].as<std::string>() << std::endl;

  try
  {
    nodes_map[vm["my_addr"].as<std::string>() + ":" + vm["port"].as<std::string>()] = node(vm["my_addr"].as<std::string>(), vm["port"].as<std::string>());
    if (vm["server"].as<std::string>().length() > 0 && vm["server_port"].as<std::string>().length() > 0)
    {
      tcp::socket sock(io_context);
      std::make_shared<session>(std::move(sock))->connect(vm["server"].as<std::string>(), vm["server_port"].as<std::string>());
    }

    server s(io_context, std::atoi(vm["port"].as<std::string>().c_str()));

    io_context.run();
  }
  catch (std::exception &e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
