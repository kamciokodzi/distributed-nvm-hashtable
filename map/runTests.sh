cd tests
mkdir -p results
cd results
sudo rm -f NvmHashMap.csv
sudo rm -f UnorderedMap.csv
sudo rm -f Combined.csv
cd ..
make
cd ..
sudo rm -rf file*

# run NvmHashMap<int, int> correctness tests
sudo tests/runTestsIntIterate fileIntIterate
sudo rm -rf file*
sudo tests/runTestsIntInsertGet fileIntInsertGet
sudo rm -rf file*
sudo tests/runTestsIntInsertRemove fileIntInsertRemove
sudo rm -rf file*
sudo tests/runTestsIntInsertGetParallel fileIntInsertGetParallel
sudo rm -rf file*
sudo tests/runTestsIntInsertRemoveParallel fileIntInsertRemoveParallel
sudo rm -rf file*

# run NvmHashMap<string, string> correctness tests
sudo tests/runTestsStringIterate fileStringIterate
sudo rm -rf file*
sudo tests/runTestsStringInsertGet fileStringInsertGet
sudo rm -rf file*
sudo tests/runTestsStringInsertRemove fileStringInsertRemove
sudo rm -rf file*
sudo tests/runTestsStringInsertGetParallel fileStringInsertGetParallel
sudo rm -rf file*
sudo tests/runTestsStringInsertRemoveParallel fileStringInsertRemoveParallel
sudo rm -rf file*

# run NvmHashMap<int, int> performance tests
sudo tests/runTestsPerformance1 filePerformance1
sudo rm -rf file*
sudo tests/runTestsPerformance2 filePerformance2
sudo rm -rf file*
sudo tests/runTestsPerformance4 filePerformance4
sudo rm -rf file*
sudo tests/runTestsPerformance8 filePerformance8
sudo rm -rf file*
sudo tests/runTestsPerformance16 filePerformance16
sudo rm -rf file*

# run unordered_map performance tests
sudo tests/runTestsPerformanceUnordered

# merge performance results
cd tests/results
paste -d, Outline.csv NvmHashMap.csv UnorderedMap.csv > Combined.csv

