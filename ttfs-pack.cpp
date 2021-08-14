#include <bits/stdc++.h>
using namespace std;

string read_file(const auto& path)
{
	ifstream fin(path, ios::binary);
	fin.seekg(0, std::ios::end);
	uint32_t size = fin.tellg();
	fin.seekg(0, std::ios::beg);

	string data(size, 0);
	char* begin = &data[0];
	fin.read(begin, size);
	return data;
}

void write_file(const auto& path, const string_view& data)
{
	ofstream fout(path, ios::binary);
	uint32_t size = data.size();
	auto begin = &data[0];
	fout.write(begin, size);
}

class uint32_proxy
{
	string& data;
	const uint32_t offset;

public:
	uint32_proxy(string& data, uint32_t offset)
		: data(data)
		, offset(offset)
	{
	}

public:
	void operator=(uint32_t value)
	{
		char* addr = &data[offset];
		uint32_t* ref = reinterpret_cast<uint32_t*>(addr);
		*ref = value;
	}
};

class ttfs_writer
{
	const filesystem::path& path;
	string data;

public:
	ttfs_writer(const filesystem::path& path)
		: path(path)
		, data({ 't', 't', 'f', 's', 0, 0, 0x05, 0, 0, 0, 0, 0, 0, 0, 0, 0, 'M', 'I', 'D', 'I', 0 })
	{
		vector<pair<filesystem::path, uint32_proxy>> files;
		scan_directory(path, files);

		for (auto& [path, offset] : files)
		{
			uint32_t size = data.size();
			size = (size + 3) & ~3; // size align 4

			data.resize(size);
			offset = size;

			string content = read_file(path);
			data += content;
		}
	}

	operator string() &&
	{
		// usage: string data = ttfs_writer(path)
		return move(data);
	}

private:
	uint32_proxy write_u32(uint32_t value)
	{
		uint32_t offset = data.size();
		data.resize(offset + sizeof(uint32_t));
		auto proxy = uint32_proxy(data, offset);
		proxy = value;
		return proxy;
	}

	void write_str(const string& str)
	{
		data += str;
		data += '\0';
	}

private:
	static auto list_dir(const filesystem::path& path)
	{
		vector<filesystem::directory_entry> dirs;
		vector<filesystem::directory_entry> files;

		for (const auto& entry : filesystem::directory_iterator(path))
		{
			bool isdir = entry.is_directory();
			if (!isdir)
				files.emplace_back(entry);
			else
				dirs.emplace_back(entry);
		}

		auto db = dirs.begin(), de = dirs.end();
		sort(db, de);

		auto fb = files.begin(), fe = files.end();
		sort(fb, fe);

		dirs.insert(de, make_move_iterator(fb), make_move_iterator(fe));
		return dirs;
	}

	void scan_directory(const filesystem::path& path, vector<pair<filesystem::path, uint32_proxy>>& files)
	{
		vector<pair<filesystem::path, uint32_proxy>> dirs;

		uint32_proxy count = write_u32(0);
		uint32_t value = 0;

		for (const auto& entry : list_dir(path))
		{
			++value;

			uint32_proxy offset = write_u32(UINT32_MAX);

			uint32_t size = 0;
			if (!entry.is_directory())
				size = entry.file_size();
			write_u32(size);

			filesystem::path path = entry.path();
			string name = path.filename().string();
			write_str(name);

			if (entry.is_directory())
				dirs.emplace_back(path, offset);
			else if (size > 0)
				files.emplace_back(path, offset);
		}

		count = value;

		for (auto& [path, offset] : dirs)
		{
			offset = data.size();
			scan_directory(path, files);
		}
	}
};

int main(int argc, const char** argv)
{
	while ((--argc) > 0)
	{
		auto path = filesystem::canonical(*++argv);
		string data = ttfs_writer(path);
		string name = path.filename().string();
		path.replace_filename(name + ".archive");
		write_file(path, data);
	}
}
