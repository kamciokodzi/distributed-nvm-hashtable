cd /mnt/mem
sudo rm -rf file*
cd -
g++ main.cpp -o main -std=c++17 -lpmem -lpmemobj -lpthread
