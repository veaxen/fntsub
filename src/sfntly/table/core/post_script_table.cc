//
// Created by veaxen on 18-10-31.
//


#include "post_script_table.h"

namespace sfntly {

const int32_t PostScriptTable::VERSION_1 = 0x10000;
const int32_t PostScriptTable::VERSION_2 = 0x20000;
const int32_t PostScriptTable::NUM_STANDARD_NAMES = 258;

const char* const PostScriptTable::STANDARD_NAMES[] = {
        ".notdef",
        ".null",
        "nonmarkingreturn",
        "space",
        "exclam",
        "quotedbl",
        "numbersign",
        "dollar",
        "percent",
        "ampersand",
        "quotesingle",
        "parenleft",
        "parenright",
        "asterisk",
        "plus",
        "comma",
        "hyphen",
        "period",
        "slash",
        "zero",
        "one",
        "two",
        "three",
        "four",
        "five",
        "six",
        "seven",
        "eight",
        "nine",
        "colon",
        "semicolon",
        "less",
        "equal",
        "greater",
        "question",
        "at",
        "A",
        "B",
        "C",
        "D",
        "E",
        "F",
        "G",
        "H",
        "I",
        "J",
        "K",
        "L",
        "M",
        "N",
        "O",
        "P",
        "Q",
        "R",
        "S",
        "T",
        "U",
        "V",
        "W",
        "X",
        "Y",
        "Z",
        "bracketleft",
        "backslash",
        "bracketright",
        "asciicircum",
        "underscore",
        "grave",
        "a",
        "b",
        "c",
        "d",
        "e",
        "f",
        "g",
        "h",
        "i",
        "j",
        "k",
        "l",
        "m",
        "n",
        "o",
        "p",
        "q",
        "r",
        "s",
        "t",
        "u",
        "v",
        "w",
        "x",
        "y",
        "z",
        "braceleft",
        "bar",
        "braceright",
        "asciitilde",
        "Adieresis",
        "Aring",
        "Ccedilla",
        "Eacute",
        "Ntilde",
        "Odieresis",
        "Udieresis",
        "aacute",
        "agrave",
        "acircumflex",
        "adieresis",
        "atilde",
        "aring",
        "ccedilla",
        "eacute",
        "egrave",
        "ecircumflex",
        "edieresis",
        "iacute",
        "igrave",
        "icircumflex",
        "idieresis",
        "ntilde",
        "oacute",
        "ograve",
        "ocircumflex",
        "odieresis",
        "otilde",
        "uacute",
        "ugrave",
        "ucircumflex",
        "udieresis",
        "dagger",
        "degree",
        "cent",
        "sterling",
        "section",
        "bullet",
        "paragraph",
        "germandbls",
        "registered",
        "copyright",
        "trademark",
        "acute",
        "dieresis",
        "notequal",
        "AE",
        "Oslash",
        "infinity",
        "plusminus",
        "lessequal",
        "greaterequal",
        "yen",
        "mu",
        "partialdiff",
        "summation",
        "product",
        "pi",
        "integral",
        "ordfeminine",
        "ordmasculine",
        "Omega",
        "ae",
        "oslash",
        "questiondown",
        "exclamdown",
        "logicalnot",
        "radical",
        "florin",
        "approxequal",
        "Delta",
        "guillemotleft",
        "guillemotright",
        "ellipsis",
        "nonbreakingspace",
        "Agrave",
        "Atilde",
        "Otilde",
        "OE",
        "oe",
        "endash",
        "emdash",
        "quotedblleft",
        "quotedblright",
        "quoteleft",
        "quoteright",
        "divide",
        "lozenge",
        "ydieresis",
        "Ydieresis",
        "fraction",
        "currency",
        "guilsinglleft",
        "guilsinglright",
        "fi",
        "fl",
        "daggerdbl",
        "periodcentered",
        "quotesinglbase",
        "quotedblbase",
        "perthousand",
        "Acircumflex",
        "Ecircumflex",
        "Aacute",
        "Edieresis",
        "Egrave",
        "Iacute",
        "Icircumflex",
        "Idieresis",
        "Igrave",
        "Oacute",
        "Ocircumflex",
        "apple",
        "Ograve",
        "Uacute",
        "Ucircumflex",
        "Ugrave",
        "dotlessi",
        "circumflex",
        "tilde",
        "macron",
        "breve",
        "dotaccent",
        "ring",
        "cedilla",
        "hungarumlaut",
        "ogonek",
        "caron",
        "Lslash",
        "lslash",
        "Scaron",
        "scaron",
        "Zcaron",
        "zcaron",
        "brokenbar",
        "Eth",
        "eth",
        "Yacute",
        "yacute",
        "Thorn",
        "thorn",
        "minus",
        "multiply",
        "onesuperior",
        "twosuperior",
        "threesuperior",
        "onehalf",
        "onequarter",
        "threequarters",
        "franc",
        "Gbreve",
        "gbreve",
        "Idotaccent",
        "Scedilla",
        "scedilla",
        "Cacute",
        "cacute",
        "Ccaron",
        "ccaron",
        "dcroat"
};

PostScriptTable::PostScriptTable(sfntly::Header *header, sfntly::ReadableFontData *data)
    :Table(header, data) {

}

int32_t PostScriptTable::Version() {
    return this->data_->ReadFixed(Offset::kVersion);
}

int64_t PostScriptTable::IsFixedPitchRaw() {
    return this->data_->ReadULong(Offset::kIsFixedPitch);
}

int32_t PostScriptTable::NumberOfGlyphs() {
    if (this->Version() == VERSION_1) {
        return NUM_STANDARD_NAMES;
    } else if (this->Version() == VERSION_2) {
        return this->data_->ReadUShort(Offset::kNumberOfGlyphs);
    } else {
        // TODO: should probably be better at signaling unsupported format
        return -1;
    }
}

std::string PostScriptTable::GlyphName(int32_t glyphNum) {
    int32_t numberOfGlyphs = this->NumberOfGlyphs();
    if (numberOfGlyphs > 0 && (glyphNum < 0 || glyphNum >= numberOfGlyphs)) {
        fprintf(stderr, "numberOfGlyphs > 0 && (glyphNum < 0 || glyphNum >= numberOfGlyphs)");
        return "";
    }
    int32_t glyphNameIndex = 0;
    if (this->Version() == VERSION_1){
        glyphNameIndex = glyphNum;
    } else if (this->Version() == VERSION_2) {
        glyphNameIndex = this->data_->ReadUShort(Offset::kGlyphNameIndex + 2 * glyphNum);
    } else {
        return "";
    }

    if (glyphNameIndex < NUM_STANDARD_NAMES) {
        return STANDARD_NAMES[glyphNameIndex];
    }
    return getNames()[glyphNameIndex - NUM_STANDARD_NAMES];
}

std::vector<std::string> PostScriptTable::getNames() {
    std::vector<std::string> result = names_;
    if (result.empty() && this->Version() == VERSION_2){
        result = this->parse();
        names_ = result;
    }
    return result;
}

std::vector<std::string> PostScriptTable::parse() {
    std::vector<std::string> names;
    if (this->Version() == VERSION_2) {
        int32_t  index = Offset::kGlyphNameIndex + 2 * NumberOfGlyphs();
        while (index < this->DataLength()) {
            int32_t strlen = this->data_->ReadUByte(index);
            uint8_t* nameBytes = new uint8_t[strlen];
            this->data_->ReadBytes(index + 1, nameBytes, 0, strlen);
            names.emplace_back((char*)nameBytes);
            index += 1 + strlen;
            delete[] nameBytes;
        }
    } else if (Version() == VERSION_1) {
        // 不支持
        fprintf(stderr, "Not meaningful to parse version 1 table");
        return std::vector<std::string>();
    }
    return names;
}

PostScriptTable::Builder::Builder(sfntly::Header *header, sfntly::ReadableFontData *data)
    : TableBasedTableBuilder(header, data) {

}

PostScriptTable::Builder::Builder(sfntly::Header *header, sfntly::WritableFontData *data)
    : TableBasedTableBuilder(header, data) {

}

CALLER_ATTACH
PostScriptTable::Builder * PostScriptTable::Builder::CreateBuilder(sfntly::Header *header,
                                                                   sfntly::WritableFontData *data) {
    Ptr<PostScriptTable::Builder> builder;
    builder = new PostScriptTable::Builder(header, data);
    return builder.Detach();
}

CALLER_ATTACH
FontDataTable*  PostScriptTable::Builder::SubBuildTable(ReadableFontData* data) {
    FontDataTablePtr table = new PostScriptTable(header(), data);
    return table.Detach();
}

}