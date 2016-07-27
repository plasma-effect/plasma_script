#pragma once
#include<type_traits>
#include<functional>

namespace plasma_script
{
	namespace utility
	{
		namespace detail
		{
			template<class FTy>struct currying;
			template<class Return, class Argument, class... Arguments>struct currying<Return(Argument,Arguments...)>
			{
				typedef typename currying<Return(Arguments...)>::value_type return_type;
				typedef std::function<return_type(Argument)> value_type;
				template<class Func>static value_type function_call(Func func)
				{
					return [=](Argument arg)
					{
						return currying<Return(Arguments...)>::function_call([=](Arguments... args)
						{
							return func(arg, args...);
						});
					};
				}
			};
			template<class Return, class Argument>struct currying<Return(Argument)>
			{
				typedef std::function<Return(Argument)> value_type;
				template<class Func>static value_type function_call(Func func)
				{
					return [=](Argument arg)
					{
						return func(arg);
					};
				}
			};
		}
		template<class FTy, class Func>auto currying(Func func)
		{
			return detail::currying<FTy>::function_call(func);
		}
	}
}