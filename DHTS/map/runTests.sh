cd tests
mkdir -p results
cd results
sudo rm -f NvmHashMap.csv
sudo rm -f HashMap.csv
sudo rm -f Combined.csv
cd ..
make
cd ..
sudo rm -rf /mnt/ramdisk/file*

# run NvmHashMap<int, int> correctness tests
sudo tests/runTestsIntIterate /mnt/ramdisk/fileIntIterate
sudo rm -rf /mnt/ramdisk/file*
sudo tests/runTestsIntInsertGet /mnt/ramdisk/fileIntInsertGet
sudo rm -rf /mnt/ramdisk/file*
sudo tests/runTestsIntInsertRemove /mnt/ramdisk/fileIntInsertRemove
sudo rm -rf /mnt/ramdisk/file*
sudo tests/runTestsIntInsertGetParallel /mnt/ramdisk/fileIntInsertGetParallel
sudo rm -rf /mnt/ramdisk/file*
sudo tests/runTestsIntInsertRemoveParallel /mnt/ramdisk/fileIntInsertRemoveParallel
sudo rm -rf /mnt/ramdisk/file*

# run NvmHashMap<string, string> correctness tests
sudo tests/runTestsStringIterate /mnt/ramdisk/fileStringIterate
sudo rm -rf /mnt/ramdisk/file*
sudo tests/runTestsStringInsertGet /mnt/ramdisk/fileStringInsertGet
sudo rm -rf /mnt/ramdisk/file*
sudo tests/runTestsStringInsertRemove /mnt/ramdisk/fileStringInsertRemove
sudo rm -rf /mnt/ramdisk/file*
sudo tests/runTestsStringInsertGetParallel /mnt/ramdisk/fileStringInsertGetParallel
sudo rm -rf /mnt/ramdisk/file*
sudo tests/runTestsStringInsertRemoveParallel /mnt/ramdisk/fileStringInsertRemoveParallel
sudo rm -rf /mnt/ramdisk/file*

# run NvmHashMap<int, int> performance tests
sudo tests/runTestsPerformance1 /mnt/ramdisk/filePerformance1
sudo rm -rf /mnt/ramdisk/file*
sudo tests/runTestsPerformance2 /mnt/ramdisk/filePerformance2
sudo rm -rf /mnt/ramdisk/file*
sudo tests/runTestsPerformance4 /mnt/ramdisk/filePerformance4
sudo rm -rf /mnt/ramdisk/file*
sudo tests/runTestsPerformance8 /mnt/ramdisk/filePerformance8
sudo rm -rf /mnt/ramdisk/file*
sudo tests/runTestsPerformance16 /mnt/ramdisk/filePerformance16
sudo rm -rf /mnt/ramdisk/file*

# run unordered_map performance tests
sudo tests/runTestsPerformanceHashMap

# merge performance results
cd tests/results
paste -d, Outline.csv NvmHashMap.csv HashMap.csv > Combined.csv

