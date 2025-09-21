#include <iostream>
#include <string>

const int CACHE_LINE_SIZE = (16 / 4);

struct cacheLine {
	int data[CACHE_LINE_SIZE]{ 0 };
	bool validBit = false;
	bool dirtyBit = false;
	int tag;
	bool justUsed = false;
};

//Function prototypes
void writeAccess(cacheLine[][2], int[], int);
void displayMemory(cacheLine[][2], int[]);
int extractOffset(short int);
int extractCacheLineIndex(short int);
int extractTag(short int);

int main() {

	//Total cache memory is 2048 bytes, but since each of our cache line has 16 bytes of data (4 integers) each, we divide by 16 for each cache line in the array.
	const int CACHE_MEM_SIZE = ((2048 / 16) / 2);
	const int PROCESSOR_MEM_SIZE = (32768 / 4);
	const int associativity = 2;

	//Arrays simulating processor and cache storage
	cacheLine cacheMemory[CACHE_MEM_SIZE][2]{ 0 };
	int totalProcessorMemory[PROCESSOR_MEM_SIZE]{ 0 };

	char input;

	//Loop to allow for multiple entries
	do {
		std::cout << "Enter a command, A or B: ";
		std::cin >> input;
		
		while (toupper(input) != 'A' && toupper(input) != 'B') {
			std::cout << "Command must be A, a, B, or b: ";
			std::cin >> input;
		}
		
		//Processing input
		if (toupper(input) == 'A') {
			writeAccess(cacheMemory, totalProcessorMemory, associativity);
		}
		if (toupper(input) == 'B') {
			displayMemory(cacheMemory, totalProcessorMemory);
		}

		std::cout << "Another run? (Y/N): ";
		std::cin >> input;

	} while (toupper(input) == 'Y');

	std::cout << std:: endl << "Program end." << std::endl;

	return 0;
}

void writeAccess(cacheLine cacheMemory[][2], int mainMemory[], int associativity) {
	int inputData;
	short int cacheAddress;
	int mainMemAddressBase;
	int oldTag;
	//setChoice is -1 as a flag
	int setChoice = -1;

	std::cout << "Enter integer data to be written: ";
	std::cin >> inputData;
	std::cout << "Enter the address: ";
	std::cin >> cacheAddress;

	//Error checking input address
	if (cacheAddress < 0 || cacheAddress >= 32768) {
		std::cout << "Input address " << cacheAddress << " invalid" << std::endl;
		return;
	}

	//If the address is not divisible by 4, set it to the next lowest multiple
	if (cacheAddress % 4 != 0) {
		std::cout << "Setting address to next lower multiple of 4" << std::endl;
		cacheAddress = cacheAddress - (cacheAddress % 4);
	}

	int tag = extractTag(cacheAddress);
	int index = extractCacheLineIndex(cacheAddress);
	int offset = extractOffset(cacheAddress);

	//Checking both cache lines in a set to see if tag matches
	for (int i = 0; i < associativity; i++) {
		if (cacheMemory[index][i].validBit && cacheMemory[index][i].tag == tag) {
			//In the event that a tag matches, write data to cache line and update bits
			cacheMemory[index][i].data[offset] = inputData;

			cacheMemory[index][i].dirtyBit = true;
			cacheMemory[index][i].justUsed = true;
			cacheMemory[index][1 - i].justUsed = false;
			return;
		}
	}

	//If the tag doesn't match, we attempt to choose a set based on if one has data or not
	for (int i = 0; i < associativity; i++) {
		if (!cacheMemory[index][i].validBit) {
			setChoice = i;
			i = associativity;
		}
	}
	//If both have data, we use our least recentely used setup
	if (setChoice == -1) {
		if (cacheMemory[index][0].justUsed) {
			setChoice = 1;
		}
		else {
			setChoice = 0;
		}
	}

	//Creating reference to chosen cacheline to ease processing and pass by reference
	cacheLine& cacheLineReference = cacheMemory[index][setChoice];

	//Performing "write-back" in the case that the line is dirty
	if (cacheLineReference.validBit && cacheLineReference.dirtyBit) {
		oldTag = cacheLineReference.tag;
		mainMemAddressBase = ((oldTag << 10) | (index << 4)) & 15;
		for (int i = 0; i < CACHE_LINE_SIZE; i++) {
			//For each "word" in memory
			mainMemory[(mainMemAddressBase + i * 4) / 4] = cacheLineReference.data[i];
		}
	}

	//Replace cache line after eviction takes place
	cacheLineReference.tag = tag;
	cacheLineReference.validBit = true;
	cacheLineReference.dirtyBit = true;
	cacheLineReference.justUsed = true;
	cacheMemory[index][1 - setChoice].justUsed = false;
	//To set data in evicted cache line to 0
	for (int i = 0; i < CACHE_LINE_SIZE; i++) {
		cacheLineReference.data[i] = 0;
	}
	cacheLineReference.data[offset] = inputData;
}

void displayMemory(cacheLine cacheMemory[][2], int mainMemory[]) {
	int memAddress;
	int addressWordFormat;

	std::cout << "Enter the address: ";
	std::cin >> memAddress;

	//Error checking input address
	if (memAddress < 0 || memAddress >= 32768) {
		std::cout << "Input address " << memAddress << " invalid" << std::endl;
		return;
	}
	//If the address is not divisible by 4, set it to the next lowest multiple
	if (memAddress % 4 != 0) {
		std::cout << "Setting address to next lower multiple of 4" << std::endl;
		memAddress = memAddress - (memAddress % 4);
	}

	int index = extractCacheLineIndex(memAddress);
	addressWordFormat = memAddress / 4;

	std::cout << "Address: " << memAddress << " Memory: " << mainMemory[addressWordFormat] << " Cache: ";

	for (int i = 0; i < CACHE_LINE_SIZE; i++) {
		std::cout << cacheMemory[index][0].data[i] << " ";
	}
	std::cout << " ";
	for (int i = 0; i < CACHE_LINE_SIZE; i++) {
		std::cout << cacheMemory[index][1].data[i] << " ";
	}
	if (!cacheMemory[index][0].validBit) {
		std::cout << " -1 ";
	}
	else {
		std::cout << " " << cacheMemory[index][0].validBit << " ";
	}
	std::cout << cacheMemory[index][0].dirtyBit;
	if (!cacheMemory[index][1].validBit) {
		std::cout << " -1 ";
	}
	else {
		std::cout << " " << cacheMemory[index][1].validBit << " ";
	}
	std::cout << cacheMemory[index][1].dirtyBit << std::endl;
}

//Functions to extract byte offset, cache line number, and tag
int extractOffset(short int address) {
	//Anding with 15 to extract bit offset, divide by 4 to ensure our offset properly matches
	return ((address & 15)) / 4;
}

int extractCacheLineIndex(short int address) {
	//Shift right 4 and and with 63 (6 1s in binary 0011 1111)
	return ((address >> 4) & 63);
}

int extractTag(short int address) {
	//Tag is the furtherst 6 bits, so shift right 10 to shift other values of address out
	return (address >> 10);
}
