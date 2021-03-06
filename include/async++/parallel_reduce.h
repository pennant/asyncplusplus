// Copyright (c) 2013 Amanieu d'Antras
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#ifndef ASYNCXX_H_
# error "Do not include this header directly, include <async++.h> instead."
#endif

namespace async {
namespace detail {

// Default map function which simply passes its parameter through unmodified
struct default_map {
	template<typename T>
	T&& operator()(T&& x) const
	{
		return std::forward<T>(x);
	}
};

} // namespace detail

// Run a function for each element in a range and then reduce the results of that function to a single value
template<typename Range, typename Result, typename MapFunc, typename ReduceFunc>
Result parallel_map_reduce(scheduler& sched, Range&& range, const Result& init, const MapFunc& map, const ReduceFunc& reduce)
{
	// Split the partition, run inline if no more splits are possible
	auto partitioner = async::to_partitioner(std::forward<Range>(range));
	auto subpart = partitioner.split();
	if (std::begin(subpart) == std::end(subpart)) {
		Result out = init;
		for (auto&& i: partitioner)
			out = reduce(std::move(out), map(std::forward<decltype(i)>(i)));
		return out;
	}

	// Run the function over each half in parallel
	auto&& t = async::local_spawn(sched, [&sched, subpart, init, &map, &reduce] {
		return async::parallel_map_reduce(sched, subpart, init, map, reduce);
	});
	Result out = async::parallel_map_reduce(sched, partitioner, init, map, reduce);
	return reduce(std::move(out), t.get());
}

// Overload with default scheduler
template<typename Range, typename Result, typename MapFunc, typename ReduceFunc>
Result parallel_map_reduce(Range&& range, const Result& init, const MapFunc& map, const ReduceFunc& reduce)
{
	return async::parallel_map_reduce(LIBASYNC_DEFAULT_SCHEDULER, range, init, map, reduce);
}

// Overloads with std::initializer_list
template<typename T, typename Result, typename MapFunc, typename ReduceFunc>
Result parallel_map_reduce(scheduler& sched, std::initializer_list<T> range, const Result& init, const MapFunc& map, const ReduceFunc& reduce)
{
	return async::parallel_map_reduce(sched, async::make_range(range.begin(), range.end()), init, map, reduce);
}
template<typename T, typename Result, typename MapFunc, typename ReduceFunc>
Result parallel_map_reduce(std::initializer_list<T> range, const Result& init, const MapFunc& map, const ReduceFunc& reduce)
{
	return async::parallel_map_reduce(async::make_range(range.begin(), range.end()), init, map, reduce);
}

// Variant with identity map operation
template<typename Range, typename Result, typename ReduceFunc>
Result parallel_reduce(scheduler& sched, Range&& range, const Result& init, const ReduceFunc& reduce)
{
	return async::parallel_map_reduce(sched, range, init, detail::default_map(), reduce);
}
template<typename Range, typename Result, typename ReduceFunc>
Result parallel_reduce(Range&& range, const Result& init, const ReduceFunc& reduce)
{
	return async::parallel_reduce(LIBASYNC_DEFAULT_SCHEDULER, range, init, reduce);
}
template<typename T, typename Result, typename ReduceFunc>
Result parallel_reduce(scheduler& sched, std::initializer_list<T> range, const Result& init, const ReduceFunc& reduce)
{
	return async::parallel_reduce(sched, async::make_range(range.begin(), range.end()), init, reduce);
}
template<typename T, typename Result, typename ReduceFunc>
Result parallel_reduce(std::initializer_list<T> range, const Result& init, const ReduceFunc& reduce)
{
	return async::parallel_reduce(async::make_range(range.begin(), range.end()), init, reduce);
}

} // namespace async
