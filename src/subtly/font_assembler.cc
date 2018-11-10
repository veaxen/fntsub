/*
 * Copyright 2011 Google Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "sfntly/table/core/post_script_table.h"
#include "sfntly/data/memory_byte_array.h"
#include "sfntly/port/memory_output_stream.h"
#include "sfntly/table/core/horizontal_header_table.h"
#include "sfntly/table/core/horizontal_metrics_table.h"
#include "subtly/font_assembler.h"

#include <stdio.h>

#include <set>
#include <map>

#include "sfntly/tag.h"
#include "sfntly/font.h"
#include "sfntly/font_factory.h"
#include "sfntly/table/core/cmap_table.h"
#include "sfntly/table/truetype/loca_table.h"
#include "sfntly/table/truetype/glyph_table.h"
#include "sfntly/table/core/maximum_profile_table.h"
#include "sfntly/port/type.h"
#include "sfntly/port/refcount.h"
#include "subtly/font_info.h"

namespace subtly {
using namespace sfntly;

const int32_t FontAssembler::VERSION_2            = 0x20000;
const int32_t FontAssembler::NUM_STANDARD_NAMES   = 258;
const int32_t FontAssembler::V1_TABLE_SIZE        = 32;

const int32_t FontAssembler::Offset::version              = 0;
const int32_t FontAssembler::Offset::italicAngle          = 4;
const int32_t FontAssembler::Offset::underlinePosition    = 8;
const int32_t FontAssembler::Offset::underlineThickness   = 10;
const int32_t FontAssembler::Offset::isFixedPitch         = 12;
const int32_t FontAssembler::Offset::minMemType42         = 16;
const int32_t FontAssembler::Offset::maxMemType42         = 20;
const int32_t FontAssembler::Offset::minMemType1          = 24;
const int32_t FontAssembler::Offset::maxMemType1          = 28;
const int32_t FontAssembler::Offset::numberOfGlyphs       = 32;
const int32_t FontAssembler::Offset::glyphNameIndex       = 34;

const static std::unordered_map<std::string, int32_t >* invertNameMap() {

  static std::unordered_map<std::string, int32_t > nameMap;
  for(int32_t i = 0; i < PostScriptTable::NUM_STANDARD_NAMES; ++i) {
    nameMap[PostScriptTable::STANDARD_NAMES[i]] = i;
  }
  return &nameMap;
}

const std::unordered_map<std::string, int32_t >*
        FontAssembler::INVERTED_STANDARD_NAMES = invertNameMap();

FontAssembler::FontAssembler(FontInfo* font_info,
                             IntegerSet* table_blacklist)
    : table_blacklist_(table_blacklist) {
  font_info_ = font_info;
  Initialize();
}

FontAssembler::FontAssembler(FontInfo* font_info)
    : table_blacklist_(NULL) {
  font_info_ = font_info;
  Initialize();
}

void FontAssembler::Initialize() {
  font_factory_.Attach(sfntly::FontFactory::GetInstance());
  font_builder_.Attach(font_factory_->NewFontBuilder());
}

CALLER_ATTACH Font* FontAssembler::Assemble() {
  // Assemble tables we can subset.
  // Note:一定要按照这个调用顺序,因为可以避免重复处理char和glypgid的对应关系
  if (!AssembleGlyphAndLocaTables() || !AssembleCMapTable() ||
      !AssembleHorizontalMetricsTable() || !AssemblePostScriptTabble()) {
    return NULL;
  }
  // For all other tables, either include them unmodified or don't at all.
  const TableMap* common_table_map =
      font_info_->GetTableMap(font_info_->fonts()->begin()->first);
  for (TableMap::const_iterator it = common_table_map->begin(),
           e = common_table_map->end(); it != e; ++it) {
    if (table_blacklist_
        && table_blacklist_->find(it->first) != table_blacklist_->end()) {
      continue;
    }
    font_builder_->NewTableBuilder(it->first, it->second->ReadFontData());
  }
  return font_builder_->Build();
}

bool FontAssembler::AssembleCMapTable() {
  // Creating the new CMapTable and the new format 4 CMap
  Ptr<CMapTable::Builder> cmap_table_builder =
      down_cast<CMapTable::Builder*>
      (font_builder_->NewTableBuilder(Tag::cmap));
  if (!cmap_table_builder)
    return false;
  Ptr<CMapTable::CMapFormat4::Builder> cmap_builder =
      down_cast<CMapTable::CMapFormat4::Builder*>
      (cmap_table_builder->NewCMapBuilder(CMapFormat::kFormat4,
                                          CMapTable::WINDOWS_BMP));
  if (!cmap_builder)
    return false;
  // Creating the segments and the glyph id array
  CharacterMap* chars_to_glyph_ids = font_info_->chars_to_glyph_ids();
  SegmentList* segment_list = new SegmentList;
  IntegerList* new_glyph_id_array = new IntegerList;
  int32_t last_chararacter = -2;
  int32_t last_offset = 0;
  Ptr<CMapTable::CMapFormat4::Builder::Segment> current_segment;

  // For simplicity, we will have one segment per contiguous range.
  // To test the algorithm, we've replaced the original CMap with the CMap
  // generated by this code without removing any character.
  // Tuffy.ttf: CMap went from 3146 to 3972 bytes (1.7% to 2.17% of file)
  // AnonymousPro.ttf: CMap went from 1524 to 1900 bytes (0.96% to 1.2%)
  for (CharacterMap::iterator it = chars_to_glyph_ids->begin(),
           e = chars_to_glyph_ids->end(); it != e; ++it) {
    int32_t character = it->first;
    if (character != last_chararacter + 1) {  // new segment
      if (current_segment != NULL) {
        current_segment->set_end_count(last_chararacter);
        segment_list->push_back(current_segment);
      }
      // start_code = character
      // end_code = -1 (unknown for now)
      // id_delta = 0 (we don't use id_delta for this representation)
      // id_range_offset = last_offset (offset into the glyph_id_array)
      current_segment =
          new CMapTable::CMapFormat4::Builder::
          Segment(character, -1, 0, last_offset);
    }
    int32_t old_glyphid = it->second.glyph_id();
    new_glyph_id_array->push_back(old_to_new_glyphid_[old_glyphid]);
    last_offset += DataSize::kSHORT;
    last_chararacter = character;
  }
  // The last segment is still open.
  if (current_segment != NULL){
      current_segment->set_end_count(last_chararacter);
      segment_list->push_back(current_segment);
  }
  // Updating the id_range_offset for every segment.
  for (int32_t i = 0, num_segs = segment_list->size(); i < num_segs; ++i) {
    Ptr<CMapTable::CMapFormat4::Builder::Segment> segment = segment_list->at(i);
    segment->set_id_range_offset(segment->id_range_offset()
                                 + (num_segs - i + 1) * DataSize::kSHORT);
  }
  // Adding the final, required segment.
  current_segment =
      new CMapTable::CMapFormat4::Builder::Segment(0xffff, 0xffff, 1, 0);
  new_glyph_id_array->push_back(0);//边界处理
  segment_list->push_back(current_segment);
  // Writing the segments and glyph id array to the CMap
  cmap_builder->set_segments(segment_list);
  cmap_builder->set_glyph_id_array(new_glyph_id_array);
  delete segment_list;
  delete new_glyph_id_array;
  return true;
}

bool FontAssembler::AssembleGlyphAndLocaTables() {
  Ptr<LocaTable::Builder> loca_table_builder =
      down_cast<LocaTable::Builder*>
      (font_builder_->NewTableBuilder(Tag::loca));
  Ptr<GlyphTable::Builder> glyph_table_builder =
      down_cast<GlyphTable::Builder*>
      (font_builder_->NewTableBuilder(Tag::glyf));

  GlyphIdSet* resolved_glyph_ids = font_info_->resolved_glyph_ids();
  // Basic sanity check: all LOCA tables are of the same size
  // This is necessary but not suficient!
  int32_t previous_size = -1;
  for (FontIdMap::iterator it = font_info_->fonts()->begin();
       it != font_info_->fonts()->end(); ++it) {
    Ptr<LocaTable> loca_table =
        down_cast<LocaTable*>(font_info_->GetTable(it->first, Tag::loca));
    int32_t current_size = loca_table->header_length();
    if (previous_size != -1 && current_size != previous_size) {
      return false;
    }
    previous_size = current_size;
  }

  GlyphTable::GlyphBuilderList* glyph_builders =
      glyph_table_builder->GlyphBuilders();

  int32_t new_glyphid = 0;
  for (GlyphIdSet::iterator it = resolved_glyph_ids->begin(),
           e = resolved_glyph_ids->end(); it != e; ++it) {
    // Get the glyph for this resolved_glyph_id.
    int32_t resolved_glyph_id = it->glyph_id();
    old_to_new_glyphid_[resolved_glyph_id] = new_glyphid++;
    new_to_old_glyphid_.push_back(resolved_glyph_id);
    int32_t font_id = it->font_id();
    // Get the LOCA table for the current glyph id.
    Ptr<LocaTable> loca_table =
        down_cast<LocaTable*>
        (font_info_->GetTable(font_id, Tag::loca));
    int32_t length = loca_table->GlyphLength(resolved_glyph_id);
    int32_t offset = loca_table->GlyphOffset(resolved_glyph_id);

    // Get the GLYF table for the current glyph id.
    Ptr<GlyphTable> glyph_table =
        down_cast<GlyphTable*>
        (font_info_->GetTable(font_id, Tag::glyf));
    GlyphPtr glyph;
    glyph.Attach(glyph_table->GetGlyph(offset, length));

    // The data reference by the glyph is copied into a new glyph and
    // added to the glyph_builders belonging to the glyph_table_builder.
    // When Build gets called, all the glyphs will be built.
    // TODO（veaxen）这里需要考虑下glyphid是kComposite的情况
    Ptr<ReadableFontData> data = glyph->ReadFontData();
    Ptr<WritableFontData> copy_data;
    copy_data.Attach(WritableFontData::CreateWritableFontData(data->Length()));
    data->CopyTo(copy_data);
    GlyphBuilderPtr glyph_builder;
    glyph_builder.Attach(glyph_table_builder->GlyphBuilder(copy_data));
    glyph_builders->push_back(glyph_builder);
  }

  IntegerList loca_list;
  glyph_table_builder->GenerateLocaList(&loca_list);
  loca_table_builder->SetLocaList(&loca_list);

  Ptr<ReadableFontData> rFontData = font_info_->GetTable(font_info_->fonts()->begin()->first, Tag::maxp)->ReadFontData();
  font_builder_->NewTableBuilder(Tag::maxp, rFontData);
  MaximumProfileTableBuilderPtr maxpBuilder =
          down_cast<MaximumProfileTable::Builder*>(font_builder_->GetTableBuilder(Tag::maxp));
  maxpBuilder->SetNumGlyphs(loca_table_builder->NumGlyphs());
  return true;
}

struct LongHorMetric{
    int32_t advanceWidth;
    int32_t lsb;
};
bool FontAssembler::AssembleHorizontalMetricsTable() {
  HorizontalMetricsTablePtr origMetrics =
          down_cast<HorizontalMetricsTable*>(font_info_->GetTable(0, Tag::hmtx));
  if (origMetrics == NULL) {
    return false;
  }

  std::vector<LongHorMetric> metrics;
  for (size_t i = 0; i < new_to_old_glyphid_.size(); ++i) {
    int32_t origGlyphId = new_to_old_glyphid_[i];
    int32_t advanceWidth = origMetrics->AdvanceWidth(origGlyphId);
    int32_t lsb = origMetrics->LeftSideBearing(origGlyphId);
    metrics.push_back(LongHorMetric{advanceWidth, lsb});
  }

  int32_t lastWidth = metrics.back().advanceWidth;
  int32_t numberOfHMetrics = (int32_t)metrics.size();
  while (numberOfHMetrics > 1 && metrics[numberOfHMetrics-2].advanceWidth == lastWidth) {
    numberOfHMetrics--;
  }
  int32_t size = 4 * numberOfHMetrics + 2 * ((int32_t)metrics.size() - numberOfHMetrics);
  WritableFontDataPtr data;
  data.Attach(WritableFontData::CreateWritableFontData(size));
  int32_t index = 0;
  int32_t advanceWidthMax = 0;
  for (int32_t i=0; i < numberOfHMetrics; ++i) {
    int32_t adw = metrics[i].advanceWidth;
    advanceWidthMax = std::max(adw, advanceWidthMax);
    index += data->WriteUShort(index, adw);
    index += data->WriteShort(index, metrics[i].lsb);
  }
  int32_t nMetric = (int32_t)metrics.size();
  for (int32_t j = numberOfHMetrics; j < nMetric; ++j) {
    index += data->WriteShort(index, metrics[j].lsb);
  }
  font_builder_->NewTableBuilder(Tag::hmtx, data);
  font_builder_->NewTableBuilder(Tag::hhea, font_info_->GetTable(0, Tag::hhea)->ReadFontData());
  HorizontalHeaderTableBuilderPtr hheaBuilder =
          down_cast<HorizontalHeaderTable::Builder*>(font_builder_->GetTableBuilder(Tag::hhea));
  hheaBuilder->SetNumberOfHMetrics(numberOfHMetrics);
  hheaBuilder->SetAdvanceWidthMax(advanceWidthMax);

  return true;
}

bool FontAssembler::AssemblePostScriptTabble() {
  if (new_to_old_glyphid_.empty()) {
    return false;
  }
  Ptr<PostScriptTable> post = down_cast<PostScriptTable*>(font_info_->GetTable(0, Tag::post));
  WritableFontDataPtr v1Data;
  v1Data.Attach(WritableFontData::CreateWritableFontData(V1_TABLE_SIZE));
  ReadableFontDataPtr tmp = post->ReadFontData();
  FontDataPtr fontDataPtr;
  fontDataPtr.Attach(tmp->Slice(0, V1_TABLE_SIZE));
  ReadableFontDataPtr srcData = down_cast<ReadableFontData*>(fontDataPtr.p_);
  srcData->CopyTo(v1Data);

  int32_t post_version = post->ReadFontData()->ReadFixed(Offset::version);
  std::vector<std::string> names;
  if (post_version == 0x10000 || post_version == 0x20000) {
    for (size_t i = 0; i < new_to_old_glyphid_.size(); ++i) {
      names.push_back(post->GlyphName(new_to_old_glyphid_[i]));
    }
  }

  if (names.empty()){
      font_builder_->NewTableBuilder(Tag::post, v1Data);
      return true;
  }

  std::vector<int32_t > glyphNameIndices;
  MemoryOutputStream nameBos;
  size_t nGlyphs = names.size();
  int32_t tableIndex = NUM_STANDARD_NAMES;
  for (const auto &name : names) {
    int32_t glyphNameIndex;
    if(INVERTED_STANDARD_NAMES->find(name) != INVERTED_STANDARD_NAMES->end()) {
      glyphNameIndex = INVERTED_STANDARD_NAMES->at(name);
    } else {
      glyphNameIndex = tableIndex++;
      auto len = (int32_t)name.size();
      auto* lenptr = (uint8_t*)&len;
      nameBos.Write(lenptr, 0, sizeof(int32_t));
      nameBos.Write((uint8_t*)name.c_str(), 0, (int32_t)name.size());
    }
    glyphNameIndices.push_back(glyphNameIndex);
  }
  std::vector<uint8_t > nameBytes(nameBos.Size());
  for (size_t j = 0; j < nameBos.Size(); ++j) {
    nameBytes.push_back(nameBos.Get()[j]);
  }
  int32_t newLength = 34 + 2 * (int32_t)nGlyphs + (int32_t)nameBytes.size();
  WritableFontDataPtr data;
  data.Attach(WritableFontData::CreateWritableFontData(newLength));
  v1Data->CopyTo(data);
  data->WriteFixed(Offset::numberOfGlyphs, (int32_t)nGlyphs);
  int32_t index = Offset::glyphNameIndex;
  for (const auto &glyphNameIndex : glyphNameIndices) {
    index += data->WriteUShort(index, glyphNameIndex);
  }
  if (!nameBytes.empty()) {
    data->WriteBytes(index, &nameBytes);
  }

  font_builder_->NewTableBuilder(Tag::post, data);
  return true;
}
}
