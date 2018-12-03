#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include <sstream>

// Rethrow exception and fill stacktrace
#define rethrow(a) throw Exception(a, __FILE__, __LINE__, __PRETTY_FUNCTION__)
#define createException(msg) Exception(msg, __FILE__, __LINE__, __PRETTY_FUNCTION__)

// Exception class with stacktrace
// Extends std::exception so we can catch both this exceptions and libpmemobj exceptions
// with catch(std::exception& e). Do not forget about & in catch, so overriden what() 
//function will be used
class Exception : public std::exception
{
private:
	std::string msg;
	std::string params;
	std::string stack;
	std::string result;

public:
	Exception() 
	{
		msg = "";
		stack = "";
		params = "";
	}

	Exception(std::exception& exp)
	{
		stack = exp.what();
		result = exp.what();
	}

	Exception(std::string exp)
	{
		msg = exp;
		result = exp;
	}

	Exception(std::string exp, std::string file, int line, std::string function)
	{
		msg = "Error in function: " + function + ", file " + file 
		+ ", line " + std::to_string(line) + "\n";
		params = "";
		stack =  "Caused by: " + exp;
		result = msg + params + stack;
	}

	void addParameter(std::string name, int param)
	{
		if(params == "")
			params = "  Params: \n";
		std::string newParam = "\t" + name + " -> " + std::to_string(param) + "\n";
		params += newParam;
		result = msg + params + stack;
	}

	void addParameter(std::string name, double param)
	{
		if(params == "")
			params = "  Params: \n";
		std::string newParam = "\t" + name + " -> " + std::to_string(param) + "\n";
		params += newParam;
		result = msg + params + stack;
	}

	void addParameter(std::string name, std::string param)
	{
		if(params == "")
			params = "  Params: \n";
		std::string newParam = "\t" + name + " -> " + param + "\n";
		params += newParam;
		result = msg + params + stack;
	}

	Exception(std::exception& exp, std::string file, int line, std::string function)
	{
		msg = "Error in function: " + function + ", file " + file 
		+ ", line " + std::to_string(line) + "\n";
		params = "";
		stack = "Caused by: " + std::string(exp.what());
		result = msg + params + stack;
	}

	virtual const char* what() const throw()
	{
		return result.c_str();
	}
};