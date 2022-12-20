#pragma once



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

