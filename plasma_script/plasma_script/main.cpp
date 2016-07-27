#include<iostream>
#include<fstream>
#include<plasma_script/plasma_script/utility.hpp>
#include<plasma_script/plasma_script/script_engine.hpp>

int int_print(int x)
{
	std::cout << x << std::endl;
	return x;
}
int int_read(boost::none_t)
{
	int v;
	std::cout << "”Žš‚ð“ü—Í‚µ‚Ä‚­‚¾‚³‚¢F";
	std::cin >> v;
	return v;
}
int sum(int lhs, int rhs)
{
	return lhs + rhs;
}
bool boolcast(int v)
{
	return v > 0;
}

int main()
{
	try 
	{
		std::fstream fst("test.txt");
		auto code = plasma_script::internal_language::parse(fst);
		std::vector<plasma_script::internal_language::user_defined_class> data;
		data.emplace_back(boost::none);
		data.emplace_back(std::function<int(int)>(int_print));
		data.emplace_back(std::function<int(boost::none_t)>(int_read));
		data.emplace_back(plasma_script::utility::currying<int(int, int)>(sum));
		data.emplace_back(std::function<bool(int)>(boolcast));
		plasma_script::internal_language::script_engine engine{ data,code };
		engine.run();
	}
	catch (std::exception& exp)
	{
		std::cout << exp.what() << std::endl;
	}
}