set -e

sh compile_nvm.sh
sh compile_kat.sh

echo 'print("Katalyn compilation successful!");' | kat -s
