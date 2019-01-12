cd tests
mkdir -p results
cd results
sudo rm -f NvmHashMap.csv
sudo rm -f UnorderedMap.csv
sudo rm -f Combined.csv
cd ..
make
cd ..
sudo rm -rf /mnt/mem/file*
sudo tests/runTestsStr /mnt/mem/fileStr
sudo rm -rf /mnt/mem/file*
sudo tests/runTestsInt /mnt/mem/fileInt
sudo rm -rf /mnt/mem/file*
sudo tests/runTestsPerformance1 /mnt/mem/filePerformance1
sudo rm -rf /mnt/mem/file*
sudo tests/runTestsPerformance2 /mnt/mem/filePerformance2
sudo rm -rf /mnt/mem/file*
sudo tests/runTestsPerformance4 /mnt/mem/filePerformance4
sudo rm -rf /mnt/mem/file*
sudo tests/runTestsPerformance8 /mnt/mem/filePerformance8
sudo rm -rf /mnt/mem/file*
sudo tests/runTestsPerformance16 /mnt/mem/filePerformance16
sudo tests/runTestsPerformanceUnordered
cd tests/results
paste -d, Outline.csv NvmHashMap.csv UnorderedMap.csv > Combined.csv

