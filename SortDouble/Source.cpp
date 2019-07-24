#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <chrono>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <memory>
#include <random>

template< typename STREAM >
struct stream_t final {
	explicit stream_t(const std::string & file_name, std::ios::openmode mode)
		: m_stream(file_name, mode)
	{
	}

	~stream_t()
	{
		m_stream.close();
	}

	STREAM m_stream;
};


class sorted_file_t final {
public:
	explicit sorted_file_t(std::ifstream & stream)
		: m_stream(stream)
		, m_cur(0.0)
	{
	}

	bool has_data()
	{
		if (!m_buf.empty())
			return true;
		else
		{
			next();

			return (!m_buf.empty());
		}
	}

	double value() const
	{
		return m_cur;
	}

	void next()
	{
		m_buf.clear();

		std::getline(m_stream, m_buf);

		if (!m_buf.empty())
		{
			std::istringstream str(m_buf);
			str >> m_cur;
		}
	}

private:
	std::ifstream & m_stream;
	std::string m_buf;
	double m_cur;
};

class sorted_splitted_data_t final {
public:
	explicit sorted_splitted_data_t(std::vector< sorted_file_t > & files)
		: m_files(files)
	{
	}

	bool has_data()
	{
		return std::any_of(m_files.begin(), m_files.end(),
			[](sorted_file_t & file) { return file.has_data(); });
	}

	double min()
	{
		std::vector< std::pair< double, std::size_t > > values;

		std::size_t count = 0;

		std::for_each(m_files.begin(), m_files.end(),
			[&](sorted_file_t & file)
		{
			if (file.has_data())
				values.push_back(std::make_pair(file.value(), count));

			++count;
		});

		if (!values.empty())
		{
			double min_value = values.front().first;
			std::size_t idx = values.front().second;

			for (const auto & p : values)
			{
				if (p.first < min_value)
				{
					min_value = p.first;
					idx = p.second;
				}
			}

			m_files[idx].next();

			return min_value;
		}
		else
			return 0.0;
	}

private:
	std::vector< sorted_file_t > & m_files;
};

size_t split_file(std::ifstream & stream)
{
	const std::size_t max_size = 1024 * 1024 * 1024 * 1 / sizeof(double);

	size_t count = 0;

	while (stream)
	{
		std::vector< double > data;
		data.reserve(max_size);

		while (data.size() <= max_size && stream)
		{
			std::string text;
			std::getline(stream, text);

			if (!text.empty())
			{
				std::istringstream str(text);
				double value = 0.0;
				str >> value;

				data.push_back(value);
			}
		}

		if (!data.empty())
		{
			std::sort(data.begin(), data.end());

			stream_t< std::ofstream > out(std::to_string(count) + ".txt",
				std::ios::out | std::ios::trunc);

			if (out.m_stream)
			{
				for (const auto & v : data)
					out.m_stream << v << "\n";
			}
			else
				return -1;

			++count;
		}
	}

	return count;
}
void generate()
{
	stream_t< std::ofstream > stream("data.txt", std::ios::out | std::ios::trunc);

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution< double > dis(DBL_MIN, DBL_MAX);

	std::fstream::pos_type size = 0;

	const auto max_file_size = 1024 * 1024 * 1024 * 1;

	while (size < max_file_size)
	{
		std::ostringstream str;
		str << dis(gen);

		const auto text = str.str();

		stream.m_stream << text << "\n";

		size += (text.length() + 1);
	}
}
int main()
{
	std::cout << "Start generating file..." << std::endl;
	generate();
	std::cout << "Start spliting file..." << std::endl;
	const auto start = std::chrono::high_resolution_clock::now();

	size_t count = 0;

	{
		stream_t< std::ifstream > in("data.txt", std::ios::in);

		if (in.m_stream)
		{
			count = split_file(in.m_stream);

			if (count < 0)
			{
				std::cout << "Couldn't split input file\n";

				return 1;
			}
		}
		else
		{
			std::cout << "Couldn't open input file.\n";

			return 1;
		}
	}

	std::vector< std::unique_ptr< stream_t< std::ifstream > > > in_streams;
	std::vector< sorted_file_t > in_files;

	for (size_t i = 0; i < count; ++i)
	{
		in_streams.push_back(std::make_unique< stream_t< std::ifstream > >(
			std::to_string(i) + ".txt", std::ios::in));

		if (!in_streams.back()->m_stream)
		{
			std::cout << "Couldn't open splitted files.\n";

			return 1;
		}

		in_files.push_back(sorted_file_t(in_streams.back()->m_stream));
	}

	sorted_splitted_data_t data(in_files);
	stream_t< std::ofstream > out("sorted_data.txt", std::ios::out | std::ios::trunc);

	if (!out.m_stream)
	{
		std::cout << "Couldn't open output file.\n";

		return 1;
	}
	std::cout << "Start sorting file..." << std::endl;

	while (data.has_data())
		out.m_stream << data.min() << "\n";

	const auto finish = std::chrono::high_resolution_clock::now();

	auto duration = finish - start;

	const auto min = std::chrono::duration_cast<std::chrono::minutes> (duration);

	duration -= min;

	const auto sec = std::chrono::duration_cast<std::chrono::seconds> (duration);

	duration -= sec;

	const auto milli = std::chrono::duration_cast<std::chrono::milliseconds> (duration);

	std::cout << "Elapsed time: "
		<< min.count() << " m "
		<< sec.count() << "."
		<< std::setw(3) << std::setfill('0') << milli.count() << " s\n";

	system("pause");
	return 0;
}