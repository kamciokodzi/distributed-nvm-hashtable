cd tests
make
cd .. 
sudo rm -rf file*
make
./tests/runTestsStr fileStr
./tests/runTestsInt fileInt

