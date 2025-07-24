#include <iostream>
#include <iterator>
#include <cstring>
#include <unistd.h>
#include <sys/mman.h>
#include <filesystem>
#include <iterator>
#include <chrono>

#include "compiler.hpp"
#include "x64generator.hpp"
#include "a64gen.hpp"
#include "tms9900gen.hpp"
#include "runtime.hpp"

namespace sp = statpascal;

bool haveParameter (const char *s, int &argc, char **argv) {
    for (int i = 1; i < argc; ++i)
        if (!std::strcmp (s, argv [i])) {
            for (; i + 1 < argc; ++i)
                argv [i] = argv [i + 1];
            --argc;
            return true;
        }
    return false;
}

std::int64_t getSystemMemory () {
    return static_cast<std::int64_t> (sysconf (_SC_PHYS_PAGES)) * sysconf (_SC_PAGE_SIZE);
}

#ifdef __amd64
#define CREATE_X64
#endif

class TTimer {
public:
    TTimer (const std::string &msg): msg (msg) { 
        t1 = std::chrono::high_resolution_clock::now (); 
    }
    
    ~TTimer () { 
        t2 = std::chrono::high_resolution_clock::now (); 
        if (!msg.empty ())
            std::cout << msg << "_ " << (std::chrono::duration_cast<std::chrono::duration<double>> (t2 - t1)).count () << " secs" << std::endl; 
    }
    
private:
    const std::string msg;
    std::chrono::high_resolution_clock::time_point t1, t2;
};


void compile9900 (int argc, char **argv) {
    sp::TRuntimeData runtimeData;
    sp::T9900Generator generator (runtimeData);
    sp::TCompiler compiler (generator);
    
    compiler.setFilename (argv [1]);
    std::filesystem::path exepath = std::filesystem::read_symlink ("/proc/self/exe");
    compiler.setUnitSearchPathes ({".", exepath.parent_path ().string () + "/../ti99units"});
    
    compiler.compile ();
    
    std::vector<std::string> listing;
    std::vector<std::uint8_t> opcodes;
    generator.getAssemblerCode (opcodes, true, listing);
    
    std::ofstream f ("out.a99");
    std::copy (listing.begin (), listing.end (), std::ostream_iterator<std::string> (f, "\n"));
}

void compile (int argc, char **argv) {
#ifdef CREATE_9900
        compile9900 (argc, argv);
        return;
#endif        

    bool createListing = haveParameter ("--listing", argc, argv),
         showTimes = haveParameter ("--time", argc, argv);
    
    sp::TRuntimeData runtimeData;
//    printf ("Runtime is at %p\n", &runtimeData);
    runtimeData.appendArgv (argv [0]);
    for (int i = 2; i < argc; ++i)
        runtimeData.appendArgv (argv [i]);
        
#ifdef CREATE_X64
    sp::TX64Generator generator (runtimeData);
#else
    createListing = true;	// asm source required
    sp::TA64Generator generator (runtimeData);
#endif    
    
    sp::TCompiler compiler (generator);
    compiler.setFilename (argv [1]);
    
    std::filesystem::path exepath = std::filesystem::read_symlink ("/proc/self/exe");
    compiler.setUnitSearchPathes ({".", exepath.parent_path ().string () + "/../units"});
    
    {
        TTimer t (showTimes ? "Compile time" : "");
        compiler.compile ();
    }
    
    std::vector<std::string> listing;
    std::vector<std::uint8_t> opcodes;
    
    {
        TTimer t (showTimes ? "Assembly time" : "");
        generator.getAssemblerCode (opcodes, createListing, listing);
    }

    if (createListing) {
        std::ofstream f ("out.asm");
        std::copy (listing.begin (), listing.end (), std::ostream_iterator<std::string> (f, "\n"));
    }

#ifdef CREATE_X64
    if (createListing) {
        std::ofstream g ("out.bin");
        g.write ((char *) opcodes.data (), opcodes.size ());
        g.close ();
    }
#else
    // ARM64 has no internal assembler yet
    std::size_t start = 0x40000;	// start address  does not matter - code is position independent
    std::stringstream cmd;
    cmd << "as -EL out.asm -o out.o; ld -Ttext " << std::hex << start << " out.o -o out.link; objcopy -O binary out.link out.bin";
    system (cmd.str ().c_str ());
    
    opcodes.reserve (std::filesystem::file_size ("out.bin"));
    std::ifstream f ("out.bin", std::ios::binary);
    opcodes.assign (std::istreambuf_iterator<char> (f), std::istreambuf_iterator<char> ());
#endif    

    void *p = mmap (nullptr, opcodes.size (), PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    memcpy (p, opcodes.data (), opcodes.size ());
    if (mprotect (p, opcodes.size (), PROT_READ | PROT_EXEC)) {
        perror ("SP mprotect");
        exit (1);
    }
    reinterpret_cast<void (*)()> (p) ();
    mprotect (p, opcodes.size (), PROT_READ | PROT_WRITE);
    munmap (p, opcodes.size ());
}

int main (int argc, char **argv) {
    compile (argc, argv);
}

