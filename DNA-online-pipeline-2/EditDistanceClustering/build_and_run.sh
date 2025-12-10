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


time ./HierarchicalAdjKMeans test_data_candidates.txt 

pingme "DONE!"