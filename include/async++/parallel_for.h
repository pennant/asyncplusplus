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

// Run a function for each element in a range
template<typename Range, typename Func>
void parallel_for(scheduler& sched, Range&& range, const Func& func)
{
	// Split the partition, run inline if no more splits are possible
	auto partitioner = async::to_partitioner(std::forward<Range>(range));
	auto subpart = partitioner.split();
	if (std::begin(subpart) == std::end(subpart)) {
		for (auto&& i: partitioner)
			func(std::forward<decltype(i)>(i));
		return;
	}

	// Run the function over each half in parallel
	auto&& t = async::local_spawn(sched, [&sched, subpart, &func] {
		async::parallel_for(sched, subpart, func);
	});
	async::parallel_for(sched, partitioner, func);
	t.get();
}

// Overload with default scheduler
template<typename Range, typename Func>
void parallel_for(Range&& range, const Func& func)
{
	async::parallel_for(LIBASYNC_DEFAULT_SCHEDULER, range, func);
}

// Overloads with std::initializer_list
template<typename T, typename Func>
void parallel_for(scheduler& sched, std::initializer_list<T> range, const Func& func)
{
	async::parallel_for(sched, async::make_range(range.begin(), range.end()), func);
}
template<typename T, typename Func>
void parallel_for(std::initializer_list<T> range, const Func& func)
{
	async::parallel_for(async::make_range(range.begin(), range.end()), func);
}

} // namespace async
