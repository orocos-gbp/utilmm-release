#include <utilmm/system/system.hh>
#include <utilmm/system/process.hh>
#include <utilmm/configfile/pkgconfig.hh>
#include <utilmm/configfile/exceptions.hh>
#include <utilmm/stringtools.hh>

using namespace utilmm;
using std::string;
using std::list;

pkgconfig::pkgconfig(string const& name_)
    : m_name(name_)
{
    if(!exists(name_))
        throw not_found(name_);
}

pkgconfig::~pkgconfig() {}

std::list<string> pkgconfig::packages()
{
    process prs;
    prs << "pkg-config" << "--list-all";
    string output = run(prs);
    // strip output
    string::size_type first = output.find_first_not_of(" \t\n");
    if (first == string::npos) output = string();
    string::size_type last  = output.find_last_not_of(" \t\n");
    if (last == string::npos) output = string(output, first);

    string out = string(output, first, last - first + 1);
    std::list<string> lines = split(out, "\n");
    // remove the package descriptions
    for (list<string>::iterator it = lines.begin(); it != lines.end(); ++it)
    {
        list<string> fields = split(*it, " ");
        *it = fields.front();
    }

    return lines;
}

string pkgconfig::name() const { return m_name; }
string pkgconfig::version() const { return run("--modversion"); }

bool pkgconfig::exists(string const& name)
{
    process prs;
    prs << "pkg-config" << "--exists" << name;

    try { run(prs); return true; } 
    catch(not_found) { return false; }
}

string pkgconfig::get(string const& varname, string const& defval) const
{
    try { return run("--variable=" + varname); }
    catch(pkgconfig_error) { return defval; } // pkg-config 0.19 crashes when varname does not exist ...
    catch(not_found) { return defval; }
}

static const char* const compiler_flags[] = { "--cflags", "--cflags-only-I", "--cflags-only-other" };
string pkgconfig::compiler(Modes mode) const
{ return run(compiler_flags[mode]); }

static const char* const linker_flags[] = { "--libs", "--libs-only-L", "--libs-only-other", "--static", "--libs-only-l",  };
string pkgconfig::linker(Modes mode) const
{ return run(linker_flags[mode]); }




#include <iostream>
string pkgconfig::run(string const& argument) const
{
    process prs;
    prs << "pkg-config" << argument << m_name;
    string output = run(prs);
    // strip output
    string::size_type first = output.find_first_not_of(" \t\n");
    if (first == string::npos) return string();
    string::size_type last  = output.find_last_not_of(" \t\n");
    if (last == string::npos) return string(output, first);
    return string(output, first, last - first + 1);
}

string pkgconfig::run(process& prs)
{
    int pipeno[2];
    pipe(pipeno);

    prs.redirect_to(process::Stdout, pipeno[1]);
    prs.redirect_to(process::Stderr, "/dev/null");
    prs.start();

    string output;
    while(true) 
    {
        char buffer[256];
        int read_count = read(pipeno[0], buffer, 256);
        if (read_count == -1)
            throw unix_error();
        if (read_count == 0)  break;
        output += string(buffer, read_count);
    }
    prs.wait();

    if (!prs.exit_normal()) 
        throw pkgconfig_error();
    if (prs.exit_status())  
        throw not_found(prs.cmdline().front());

    return output;
}


