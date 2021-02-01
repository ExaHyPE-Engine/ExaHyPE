#ifndef _CONVERT_INPUT_PEANO_META_FILE_READER_H_
#define _CONVERT_INPUT_PEANO_META_FILE_READER_H_



namespace convert {
  namespace input {
    class MetaFileReader;
  }
}


namespace convert {
  namespace data {
    class DataSet;
  }
}


class convert::input::MetaFileReader {
  public:
    virtual void parse() = 0;
};

#endif
