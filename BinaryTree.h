#pragma once
#include <stdint.h>
#include <iostream>
#include <fstream>
using namespace std;





struct ByteCounter
{
	int occurrences;
	uint8_t byte;
};

struct Node
{
public:
	Node()
	{
		right = NULL;
		left = NULL;
	}
	Node* right;
	Node* left;
	uint8_t word;
	int frequency;
};


class BinaryTree
{
public:
	BinaryTree(ByteCounter*, int);
	~BinaryTree();
	void DeleteTree(Node*);
	void Insert(int, uint8_t, int, uint8_t);
	void Insert(int, uint8_t);
	int GetRootFrequency();
	void MergeTrees(Node*);
	string Search(uint8_t);
	int Search(Node*, uint8_t, string*);
	void WriteCompressed(ofstream*, uint8_t*, int);
	uint8_t* Decompress(uint8_t*, int, int);
	uint8_t SearchWord(char**);
private:
	Node* root;
	uint8_t* data;
	int data_size;
};

