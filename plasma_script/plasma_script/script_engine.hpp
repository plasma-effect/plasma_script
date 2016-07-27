#pragma once
#include<utility>
#include<array>
#include<string>
#include<map>
#include<typeinfo>
#include<functional>
#include<vector>
#include<sstream>

#include<boost/any.hpp>
#include<boost/variant.hpp>
#include<boost/optional.hpp>
#include<boost/lexical_cast.hpp>

namespace plasma_script
{
	namespace internal_language
	{
		class user_defined_class
		{
			struct place_holder
			{
				virtual user_defined_class function_call(std::unique_ptr<place_holder>& argument) = 0;
				virtual std::type_info const& type()const = 0;
				virtual boost::any& get() = 0;
				virtual std::unique_ptr<place_holder> copy()const = 0;
			};

			template<class Derived>struct inside_t :place_holder
			{
				boost::any value;
				inside_t(Derived const& val) :value(val) {}
				inside_t(Derived&& val) :value(std::move(val)) {}
				virtual user_defined_class function_call(std::unique_ptr<place_holder>& argument)override
				{
					throw std::invalid_argument("ä÷êîÇ≈ÇÕÇ†ÇËÇ‹ÇπÇÒ");
				}
				virtual std::type_info const& type()const final override
				{
					return typeid(Derived);
				}
				virtual boost::any& get()final override
				{
					return value;
				}
				virtual std::unique_ptr<place_holder> copy()const override
				{
					return std::make_unique<inside_t<Derived>>(boost::any_cast<Derived>(value));
				}
			};
			template<class Return, class Argument>struct function :inside_t<std::function<Return(Argument)>>
			{
				function(std::function<Return(Argument)>const& func) :inside_t(func) {}
				function(std::function<Return(Argument)>&& func):inside_t(std::move(func)){}
				virtual user_defined_class function_call(std::unique_ptr<place_holder>& argument)
				{
					return user_defined_class(
						boost::any_cast<std::function<Return(Argument)>>(value)
						(boost::any_cast<Argument>(argument->get())));
				}
				virtual std::unique_ptr<place_holder> copy()const override
				{
					return std::make_unique<function<Return,Argument>>(boost::any_cast<std::function<Return(Argument)>>(value));
				}
			};

			std::unique_ptr<place_holder> value;
			user_defined_class(std::unique_ptr<place_holder>&& val, std::nullptr_t) :value(std::move(val)) {}
		public:
			user_defined_class(user_defined_class const& val) :value(val.value->copy()) {}
			user_defined_class& operator=(user_defined_class const& val) 
			{
				value = val.value->copy();
				return *this;
			}

			user_defined_class(user_defined_class&& val) :value(std::move(val.value)) {}
			user_defined_class& operator=(user_defined_class&& val)
			{
				value = std::move(val.value);
				return *this;
			}

			template<class Argument, class Return>user_defined_class(std::function<Return(Argument)> func) : value(std::make_unique<function<Return, Argument>>(func)) {}
			template<class Type>user_defined_class(Type const& val) :value(std::make_unique<inside_t<Type>>(val)) {}
			template<class Type>user_defined_class(Type&& val) : value(std::make_unique<inside_t<Type>>(std::move(val))) {}

			user_defined_class function_call(user_defined_class& val)
			{
				return value->function_call(val.value);
			}
			std::type_info const& type()const
			{
				return value->type();
			}
			bool is_true()const
			{
				return value->type() == typeid(bool) && boost::any_cast<bool>(value->get());
			}
			int get_integer()const
			{
				if (value->type() == typeid(int))
				{
					return boost::any_cast<int>(value->get());
				}
				return 0;
			}
		};

		struct address
		{
			bool absolute;
			std::size_t add;
			std::size_t get(std::size_t memory)const
			{
				return absolute ? add : add + memory;
			}
		};
		address absolute_address(std::size_t add)
		{
			return address{ true,add };
		}
		address comparative_address(std::size_t add)
		{
			return address{ false,add };
		}
		namespace command
		{
			struct constant_
			{
				int v;
				address target;
				template<std::size_t Size>void run(
					std::array<boost::optional<user_defined_class>, Size>& data, 
					std::size_t& line,
					std::size_t& memory)const
				{
					data[target.get(memory)] = user_defined_class(v);
				}
			};
			struct goto_
			{
				std::size_t toward;
				template<std::size_t Size>void run(
					std::array<boost::optional<user_defined_class>, Size>& data,
					std::size_t& line,
					std::size_t& memory)const
				{
					line = toward;
				}
			};
			struct unless_
			{
				address target;
				std::size_t toward;
				template<std::size_t Size>void run(
					std::array<boost::optional<user_defined_class>, Size>& data,
					std::size_t& line,
					std::size_t& memory)const
				{
					if (!data[target.get(memory)]->is_true())
					{
						line = toward;
					}
				}
			};
			struct call_
			{
				address func;
				address arg;
				address ret;
				template<std::size_t Size>void run(
					std::array<boost::optional<user_defined_class>, Size>& data,
					std::size_t& line,
					std::size_t& memory)const
				{
					data[ret.get(memory)].emplace(data[func.get(memory)]->function_call(*data[arg.get(memory)]));
				}
			};
			struct action_
			{
				address func;
				address arg;
				template<std::size_t Size>void run(
					std::array<boost::optional<user_defined_class>, Size>& data,
					std::size_t& line,
					std::size_t& memory)const
				{
					data[func.get(memory)]->function_call(*data[arg.get(memory)]);
				}
			};
			struct forward_
			{
				std::size_t size;
				template<std::size_t Size>void run(
					std::array<boost::optional<user_defined_class>, Size>& data,
					std::size_t& line,
					std::size_t& memory)const
				{
					memory += size;
				}
			};
			struct back_
			{
				std::size_t size;
				template<std::size_t Size>void run(
					std::array<boost::optional<user_defined_class>, Size>& data,
					std::size_t& line,
					std::size_t& memory)const
				{
					memory -= size;
				}
			};
			struct return_
			{
				address add;
				template<std::size_t Size>void run(
					std::array<boost::optional<user_defined_class>, Size>& data,
					std::size_t& line,
					std::size_t& memory)const
				{
					line = data[add.get(memory)]->get_integer();
				}
			};
			struct copy_
			{
				address from;
				address to;
				template<std::size_t Size>void run(
					std::array<boost::optional<user_defined_class>, Size>& data,
					std::size_t& line,
					std::size_t& memory)const
				{
					data[to.get(memory)] = data[from.get(memory)];
				}
			};

			using command_ = boost::variant<constant_, goto_, unless_, call_, action_, forward_, back_, return_, copy_>;
			template<std::size_t Size>void run(
				command_ const& command,
				std::array<boost::optional<user_defined_class>, Size>& data,
				std::size_t& line,
				std::size_t& memory)
			{
				boost::apply_visitor([&](auto const& com)
				{
					com.run(data, line, memory);
				}, command);
			}
		}

		class script_engine
		{
			static constexpr std::size_t size = 128;
			std::size_t line;
			std::size_t memory;
			std::array<boost::optional<user_defined_class>, size> data;
			std::vector<command::command_> commands;
		public:
			template<class DataRange, class CommandRange>
			script_engine(DataRange const& data_, CommandRange const& command_) :line{}, memory{}, data{}, commands(std::begin(command_), std::end(command_))
			{
				std::size_t index{};
				for (auto const& val : data_)
				{
					data[index++].emplace(val);
				}
			}

			bool onestep()
			{
				command::run(commands[line], data, line, memory);
				++line;
				return commands.size() == line;
			}

			void run()
			{
				while (!onestep());
			}
		};

		namespace detail
		{
			address address_parse(std::string str)
			{
				if (str[0] == '$')
				{
					return absolute_address(boost::lexical_cast<std::size_t>(str.substr(1)));
				}
				else
				{
					return comparative_address(boost::lexical_cast<std::size_t>(str));
				}
			}
		}

		std::vector<command::command_> parse(std::istream& ist)
		{
			std::vector<command::command_> ret;
			std::string str;
			while (std::getline(ist, str))
			{
				if (str.size() == 0)
					continue;
				std::string s;
				std::stringstream ss(str);
				ss >> s;
				if (s == "const")
				{
					int v;
					ss >> v;
					ss >> s;
					ret.emplace_back(command::constant_{ v,detail::address_parse(s) });

				}
				else if (s == "goto")
				{
					std::size_t v;
					ss >> v;
					ret.emplace_back(command::goto_{ v });
				}
				else if (s == "unless")
				{
					std::size_t v;
					ss >> s;
					ss >> v;
					ret.emplace_back(command::unless_{ detail::address_parse(s),v });
				}
				else if (s == "call")
				{
					ss >> s;
					address func = detail::address_parse(s);
					ss >> s;
					address arg = detail::address_parse(s);
					ss >> s;
					address r = detail::address_parse(s);
					ret.emplace_back(command::call_{ func,arg,r });
				}
				else if (s == "action")
				{
					ss >> s;
					address func = detail::address_parse(s);
					ss >> s;
					address arg = detail::address_parse(s);
					ret.emplace_back(command::action_{ func,arg });
				}
				else if (s == "forward")
				{
					std::size_t v;
					ss >> v;
					ret.emplace_back(command::forward_{ v });
				}
				else if (s == "back")
				{
					std::size_t v;
					ss >> v;
					ret.emplace_back(command::back_{ v });
				}
				else if (s == "return")
				{
					ss >> s;
					ret.emplace_back(plasma_script::internal_language::command::return_{ detail::address_parse(s) });
				}
				else if (s == "copy")
				{
					ss >> s;
					auto from = detail::address_parse(s);
					ss >> s;
					auto to = detail::address_parse(s);
					ret.emplace_back(command::copy_{ from,to });
				}
			}
			return ret;
		}
	}
}