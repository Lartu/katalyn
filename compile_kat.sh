set -e

# Compile the Katalyn Compiler (Python3)
# If you don't have nuitka, install it via python3 -m pip install nuitka
python3 -m nuitka --standalone --onefile --deployment kat.py
rm -rf kat.build
rm -rf kat.dist
rm -rf kat.onefile-build
mv kat.bin ~/bin/kat
