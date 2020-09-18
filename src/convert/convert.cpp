#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <limits>

#include "../config.h"


#include "output/VTUWriter.h"
#include "output/PeanoWriter.h"

#include "input/PeanoTextPatchFileReader.h"

#include "filter/Copy.h"
#include "filter/Intersection.h"
#include "filter/SelectValue.h"
#include "filter/SeparateResolutions.h"
#include "filter/ConvertTreeIdIntoDataField.h"


#include "tarch/Assertions.h"
#include "tarch/logging/ChromeTraceFileLogger.h"
#include "tarch/mpi/Rank.h"


#include <experimental/filesystem> // or #include <filesystem>


const std::string OutputFormatPeano = "peano";
const std::string OutputFormatVTU   = "vtu";


void createDirectory( const std::string& directory ) {
  static tarch::logging::Log _log( "/" );
  if (
    !std::experimental::filesystem::is_directory(directory)
    ||
    !std::experimental::filesystem::exists(directory)
 ) {
    try {
      std::experimental::filesystem::create_directory(directory);
      logInfo( "createDirectory(...)", "created directory " << directory );
    }
    catch (std::exception exc) {
      logError( "createDirectory(...)", "failed to create directory " << directory );
      logError( "createDirectory(...)", "error message: " << exc.what() );
      tarch::mpi::Rank::abort(-1);
    }
  }
}


void inspect( std::string filename ) {
  static tarch::logging::Log _log( "/" );

  convert::input::PeanoTextPatchFileReader reader(filename);
  reader.parse();

  std::vector< convert::data::DataSet >  data = reader.getData();

  int datasetCounter = 0;
  for (auto& pp: data) {
    logInfo( "inspect", "dataset #" << datasetCounter );
    datasetCounter++;
    for (auto& p: pp.getVariables()) {
      logInfo( "inspect", "variable " << p.name );
      logInfo( "inspect", "\ttype\t\t\t" << (p.type==convert::data::PeanoDataType::Cell_Values ? "cell values" : "vertex values" ));
      logInfo( "inspect", "\tdofs per axis\t\t" << p.dofsPerAxis );
      logInfo( "inspect", "\tunknowns per dof\t" << p.unknowns );
    }
  }
}


std::string getFileNameWithoutExtensionAndWithoutPatch( std::string& file ) {
  static tarch::logging::Log _log( "/" );

  std::string result = file;
  logDebug( "getFileNameWithoutExtensionAndWithoutPatch(std::string)", "in=" << result );
  if ( result.find_last_of(".")!=std::string::npos ) {
    result = result.erase(result.find_last_of(".") );
  }
  if ( result.find_last_of("/")!=std::string::npos ) {
    result = result.erase(0,result.find_last_of("/")+1 );
  }
  logDebug( "getFileNameWithoutExtensionAndWithoutPatch(std::string)", "out=" << result );
  return result;
}


void convertFile( const std::string& outputDirectory, const std::string& truncatedFile, const std::string& format, convert::data::Variable variable, const convert::data::DataSet& data ) {
  static tarch::logging::Log _log( "/" );

  convert::output::PeanoWriter::Writer* writer = nullptr;

  const std::string fileWithCorrectSelector = truncatedFile + "-" + variable.name;
  if (format==OutputFormatPeano) {
    writer = new convert::output::PeanoWriter( outputDirectory, fileWithCorrectSelector );
  }
  else if (format==OutputFormatVTU) {
    writer = new convert::output::VTUWriter( outputDirectory, fileWithCorrectSelector );
  }
  else {
    logError( "convertFile(...)", "unknown output format " << format );
    return;
  }

  writer->writeFile( variable, data.getData(variable) );

  delete writer;
}


void convertFile( std::string filename, const std::string& outputDirectory, const std::string& selector, const std::string& format ) {
  static tarch::logging::Log _log( "/" );

  createDirectory( outputDirectory );

  convert::input::PeanoTextPatchFileReader reader(filename);
  reader.parse();
  std::vector< convert::data::DataSet > data = reader.getData();

  std::string truncatedFile = getFileNameWithoutExtensionAndWithoutPatch( filename );
  logDebug( "convertFile(...)", "read " << filename << " and write into " << truncatedFile << " in directory " << outputDirectory );

  if (data.empty() ){
    logError( "convertFile(...)", "data file is empty" );
  }
  else if (selector=="all") {
    #pragma omp parallel for
    for (int i=0; i<data.size(); i++) {
      for (auto variable: data[i].getVariables()) {
        convertFile(
          outputDirectory, truncatedFile + "-" + std::to_string(i), format,
          variable, data[i]
        );
      }
    }
  }
  else if (data[0].hasVariable(selector)) {
    #pragma omp parallel for
    for (int i=0; i<data.size(); i++) {
      convert::data::Variable variable = data[i].getVariable(selector);
      convertFile(
        outputDirectory, truncatedFile + "-" + std::to_string(i), format,
        variable, data[i]
      );
    }
  }
  else {
    logError( "convertFile(...)", "data file does not contain any data set with name " << selector );
  }

  for (auto& p: data) {
    p.free();
  }
}


enum class Filter {
  Copy,
  ExtractFineGrid,
  SelectValue,
  SeparateResolutions,
  PlotDomainDecomposition
};


std::string toString( Filter filter ) {
  switch (filter) {
    case Filter::Copy:
      return "copy";
    case Filter::ExtractFineGrid:
      return "extract-fine-grid";
    case Filter::SelectValue:
      return "select-value";
    case Filter::SeparateResolutions:
      return "separate-resolutions";
    case Filter::PlotDomainDecomposition:
      return "plot-domain-decomposition";
  }
  return "undef";
}


void applyFilter( std::string filename, std::string outputDirectory, std::string selector, std::string filterName, std::string targetSelector) {
  static tarch::logging::Log _log( "/" );

  createDirectory( outputDirectory );

  std::string truncatedFile = getFileNameWithoutExtensionAndWithoutPatch( filename );
  logInfo( "applyFilter(...)", "writing file " << truncatedFile );

  convert::input::PeanoTextPatchFileReader reader(filename);
  reader.parse();
  std::vector< convert::data::DataSet >  data = reader.getData();

  if (data.empty()) {
    logError( "applyFilter(...)", "file already contains data set with name " << targetSelector );
  }
  else if (data[0].hasVariable(selector)) {
    logInfo( "applyFilter(...)", "apply filter to " << data.size() << " file(s)" );

    convert::output::PeanoWriter writer( outputDirectory, truncatedFile );

    const int numberOfDataSets = data.size();
    #pragma omp parallel for
    for (int i=0; i<numberOfDataSets; i++) {
      if (data[i].hasVariable(targetSelector)) {
        logError( "applyFilter(...)", "file already contains data set with name " << targetSelector );
      }
      else {
        convert::data::Variable variable = data[i].getVariable(selector);

        convert::filter::Filter* filter = nullptr;
        if (filterName==toString(Filter::Copy)) {
          filter = new convert::filter::Copy();
        }
        else if (filterName==toString(Filter::ExtractFineGrid)) {
          filter = new convert::filter::Intersection( convert::filter::Intersection::Strategy::KeepFinerGrid );
        }
        else if (filterName==toString(Filter::PlotDomainDecomposition)) {
          filter = new convert::filter::ConvertTreeIdIntoDataField();
        }
        else if (filterName==toString(Filter::SeparateResolutions)) {
          filter = new convert::filter::SeparateResolutions();
        }
        else if (filterName.compare(toString(Filter::SelectValue))>0) {
    	  std::string rangeToken = filterName.substr( filterName.find(':')+1 );
    	  std::string fromToken  = rangeToken.substr( 0, rangeToken.find(':') );
    	  std::string toToken    = rangeToken.substr( rangeToken.find(':')+1 );

          logDebug( "applyFilter(...)", "use range token " << rangeToken << " split into " << fromToken << " and " << toToken );
          filter = new convert::filter::SelectValue( std::stod(fromToken), std::stod(toToken) );
        }
        else {
          logError( "applyFilter(...)", "unknown filter " << filterName );
        }

        if (filter!=nullptr) {
          filter->apply( data[i], variable, targetSelector );
          // for most filters, this assertion would be correct. But there are
          // filters which use targetSelector as prefix and do derive various
          // selectors from thereon. The resolution splitting is an example.
          // assertion2( data[i].hasVariable(targetSelector), targetSelector, i );
          delete filter;
        }
      }
    }
    writer.writeFile( data );
  }
  else {
    logError( "applyFilter(...)", "data file does not contain any data set with name " << selector );
  }

  for (auto& p: data) {
    p.free();
  }
}


int main(int argc, char* argv[]) {
    std::cout << "Peano file format converter/inspector" << std::endl;
    std::cout << "(C) 2018/2019 Dan Tuthill-Jones, Tobias Weinzierl" << std::endl << std::endl;
    bool validParams = true;

    if(argc < 3) {
      std::cerr << "too few arguments" << std::endl;
      validParams = false;
    }
    else {
      tarch::logging::ChromeTraceFileLogger::getInstance().setOutputFile( "convert" );

      std::string mode            = argv[1];
      if (mode.compare("inspect")==0 and argc>=3) {
    	for (int i=2; i<argc; i++) {
    	  inspect( argv[i] );
    	}
      }
      else if (mode.compare("convert-file")==0 and argc>=5) {
        std::string selector        = argv[ argc-3 ];
        std::string outputDirectory = argv[ argc-2 ];
        std::string format          = argv[ argc-1 ];
    	for (int i=2; i<argc-3; i++) {
    	  convertFile( argv[i], outputDirectory, selector, format );
    	}
      }
      else if (mode.compare("apply-filter")==0 and argc>=7) {
        std::string selector        = argv[ argc-4 ];
        std::string outputDirectory = argv[ argc-3 ];
        std::string filter          = argv[ argc-2 ];
        std::string targetSelector  = argv[ argc-1 ];
    	for (int i=2; i<argc-4; i++) {
    	  applyFilter( argv[i], outputDirectory, selector, filter, targetSelector);
    	}
      }
      else {
        std::cerr << "unknown command or invalid number of parameters for particular command" << std::endl;
        validParams = false;
      }
    }

    if (!validParams) {
      std::cerr << std::endl << std::endl;
      std::cerr << "Usage:";
      std::cerr << "\t./executable inspect       InputFile1 [InputFile2 ...] " << std::endl;
      std::cerr << "\t./executable convert-file  InputFile1 [InputFile2 ...] Selector OutputFolder Format" << std::endl;
      std::cerr << "\t./executable apply-filter  InputFile1 [InputFile2 ...] Selector OutputFolder Filter TargetSelector" << std::endl;

      std::cerr << std::endl << std::endl;
      std::cerr << "Variants:" << std::endl;
      std::cerr << "\tinspect              Inspect which data sets are stored within file (cmp Selector below)" << std::endl;
      std::cerr << "\tconvert-file         Convert a single file" << std::endl;
      std::cerr << "\tapply-filter         Take a data set from the input file (identified by Selector), apply a filter to" << std::endl;
      std::cerr << "\t                     this set and add the result to the file with a given TargetSelector name" << std::endl;

      std::cerr << std::endl << std::endl;
      std::cerr << "Options:" << std::endl;
      std::cerr << "\tFormat               Either " << OutputFormatPeano << " or " << OutputFormatVTU << std::endl;
      std::cerr << "\tSelector             Use inspect to see which data sets are stored within your patch file, i.e. which you can select. Pass all to convert all data sets (works only with convert-file)" << std::endl;

      std::cerr << std::endl << std::endl;
      std::cerr << "Filters:" << std::endl;
      std::cerr << "\t" << toString(Filter::Copy) << "                 Create 1:1 copy of dataset with different name (for debugging)" << std::endl;
      std::cerr << "\t" << toString(Filter::ExtractFineGrid) << "    Extract fine grid" << std::endl;
      std::cerr << "\t" << toString(Filter::SelectValue) << "         Extract grid patches that hold values of a certain range. Append :from:to to filter to specify range" << std::endl;
      std::cerr << "\t" << toString(Filter::SeparateResolutions) << " Splits up the tree mesh into its resolutions" << std::endl;
      std::cerr << "\t" << toString(Filter::PlotDomainDecomposition) << " Don't plot any data but display which mesh parts are handled by which tree (please use this as first filter if you apply a series of filters)" << std::endl;

      std::cerr << std::endl << std::endl;
      std::cerr << "Output directory plus filename can be the same as the input file. In this case, the original file is overwritten/augmented with new data" << std::endl;
      return -1;
    }
    else return 0;
}
