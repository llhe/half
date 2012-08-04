// test - Test application for half-precision floating point functionality.
//
// Copyright (c) 2012 Christian Rau <rauy@users.sourceforge.net>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation 
// files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, 
// modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE 
// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR 
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include <half.hpp>

#include <utility>
#include <vector>
#include <string>
#include <map>
#include <iostream>
#include <iomanip>
#include <memory>
#include <algorithm>
#include <iterator>
#include <functional>
#include <fstream>
#include <random>
#include <cstdint>


using half_float::half;

half b2h(std::uint16_t bits)
{
	return *reinterpret_cast<half*>(&bits);
}

std::uint16_t h2b(half h)
{
	return *reinterpret_cast<std::uint16_t*>(&h);
}

bool comp(half a, half b)
{
	return (isnan(a) && isnan(b)) || a == b;
}
/*
bool checkmodf(half arg)
{
	half h;
	float f;
	bool eq = comp(modf(arg, &h), static_cast<half>(std::modf(arg, &f)));
	return eq && comp(h, static_cast<half>(f));
}

bool outmodf(half arg)
{
	half h;
	float f, farg = static_cast<float>(arg);
	std::cout << std::hex << *reinterpret_cast<std::uint32_t*>(&farg) << std::dec << std::endl;
	float ff = std::modf(farg, &f);
	half a = modf(arg, &h), b = static_cast<half>(ff);//std::modf(arg, &f));
	bool eq = comp(a, b);
	std::cout << "arg: " << farg << " eq: " << eq << " a: " << a << " b: " << b << " ff: " << ff << std::endl;
	return eq && comp(h, static_cast<half>(f));
}
*/

class half_test
{
public:
	half_test(std::ostream &log)
		: tests_(0), passed_(0), log_(log)
	{
		//prepare halfs
		half_vector batch;
		std::uint16_t u = 0;
		halfs_.insert(std::make_pair("positive zero", half_vector(1, b2h(u++))));
		for(; u<0x400; ++u)
			batch.push_back(b2h(u));
		halfs_.insert(std::make_pair("positive subn", std::move(batch)));
		batch.clear();
		for(; u<0x7C00; ++u)
			batch.push_back(b2h(u));
		halfs_.insert(std::make_pair("positive norm", std::move(batch)));
		batch.clear();
		halfs_.insert(std::make_pair("positive inft", half_vector(1, b2h(u++))));
		for(; u<0x7E00; ++u)
			batch.push_back(b2h(u));
		halfs_.insert(std::make_pair("positive sNaN", std::move(batch)));
		batch.clear();
		for(; u<0x8000; ++u)
			batch.push_back(b2h(u));
		halfs_.insert(std::make_pair("positive qNaN", std::move(batch)));
		batch.clear();
		halfs_.insert(std::make_pair("negative zero", half_vector(1, b2h(u++))));
		for(; u<0x8400; ++u)
			batch.push_back(b2h(u));
		halfs_.insert(std::make_pair("negative subn", std::move(batch)));
		batch.clear();
		for(; u<0xFC00; ++u)
			batch.push_back(b2h(u));
		halfs_.insert(std::make_pair("negative norm", std::move(batch)));
		batch.clear();
		halfs_.insert(std::make_pair("negative inft", half_vector(1, b2h(u++))));
		for(; u<0xFE00; ++u)
			batch.push_back(b2h(u));
		halfs_.insert(std::make_pair("negative sNaN", std::move(batch)));
		batch.clear();
		for(; u!=0; ++u)
			batch.push_back(b2h(u));
		halfs_.insert(std::make_pair("negative qNaN", std::move(batch)));
		
		//write halfs
		std::ofstream out("halfs.log");
		for(auto iterB=halfs_.begin(); iterB!=halfs_.end(); ++iterB)
		{
			out << iterB->first << ":\n";
			for(auto iterH=iterB->second.begin(); iterH!=iterB->second.end(); ++iterH)
				out << '\t' << *iterH << '\n';
		}
	}

	bool test()
	{
		//test classfication
		class_test("fpclassify", []() -> std::map<std::string,int> { std::map<std::string,int> out; 
			out["positive zero"] = FP_ZERO; out["positive subn"] = FP_SUBNORMAL; out["positive norm"] = FP_NORMAL; 
			out["positive inft"] = FP_INFINITE; out["positive qNaN"] = FP_NAN; out["positive sNaN"] = FP_NAN; 
			out["negative zero"] = FP_ZERO; out["negative subn"] = FP_SUBNORMAL; out["negative norm"] = FP_NORMAL; 
			out["negative inft"] = FP_INFINITE; out["negative qNaN"] = FP_NAN; out["negative sNaN"] = FP_NAN; 
			return out; }(), half_float::fpclassify);
		class_test("isfinite", []() -> std::vector<std::string> { std::vector<std::string> out; 
			out.push_back("positive zero"); out.push_back("positive subn"); out.push_back("positive norm"); 
			out.push_back("negative zero"); out.push_back("negative subn"); out.push_back("negative norm"); 
			return out; }(), half_float::isfinite);
		class_test("isinf", []() -> std::vector<std::string> { std::vector<std::string> out; 
			out.push_back("positive inft"); out.push_back("negative inft"); return out; }(), half_float::isinf);
		class_test("isnan", []() -> std::vector<std::string> { std::vector<std::string> out; 
			out.push_back("positive qNaN"); out.push_back("positive sNaN"); out.push_back("negative qNaN"); 
			out.push_back("negative sNaN"); return out; }(), half_float::isnan);
		class_test("isnormal", []() -> std::vector<std::string> {
			std::vector<std::string> out; out.push_back("positive norm"); out.push_back("negative norm"); 
			return out; }(), half_float::isnormal);

		//test conversion
		unary_test("conversion", [](half arg) { return comp(static_cast<half>(static_cast<float>(arg)), arg); });

		//test operators
		unary_test("prefix increment", [](half arg) -> bool { float f = static_cast<float>(arg); 
			return comp(static_cast<half>(++f), ++arg); });
		unary_test("prefix decrement", [](half arg) -> bool { float f = static_cast<float>(arg); 
			return comp(static_cast<half>(--f), --arg); });
		unary_test("postfix increment", [](half arg) -> bool { float f = static_cast<float>(arg); 
			return comp(static_cast<half>(f++), arg++); });
		unary_test("postfix decrement", [](half arg) -> bool { float f = static_cast<float>(arg); 
			return comp(static_cast<half>(f--), arg--); });
		unary_test("unary plus", [](half arg) { return comp(+arg, arg); });
		unary_test("unary minus", [](half arg) { return comp(-arg, static_cast<half>(-static_cast<float>(arg))); });
		binary_test("addition", [](half a, half b) { return comp(a+b, static_cast<half>(static_cast<float>(a)+static_cast<float>(b))); });
		binary_test("subtraction", [](half a, half b) { return comp(a-b, static_cast<half>(static_cast<float>(a)-static_cast<float>(b))); });
		binary_test("multiplication", [](half a, half b) { return comp(a*b, static_cast<half>(static_cast<float>(a)*static_cast<float>(b))); });
		binary_test("division", [](half a, half b) { return comp(a/b, static_cast<half>(static_cast<float>(a)/static_cast<float>(b))); });
		binary_test("equal", [](half a, half b) { return (a==b) == (static_cast<float>(a)==static_cast<float>(b)); });
		binary_test("not equal", [](half a, half b) { return (a!=b) == (static_cast<float>(a)!=static_cast<float>(b)); });
		binary_test("less", [](half a, half b) { return (a<b) == (static_cast<float>(a)<static_cast<float>(b)); });
		binary_test("greater", [](half a, half b) { return (a>b) == (static_cast<float>(a)>static_cast<float>(b)); });
		binary_test("less equal", [](half a, half b) { return (a<=b) == (static_cast<float>(a)<=static_cast<float>(b)); });
		binary_test("greater equal", [](half a, half b) { return (a>=b) == (static_cast<float>(a)>=static_cast<float>(b)); });

		//test basic functions
		unary_test("abs", [](half arg) { return comp(abs(arg), static_cast<half>(std::abs(static_cast<float>(arg)))); });
		unary_test("fabs", [](half arg) { return comp(abs(arg), static_cast<half>(std::fabs(static_cast<float>(arg)))); });
		binary_test("fmod", [](half a, half b) { return comp(fmod(a, b), 
			static_cast<half>(std::fmod(static_cast<float>(a), static_cast<float>(b)))); });

		//test exponential functions
		unary_test("exp", [](half arg) { return comp(exp(arg), static_cast<half>(std::exp(static_cast<float>(arg)))); });
		unary_test("log", [](half arg) { return comp(log(arg), static_cast<half>(std::log(static_cast<float>(arg)))); });
		unary_test("log10", [](half arg) { return comp(log10(arg), static_cast<half>(std::log10(static_cast<float>(arg)))); });

		//test power functions
		unary_test("sqrt", [](half arg) { return comp(sqrt(arg), static_cast<half>(std::sqrt(static_cast<float>(arg)))); });
		binary_test("pow", [](half a, half b) { return comp(pow(a, b), 
			static_cast<half>(std::pow(static_cast<float>(a), static_cast<float>(b)))); });

		//test trig functions
		unary_test("sin", [](half arg) { return comp(sin(arg), static_cast<half>(std::sin(static_cast<float>(arg)))); });
		unary_test("cos", [](half arg) { return comp(cos(arg), static_cast<half>(std::cos(static_cast<float>(arg)))); });
		unary_test("tan", [](half arg) { return comp(tan(arg), static_cast<half>(std::tan(static_cast<float>(arg)))); });
		unary_test("asin", [](half arg) { return comp(asin(arg), static_cast<half>(std::asin(static_cast<float>(arg)))); });
		unary_test("acos", [](half arg) { return comp(acos(arg), static_cast<half>(std::acos(static_cast<float>(arg)))); });
		unary_test("atan", [](half arg) { return comp(atan(arg), static_cast<half>(std::atan(static_cast<float>(arg)))); });
		binary_test("atan2", [](half a, half b) { return comp(atan2(a, b), 
			static_cast<half>(std::atan2(static_cast<float>(a), static_cast<float>(b)))); });

		//test hyp functions
		unary_test("sinh", [](half arg) { return comp(sinh(arg), static_cast<half>(std::sinh(static_cast<float>(arg)))); });
		unary_test("cosh", [](half arg) { return comp(cosh(arg), static_cast<half>(std::cosh(static_cast<float>(arg)))); });
		unary_test("tanh", [](half arg) { return comp(tanh(arg), static_cast<half>(std::tanh(static_cast<float>(arg)))); });

		//test round functions
		unary_test("ceil", [](half arg) { return comp(ceil(arg), static_cast<half>(std::ceil(static_cast<float>(arg)))); });
		unary_test("floor", [](half arg) { return comp(floor(arg), static_cast<half>(std::floor(static_cast<float>(arg)))); });

		//test float functions
		unary_test("frexp", [](half arg) -> bool { int eh, ef; bool eq = comp(frexp(arg, &eh), 
			static_cast<half>(std::frexp(static_cast<float>(arg), &ef))); return eq && eh==ef; });
		unary_test("ldexp", [](half arg) -> bool { unsigned int passed = 0; for(int i=-50; i<50; ++i) passed += 
			comp(ldexp(arg, i), static_cast<half>(std::ldexp(static_cast<float>(arg), i))); return passed==100; });
		unary_test("modf", [](half arg) -> bool { half h; float f; bool eq = comp(modf(arg, &h), static_cast<half>(
			std::modf(static_cast<float>(arg), &f))); return eq && comp(h, static_cast<half>(f)); });
		binary_test("nextafter", [](half a, half b) -> bool { half c = nextafter(a, b); std::int16_t d = std::abs(
			static_cast<std::int16_t>(h2b(a)-h2b(c))); return ((isnan(a) || isnan(b)) && isnan(c)) || 
			(comp(a, b) && comp(b, c)) || ((d==1||d==0x7FFF) && (a<b)==(a<c)); });
		binary_test("copysign", [](half a, half b) -> bool { half h = copysign(a, b); 
			return comp(abs(h), abs(a)) && signbit(h)==signbit(b); });
#ifdef HAVE_CPP11_MATH
		//test basic functions
		binary_test("remainder", [](half a, half b) { return comp(remainder(a, b), 
			static_cast<half>(std::remainder(static_cast<float>(a), static_cast<float>(b)))); });
		binary_test("remquo", [](half a, half b) -> bool { int qh, qf; bool eq = comp(remquo(a, b, &qh), 
			static_cast<half>(std::remquo(static_cast<float>(a), static_cast<float>(b), &qf))); return eq && qh==qf; });
		binary_test("fmin", [](half a, half b) { return comp(fmin(a, b), static_cast<half>(
			std::fmin(static_cast<float>(a), static_cast<float>(b)))); });
		binary_test("fmax", [](half a, half b) { return comp(fmax(a, b), static_cast<half>(
			std::fmax(static_cast<float>(a), static_cast<float>(b)))); });
		binary_test("fdim", [](half a, half b) { return comp(fdim(a, b), static_cast<half>(
			std::fdim(static_cast<float>(a), static_cast<float>(b)))); });

		//test exponential functions
		unary_test("exp2", [](half arg) { return comp(exp2(arg), static_cast<half>(std::exp2(static_cast<float>(arg)))); });
		unary_test("expm1", [](half arg) { return comp(expm1(arg), static_cast<half>(std::expm1(static_cast<float>(arg)))); });
		unary_test("log1p", [](half arg) { return comp(log1p(arg), static_cast<half>(std::log1p(static_cast<float>(arg)))); });
		unary_test("log2", [](half arg) { return comp(log2(arg), static_cast<half>(std::log2(static_cast<float>(arg)))); });

		//test power functions
		unary_test("cbrt", [](half arg) { return comp(cbrt(arg), static_cast<half>(std::cbrt(static_cast<float>(arg)))); });
		binary_test("hypot", [](half a, half b) { return comp(hypot(a, b), static_cast<half>(
			std::hypot(static_cast<float>(a), static_cast<float>(b)))); });

		//test hyp functions
		unary_test("asinh", [](half arg) { return comp(asinh(arg), static_cast<half>(std::asinh(static_cast<float>(arg)))); });
		unary_test("acosh", [](half arg) { return comp(acosh(arg), static_cast<half>(std::acosh(static_cast<float>(arg)))); });
		unary_test("atanh", [](half arg) { return comp(atanh(arg), static_cast<half>(std::atanh(static_cast<float>(arg)))); });

		//test err functions
		unary_test("erf", [](half arg) { return comp(erf(arg), static_cast<half>(std::erf(static_cast<float>(arg)))); });
		unary_test("erfc", [](half arg) { return comp(erfc(arg), static_cast<half>(std::erfc(static_cast<float>(arg)))); });
		unary_test("lgamma", [](half arg) { return comp(lgamma(arg), static_cast<half>(std::lgamma(static_cast<float>(arg)))); });
		unary_test("tgamma", [](half arg) { return comp(tgamma(arg), static_cast<half>(std::tgamma(static_cast<float>(arg)))); });

		//test round functions
		unary_test("trunc", [](half arg) { return comp(trunc(arg), 
			static_cast<half>(std::trunc(static_cast<float>(arg)))); });
		unary_test("round", [](half arg) { return comp(round(arg), 
			static_cast<half>(std::round(static_cast<float>(arg)))); });
		unary_test("lround", [](half arg) { return lround(arg) == std::lround(static_cast<float>(arg)); });
		unary_test("llround", [](half arg) { return llround(arg) == std::llround(static_cast<float>(arg)); });
		unary_test("nearbyint", [](half arg) { return comp(nearbyint(arg), 
			static_cast<half>(std::nearbyint(static_cast<float>(arg)))); });
		unary_test("rint", [](half arg) { return comp(rint(arg), 
			static_cast<half>(std::rint(static_cast<float>(arg)))); });
		unary_test("lrint", [](half arg) { return lrint(arg) == std::lrint(static_cast<float>(arg)); });
		unary_test("llrint", [](half arg) { return llrint(arg) == std::llrint(static_cast<float>(arg)); });

		//test float functions
		unary_test("scalbn", [](half arg) -> bool { unsigned int passed = 0; for(int i=-50; i<50; ++i) passed += 
			comp(scalbn(arg, i), static_cast<half>(std::scalbn(static_cast<float>(arg), i))); return passed==100; });
		unary_test("scalbln", [](half arg) -> bool { unsigned int passed = 0; for(long i=-50; i<50; ++i) passed += 
			comp(scalbln(arg, i), static_cast<half>(std::scalbln(static_cast<float>(arg), i))); return passed==100; });
		unary_test("ilogb", [](half arg) { return ilogb(arg) == std::ilogb(static_cast<float>(arg)); });
		unary_test("logb", [](half arg) { return comp(logb(arg), static_cast<half>(std::logb(static_cast<float>(arg)))); });

		//test comparison functions
		binary_test("isgreater", [](half a, half b) { return isgreater(a, b) == 
			std::isgreater(static_cast<float>(a), static_cast<float>(b)); });
		binary_test("isgreaterequal", [](half a, half b) { return isgreaterequal(a, b) == 
			std::isgreaterequal(static_cast<float>(a), static_cast<float>(b)); });
		binary_test("isless", [](half a, half b) { return isless(a, b) == 
			std::isless(static_cast<float>(a), static_cast<float>(b)); });
		binary_test("islessequal", [](half a, half b) { return islessequal(a, b) == 
			std::islessequal(static_cast<float>(a), static_cast<float>(b)); });
		binary_test("islessgreater", [](half a, half b) { return islessgreater(a, b) == 
			std::islessgreater(static_cast<float>(a), static_cast<float>(b)); });
		binary_test("isunordered", [](half a, half b) { return isunordered(a, b) == 
			std::isunordered(static_cast<float>(a), static_cast<float>(b)); });
#endif
/*
		float a = static_cast<float>(halfs_.find("negative quiet NaN")->second.front());
		float b = std::abs(a);
		std::cout << std::hex << *reinterpret_cast<std::uint32_t*>(&a) << '\n' << *reinterpret_cast<std::uint32_t*>(&b) << std::endl;

		int e;
		float s = std::frexp(std::numeric_limits<float>::infinity(), &e);
		std::cout << s << " - " << e << std::endl;

		half hi, hf = modf(b2h(0xFC00), &hi);
		std::cout << hi << " . " << hf << std::endl;
		float fi, ff = std::modf(b2h(0xFC00), &fi);
		std::cout << fi << " . " << ff << std::endl;
		std::cout << comp(hf, static_cast<half>(ff)) << " - " << comp(hi, static_cast<half>(fi)) << std::endl;
		std::cout << outmodf(b2h(0xFC00)) << std::endl;
*/
		bool passed = passed_ == tests_;
		if(passed)
			log_ << "ALL TESTS PASSED\n";
		else
			log_ << (tests_-passed_) << " OF " << tests_ << " FAILED\n";
		return passed;
	}

private:
	typedef std::vector<half> half_vector;
	typedef std::map<std::string,half_vector> test_map;

	template<typename F>
	bool class_test(const std::string &name, const std::map<std::string,int> &classes, F test)
	{
		unsigned int count = 0;
		log_ << "testing " << name << ":\n";
		for(auto iterB=halfs_.begin(); iterB!=halfs_.end(); ++iterB)
		{
			unsigned int passed = 0;
			for(auto iterH=iterB->second.begin(); iterH!=iterB->second.end(); ++iterH)
				passed += test(*iterH) == classes.at(iterB->first);
			log_ << "    " << iterB->first << ": ";
			if(passed == iterB->second.size())
			{
				log_ << "all passed\n";
				++count;
			}
			else
				log_ << (iterB->second.size()-passed) << " of " << iterB->second.size() << " failed\n";
		}
		log_ << '\n';
		bool passed = count == halfs_.size();
		++tests_;
		passed_ += passed;		
		return passed;
	}

	template<typename F>
	bool class_test(const std::string &name, const std::vector<std::string> &classes, F test)
	{
		unsigned int count = 0;
		log_ << "testing " << name << ":\n";
		for(auto iterB=halfs_.begin(); iterB!=halfs_.end(); ++iterB)
		{
			unsigned int passed = 0;
			for(auto iterH=iterB->second.begin(); iterH!=iterB->second.end(); ++iterH)
				passed += test(*iterH) == (std::find(classes.begin(), classes.end(), iterB->first) != classes.end());
			log_ << "    " << iterB->first << ": ";
			if(passed == iterB->second.size())
			{
				log_ << "all passed\n";
				++count;
			}
			else
				log_ << (iterB->second.size()-passed) << " of " << iterB->second.size() << " failed\n";
		}
		log_ << '\n';
		bool passed = count == halfs_.size();
		++tests_;
		passed_ += passed;		
		return passed;
	}

	template<typename F>
	bool unary_test(const std::string &name, F test)
	{
		unsigned int count = 0;
		log_ << "testing " << name << ":\n";
		for(auto iterB=halfs_.begin(); iterB!=halfs_.end(); ++iterB)
		{
			unsigned int passed = 0;
			for(auto iterH=iterB->second.begin(); iterH!=iterB->second.end(); ++iterH)
				passed += test(*iterH);
			log_ << "    " << iterB->first << ": ";
			if(passed == iterB->second.size())
			{
				log_ << "all passed\n";
				++count;
			}
			else
				log_ << (iterB->second.size()-passed) << " of " << iterB->second.size() << " failed\n";
		}
		log_ << '\n';
		bool passed = count == halfs_.size();
		++tests_;
		passed_ += passed;		
		return passed;
	}

	template<typename F>
	bool binary_test(const std::string &name, F test)
	{
		static std::function<std::size_t()> rand = std::bind(std::uniform_int_distribution<std::size_t>(0, 63), std::mt19937());
		unsigned int tests = 0, count = 0;
		log_ << "testing " << name << ": ";
		for(auto iterB1=halfs_.begin(); iterB1!=halfs_.end(); ++iterB1)
		{
			for(auto iterB2=halfs_.begin(); iterB2!=halfs_.end(); ++iterB2)
			{
				for(unsigned int i=std::min(rand(), iterB1->second.size()-1); i<iterB1->second.size(); i+=64)
				{
					for(unsigned int j=std::min(rand(), iterB2->second.size()-1); j<iterB2->second.size(); j+=64)
					{
						++tests;
						count += test(iterB1->second[i], iterB2->second[j]);
					}
				}
			}
		}
		bool passed = count == tests;
		if(passed)
			log_ << "all passed\n\n";
		else
			log_ << (tests-count) << " of " << tests << " failed\n\n";
		++tests_;
		passed_ += passed;		
		return passed;
	}

	test_map halfs_;
	unsigned int tests_;
	unsigned int passed_;
	std::ostream &log_;
};


int main(int argc, char *argv[])
{
/*	half a, b;
	a = (std::numeric_limits<half>::max()*static_cast<half>(2)) / static_cast<half>(2);
	b = std::numeric_limits<half>::max() * static_cast<half>(2);
	b /= static_cast<half>(2);
	std::cout << a << " - " << b << std::endl;
	a = (std::numeric_limits<half>::max()+static_cast<half>(1)) - static_cast<half>(1);
	b = std::numeric_limits<half>::max() + static_cast<half>(1);
	b -= static_cast<half>(2);
	std::cout << a << " - " << b << std::endl;
*/
	std::unique_ptr<std::ostream> file;
	if(argc > 1)
		file.reset(new std::ofstream(argv[1]));
	half_test test((argc>1) ? *file : std::cout);
	return test.test() ? 0 : -1;
}
