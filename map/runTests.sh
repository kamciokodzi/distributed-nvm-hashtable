sudo rm performance.txt
cd tests
make
cd /mnt/mem 
sudo rm -rf file*
cd -
make
sudo ./runTestsPerformance /mnt/mem/filePerformance
sudo ./runTestsStr /mnt/mem/fileStr
sudo ./runTestsInt /mnt/mem/fileInt

