#ifndef _VISUALISATION_INPUT_PEANO_META_FILE_READER_H_
#define _VISUALISATION_INPUT_PEANO_META_FILE_READER_H_



namespace visualisation {
  namespace input {
    class MetaFileReader;
  }
}


namespace visualisation {
  namespace data {
    class DataSet;
  }
}


class visualisation::input::MetaFileReader {
  public:
	virtual void parse() = 0;
//	virtual PeanoMetaFile* getFile() = 0;
};

#endif
