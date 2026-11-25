
#ifndef CONCAL_MPMC_QUEUE_H
#define CONCAL_MPMC_QUEUE_H
#include <atomic>
#include <cassert>


// TODO : UNDERSTANDING HOW IT DOES WORK (I just took that codes...)
template<typename T>
class MPMCQueue
{
public:
	explicit MPMCQueue(size_t buffer_size)
		: _buffer(new Cell[buffer_size])
		, _buffer_mask(buffer_size - 1)
	{
		assert((buffer_size >= 2) && ((buffer_size & (buffer_size - 1)) == 0));

		for (size_t i = 0; i != buffer_size; i += 1)
			_buffer[i].sequence_.store(i, std::memory_order_relaxed);

		_tail.store(0, std::memory_order_relaxed);
		_head.store(0, std::memory_order_relaxed);
	}

	~MPMCQueue()
	{
		delete[] _buffer;
	}

	bool enqueue(T const& data)
	{
		Cell* cell;
		size_t pos = _tail.load(std::memory_order_relaxed);

		for (;;)
		{
			cell = &_buffer[pos & _buffer_mask];
			size_t seq = cell->sequence_.load(std::memory_order_acquire);
			intptr_t dif = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos);

			if (dif == 0)
			{
				if (_tail.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed))
					break;
			}
			else if (dif < 0)
				return false;
			else
				pos = _tail.load(std::memory_order_relaxed);
		}

		cell->data_ = data;
		cell->sequence_.store(pos + 1, std::memory_order_release);

		return true;
	}

	bool dequeue(T& data)
	{
		Cell* cell;
		size_t pos = _head.load(std::memory_order_relaxed);

		for (;;)
		{
			cell = &_buffer[pos & _buffer_mask];
			size_t seq =
				cell->sequence_.load(std::memory_order_acquire);
			intptr_t dif = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos + 1);

			if (dif == 0)
			{
				if (_head.compare_exchange_weak
				(pos, pos + 1, std::memory_order_relaxed))
					break;
			}
			else if (dif < 0)
				return false;
			else
				pos = _head.load(std::memory_order_relaxed);
		}

		data = cell->data_;
		cell->sequence_.store(pos + _buffer_mask + 1, std::memory_order_release);

		return true;
	}

private:
	struct Cell
	{
		std::atomic<size_t>	sequence_;
		T data_;
	};

	static constexpr size_t cacheline_size = 64;
	typedef char cacheline_pad_t[cacheline_size];
	cacheline_pad_t _pad0;
	Cell* _buffer;
	const size_t _buffer_mask;
	cacheline_pad_t _pad1;
	std::atomic<size_t> _tail;
	cacheline_pad_t _pad2;
	std::atomic<size_t>	_head;
	cacheline_pad_t _pad3;

	MPMCQueue(MPMCQueue const&);
	void operator = (MPMCQueue const&);
};
#endif //CONCAL_MPMC_QUEUE_H
