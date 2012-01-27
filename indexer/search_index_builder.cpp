#include "search_index_builder.hpp"

#include "feature_utils.hpp"
#include "features_vector.hpp"
#include "search_delimiters.hpp"
#include "search_trie.hpp"
#include "search_string_utils.hpp"
#include "string_file.hpp"

#include "../defines.hpp"

#include "../platform/platform.hpp"

#include "../coding/trie_builder.hpp"
#include "../coding/writer.hpp"
#include "../coding/reader_writer_ops.hpp"

#include "../base/string_utils.hpp"
#include "../base/logging.hpp"

#include "../std/algorithm.hpp"
#include "../std/vector.hpp"


namespace
{

struct FeatureNameInserter
{
  StringsFile & m_names;
  uint32_t m_pos;
  uint32_t m_rank;

  FeatureNameInserter(StringsFile & names, uint32_t pos, uint8_t rank)
    : m_names(names), m_pos(pos), m_rank(rank) {}

  void AddToken(signed char lang, strings::UniString const & s) const
  {
    AddToken(lang, s, m_rank);
  }

  void AddToken(signed char lang, strings::UniString const & s, uint32_t /*rank*/) const
  {
    m_names.AddString(StringsFile::StringT(s, lang, m_pos));
  }

  bool operator()(signed char lang, string const & name) const
  {
    strings::UniString uniName = search::NormalizeAndSimplifyString(name);
    buffer_vector<strings::UniString, 32> tokens;
    SplitUniString(uniName, MakeBackInsertFunctor(tokens), search::Delimiters());
    if (tokens.size() > 30)
    {
      LOG(LWARNING, ("Name has too many tokens:", name));
      tokens.resize(30);
    }
    for (size_t i = 0; i < tokens.size(); ++i)
      AddToken(lang, tokens[i], /*i < 3 ? m_rank + 10 * (3 - i) : */m_rank);
    return true;
  }
};

struct FeatureInserter
{
  StringsFile & m_names;

  explicit FeatureInserter(StringsFile & names) : m_names(names) {}

  void operator() (FeatureType const & feature, uint64_t pos) const
  {
    feature::TypesHolder types(feature);

    // Add names of the feature.
    FeatureNameInserter f(m_names, static_cast<uint32_t>(pos),
                          feature::GetSearchRank(types, feature.GetPopulation()));
    feature.ForEachNameRef(f);

    // Add names of categories of the feature.
    for (size_t i = 0; i < types.Size(); ++i)
      f.AddToken(0, search::FeatureTypeToString(types[i]));
  }
};

}  // unnamed namespace

void indexer::BuildSearchIndex(FeaturesVector const & featuresVector, Writer & writer,
                               string const & tmpFilePath)
{
  {
    StringsFile names(tmpFilePath);
    featuresVector.ForEachOffset(FeatureInserter(names));

    names.EndAdding();
    names.OpenForRead();

    trie::Build(writer, names.Begin(), names.End(), trie::builder::EmptyEdgeBuilder());

    // at this point all readers should be dead
  }

  FileWriter::DeleteFileX(tmpFilePath);
}

bool indexer::BuildSearchIndexFromDatFile(string const & fName)
{
  LOG(LINFO, ("Start building search index ..."));

  try
  {
    Platform & pl = GetPlatform();
    string const datFile = pl.WritablePathForFile(fName);
    string const tmpFile = pl.WritablePathForFile(fName + ".search_index_2.tmp");

    {
      FilesContainerR readCont(datFile);

      feature::DataHeader header;
      header.Load(readCont.GetReader(HEADER_FILE_TAG));

      FeaturesVector featuresVector(readCont, header);

      FileWriter writer(tmpFile);
      BuildSearchIndex(featuresVector, writer, pl.WritablePathForFile(fName + ".search_index_1.tmp"));
    }

    {
      // Write to container in reversed order.
      FilesContainerW writeCont(datFile, FileWriter::OP_WRITE_EXISTING);
      FileWriter writer = writeCont.GetWriter(SEARCH_INDEX_FILE_TAG);
      rw_ops::Reverse(FileReader(tmpFile), writer);
    }

    FileWriter::DeleteFileX(tmpFile);
  }
  catch (Reader::Exception const & e)
  {
    LOG(LERROR, ("Error while reading file: ", e.what()));
    return false;
  }
  catch (Writer::Exception const & e)
  {
    LOG(LERROR, ("Error writing index file: ", e.what()));
    return false;
  }

  LOG(LINFO, ("End building search index."));
  return true;
}
