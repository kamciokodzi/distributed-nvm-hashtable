sudo rm performance.txt
cd tests
make
cd -
make
sudo rm -rf /mnt/mem/file*
sudo tests/runTestsStr /mnt/mem/fileStr
sudo rm -rf /mnt/mem/file*
sudo tests/runTestsInt /mnt/mem/fileInt
sudo rm -rf /mnt/mem/file*
sudo tests/runTestsPerformance16 /mnt/mem/filePerformance16
sudo rm -rf /mnt/mem/file*
sudo tests/runTestsPerformance8 /mnt/mem/filePerformance8
sudo rm -rf /mnt/mem/file*
sudo tests/runTestsPerformance4 /mnt/mem/filePerformance4
sudo rm -rf /mnt/mem/file*
sudo tests/runTestsPerformance2 /mnt/mem/filePerformance2
sudo rm -rf /mnt/mem/file*
sudo tests/runTestsPerformance1 /mnt/mem/filePerformance1
sudo tests/runTestsPerformanceUnordered


