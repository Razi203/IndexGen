#!/bin/bash
set -e

# Compile
echo "Compiling..."
# Compile DNAV8.cpp with main hidden
g++ -std=c++17 -O3 -c ../CPL/DNAV8.cpp -o DNAV8.o -Dmain=DNAV8_main_ignored -I. -I.. -I../CPL

# Compile main project linking against DNAV8.o
g++ -std=c++17 -O3 -o HierarchicalAdjKMeans \
    HierarchicalAdjKMeans.cpp \
    HDEQED.cpp \
    ../utils.cpp \
    ../CPL/DNAV8_amir.cpp \
    ../CPL/Cluster.cpp \
    ../CPL/FreqFunctions.cpp \
    ../CPL/GuessFunctions.cpp \
    DNAV8.o \
    ../CPL/EditDistance.cpp \
    ../CPL/Graph.cpp \
    ../CPL/Utils.cpp \
    -I. -I.. -I../CPL

if [ $? -eq 0 ]; then
    echo "Compilation successful."
else
    echo "Compilation failed."
    exit 1
fi

# function to send notifications
function pingme() {
    local msg="${1:-Job Done!}"
    curl -d "$msg" ntfy.sh/IndexGen-Newton
}


INPUT_FILE="test_data_candidates.txt"

# # Run 1: h=1000
# echo "Running h=1000..."
# start=$SECONDS
# ./HierarchicalAdjKMeans "$INPUT_FILE" 1000
# duration=$(( SECONDS - start ))
# pingme "Finished h=1000 in ${duration}s"

# # Run 2: h=2000
# echo "Running h=2000..."
# start=$SECONDS
# ./HierarchicalAdjKMeans "$INPUT_FILE" 2000
# duration=$(( SECONDS - start ))
# pingme "Finished h=2000 in ${duration}s"

# Run 3: h={100, 100}
#echo "Running h={100, 100}..."
#start=$SECONDS
#./HierarchicalAdjKMeans "$INPUT_FILE" 100 100
#duration=$(( SECONDS - start ))
#pingme "Finished h={100, 100} in ${duration}s"

# Run 4: h={50, 50, 5}
echo "Running h={50, 50, 5}..."
start=$SECONDS
./HierarchicalAdjKMeans "$INPUT_FILE" 50 50 5
duration=$(( SECONDS - start ))
pingme "Finished h={50, 50, 5} in ${duration}s"
