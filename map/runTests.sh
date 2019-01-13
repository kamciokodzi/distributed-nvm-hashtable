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

# run NvmHashMap<int, int> correctness tests
sudo tests/runTestsIntIterate /mnt/mem/fileIntIterate
sudo rm -rf /mnt/mem/file*
sudo tests/runTestsIntInsertGet /mnt/mem/fileIntInsertGet
sudo rm -rf /mnt/mem/file*
sudo tests/runTestsIntInsertRemove /mnt/mem/fileIntInsertRemove
sudo rm -rf /mnt/mem/file*
sudo tests/runTestsIntInsertGetParallel /mnt/mem/fileIntInsertGetParallel
sudo rm -rf /mnt/mem/file*
sudo tests/runTestsIntInsertRemoveParallel /mnt/mem/fileIntInsertRemoveParallel
sudo rm -rf /mnt/mem/file*

# run NvmHashMap<string, string> correctness tests
sudo tests/runTestsStringIterate /mnt/mem/fileStringIterate
sudo rm -rf /mnt/mem/file*
sudo tests/runTestsStringInsertGet /mnt/mem/fileStringInsertGet
sudo rm -rf /mnt/mem/file*
sudo tests/runTestsStringInsertRemove /mnt/mem/fileStringInsertRemove
sudo rm -rf /mnt/mem/file*
sudo tests/runTestsStringInsertGetParallel /mnt/mem/fileStringInsertGetParallel
sudo rm -rf /mnt/mem/file*
sudo tests/runTestsStringInsertRemoveParallel /mnt/mem/fileStringInsertRemoveParallel
sudo rm -rf /mnt/mem/file*

# run NvmHashMap<int, int> performance tests
sudo tests/runTestsPerformance1 /mnt/mem/filePerformance1
sudo rm -rf /mnt/mem/file*
sudo tests/runTestsPerformance2 /mnt/mem/filePerformance2
sudo rm -rf /mnt/mem/file*
sudo tests/runTestsPerformance4 /mnt/mem/filePerformance4
sudo rm -rf /mnt/mem/file*
sudo tests/runTestsPerformance8 /mnt/mem/filePerformance8
sudo rm -rf /mnt/mem/file*
sudo tests/runTestsPerformance16 /mnt/mem/filePerformance16
sudo rm -rf /mnt/mem/file*

# run unordered_map performance tests
sudo tests/runTestsPerformanceUnordered

# merge performance results
cd tests/results
paste -d, Outline.csv NvmHashMap.csv UnorderedMap.csv > Combined.csv

