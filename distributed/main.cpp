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

auto start_time = std::chrono::system_clock::now();

std::string message_format = "_";
std::string message_const = "";

struct root
{
  pmem::obj::persistent_ptr<NvmHashMap<std::string, std::string>> pmap[REPLICATION_FACTOR];
};

pmem::obj::persistent_ptr<root> root_ptr;
pmem::obj::pool<root> pop;

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
const int32_t range = 10;

using boost::asio::ip::tcp;
namespace bpo = boost::program_options;

bpo::options_description desc("Allowed options");
bpo::variables_map vm;
boost::asio::io_context io_context;
std::string my_addr;

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

int32_t hash(uint64_t key, int32_t num_buckets = 10)
{
  int64_t b = 1;
  int64_t j = 0;
  for (int i = 0; i < 5; i++)
  {
    b = j;
    key = key * 2862933555777941757ULL + j;
    j = (b + 1) * (double(1LL << 31) / double((key >> 33) + 1));
  }
  return fabs(b % num_buckets);
}
int32_t hash(std::string arg, int32_t num_buckets = range)
{

  uint64_t key = 0;
  for (int i = 0; i < arg.length(); i++)
  {
    key += pow(arg[i], i);
  }

  int64_t b = 1;
  int64_t j = 0;
  for (int i = 0; i < 5; i++)
  {
    b = j;
    key = key * 2862933555777941757ULL + j;
    j = (b + 1) * (double(1LL << 31) / double((key >> 33) + 1));
  }
  return fabs(b % num_buckets);
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

  int count = std::min(REPLICATION_FACTOR, (int)nodes_map.size());

  for (int i = 0; i < count; i++)
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

    result_vec.push_back(result);
  }
  return result_vec;
}

std::vector<std::string> find_nodes_back(int32_t hash)
{
  std::vector<std::string> result_vec;
  std::shared_lock lock(nodes_mutex);

  int count = std::min(REPLICATION_FACTOR, (int)nodes_map.size());

  for (int i = 0; i < count; i++)
  {
    std::string result;
    int32_t temp = -1;
    std::string max_result;
    int32_t max = -1;

    for (const auto &[key, value] : nodes_map)
    {
      if (value.hash <= hash && value.hash > temp && (std::find(result_vec.begin(), result_vec.end(), key) == result_vec.end()))
      {
        temp = value.hash;
        result = key;
      }
      if (value.hash > max && (std::find(result_vec.begin(), result_vec.end(), key) == result_vec.end()))
      {
        max = value.hash;
        max_result = key;
      }
    }

    if (temp == -1)
    {
      result = max_result;
    }

    result_vec.push_back(result);
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


void broadcast_insert(std::string key, std::string value, long time_stamp);

void broadcast_remove(std::string key, long time_stamp);

void insertNew(int i, std::string key, std::string value, long time_stamp)
{
  try
  {
    std::string val = root_ptr->pmap[i]->get(key);
    auto vec = deserialize(val);
    long time_inserted = std::stol(vec[0]);

    if (time_stamp > time_inserted)
    {
      root_ptr->pmap[i]->insertNew(key, std::to_string(time_stamp) + "_" + value);
      broadcast_insert(key, value, time_stamp);
    }
  }
  catch (...)
  {
    root_ptr->pmap[i]->insertNew(key, std::to_string(time_stamp) + "_" + value);
    broadcast_insert(key, value, time_stamp);
  }
}
std::string removeElement(int i, std::string key, long time_stamp)
{
  try
  {
    std::string val = root_ptr->pmap[i]->get(key);
    auto vec = deserialize(val);
    long time_inserted = std::stol(vec[0]);

    if (time_stamp > time_inserted)
    {
      root_ptr->pmap[i]->insertNew(key, std::to_string(time_stamp));
      broadcast_remove(key, time_stamp);
      if (vec.size() > 1)
      {
        return vec[1];
      }
    }
    return "None";
  }
  catch (...)
  {
    root_ptr->pmap[i]->insertNew(key, std::to_string(time_stamp));
    broadcast_remove(key, time_stamp);
    return "None";
  }
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
    try
    {
      boost::asio::connect(socket_, resolver.resolve(addr, port));
    }
    catch (...)
    {
      std::cout << "Can't connect to: " << addr + ":" + port << std::endl;
      nodes_map[addr + ":" + port]._session = nullptr;
      return;
    }

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
                                // std::cout<<data_<<std::endl;

                                std::vector<std::string> cmd = deserialize(std::string(data_));
                                if (cmd[0] == "connect")
                                {
                                  std::cout << "New node: " << cmd[2] << ":" << cmd[3] << std::endl;
                                  std::cout << cmd[4] << std::endl;

                                  node n = node(this, cmd[2], cmd[3], strtoul(cmd[4].c_str(), nullptr, 0));
                                  std::unique_lock lock(nodes_mutex);
                                  nodes_map[cmd[2] + ":" + cmd[3]] = n;
                                  lock.unlock();
                                  std::cout << serialize(nodes_map) << std::endl;
                                  write("nodes_" + str_timestamp() + "_" + serialize(nodes_map));

                                  auto vec = find_nodes_back(ip_hash(vm["my_addr"].as<std::string>(), vm["port"].as<std::string>()));

                                  for (int i = 0; i < vec.size(); i++)
                                  {
                                    std::cout << "Before: " << vec[i] << std::endl;
                                  }

                                  int place = -1;

                                  for (int i = 0; i < vec.size(); i++)
                                  {
                                    if (vec[i] == (cmd[2] + ":" + cmd[3]))
                                    {
                                      place = i;
                                      //std::cout << "Place: " << place << std::endl;
                                    }
                                  }

                                  if (place > 0)
                                  {

                                    for (int j = vec.size() - 1; j > place; j--)
                                    {
                                      root_ptr->pmap[j] = root_ptr->pmap[j - 1];
                                    }
                                    pmem::obj::transaction::run(pop, [&] {
                                      std::cout << "[MAP] Creating NvmHashMap" << std::endl;
                                      root_ptr->pmap[place] = pmem::obj::make_persistent<NvmHashMap<std::string, std::string>>(8);
                                    });

                                    if (vec[1] == (cmd[2] + ":" + cmd[3]))
                                    {
                                      std::cout << "Send part of my items" << std::endl;
                                      Iterator<std::string, std::string> it(root_ptr->pmap[0]);
                                      try
                                      {
                                        std::string key;
                                        std::string value;

                                        while (it.next())
                                        {
                                          key = it.getKey();
                                          std::cout << find_nodes(hash(key))[0] << std::endl;
                                          if (find_nodes(hash(key))[0] == (cmd[2] + ":" + cmd[3]))
                                          {
                                            value = it.getValue();
                                            std::cout << key << std::endl;
                                            insert(key, value);
                                            root_ptr->pmap[0]->remove(key);
                                          }
                                        }
                                      }
                                      catch (...)
                                      {
                                        std::cout << "Error" << std::endl;
                                      }
                                    }
                                  }
                                  else
                                  {
                                    vec = find_nodes(ip_hash(vm["my_addr"].as<std::string>(), vm["port"].as<std::string>()));

                                    for (int i = 0; i < vec.size(); i++)
                                    {
                                      if (vec[i] == (cmd[2] + ":" + cmd[3]))
                                      {
                                        std::cout << "Send my items to replicate" << std::endl;
                                        Iterator<std::string, std::string> it(root_ptr->pmap[0]);
                                        try
                                        {
                                          std::string key;
                                          std::string value;

                                          while (it.next())
                                          {
                                            key = it.getKey();
                                            std::cout << key << std::endl;
                                            value = it.getValue();
                                            insert(key, value);
                                          }
                                        }
                                        catch (...)
                                        {
                                        }
                                      }
                                    }
                                  }
                                }
                                else if (cmd[0] == "nodes")
                                {
                                  std::cout << "Get nodes: " << data_ << std::endl;
                                  std::unique_lock lock(nodes_mutex);
                                  bool last = true;
                                  for (int i = 2; i < cmd.size(); i += 2)
                                  {
                                    if (cmd[i].size() > 4 && cmd[i].size() < 20)
                                    {
                                      if (nodes_map.find(cmd[i] + ":" + cmd[i + 1]) == nodes_map.end())
                                      {
                                        last = false;
                                        try
                                        {
                                          tcp::socket sock(io_context);
                                          std::make_shared<session>(std::move(sock))->connect(cmd[i], cmd[i + 1]);
                                        }
                                        catch (...)
                                        {
                                        }
                                      }
                                    }
                                  }
                                  lock.unlock();
                                  if (last)
                                  {
                                    auto vec = find_nodes(ip_hash(vm["my_addr"].as<std::string>(), vm["port"].as<std::string>()));

                                    std::cout << "Send my items to replicate" << std::endl;
                                    Iterator<std::string, std::string> it(root_ptr->pmap[0]);
                                    try
                                    {
                                      std::string key;
                                      std::string value;

                                      while (it.next())
                                      {
                                        key = it.getKey();
                                        std::cout << key << std::endl;
                                        value = it.getValue();
                                        for (int i = 1; i < vec.size(); i++)
                                        {
                                          std::shared_lock lock(nodes_mutex);
                                          nodes_map[vec[i]]._session->insert(key, value);
                                          lock.unlock();
                                        }
                                      }
                                    }
                                    catch (...)
                                    {
                                    }
                                  }
                                }

                                else if (cmd[0] == "get")
                                {
                                  try
                                  {
                                    std::string value = root_ptr->pmap[0]->get(cmd[2]);
                                    //std::cout << "[MAP] Found element with key=" << cmd[2] << " and value=" << value << std::endl;
                                    if (cmd.size() >= 4)
                                    {
                                      write("getResult_" + str_timestamp() + "_" + cmd[2] + "_" + value + "_last");
                                    }
                                    else
                                    {
                                      write("getResult_" + str_timestamp() + "_" + cmd[2] + "_" + value);
                                    }
                                  }
                                  catch (...)
                                  {
                                    //std::cout << "[MAP] Element not found" << std::endl;
                                    if (cmd.size() >= 4)
                                    {
                                      write("getResultBad_" + str_timestamp() + "_" + cmd[2] + "_last");
                                    }
                                    else
                                    {
                                      write("getResultBad_" + str_timestamp() + "_" + cmd[2]);
                                    }
                                  }
                                }
                                else if (cmd[0] == "insert")
                                {
                                  try
                                  {
                                    bool done = false;
                                    auto vec = find_nodes(hash(cmd[2]));
                                    for (int i = 0; i < vec.size(); i++)
                                    {
                                      if (vec[i] == my_addr)
                                      {
                                        insertNew(i, cmd[2], cmd[3], std::stol(cmd[1]));
                                        //root_ptr->pmap[i]->insertNew(cmd[2], cmd[3]);
                                        //std::cout << "[MAP] Inserted element in map=" << i << " with key=" << cmd[2] << " and value=" << cmd[3] << std::endl;
                                        done = true;
                                      }
                                    }
                                    if (!done)
                                    {
                                      nodes_map[vec[0]]._session->insert(cmd[2], cmd[3]);
                                    }
                                    if (cmd.size() >= 5)
                                    {
                                      if (cmd[4] == "last")
                                      {
                                        write("insertResult_" + str_timestamp() + "_" + cmd[2] + "_" + cmd[3] + "_last");
                                      }
                                      else
                                      {
                                        write("insertResult_" + str_timestamp() + "_" + cmd[2] + "_" + cmd[3]);
                                      }
                                    }
                                    else
                                    {
                                      write("insertResult_" + str_timestamp() + "_" + cmd[2] + "_" + cmd[3]);
                                    }
                                  }
                                  catch (...)
                                  {
                                    std::cout << "[MAP] Element not found" << std::endl;
                                    if (cmd.size() >= 5)
                                    {
                                      write("insertResultBad_" + str_timestamp() + "_" + cmd[2] + "_" + cmd[3] + "_last");
                                    }
                                    else
                                    {
                                      write("insertResultBad_" + str_timestamp() + "_" + cmd[2] + "_" + cmd[3]);
                                    }
                                  }
                                }
                                else if (cmd[0] == "remove")
                                {
                                  try
                                  {
                                    bool done = false;
                                    auto vec = find_nodes(hash(cmd[2]));
                                    for (int i = 0; i < vec.size(); i++)
                                    {
                                      if (vec[i] == my_addr)
                                      {
                                        auto value = removeElement(i, cmd[2], std::stol(cmd[1]));
                                        //auto value = root_ptr->pmap[i]->remove(cmd[2]);
                                        //std::cout << "[MAP] Removed element in map=" << i << " with key=" << cmd[2] << std::endl;

                                        if (cmd.size() >= 5)
                                        {
                                          write("removeResult_" + str_timestamp() + "_" + cmd[2] + "_" + value + "_last");
                                        }
                                        else
                                        {
                                          write("removeResult_" + str_timestamp() + "_" + cmd[2] + "_" + value);
                                        }
                                        done = true;
                                      }
                                    }
                                    if (!done)
                                    {
                                      nodes_map[vec[0]]._session->remove(cmd[2]);
                                    }
                                    //std::cout << "[MAP] Removed element with key=" << cmd[1] << " and value=" << value << std::endl;
                                  }
                                  catch (...)
                                  {
                                    std::cout << "[MAP] Element not found" << std::endl;
                                    write("removeResultBad_" + str_timestamp() + "_" + cmd[2]);
                                  }
                                }
                                else if (cmd[0] == "getResult")
                                {
                                  //std::cout << "[MAP] Found element with key=" << cmd[2] << " and value=" << cmd[3] << std::endl;
                                  if (cmd.size() >= 5)
                                  {
                                    if (cmd[4] == "last")
                                    {
                                      auto end = std::chrono::system_clock::now();
                                      std::chrono::duration<double> elapsed_time = end - start_time;
                                      std::cout << "End test msg: " << elapsed_time.count() << " seconds" << std::endl;
                                    }
                                  }
                                }
                                else if (cmd[0] == "getResultBad")
                                {
                                  std::cout << "[MAP] Element not found key=" << cmd[2] << std::endl;
                                }
                                else if (cmd[0] == "insertResult")
                                {
                                  //std::cout << "[MAP] Inserted element with key=" << cmd[2] << " and value=" << cmd[3] << std::endl;
                                  if (cmd.size() >= 5)
                                  {
                                    if (cmd[4] == "last")
                                    {
                                      auto end = std::chrono::system_clock::now();
                                      std::chrono::duration<double> elapsed_time = end - start_time;
                                      std::cout << "End test msg: " << elapsed_time.count() << " seconds" << std::endl;
                                    }
                                  }
                                }
                                else if (cmd[0] == "insertResultBad")
                                {
                                  std::cout << "[MAP] Error inserting element with key=" << cmd[2] << " and value=" << cmd[3] << std::endl;
                                }
                                else if (cmd[0] == "removeResult")
                                {
                                  // std::cout << "[MAP] Removed element with key=" << cmd[2] << " and value=" << cmd[3] << std::endl;
                                  if (cmd.size() >= 5)
                                  {
                                    if (cmd[4] == "last")
                                    {
                                      auto end = std::chrono::system_clock::now();
                                      std::chrono::duration<double> elapsed_time = end - start_time;
                                      std::cout << "End test msg: " << elapsed_time.count() << " seconds" << std::endl;
                                    }
                                  }
                                }
                                else if (cmd[0] == "removeResultBad")
                                {
                                  std::cout << "[MAP] Error removing element with key=" << cmd[2] << std::endl;
                                }
                                else if (cmd[0] == "test")
                                {
                                  int size = nodes_map.size();

                                  int l = 2600000;
                                  int s = std::stoi(cmd[1]);
                                  int el = std::stoi(cmd[2]);
                                  int c = std::stoi(cmd[3]);
                                  int j = 1;

                                  std::cout << "Start test: " << l << " elements from index: " << s << std::endl;
                                  start_time = std::chrono::system_clock::now();

                                  // for (const auto &[key, value] : nodes_map)
                                  // {
                                  //   if (key != my_addr) {
                                  //     value._session->write("test_" + std::to_string(el*j) + "_" + el + "_" + c);
                                  //     j++;
                                  //   }
                                  // }

                                  for (int i = s; i < el; i += c)
                                  {
                                    insert(std::to_string(i), message_const);
                                  }
                                  insert(std::to_string(l), message_const, true);

                                  auto end = std::chrono::system_clock::now();
                                  std::chrono::duration<double> elapsed_time = end - start_time;
                                  std::cout << "End test local: " << elapsed_time.count() << " seconds" << std::endl;
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

    int size = msg.length() + 1;

    msg.append(message_format.substr(0, max_length - size));

    //std::cout<<msg.length() + 1<<std::endl;

    boost::asio::async_write(socket_, boost::asio::buffer(msg.c_str(), msg.length() + 1),
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
  void get(std::string key, bool last)
  {
    write("get_" + str_timestamp() + "_" + key + "_last");
  }
  void insert(std::string key, std::string value)
  {
    write("insert_" + str_timestamp() + "_" + key + "_" + value);
  }
  void insert(std::string key, std::string value, bool last)
  {
    write("insert_" + str_timestamp() + "_" + key + "_" + value + "_last");
  }
  void insert(std::string key, std::string value, long time_stamp)
  {
    write("insert_" + std::to_string(time_stamp) + "_" + key + "_" + value);
  }
  void remove(std::string key)
  {
    write("remove_" + str_timestamp() + "_" + key);
  }
  void remove(std::string key, bool last)
  {
    write("remove_" + str_timestamp() + "_" + key + "_last");
  }
  void remove(std::string key, long time_stamp)
  {
    write("remove_" + std::to_string(time_stamp) + "_" + key);
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
void broadcast_insert(std::string key, std::string value, long time_stamp)
{
  auto vec = find_nodes(hash(key));
  for (int i = 0; i < vec.size(); i++)
  {
    std::string location = vec[i];
    if (location != (vm["my_addr"].as<std::string>() + ":" + vm["port"].as<std::string>()))
    {
      std::unique_lock lock(nodes_mutex);
      nodes_map[location]._session->insert(key, value, time_stamp);
      lock.unlock();
    }
  }
}
void broadcast_remove(std::string key, long time_stamp)
{
  auto vec = find_nodes(hash(key));
  for (int i = 0; i < vec.size(); i++)
  {
    std::string location = vec[i];
    if (location != (vm["my_addr"].as<std::string>() + ":" + vm["port"].as<std::string>()))
    {
      std::unique_lock lock(nodes_mutex);
      nodes_map[location]._session->remove(key, time_stamp);
      lock.unlock();
    }
  }
}

void insert(std::string key, std::string value, bool last = false)
{
  try
  {
    // std::cout<<key<<std::endl;

    auto vec = find_nodes(hash(key));
    for (int i = 0; i < vec.size(); i++)
    {
      std::string location = vec[i];
      //std::cout << "Server: ";
      if (location != (vm["my_addr"].as<std::string>() + ":" + vm["port"].as<std::string>()))
      {
        //std::cout << location << std::endl;
        std::unique_lock lock(nodes_mutex);
        if (last)
        {
          nodes_map[location]._session->insert(key, value, true);
        }
        else
        {
          nodes_map[location]._session->insert(key, value);
        }
        lock.unlock();
      }
      else
      {
        //std::cout << "local" << std::endl;
        insertNew(i, key, value, timestamp());
        //root_ptr->pmap[i]->insertNew(key, value);
        //std::cout << "[MAP] Inserted element in map=" << i << " with key=" << key << " and value=" << value << std::endl;
      }
    }
  }
  catch (...)
  {
    std::cout << "[MAP] Element not found" << std::endl;
  }
}
void remove(std::string key, bool last = false)
{
  try
  {
    auto vec = find_nodes(hash(key));
    for (int i = 0; i < vec.size(); i++)
    {
      std::string location = vec[i];
      // std::cout << "Server: ";
      if (location != (vm["my_addr"].as<std::string>() + ":" + vm["port"].as<std::string>()))
      {
        // std::cout << location << std::endl;
        std::unique_lock lock(nodes_mutex);
        if (last)
        {
          nodes_map[location]._session->remove(key, true);
        }
        else
        {
          nodes_map[location]._session->remove(key);
        }
        lock.unlock();
      }
      else
      {
        // std::cout << "local" << std::endl;
        auto value = removeElement(i, key, timestamp());
        //std::string value = root_ptr->pmap[i]->remove(key);
        // std::cout << "[MAP] Removed element with key=" << key << " and value=" << value << std::endl;
      }
    }
  }
  catch (...)
  {
    std::cout << "[MAP] Element not found" << std::endl;
  }
}
void get(std::string key, bool last = false)
{
  try
  {
    auto vec = find_nodes(hash(key));

    std::string location = vec[0];
    //std::cout << "Server: ";
    if (location != my_addr)
    {
      //std::cout << location << std::endl;
      std::unique_lock lock(nodes_mutex);
      if (last)
      {
        nodes_map[location]._session->get(key, true);
      }
      else
      {
        std::cout << "Get: " << key << std::endl;
        nodes_map[location]._session->get(key);
      }
      lock.unlock();
    }
    else
    {
      //std::cout << "local" << std::endl;
      std::string value = root_ptr->pmap[0]->get(key);
      //std::cout << "[MAP] Found element with key=" << key << " and value=" << value << std::endl;
    }
  }
  catch (...)
  {
    std::cout << "[MAP] Element not found" << std::endl;
  }
}

void *keyboard(void *arg)
{
  std::string command;
  while (true)
  {
    std::getline(std::cin, command);
    std::vector<std::string> cmd = deserialize(command, ' ');

    if (cmd[0] == "test")
    {
      if (cmd[1] == "insert")
      {
        int l = std::stoi(cmd[3]) + std::stoi(cmd[2]);
        int s = std::stoi(cmd[2]);
        int c = std::stoi(cmd[4]);
        std::cout << "Start test: " << l << " elements from index: " << s << std::endl;

        start_time = std::chrono::system_clock::now();

        for (int i = s; i < l; i += c)
        {
          insert(std::to_string(i), std::to_string(i));
        }
        insert(std::to_string(l), std::to_string(l), true);

        auto end = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed_time = end - start_time;
        std::cout << "End test local: " << elapsed_time.count() << " seconds" << std::endl;
      }
      else if (cmd[1] == "1")
      {
        int l = 2600000;
        int s = 0;
        int c = 13;
        std::cout << "Start test: " << l << " elements from index: " << s << std::endl;

        start_time = std::chrono::system_clock::now();

        for (int i = s; i < l; i += c)
        {
          insert(std::to_string(i), message_const);
        }
        insert(std::to_string(l), message_const, true);

        auto end = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed_time = end - start_time;
        std::cout << "End test local: " << elapsed_time.count() << " seconds" << std::endl;
      }
      else if (cmd[1] == "2")
      {

        int size = nodes_map.size();

        int l = 2600000;
        int s = 0;
        int el = 2600000 / size;
        int c = 13;
        int j = 1;

        std::cout << "Start test: " << l << " elements from index: " << s << std::endl;
        start_time = std::chrono::system_clock::now();

        for (const auto &[key, value] : nodes_map)
        {
          if (key != my_addr)
          {
            value._session->write("test_" + std::to_string(el * j) + "_" + std::to_string(el) + "_" + std::to_string(c));
            j++;
          }
        }

        for (int i = s; i < el; i += c)
        {
          insert(std::to_string(i), message_const);
        }
        insert(std::to_string(l), message_const, true);

        auto end = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed_time = end - start_time;
        std::cout << "End test local: " << elapsed_time.count() << " seconds" << std::endl;
      }
      else if (cmd[1] == "get")
      {
        int l = std::stoi(cmd[3]) + std::stoi(cmd[2]);
        int s = std::stoi(cmd[2]);
        int c = std::stoi(cmd[4]);
        std::cout << "Start test: " << l << " elements from index: " << s << std::endl;

        start_time = std::chrono::system_clock::now();

        for (int i = s; i < l; i += c)
        {
          get(std::to_string(i));
        }
        get(std::to_string(l), true);

        auto end = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed_time = end - start_time;
        std::cout << "End test local: " << elapsed_time.count() << " seconds" << std::endl;
      }
      if (cmd[1] == "remove")
      {
        int l = std::stoi(cmd[3]) + std::stoi(cmd[2]);
        int s = std::stoi(cmd[2]);
        std::cout << "Start test: " << l << " elements from index: " << s << std::endl;

        start_time = std::chrono::system_clock::now();

        for (int i = s; i < l; i++)
        {
          remove(std::to_string(i));
        }
        remove(std::to_string(l), true);

        auto end = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed_time = end - start_time;
        std::cout << "End test local: " << elapsed_time.count() << " seconds" << std::endl;
      }
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
        //insert(cmd[1], cmd[2]);
        try
        {
          auto vec = find_nodes(hash(cmd[1]));
          for (int i = 0; i < vec.size(); i++)
          {
            std::string location = vec[i]; //find_node_key(hash(cmd[1]));
            std::cout << "Server: ";
            if (location != (vm["my_addr"].as<std::string>() + ":" + vm["port"].as<std::string>()))
            {
              std::cout << location << std::endl;
              std::unique_lock lock(nodes_mutex);
              nodes_map[location]._session->insert(cmd[1], cmd[2]);
              lock.unlock();
            }
            else
            {
              std::cout << "local" << std::endl;
              insertNew(i, cmd[1], cmd[2], timestamp());
              //root_ptr->pmap[i]->insertNew(cmd[1], cmd[2]);
              std::cout << "[MAP] Inserted element in map=" << i << " with key=" << cmd[1] << " and value=" << cmd[2] << std::endl;
            }
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
          auto vec = find_nodes(hash(cmd[1]));
          for (int i = 0; i < vec.size(); i++)
          {
            std::string location = vec[i]; //find_node_key(hash(cmd[1]));
            std::cout << "Server: ";
            if (location != (vm["my_addr"].as<std::string>() + ":" + vm["port"].as<std::string>()))
            {
              std::cout << location << std::endl;
              std::unique_lock lock(nodes_mutex);
              nodes_map[location]._session->remove(cmd[1]);
              lock.unlock();
            }
            else
            {
              std::cout << "local" << std::endl;
              auto value = removeElement(i, cmd[1], timestamp());
              //std::string value = root_ptr->pmap[i]->remove(cmd[1]);
              std::cout << "[MAP] Removed element with key=" << cmd[1] << " and value=" << value << std::endl;
            }
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

      while (it.next())
      {
        try
        {
          std::cout << it.getKey() << " " << it.getValue() << " ";
        }
        catch (...)
        {
        }
      }
      std::cout << std::endl;
    }

    else if (cmd[0] == "count")
    {
      for (int i = 0; i < REPLICATION_FACTOR; i++)
      {

        Iterator<std::string, std::string> it(root_ptr->pmap[i]);

        int count = 0;

        while (it.next())
        {
          try
          {
            count++;
          }
          catch (...)
          {
          }
        }
        std::cout << "Elements in map " << i << ": " << count << std::endl;
      }
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

  my_addr = vm["my_addr"].as<std::string>() + ":" + vm["port"].as<std::string>();

  std::string path = "/mnt/ramdisk/hashmapFile" + vm["port"].as<std::string>();
  //std::string path = "hashmapFile" + vm["port"].as<std::string>();

  for (int i = 0; i < 1024; i++)
  {
    message_format.append("0");
  }
  for (int i = 0; i < 512; i++)
  {
    message_const.append("m");
  }

  try
  {
    if (!file_exists(path.c_str()))
    {
      std::cout << "[MAP] File doesn't exists, creating pool" << std::endl;
      pop = pmem::obj::pool<root>::create(path, "",
                                          PMEMOBJ_MIN_POOL * 63, 0777);

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
