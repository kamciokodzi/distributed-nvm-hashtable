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
#include <unistd.h>
#include <mutex>
#include <shared_mutex>
#include "../map/NvmHashMap.hpp"

#define REPLICATION_FACTOR 3

struct root
{
  pmem::obj::persistent_ptr<NvmHashMap<std::string, std::string>> pmap[REPLICATION_FACTOR];
};

pmem::obj::persistent_ptr<root> root_ptr;

bool file_exists(const char *fname)
{
  FILE *file;
  if ((file = fopen(fname, "r")))
  {
    fclose(file);
    return true;
  }
  return false;
}
const int32_t range = 360;

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
  std::int32_t hash;
  node(session *s, std::string a, std::string p, std::int32_t h)
  {
    this->_session = s;
    this->addr = a;
    this->port = p;
    this->hash = h;
  }
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
  node(std::string a, std::string p, std::int32_t h)
  {
    this->_session = nullptr;
    this->addr = a;
    this->port = p;
    this->hash = h;
  }
  node()
  {
    this->_session = nullptr;
  }
};

int32_t hash(uint64_t key, int32_t num_buckets = 360)
{
  int64_t b = 1;
  int64_t j = 0;
  while (j < num_buckets)
  {
    b = j;
    key = key * 2862933555777941757ULL + 1;
    j = (b + 1) * (double(1LL << 31) / double((key >> 33) + 1));
  }
  return b;
}
int32_t hash(std::string arg, int32_t num_buckets = 360)
{

  uint64_t key = 0;
  for (int i = 0; i < arg.length(); i++)
  {
    key += pow(arg[i], i);
  }

  int64_t b = 1;
  int64_t j = 0;
  while (j < num_buckets)
  {
    b = j;
    key = key * 2862933555777941757ULL + 1;
    j = (b + 1) * (double(1LL << 31) / double((key >> 33) + 1));
  }
  return b;
}

std::unordered_map<std::string, node> nodes_map;
std::shared_mutex nodes_mutex;

std::string find_node_key(int32_t hash)
{
  std::string result;
  int32_t temp = range + 1;
  std::string min_result;
  int32_t min = range + 1;
  std::shared_lock lock(nodes_mutex);
  for (const auto &[key, value] : nodes_map)
  {
    if (value.hash >= hash && value.hash < temp)
    {
      temp = value.hash;
      result = key;
    }
    if (value.hash < min)
    {
      min = value.hash;
      min_result = key;
    }
  }
  if (temp == range + 1)
  {
    result = min_result;
  }
  if (nodes_map[result].addr == vm["my_addr"].as<std::string>() && nodes_map[result].port == vm["port"].as<std::string>())
  {
    return "";
  }
  return result;
}

std::vector<std::string> find_nodes(int32_t hash)
{
  std::vector<std::string> result_vec;
  std::shared_lock lock(nodes_mutex);

  for (int i = 0; i < REPLICATION_FACTOR; i++)
  {
    std::string result;
    int32_t temp = range + 1;
    std::string min_result;
    int32_t min = range + 1;

    for (const auto &[key, value] : nodes_map)
    {
      if (value.hash >= hash && value.hash < temp && (std::find(result_vec.begin(), result_vec.end(), key) == result_vec.end()))
      {
        temp = value.hash;
        result = key;
      }
      if (value.hash < min && (std::find(result_vec.begin(), result_vec.end(), key) == result_vec.end()))
      {
        min = value.hash;
        min_result = key;
      }
    }
    if (temp == range + 1)
    {
      result = min_result;
    }

    if (nodes_map[result].addr == vm["my_addr"].as<std::string>() && nodes_map[result].port == vm["port"].as<std::string>())
    {
      result_vec.push_back("");
    }
    else
    {
      result_vec.push_back(result);
    }
  }
  return result_vec;
}

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

uint32_t ip_hash(std::string ip, std::string port)
{
  std::vector<std::string> vec = deserialize(ip, '.');
  uint64_t result = 0;
  result += ((uint64_t)std::stoi(vec[0]) * 16777216);
  result += ((uint64_t)std::stoi(vec[1]) * 65536);
  result += ((uint64_t)std::stoi(vec[2]) * 256);
  result += ((uint64_t)std::stoi(vec[3]));
  result *= (uint64_t)std::stoi(port);
  return hash(result, range);
}

long timestamp()
{
  return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}
std::string str_timestamp()
{
  return std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
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

    write("connect_" + str_timestamp() + "_" + vm["my_addr"].as<std::string>() + "_" + vm["port"].as<std::string>() + "_" + std::to_string(nodes_map[vm["my_addr"].as<std::string>() + ":" + vm["port"].as<std::string>()].hash));
    node n = node(this, addr, port, ip_hash(addr, port));
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
                                  std::cout << cmd[4] << std::endl;

                                  node n = node(this, cmd[2], cmd[3], strtoul(cmd[4].c_str(), nullptr, 0));
                                  std::unique_lock lock(nodes_mutex);
                                  nodes_map[cmd[2] + ":" + cmd[3]] = n;
                                  write("nodes_" + str_timestamp() + "_" + serialize(nodes_map));
                                  lock.unlock();
                                }
                                else if (cmd[0] == "nodes")
                                {
                                  std::cout << "Get nodes: " << data_ << std::endl;
                                  std::unique_lock lock(nodes_mutex);
                                  for (int i = 2; i < cmd.size(); i += 2)
                                  {
                                    if (nodes_map.find(cmd[i] + ":" + cmd[i + 1]) == nodes_map.end())
                                    {
                                      tcp::socket sock(io_context);
                                      std::make_shared<session>(std::move(sock))->connect(cmd[i], cmd[i + 1]);
                                    }
                                  }
                                  lock.unlock();
                                }

                                else if (cmd[0] == "get")
                                {
                                  try
                                  {
                                    std::string value = root_ptr->pmap[0]->get(cmd[2]);
                                    //std::cout << "[MAP] Found element with key=" << cmd[2] << " and value=" << value << std::endl;
                                    write("getResult_" + str_timestamp() + "_" + cmd[2] + "_" + value);
                                  }
                                  catch (...)
                                  {
                                    //std::cout << "[MAP] Element not found" << std::endl;
                                    write("getResultBad_" + str_timestamp() + "_" + cmd[2]);
                                  }
                                }
                                else if (cmd[0] == "insert")
                                {
                                  try
                                  {
                                    root_ptr->pmap[0]->insertNew(cmd[2], cmd[3]);
                                    //std::cout << "[MAP] Inserted element with key=" << cmd[1] << " and value=" << cmd[2] << std::endl;
                                    write("insertResult_" + str_timestamp() + "_" + cmd[2] + "_" + cmd[3]);
                                  }
                                  catch (...)
                                  {
                                    //std::cout << "[MAP] Element not found" << std::endl;
                                    write("insertResultBad_" + str_timestamp() + "_" + cmd[2] + "_" + cmd[3]);
                                  }
                                }
                                else if (cmd[0] == "remove")
                                {
                                  try
                                  {
                                    std::string value = root_ptr->pmap[0]->remove(cmd[2]);
                                    //std::cout << "[MAP] Removed element with key=" << cmd[1] << " and value=" << value << std::endl;
                                    write("removeResult_" + str_timestamp() + "_" + cmd[2] + "_" + value);
                                  }
                                  catch (...)
                                  {
                                    //std::cout << "[MAP] Element not found" << std::endl;
                                    write("removeResultBad_" + str_timestamp() + "_" + cmd[2]);
                                  }
                                }
                                else if (cmd[0] == "getResult")
                                {
                                  std::cout << "[MAP] Found element with key=" << cmd[2] << " and value=" << cmd[3] << std::endl;
                                }
                                else if (cmd[0] == "getResultBad")
                                {
                                  std::cout << "[MAP] Element not found key=" << cmd[2] << std::endl;
                                }
                                else if (cmd[0] == "insertResult")
                                {
                                  std::cout << "[MAP] Inserted element with key=" << cmd[2] << " and value=" << cmd[3] << std::endl;
                                }
                                else if (cmd[0] == "insertResultBad")
                                {
                                  std::cout << "[MAP] Error inserting element with key=" << cmd[2] << " and value=" << cmd[3] << std::endl;
                                }
                                else if (cmd[0] == "removeResult")
                                {
                                  std::cout << "[MAP] Removed element with key=" << cmd[2] << " and value=" << cmd[3] << std::endl;
                                }
                                else if (cmd[0] == "removeResultBad")
                                {
                                  std::cout << "[MAP] Error removing element with key=" << cmd[2] << std::endl;
                                }
                                else if (cmd[0] == "test")
                                {
                                  std::cout << "test" << std::endl;
                                }
                                else if (cmd[0] == "sleep")
                                {
                                  std::cout << "Start sleep" << std::endl;
                                  usleep(15000000);
                                  std::cout << "Stop sleep" << std::endl;
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
                               if (ec)
                               {
                                 std::cout << ec << std::endl;
                               }
                             });
  }
  void get(std::string key)
  {
    write("get_" + str_timestamp() + "_" + key);
  }
  void insert(std::string key, std::string value)
  {
    write("insert_" + str_timestamp() + "_" + key + "_" + value);
  }
  void remove(std::string key)
  {
    write("remove_" + str_timestamp() + "_" + key);
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

void *keyboard(void *arg)
{
  std::string command;
  while (true)
  {
    std::getline(std::cin, command);
    std::vector<std::string> cmd = deserialize(command, ' ');

    if (cmd[0] == "test" && cmd.size() >= 3)
    {
      std::cout << "test on " << cmd[1] << ":" << cmd[2] << std::endl;

      std::shared_lock lock(nodes_mutex);
      nodes_map[cmd[1] + ":" + cmd[2]]._session->write("test");
      lock.unlock();
    }

    else if (cmd[0] == "sleep" && cmd.size() >= 3)
    {
      std::cout << "sleep on " << cmd[1] << ":" << cmd[2] << std::endl;
      std::unique_lock lock(nodes_mutex);
      nodes_map[cmd[1] + ":" + cmd[2]]._session->write("sleep");
      lock.unlock();
    }

    else if (cmd[0] == "ls")
    {
      std::unique_lock lock(nodes_mutex);
      std::string map = serialize(nodes_map);
      lock.unlock();
      std::cout << map << std::endl;
    }

    else if (cmd[0] == "insert")
    {
      if (cmd.size() < 3)
      {
        std::cout << "[MAP] Not enough values" << std::endl;
      }
      else
      {
        try
        {
          std::string location = find_node_key(hash(cmd[1]));
          std::cout << "Server: ";
          if (location != "")
          {
            std::cout << location << std::endl;
            std::unique_lock lock(nodes_mutex);
            nodes_map[location]._session->insert(cmd[1], cmd[2]);
            lock.unlock();
          }
          else
          {
            std::cout << "local" << std::endl;
            root_ptr->pmap[0]->insertNew(cmd[1], cmd[2]);
            std::cout << "[MAP] Inserted element with key=" << cmd[1] << " and value=" << cmd[2] << std::endl;
          }
        }
        catch (...)
        {
          std::cout << "[MAP] Element not found" << std::endl;
        }
      }
    }

    else if (cmd[0] == "get")
    {
      if (cmd.size() < 2)
      {
        std::cout << "[MAP] Not enough values" << std::endl;
      }
      else
      {
        try
        {

          auto vec = find_nodes(hash(cmd[1]));

          for (int i = 0; i < vec.size(); i++)
          {
            std::cout << vec[i] << std::endl;
          }

          std::string location = find_node_key(hash(cmd[1]));
          std::cout << "Server: ";
          if (location != "")
          {
            std::cout << location << std::endl;
            nodes_map[location]._session->get(cmd[1]);
          }
          else
          {
            std::cout << "local" << std::endl;
            std::string value = root_ptr->pmap[0]->get(cmd[1]);
            std::cout << "[MAP] Found element with key=" << cmd[1] << " and value=" << value << std::endl;
          }
        }
        catch (...)
        {
          std::cout << "[MAP] Element not found" << std::endl;
        }
      }
    }

    else if (cmd[0] == "remove")
    {
      if (cmd.size() < 2)
      {
        std::cout << "[MAP] Not enough values" << std::endl;
      }
      else
      {
        try
        {
          std::string location = find_node_key(hash(cmd[1]));
          std::cout << "Server: ";
          if (location != "")
          {
            std::cout << location << std::endl;
            std::unique_lock lock(nodes_mutex);
            nodes_map[location]._session->remove(cmd[1]);
            lock.unlock();
          }
          else
          {
            std::cout << "local" << std::endl;
            std::string value = root_ptr->pmap[0]->remove(cmd[1]);
            std::cout << "[MAP] Removed element with key=" << cmd[1] << " and value=" << value << std::endl;
          }
        }
        catch (...)
        {
          std::cout << "[MAP] Element not found" << std::endl;
        }
      }
    }

    else if (cmd[0] == "iterate")
    {

      Iterator<std::string, std::string> it(root_ptr->pmap[0]);
      try
      {
        std::cout << "[MAP] " << it.get() << " ";
      }
      catch (...)
      {
      }

      while (it.next())
      {
        try
        {
          std::cout << it.get() << " ";
        }
        catch (...)
        {
        }
      }
      std::cout << std::endl;
    }

    else if (cmd[0] == "q")
    {
      break;
    }
  }
  return nullptr;
}

int main(int argc, char *argv[])
{
  desc.add_options()("port", bpo::value<std::string>()->default_value("10000"), "TCP server port")("my_addr", bpo::value<std::string>()->required(), "My address")("server", bpo::value<std::string>()->default_value(""), "Server address")("server_port", bpo::value<std::string>()->default_value(""), "Server port");

  bpo::store(bpo::parse_command_line(argc, argv, desc), vm);
  bpo::notify(vm);

  pmem::obj::pool<root> pop;
  std::string path = "hashmapFile" + vm["port"].as<std::string>();

  try
  {
    if (!file_exists(path.c_str()))
    {
      std::cout << "[MAP] File doesn't exists, creating pool" << std::endl;
      pop = pmem::obj::pool<root>::create(path, "",
                                          PMEMOBJ_MIN_POOL * 100, 0777);

      root_ptr = pop.root();

      if (!root_ptr->pmap[0])
      {
        pmem::obj::transaction::run(pop, [&] {
          std::cout << "[MAP] Creating NvmHashMap" << std::endl;
          for (int i = 0; i < REPLICATION_FACTOR; i++)
          {
            root_ptr->pmap[i] = pmem::obj::make_persistent<NvmHashMap<std::string, std::string>>(8);
          }
        });
      }
    }
    else
    {
      std::cout << "[MAP] File exists, opening pool" << std::endl;
      pop = pmem::obj::pool<root>::open(path, "");
      root_ptr = pop.root();
    }
  }
  catch (pmem::pool_error &e)
  {
    std::cerr << e.what() << std::endl;
    return 1;
  }

  pthread_t keyboard_thread;
  pthread_create(&keyboard_thread, NULL, keyboard, NULL);

  std::cout << "[MAP] Hashmap interface: \n"
               "[MAP] insert K V\n[MAP] get K\n[MAP] remove K\n[MAP] iterate\n"
            << std::endl;
  std::cout << "TCP server listening on port: " << vm["port"].as<std::string>() << std::endl;
  std::cout << ip_hash(vm["my_addr"].as<std::string>(), vm["port"].as<std::string>()) << std::endl;

  try
  {
    nodes_map[vm["my_addr"].as<std::string>() + ":" + vm["port"].as<std::string>()] = node(vm["my_addr"].as<std::string>(), vm["port"].as<std::string>(), ip_hash(vm["my_addr"].as<std::string>(), vm["port"].as<std::string>()));

    if (vm["server"].as<std::string>().length() > 0 && vm["server_port"].as<std::string>().length() > 0)
    {
      tcp::socket sock(io_context);
      std::make_shared<session>(std::move(sock))->connect(vm["server"].as<std::string>(), vm["server_port"].as<std::string>());
    }

    server s(io_context, std::atoi(vm["port"].as<std::string>().c_str()));

    //io_context.run();
    const unsigned cores = (int)std::thread::hardware_concurrency() - 1;

    std::thread *threads = new std::thread[cores];

    for (int i = 0; i < cores; i++)
    {
      threads[i] = std::thread{[]() { io_context.run(); }};
    }

    for (int i = 0; i < cores; i++)
    {
      threads[i].join();
    }
  }
  catch (std::exception &e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
