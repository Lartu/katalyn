set -e

sh compile_nvm.sh

# Compile the Katalyn Compiler (Python3)
python3.11 -m nuitka --standalone --onefile --deployment kat.py
rm -rf kat.build
rm -rf kat.dist
rm -rf kat.onefile-build
mv kat.bin ~/bin/kat

echo 'print("Katalyn compilation successful!");' | kat -s
