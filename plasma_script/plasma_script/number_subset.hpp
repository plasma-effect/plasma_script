#pragma once
#include<array>
#include<vector>

#include<boost/range/adaptors.hpp>
#include<boost/range/irange.hpp>

namespace plasma_script
{
	struct number_subset
	{
		static constexpr int size = sizeof(int) * 8;
		std::array<int, 65536 / size> data;
		number_subset() :data{}
		{
			for (auto& v : data)
			{
				v = -1;
			}
		}
		template<class Predicate>void predicate_limit(Predicate const& pred)
		{
			for (std::size_t index{};index < 65536 / size;++index)
			{
				int v{};
				for (std::size_t sub{};sub < size;++sub)
				{
					v += static_cast<int>(pred(index*size + sub)) << sub;
				}
				data[index] = data[index] & v;
			}
		}
		bool check(std::size_t index)const
		{
			return (data[index / size] & (1 << (index%size))) > 0;
		}
		bool set(std::size_t index, bool value)
		{
			data[index / size] = data[index / size] | (static_cast<int>(value) << (index%size));
			return check(index);
		}
		auto get_range()const
		{
			return boost::irange(0, 65536) | boost::adaptors::filtered([range = *this](int v)
			{
				return range.check(v);
			});
		}
	};
}