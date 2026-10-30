#include "Number.h"
#include <algorithm>
#include <cmath>
#include <stdio.h>
#include <stdlib.h>

std::string number::format(double value, int digits)
{
	if(std::isnan(value))
		return "nan";
	if(std::isinf(value))
		return value > 0 ? "inf" : "-inf";
	char str[64];
	// os grandes em notação científica para caber no rótulo: 1,23e15
	snprintf(str, sizeof(str), fabs(value) >= 1e6 ? "%.*e" : "%.*f", digits, value);
	std::string s(str), exponent;
	size_t e = s.find('e');
	if(e != std::string::npos)
	{
		exponent = "e" + std::to_string(atoi(s.c_str() + e + 1));
		s.resize(e);
	}
	if(s.find('.') != std::string::npos)
	{
		size_t c = s.size();
		while(s[c - 1] == '0')
			c--;
		if(s[c - 1] == '.')
			c--;
		s.resize(c);
	}
	if(s == "-0")
		s = "0";
	size_t dot = s.find('.');
	if(dot != std::string::npos)
		s[dot] = ',';
	return s + exponent;
}

bool number::parse(const std::string& text, double& value)
{
	std::string str = text;
	std::replace(str.begin(), str.end(), ',', '.');
	char* end;
	double result = strtod(str.c_str(), &end);
	// strtod também aceita "nan" e "inf", que estragariam a rede
	if(end == str.c_str() || !std::isfinite(result))
		return false;
	value = result;
	return true;
}
