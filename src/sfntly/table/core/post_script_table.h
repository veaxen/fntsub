//
// Created by veaxen on 18-10-31.
//

#ifndef FONT_SUBSETTER_POST_SCRIPT_TABLE_H
#define FONT_SUBSETTER_POST_SCRIPT_TABLE_H

#include "sfntly/table/table.h"
#include <sfntly/table/table_based_table_builder.h>

namespace sfntly {

class PostScriptTable : public Table,
                        public RefCounted<PostScriptTable>{
public:
    static const int32_t VERSION_1;
    static const int32_t VERSION_2;
    static const int32_t NUM_STANDARD_NAMES;

    struct Offset {
        enum {
            kVersion = 0,
            kItalicAngle = 4,
            kUnderlinePosition = 8,
            kUnderlineThickness = 10,
            kIsFixedPitch = 12,
            kMinMemType42 = 16,
            kMaxMemType42 = 20,
            kMinMemType1 = 24,
            kMaxMemType1 = 28,
            kNumberOfGlyphs = 32,
            kGlyphNameIndex = 34,
        };
    };

    static const char* const STANDARD_NAMES[];

    PostScriptTable(Header* header, ReadableFontData* data);
    int32_t Version();
    int64_t IsFixedPitchRaw();
    int32_t NumberOfGlyphs();

    std::string GlyphName(int32_t glyphNum);

private:
    std::vector<std::string> getNames();
    std::vector<std::string> parse();


    std::vector<std::string> names_;

public:
    class Builder: public TableBasedTableBuilder, public RefCounted<Builder> {
    public:
        Builder(Header* header, WritableFontData* data);
        Builder(Header* header, ReadableFontData* data);
        virtual ~Builder() {};
        virtual CALLER_ATTACH FontDataTable* SubBuildTable(ReadableFontData* data);

        static CALLER_ATTACH Builder* CreateBuilder(Header* header,
                                                    WritableFontData* data);

    };
};

}


#endif //FONT_SUBSETTER_POST_SCRIPT_TABLE_H
