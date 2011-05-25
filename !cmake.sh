cd /var/tmp
mkdir programming
cd programming
mkdir SitesChecker
cd SitesChecker

mkdir build_unix_release
cd build_unix_release
cmake -D CMAKE_BUILD_TYPE=Release /mnt/work/program/SitesChecker
cd ..

mkdir build_unix_debug
cd build_unix_debug
cmake -D CMAKE_BUILD_TYPE=Debug /mnt/work/program/SitesChecker
cd ..

