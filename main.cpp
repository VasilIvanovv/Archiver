#include "BinaryTree.h"
#include <iostream>
#include <windows.h>
#include <fstream>
#include <stdint.h>
#include <bitset>
using namespace std;

const int FILE_BUFFER_SIZE = 4096;


struct DynamicArray {
	uint8_t *data;
	size_t used;
	DynamicArray(size_t new_size)
	{
		used = 0;
		data = (uint8_t *)malloc(new_size);
	};
	~DynamicArray()
	{
		delete[] data;
	}
	void Insert(uint8_t byte)
	{
		data[used++] = byte;
	}
};

DynamicArray* ConvertToRLE(uint8_t* data, int data_size)
{
	// abcdaaacccdddsde       4abcd3a3c3d3sde
	DynamicArray* out = new DynamicArray(data_size * 2);

	const uint8_t SEQUENCE_MAX = 255;
	uint8_t literals_count = 0;
	uint8_t literals[SEQUENCE_MAX];
	uint8_t *end = data + data_size;

	while (data < end)
	{
		uint8_t starting_value = *data;
		uint8_t run = 1;
		while ((data[run] == starting_value) &&
			(run < SEQUENCE_MAX) &&
			(data + run < end))
		{
			++run;
		}

		if (run > 1 || literals_count == SEQUENCE_MAX)
		{
			out->Insert(literals_count);
			for (int i = 0; i < literals_count; i++)
			{
				out->Insert(literals[i]);
			}
			literals_count = 0;

			out->Insert(run);
			out->Insert(starting_value);
		}
		else
		{
			literals[literals_count++] = starting_value;
		}

		data += run;
	}

	if (literals_count > 0)
	{
		out->Insert(literals_count);
		for (int i = 0; i < literals_count; i++)
		{
			out->Insert(literals[i]);
		}
	}

	return out;
}


DynamicArray* ConvertFromRLE(uint8_t* data, size_t data_size, size_t out_size)
{
	// abcdaaacccdddsde       4abcd3a3c3d3sde
	DynamicArray* out = new DynamicArray(out_size);
	uint8_t *end = data + data_size;

	while (data < end)
	{
		uint8_t literals_count = *data++;
		for (int i = 0; i < literals_count; i++)
		{
			out->Insert(*data++);
		}
		if (data >= end)
		{
			break;
		}
		uint8_t run = *data++;
		if (run > 0)
		{
			for (int i = 0; i < run; i++)
			{
				out->Insert(*data);
			}
			++data;
		}
	}

	return out;
}


int GetByteFrequency(uint8_t* data, size_t data_size ,ByteCounter* byte_counters)
{
	for (int i = 0; i < 256; i++)
	{
		byte_counters[i].byte = i;
	}
	for (int i = 0; i < data_size; i++)
	{
		byte_counters[(uint8_t)data[i]].occurrences++;
	}

	for (int i = 0; i < 256; i++)
	{
		for (int j = 0; j < 255; j++)
		{
			if (byte_counters[j].occurrences < byte_counters[j + 1].occurrences)
			{
				ByteCounter temp = byte_counters[j];
				byte_counters[j] = byte_counters[j + 1];
				byte_counters[j + 1] = temp;
			}
		}
	}
	int elements_count = 0;
	for (int i = 0; i < 256; i++)
	{
		if (byte_counters[i].occurrences == 0)
		{
			elements_count = i;
			break;
		}
	}
	return elements_count;
}

void CreateDirectories(const char* filepath)
{
	cout << "create dir: " << filepath << endl;
	string folder;
	for (size_t i = 0; i < strlen(filepath); ++i)
	{
		if (filepath[i] != '/' && filepath[i] != '\\')
		{
			folder += filepath[i];
		}
		else
		{
			CreateDirectory(folder.c_str(), NULL);
			folder += '/';
		}
	}

}

void PackFile(string input_filepath, uint64_t input_file_size, ofstream *output_file)
{
	cout << "Packing file: " << input_filepath << endl;
	uint8_t* filebuffer = new uint8_t[FILE_BUFFER_SIZE];
	size_t filepath_len = strlen(input_filepath.c_str());
	output_file->write((char *)&filepath_len, sizeof(size_t));
	output_file->write(input_filepath.c_str(), filepath_len);

	ifstream input_file(input_filepath.c_str(), ios::binary);
	output_file->write((char*)&input_file_size, sizeof(uint64_t));

	while (!input_file.eof())
	{
		input_file.read((char*)filebuffer, FILE_BUFFER_SIZE);
		streamsize bytes_read = input_file.gcount();
		
		DynamicArray* compressed = ConvertToRLE(filebuffer, bytes_read);
		output_file->write((char*)&compressed->used, sizeof(size_t));

		ByteCounter byte_counters[256] = {};
		int byte_frequency_size = GetByteFrequency(compressed->data, compressed->used, byte_counters);
		output_file->write((char*)&byte_frequency_size,sizeof(int));
		for (int i = 0; i < byte_frequency_size; i++)
		{
			output_file->write((char*)&byte_counters[i], sizeof(ByteCounter));
		}

		BinaryTree tree(byte_counters, byte_frequency_size);
		cout << compressed->data << endl;
		tree.WriteCompressed(output_file, compressed->data, compressed->used);
		delete compressed;
	}
	input_file.close();
	delete[] filebuffer;
}


void PackDir(string input_filepath, ofstream *output_file)
{

	cout << "Packing directory: " << input_filepath << endl;
	WIN32_FIND_DATA find_data;
	HANDLE find_handle = FindFirstFile((input_filepath + "/*").c_str(), &find_data);
	int counter = 0;
	if (find_handle != INVALID_HANDLE_VALUE)
	{
		do
		{

			if (!strcmp(find_data.cFileName, ".") ||
				!strcmp(find_data.cFileName, ".."))
			{
				continue;
			}
			counter++;

			if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				PackDir(input_filepath + "/" + find_data.cFileName, output_file);
			}
			else
			{
				uint64_t file_size;
				((uint32_t*)&file_size)[1] = find_data.nFileSizeHigh;
				((uint32_t*)&file_size)[0] = find_data.nFileSizeLow;
				PackFile(input_filepath + "/" + find_data.cFileName, file_size, output_file);
			}
		} while (FindNextFile(find_handle, &find_data));
		if (counter == 0)
		{
			input_filepath += "/";
			size_t filepath_len = strlen(input_filepath.c_str());
			output_file->write((char *)&filepath_len, sizeof(size_t));
			output_file->write(input_filepath.c_str(), filepath_len);
		}
	}
}


void Pack(const char* output_filepath, string input_filepath)
{
	DWORD attributes = GetFileAttributes(input_filepath.c_str());
	if (attributes == INVALID_FILE_ATTRIBUTES)
	{
		cerr << "Can not open file or directoy" << endl;
		return;
	}
	ofstream output_file(output_filepath, ios::binary);
	if (attributes & FILE_ATTRIBUTE_DIRECTORY)
	{
		PackDir(input_filepath, &output_file);
	}
	else
	{
		HANDLE file_handle = CreateFile(input_filepath.c_str(),
										GENERIC_READ,
										FILE_SHARE_READ|FILE_SHARE_WRITE,
										NULL,
										OPEN_EXISTING,
										FILE_ATTRIBUTE_NORMAL,
										NULL);
		LARGE_INTEGER file_size;
		BOOL get_filesize_success = GetFileSizeEx(file_handle, &file_size);
		if (!get_filesize_success)
		{
			cerr << "Can not get file size" << endl;
		}
		PackFile(input_filepath, file_size.QuadPart, &output_file);
	}

	output_file.close();
}

void Unpack(const char* input_filepath, const char* output_filepath)
{
	string output_directory;
	if (output_filepath)
	{
		output_directory = output_filepath;
		output_directory += '\\';
	}

	ifstream input_file(input_filepath, ios::binary);
	char* filebuffer = new char[FILE_BUFFER_SIZE];

	while (!input_file.eof())
	{
		size_t filepath_len;
		input_file.read((char *)&filepath_len, sizeof(size_t));
		if (input_file.eof())
		{
			break;
		}
		char *filepath = new char[filepath_len + 1];
		input_file.read(filepath, filepath_len);
		filepath[filepath_len] = '\0';

		if (output_filepath)
		{
			CreateDirectories((output_directory + filepath).c_str());
		}

		if (filepath[filepath_len - 1] == '/')
		{
			if (output_filepath)
			{
				cout << "Unpacking directory: " << filepath << endl;
			}
			else
			{
				cout << filepath << endl;
			}
			continue;
		}

		if (output_filepath)
		{
			cout << "Unpacking file: " << filepath << endl;
		}
		else
		{
			cout << filepath << endl;
		}

		uint64_t filesize;
		input_file.read((char *)&filesize, sizeof(uint64_t));
		ofstream output_file;

		if (output_filepath)
		{
			output_file.open((output_directory + filepath).c_str(), ios::binary);
			if (!output_file)
			{
				cerr << "Can't open file for writing" << endl;
				return;
			}
		}

		while (filesize > 0)
		{
			size_t decompressed_RLE_size;
			input_file.read((char*)&decompressed_RLE_size, sizeof(size_t));
			int byte_frequency_size;
			input_file.read((char*)&byte_frequency_size, sizeof(int));
			ByteCounter byte_frequencies[256];
			for (int i = 0; i < byte_frequency_size; i++)
			{
				input_file.read((char*)&byte_frequencies[i], sizeof(ByteCounter));
			}

			int compressed_size;
			input_file.read((char*)&compressed_size, sizeof(int));
			int data_size;
			input_file.read((char*)&data_size, sizeof(int));


			BinaryTree tree(byte_frequencies, byte_frequency_size);

			uint8_t* compressed_data = new uint8_t[compressed_size];
			input_file.read((char*)compressed_data, compressed_size);

			uint8_t* RLE_data = tree.Decompress(compressed_data, compressed_size, data_size);
			DynamicArray* decompressed_data = ConvertFromRLE(RLE_data, data_size, decompressed_RLE_size);
			if (output_filepath)
			{
				output_file.write((char*)decompressed_data->data, decompressed_data->used);
			}
		

			filesize -= decompressed_data->used;
			delete[] compressed_data;
			delete[] RLE_data;
		//	delete decompressed_data;
		}

		if (output_filepath)
		{
			output_file.close();
		}

		delete[] filepath;
	}

	delete[] filebuffer;
}


int main(int argc, char* argv[])
{
	

	if (argc == 3 && !strcmp(argv[1], "-List"))
	{
		Unpack(argv[2], NULL);
	}
	else if (argc == 4)
	{
		if (!strcmp(argv[1], "-Pack"))
		{
			char *input_filepath = argv[2];
			char *output_filepath = argv[3];
			Pack(output_filepath, input_filepath);
		}
		else if (!strcmp(argv[1], "-Unpack"))
		{
			char *input_filepath = argv[2];
			char *output_filepath = argv[3];
			Unpack(input_filepath, output_filepath);
		}
		else
		{
			cout << "error2" << endl;

		}
	}
	else
	{
		cout << "error3" << endl;
	}
	system("pause");
	return 0;
}