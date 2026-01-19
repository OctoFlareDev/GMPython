#include <pybind11/embed.h>
namespace py = pybind11;

#include <string>

#ifdef _WIN32
#define GMEXPORT extern "C" __declspec (dllexport)
#else
#define GMEXPORT extern "C"
#endif

#if defined(__linux__)
  #include <dlfcn.h>
  #include <stdio.h>
  #include <string.h>
  #include <string>

// Return directory of this .so using dladdr
static std::string this_so_dir() {
    Dl_info info{};
    if (dladdr((void*)&this_so_dir, &info) && info.dli_fname) {
        std::string path = info.dli_fname;
        auto slash = path.find_last_of('/');
        if (slash != std::string::npos) return path.substr(0, slash);
    }
    return ".";
}

static void ensure_libpython_global() {
    const char* soname = "libpython3.8.so.1.0";

    // 1) If already loaded, "promote" it to GLOBAL (works on glibc)
    void* h = dlopen(soname, RTLD_NOW | RTLD_GLOBAL | RTLD_NOLOAD);
    if (h) return;

    // 2) Try loading from same folder as this extension .so (common GM layout)
    std::string local = this_so_dir() + "/" + soname;
    h = dlopen(local.c_str(), RTLD_NOW | RTLD_GLOBAL);
    if (h) return;

    // 3) Fallback: try normal loader search paths
    h = dlopen(soname, RTLD_NOW | RTLD_GLOBAL);
    if (h) return;

    fprintf(stderr, "[pygml] Failed to dlopen %s RTLD_GLOBAL: %s\n", soname, dlerror());
}

// Run as soon as the extension .so is loaded (before your exported funcs run)
__attribute__((constructor))
static void pygml_ctor() {
    ensure_libpython_global();
}
#endif

struct buffer {
    char* pos;
public:
    buffer(char* origin) : pos(origin) {}
    template<class T> T read() {
        T r = *(T*)pos;
        pos += sizeof(T);
        return r;
    }
    template<class T> void write(T val) {
        *(T*)pos = val;
        pos += sizeof(T);
    }
    //
    char* read_string() {
        char* r = pos;
        while (*pos != 0) pos++;
        pos++;
        return r;
    }
    void write_string(const char* s) {
        for (int i = 0; s[i] != 0; i++) write<char>(s[i]);
        write<char>(0);
    }
};

GMEXPORT const char* _python_initialize() {
    py::initialize_interpreter();
    return "";
}

GMEXPORT const char* _python_finalize() {
    py::finalize_interpreter();
    return "";
}

GMEXPORT double _python_call_function(char* cbuf) {
    // Set up buffer for writing result
    buffer b(cbuf);

    // Read arguments from buffer
    char* module = b.read_string();
    char* callable = b.read_string();
    char* args = b.read_string();
    char* kwargs = b.read_string();
    
    try {
        // Load JSON module for serializing and parsing args/result
        py::module_ json = py::module_::import("json");
        py::object json_dumps = json.attr("dumps");
        py::object json_loads = json.attr("loads");

        // Implement default conversion to 'str' for unsupported types
        //
        // TODO
        //
 
        // Import module and parse arguments for call
        py::module_ pModule = py::module_::import(module);
        py::object pFunc = pModule.attr(callable);
        py::object pArgs = json_loads(args);
        py::object pKwargs = json_loads(kwargs);

        // Wrap single argument into a list
        py::list pArgsList;
        if (!py::isinstance<py::list>(pArgs)) {
            pArgsList.append(pArgs);
        }
        else {
            pArgsList = pArgs;
        }

        // Call function and return result as JSON string
        py::object result = pFunc(*pArgsList, **pKwargs);
        std::string sResult = json_dumps(result).cast<std::string>();
        const char* cstr = sResult.c_str();
        b.write_string(cstr);
        return 1;
    }
    catch (py::error_already_set& e) {
        // Return exception and traceback as a string
        const char* exc = e.what();
        b.write_string(exc);
        return -1;
    }
}
