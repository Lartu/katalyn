set -e
# g++ -std=c++11 -stdlib=libc++ -target arm64-apple-macos narivm.cpp -o narivm
g++ -std=c++11 narivm.cpp -o narivm -O3
kat -i cpptest.kat | ./narivm
