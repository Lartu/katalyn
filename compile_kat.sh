set -e

# Compile the Katalyn Compiler (Python3)
python3 -m pip install nuitka --user --break-system-packages
python3 -m nuitka --standalone --onefile --deployment kat.py
rm -rf kat.build
rm -rf kat.dist
rm -rf kat.onefile-build
mv kat.bin ~/bin/kat
