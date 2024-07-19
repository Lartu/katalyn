set -e
python3.11 -m nuitka --standalone --onefile --deployment kat.py
rm -rf kat.build
rm -rf kat.dist
rm -rf kat.onefile-build
mv kat.bin ~/bin/kat
echo 'print("Compilation successful!");' | kat -s