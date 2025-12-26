#include <map>
#include <iostream>
#include <string>
#include <cstdint>
#include <sstream>
#include <iostream>
#include <vector>
#include <fstream>
#include <algorithm>

struct TRoutineData {
    std::string name;
    std::uint64_t cycles;
    
    TRoutineData (const std::string &name): name (name), cycles (0) {}
};

struct TKeyData {
    std::uint16_t addr;
    std::uint16_t bank;
    
    TKeyData (std::uint16_t addr, std::uint16_t bank): addr (addr), bank (bank) {}
};

bool operator < (const TKeyData &a, const TKeyData &b) {
    return a.bank < b.bank || (a.bank == b.bank && a.addr < b.addr);
}

using TProfilerMap = std::map<TKeyData, TRoutineData *>;

TProfilerMap profilerMap;
std::vector<TRoutineData *> routineVec;
std::uint64_t totalCycles = 0;

void appendMapLine (const std::string &s) {
    std::stringstream ss (s);
    char dummy;
    std::uint16_t bank;
    std::uint16_t addr, length;
    std::string name;
    if (ss >> dummy >> bank >> std::hex >> addr >> std::dec >> length >> name) {
        TRoutineData *routineData = new TRoutineData (name);
        routineVec.push_back (routineData);
        for (std::uint16_t v = addr; v < addr + length; v += 2) {
//            std::cout << "ENTER: " << v << ':' << bank << std::endl;
            profilerMap [TKeyData (v, bank)] = routineData;
        }
    }
}

void readPerformanceData (const std::string &fn) {
    std::ifstream f (fn);
    std::string s;
    std::getline (f, s);
    std::uint16_t addr;
    std::uint16_t bank;
    std::uint64_t count, cycles;
    std::cout << "Entries in map: " << profilerMap.size () << std::endl;
    while (f >> std::hex >> addr >> std::dec >> bank >> count >> cycles) {
        TProfilerMap::iterator it = profilerMap.find (TKeyData (addr, bank));
//        std::cout << addr << ':' << bank << ':' << cycles << std::endl;
        if (it != profilerMap.end ()) {
            it->second->cycles += cycles;
            totalCycles += cycles;
        }
    }
}

void readMapData (const std::string &fn) {
    std::string s;
    std::ifstream f (fn);
    do 
        std::getline (f, s);
    while (s != "; Bank Addr Length Name");
    while (!f.eof ()) {
        std::getline (f, s);
        appendMapLine (s);
    }
}

int main (int argc, char **argv) {
    readMapData (argv [1]);
    readPerformanceData (argv [2]);
    
    std::sort (routineVec.begin (), routineVec.end (), [] (TRoutineData *a, TRoutineData *b) { return a->cycles > b->cycles; });

    for (TRoutineData *p: routineVec)
        printf ("%12ld %4.1lf%% %s\n", p->cycles, 100.0 * static_cast<double> (p->cycles) / totalCycles, p->name.c_str ());    
}
