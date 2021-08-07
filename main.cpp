#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

using namespace std;
using filesystem::path;

path operator""_p(const char* data, std::size_t sz) {
    return path(data, data + sz);
}

bool Preprocessreq(ifstream& in_file, ofstream& out_file, path name_infile, const vector<path>& direc) {
    if (!in_file.is_open()) {
        return false;
    }
    static regex first(R"/(\s*#\s*include\s*"([^"]*)"\s*)/");
    static regex second(R"/(\s*#\s*include\s*<([^>]*)>\s*)/");
    smatch m;
    std::string buffer;
    int line = 0;
    while(getline(in_file, buffer)) {
        line++;
        bool flag = false;
        if (regex_match(buffer, m, first)) {
            path p = string(m[1]);
            path search_file = name_infile.parent_path() / p;
            ifstream file(search_file);
            if (file.is_open()) {
                flag = true;
                Preprocessreq(file, out_file, search_file, direc);
            } else {
                for(auto& el : direc) {
                    search_file = el / p;
                    ifstream file(search_file);
                    if (file.is_open()) {
                        flag = true;
                        Preprocessreq(file, out_file, search_file, direc);
                    }
                }
            }
            if (!flag) {
                std::cout << "unknown include file "s << p.string() << " at file "s << name_infile.string()
                          << " at line "s << line << std::endl;
                return false;
            }
        } else if (regex_match(buffer, m, second)) {
            path p = string(m[1]);
            for(auto& el : direc) {
                path search_file = el / p;
                ifstream file(search_file);
                if (file.is_open()) {
                    flag = true;
                    Preprocessreq(file, out_file, search_file, direc);
                }
            }
            if (!flag) {
                std::cout << "unknown include file "s << p.string() << " at file "s << name_infile.string()
                          << " at line "s << line << std::endl;
                return false;
            }
        } else {
            out_file << buffer << std::endl;
        }
    }
    return true;
}

// напишите эту функцию
bool Preprocess(const path& in_file, const path& out_file, const vector<path>& include_directories) {
    ifstream infile(in_file);
    if (!infile) {
        return false;
    }
    ofstream outfile(out_file);
    return Preprocessreq(infile, outfile, in_file, include_directories);
}

string GetFileContents(string file) {
    ifstream stream(file);

    // конструируем string по двум итераторам
    return {(istreambuf_iterator<char>(stream)), istreambuf_iterator<char>()};
}

void Test() {
    error_code err;
    filesystem::remove_all("sources"_p, err);
    filesystem::create_directories("sources"_p / "include2"_p / "lib"_p, err);
    filesystem::create_directories("sources"_p / "include1"_p, err);
    filesystem::create_directories("sources"_p / "dir1"_p / "subdir"_p, err);

    {
        ofstream file("sources/a.cpp");
        file << "// this comment before include\n"
                "#include \"dir1/b.h\"\n"
                "// text between b.h and c.h\n"
                "#include \"dir1/d.h\"\n"
                "\n"
                "int SayHello() {\n"
                "    cout << \"hello, world!\" << endl;\n"
                "#   include<dummy.txt>\n"
                "}\n"sv;
    }
    {
        ofstream file("sources/dir1/b.h");
        file << "// text from b.h before include\n"
                "#include \"subdir/c.h\"\n"
                "// text from b.h after include"sv;
    }
    {
        ofstream file("sources/dir1/subdir/c.h");
        file << "// text from c.h before include\n"
                "#include <std1.h>\n"
                "// text from c.h after include\n"sv;
    }
    {
        ofstream file("sources/dir1/d.h");
        file << "// text from d.h before include\n"
                "#include \"lib/std2.h\"\n"
                "// text from d.h after include\n"sv;
    }
    {
        ofstream file("sources/include1/std1.h");
        file << "// std1\n"sv;
    }
    {
        ofstream file("sources/include2/lib/std2.h");
        file << "// std2\n"sv;
    }

    assert((!Preprocess("sources"_p / "a.cpp"_p, "sources"_p / "a.in"_p,
                                  {"sources"_p / "include1"_p,"sources"_p / "include2"_p})));

    ostringstream test_out;
    test_out << "// this comment before include\n"
                "// text from b.h before include\n"
                "// text from c.h before include\n"
                "// std1\n"
                "// text from c.h after include\n"
                "// text from b.h after include\n"
                "// text between b.h and c.h\n"
                "// text from d.h before include\n"
                "// std2\n"
                "// text from d.h after include\n"
                "\n"
                "int SayHello() {\n"
                "    cout << \"hello, world!\" << endl;\n"sv;

    assert(GetFileContents("sources/a.in"s) == test_out.str());
}

int main() {
    Test();
}

