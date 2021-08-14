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

class ttfs_extract_dir
{
	const string& data;
	uint32_t offset;
	const filesystem::path& path;

public:
	ttfs_extract_dir(const string& data, uint32_t offset, const filesystem::path& path)
		: data(data)
		, offset(offset)
		, path(path)
	{
		filesystem::create_directories(path);

		for (uint32_t count = read_u32(); count > 0; --count)
		{
			uint32_t offset = read_u32();
			uint32_t size = read_u32();
			filesystem::path name = path / read_str();

			if (size > 0 || offset == UINT32_MAX)
			{
				if (offset == UINT32_MAX)
					offset = 0;
				const char* addr = &data[offset];
				assert(offset + size <= data.size());
				string_view content(addr, size);
				write_file(name, content);
				continue;
			}

			ttfs_extract_dir(data, offset, name);
		}
	}

private:
	uint32_t read_u32()
	{
		const char* addr = &data[offset];
		const uint32_t* value = reinterpret_cast<const uint32_t*>(addr);
		offset += sizeof(uint32_t);
		assert(offset <= data.size());
		return *value;
	}
	string_view read_str()
	{
		const char* addr = &data[offset];
		string_view value = addr;
		offset += (value.size() + 1);
		assert(offset <= data.size());
		return value;
	}
};

int main(int argc, const char** argv)
{
	while ((--argc) > 0)
	{
		filesystem::path path = filesystem::canonical(*++argv);
		string data = read_file(path);
		string name = path.filename().string();
		path.replace_filename(name + ".dir");
		uint32_t offset = data.at(6) + 0x10u; // root directory
		ttfs_extract_dir(data, offset, path);
	}
}
