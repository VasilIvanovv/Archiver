#include "BinaryTree.h"



BinaryTree::BinaryTree(ByteCounter* byte_counters,int byte_counter_size)
{

	root = new Node;
	root->left = new Node;
	root->left->frequency = byte_counters[--byte_counter_size].occurrences;
	root->left->word = byte_counters[byte_counter_size].byte;
	root->right = new Node;
	root->right->frequency = byte_counters[--byte_counter_size].occurrences;
	root->right->word = byte_counters[byte_counter_size].byte;
	root->frequency = root->left->frequency + root->right->frequency;

	while (byte_counter_size > 1)
	{
		int right_frequency = byte_counters[--byte_counter_size ].occurrences;
		uint8_t right_word = byte_counters[byte_counter_size].byte;
		int left_frequency = byte_counters[--byte_counter_size].occurrences;
		uint8_t left_word = byte_counters[byte_counter_size].byte;
		Insert(left_frequency, left_word, right_frequency, right_word);
	}

	if (byte_counter_size == 1)
	{
		int left_frequency = byte_counters[--byte_counter_size].occurrences;
		uint8_t left_word = byte_counters[byte_counter_size].byte;
		Insert(left_frequency, left_word);
	}
}


BinaryTree::~BinaryTree()
{
	DeleteTree(root);
}


void BinaryTree::DeleteTree(Node* root)
{
	if (root != NULL)
	{
		DeleteTree(root->left);
		DeleteTree(root->right);
		delete root;
	}
}

void BinaryTree::Insert(int frequency1, uint8_t word1, int frequency2, uint8_t word2)
{
	Node* temp_root = new Node;
	temp_root->frequency = frequency1 + frequency2;
	temp_root->right = new Node;
	temp_root->right->frequency = frequency2;
	temp_root->right->word = word2;
	temp_root->left = new Node;
	temp_root->left->frequency = frequency1;
	temp_root->left->word = word1;
	Node* new_root = new Node;
	new_root->frequency = temp_root->frequency + root->frequency;
	new_root->right = root;
	new_root->left = temp_root;
	root = new_root;
}

void BinaryTree::Insert(int frequency, uint8_t word)
{
	Node* new_root = new Node;
	new_root->frequency = frequency + root->frequency;
	new_root->left = new Node;
	new_root->left->frequency = frequency;
	new_root->left->word = word;
	new_root->right = root;
	root = new_root;

}

int BinaryTree::GetRootFrequency()
{
	return root->frequency;
}

void BinaryTree::MergeTrees(Node* other_root)
{
	Node* new_root = new Node;
	new_root->frequency = other_root->frequency + root->frequency;
	new_root->left = other_root;
	new_root->right = root;
	root = new_root;
}

int BinaryTree::Search(Node* node, uint8_t word, string* code)
{
	if (!node->left && !node->right)
	{
		if (node->word == word)
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}
	else if (Search(node->left, word, code))
	{
		*code += '0';
		return 1;
	}
	else if (Search(node->right, word, code))
	{
		*code += '1';
		return 1;
	}

	return 0;
}

string BinaryTree::Search(uint8_t word)
{
	string word_code;
	Search(root, word, &word_code);
	return word_code;
}

void BinaryTree::WriteCompressed(ofstream* output_file, uint8_t* data, int data_size)
{
	int bytes_size = 1024;
	uint8_t* bytes = new uint8_t[bytes_size];
	int byte_index = 0;
	bytes[byte_index] = 0;
	int byte_pos = 0;
	char* data_ = (char*)data;
	cout << data << endl;
	cout << data_ << endl;
	for (int i = 0; i < data_size; i++)
	{
		string code = Search(data[i]);
		for (int j = code.size() - 1; j >= 0; j--)
		{
			if (byte_pos == 8)
			{

				byte_pos = 0;
				byte_index++;
				if (byte_index >= bytes_size)
				{
					bytes_size *= 2;
					uint8_t* new_bytes = new uint8_t[bytes_size];
					for (int k = 0; k <= byte_index; k++)
					{
						new_bytes[k] = bytes[k];
					}
					delete[] bytes;
					bytes = new_bytes;
				}
				bytes[byte_index] = 0;
			}
			uint8_t byte = 0;
			if (code[j] == '1')
			{
				bytes[byte_index] |= 1 << byte_pos;
			}
			byte_pos++;
		}
	}

	int compressed_size = byte_index + 1;
	output_file->write((char*)&compressed_size, sizeof(int));
	output_file->write((char*)&data_size, sizeof(int));
	output_file->write((char*)bytes, compressed_size);
}

uint8_t* BinaryTree::Decompress(uint8_t* data, int compressed_size, int data_size)
{


	int byte_pos = 0;
	int byte_index = 0;
	char *codes = new char[compressed_size * 8 + 8];
	int codes_count = 0;
	for (int i = 0; i < compressed_size;)
	{
		/*char bit;
		if (data[i] & (1 << byte_pos))
		{
			bit = '1';
		}
		else
		{
			bit = '0';
		}*/
		char bit = (data[i] & (1 << byte_pos)) ? '1' : '0';
		codes[codes_count++] = bit;
		byte_pos++;

		if (byte_pos == 8)
		{
			i++;
			byte_pos = 0;
		}
	}

	codes[codes_count] = '\0';

	uint8_t *output_buffer = new uint8_t[data_size];
	char* codes_p = codes;
	for (int i = 0; i < data_size; i++)
	{
		output_buffer[i] = SearchWord(&codes_p);
	}

	delete[] codes;
	return output_buffer;
}

uint8_t BinaryTree::SearchWord(char** codes)
{
	Node* node = root;


	while (node->left && node->right)
	{
		if (**codes == '0')
		{
			node = node->left;
		}
		else
		{
			node = node->right;
		}

		(*codes)++;
	}

	return node->word;
}