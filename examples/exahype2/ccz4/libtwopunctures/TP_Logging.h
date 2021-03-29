#ifndef __TWO_PUNCTURES_LOGGING__
#define __TWO_PUNCTURES_LOGGING__

// Output and error reporting beyond Cactus:
// A standalone approach for TwoPunctures.
// SvenK, 2017-04-10

#include <cstdarg>
#include <string>
#include <iostream>

namespace TP {

/**
 * By inheriting this class, you can do something with output if you need.
 * 
 * @param msg Is some (probably multiline) message
 * @param codepos Is the line and file, as created by the macros in logging.h.
 *            Can be the empty string.
 **/
struct logger {
	virtual void log(const std::string& msg) = 0;
	virtual void error(const std::string& msg) = 0;
	virtual void info(const std::string& msg) = 0;
	virtual void warn(const std::string& msg) = 0;
};

/**
 * Default stdout logger, using iostream. You might want to take this as a basis
 * for coupling to your own logging system.
 */
struct std_cerr_logger : public logger {
	void print(const std::string& msg) { std::cerr << "TP: " << msg << "\n"; }

	void log(const std::string& msg) override { print(msg); }
	void error(const std::string& msg) override { print(msg); }
	void info(const std::string& msg) override { print(msg); }
	void warn(const std::string& msg) override { print(msg); }
};

/**
 * The LoggingAdapter: Logging in Cactus/printf style without macros.
 * Just inherit from this class to use the facilities.
 **/
struct LoggingAdapter {
	// the TwoPunctures library can use this anywhere
	void TP_LOG(const char *fmt, ...);
	void TP_ERROR(const char *fmt, ...);
	void TP_INFO(const char *fmt, ...);
	void TP_WARN(const char *fmt, ...);
	
	logger* log; // Users can set this to anything
	
	LoggingAdapter() {
		log = new std_cerr_logger;
	}
}; // struct

/**
 * Another nice thing: The TwoPunctures Error, borrowed from Pizza.
 * Use like:
 *    error::unless(something_to_check, "msg");
 *    if(something) throw error("foo");
 *    etc.
 **/
class error: public std::exception
{
  const char* msg;
  public:
  error(const char* msg_) : msg(msg_) {}
  virtual const char* what() const throw()
  {
    return msg;
  }
  static void unless(bool c, const char* m)
  {
    if (!c) throw error(m);
  }
  static void incase(bool c, const char* m)
  {
    unless(!c,m);
  }
};

} // ns

#endif /* __TWO_PUNCTURES_LOGGING__ */