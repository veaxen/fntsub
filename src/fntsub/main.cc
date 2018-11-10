#include <time.h>
#include <iostream>

#include <stdio.h>
#include <stdlib.h>

#include <locale>
#include <codecvt>
#include <string>
#include <map>
#include <utility>

#include "sfntly/font.h"
#include "subtly/character_predicate.h"
#include "subtly/stats.h"
#include "subtly/subsetter.h"
#include "subtly/utils.h"

using namespace subtly;

void PrintUsage(const char* program_name) {
    fprintf(stdout, "Usage: %s <input_font_file> <output_font_file>"
                    " <string>\n", program_name);
}

std::wstring StringToWstring(const char* utf8Bytes)
{
    using convert_type = std::codecvt_utf8<typename std::wstring::value_type >;
    std::wstring_convert<convert_type, typename std::wstring::value_type> converter;
    return converter.from_bytes(utf8Bytes);
}

//根据路径获取文件名
std::string GetPathOrURLShortName(const std::string &strFullName) {
    if (strFullName.empty()){
        return "";
    }
    std::string::size_type iPos = strFullName.find_last_of('/') + 1;
    return strFullName.substr(iPos, strFullName.length() - iPos);
}

//解析出所有路径,以"|"为分割
std::vector<std::string> GetAllFontPath(const std::string &allPath) {
    std::vector<std::string> result;
    if(allPath.empty()) {
        return result;
    }

    std::string::size_type start = 0, skip = 1;
    while (start != std::string::npos){
        std::string::size_type finsh = allPath.find('|', start);
        std::string token = allPath.substr(start, finsh - start);
        if (!token.empty()){
            result.push_back(token);
        }
        if ((start = finsh) != std::string::npos) {
            start = start + skip;
        }
    }

    return result;
}

int Subset(const char*, const char*, const char*);

int main(int argc, const char* argv[]) {
    const char* program_name = argv[0];
    if (argc < 4) {
        PrintUsage(program_name);
        exit(1);
    }
    clock_t start,end;
    start = clock();
    const char* input_font_paths = argv[1];
    const char* output_font_path = argv[2];
    const char* text = argv[3];
    std::vector<std::string> allPath = GetAllFontPath(input_font_paths);

    for (const auto &path : allPath) {
        Subset(path.data(), output_font_path, text);
    }
    end = clock();
    printf("转换耗时 %.2f 毫秒", (end - start)/(double)CLOCKS_PER_SEC*1000);

    return 0;
}

int Subset(const char* font_path, const char* output_dir, const char* text) {
    FontPtr font;
    font.Attach(subtly::LoadFont(font_path));
    if (font->num_tables() == 0) {
        fprintf(stderr, "Could not load font %s.\n", font_path);
        exit(1);
    }

    std::wstring wstr = StringToWstring(text);
    auto charaters = new std::set<int32_t >;
    for (auto &unicode : wstr) {
        charaters->insert((int32_t)unicode);
    }

    Ptr<CharacterPredicate> set_predicate =
            new AcceptSet(charaters);
    Ptr<Subsetter> subsetter = new Subsetter(font, set_predicate);
    Ptr<Font> new_font;
    new_font.Attach(subsetter->Subset());
    if (!new_font) {
        fprintf(stderr, "Cannot create subset.\n");
        exit(1);
    }

    auto file_name = GetPathOrURLShortName(font_path);
    auto output_path = output_dir + std::string("/") + file_name;
    bool success = subtly::SerializeFont(output_path.data(), new_font);
    if (!success) {
        fprintf(stderr, "Cannot create font file.\n");
        exit(1);
    }
    return 0;
}
