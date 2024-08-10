// Analyse.cpp : Defines the entry point for the console application.
//

#include <iostream>
#include <fstream>
#include <cmath>
#include <iomanip>
#include <cstring>
#include <vector>
#include <chrono>
#include <random>
#include <algorithm>

#define _USE_MATH_DEFINES
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace std;


const int _drawRange = 49;
const int _drawCardSize = 7;
const int _drawSeedPool = 500;
const int _ordinalSeedPool = 500;
char _primeNumbers[15] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47};
typedef int Card[_drawCardSize];

struct DrawStatisticList{

	int totalTimesDrawn;
	int drawNumber;
	bool isDrawn;
	double ordinalChance;
	int opportunities; //each attempt to draw this number from the avaliable balls. 
	double average;
	int lastDrawn;
	DrawStatisticList *_next;
};

/*This represents the statistics of the Draw Statistics Lists being correct
	after sorting. A seed pool of Draw data is needed first. The Ordinal variable
	referrences the previous Node in the node list unless it is the first list,
	the first list references the DrawStatisticList */
struct ordinalListNode{

	int landedTotal;
	int ordinal;
	int opportunities;
	double ordinalChance;
	double average;
	bool isDrawn;
	ordinalListNode *_next;
};
struct ordinalBranch{

	ordinalListNode *listNode;
	int SampleSize;  //Number of times this List recorded an event.
	ordinalBranch *_next;
	ordinalBranch *_previous; 
};

struct Config {
    string combinationCollectionFile;
    string drawHistoryFile;
    bool debugMode;

    Config() : combinationCollectionFile("./combinationCollectionFile.dat"),
               drawHistoryFile("./new_draw_order.csv"),
               debugMode(false) {}
};

class Analyse
{
public:
	void display_draw_statistics();
	void display_ordinal_lists();
	void init_all();
	void sort_ordinal_lists();
	void analyse_all_draws();
	void create_all_combinations();
	void calculate_ordinal_event(int,ordinalBranch*&);
	void InitialStatisticalBase();
	bool prime_number_check(Card);
	bool validate_draw_combination(Card);
	void record_ordinal_opportunity(int,ordinalBranch*&);
	void Print();
	void correlate_data();
	// bool LoadCombinationsList();
	void calculate_draw_event(DrawStatisticList*&);
	void propagate_statistical_tree(int, double, ordinalBranch*&);
	void Picker();
	void sort_draws_average();
	void sort_ordinal_average(ordinalBranch*&);
	void initialize_ordinal_list(ordinalListNode*&);
	bool load_config(const string&, Config&);



	struct ValidCombinationList{

		char _validCombination[_drawCardSize];
		struct ValidCombinationList *_next;
	};

	std::vector<ValidCombinationList> _validCombination;
	std::vector<DrawStatisticList*> _drawStatistics;

	DrawStatisticList *_drawNumbersStart;
	ordinalBranch *_ordinalBranchStart;
	Card _lastDraw;
	ValidCombinationList *_validCombinationsStart;
	int _drawHistoryTotal;
	int _totalPicks;
	int _totalNumbersDrawn;
	int _totalValidCombinationCards;
	int _ordinalBranchTotalNodes;
	char _drawHistoryFile[50];
	char _combinationCollectionFile[50];
	bool _debugMode;
};

void Analyse::init_all()
{
	// Initialize the linked list
	_drawNumbersStart = new DrawStatisticList;
	if (!_drawNumbersStart) {
		cerr << "[Error] Failed to allocate memory for _drawNumbersStart." << endl;
		return; // You may want to handle this more gracefully in your actual application
	}

	DrawStatisticList *currentDrawNumber = _drawNumbersStart;
	int ballValue = 0;

	do {
		// Initialize the current node
		currentDrawNumber->totalTimesDrawn = 0;
		currentDrawNumber->drawNumber = (ballValue + 1);
		currentDrawNumber->isDrawn = false;
		currentDrawNumber->opportunities = 0;
		currentDrawNumber->average = 0.0;
		currentDrawNumber->lastDrawn = 0;
		currentDrawNumber->ordinalChance = 0.0;

		ballValue++;

		if (ballValue == _drawRange) {
			currentDrawNumber->_next = NULL;
			break;
		}

		// Allocate memory for the next node
		currentDrawNumber->_next = new DrawStatisticList;
		if (!currentDrawNumber->_next) {
			cerr << "[Error] Failed to allocate memory for next DrawStatisticList node." << endl;
			currentDrawNumber->_next = NULL;
			break; // Exit the loop if memory allocation fails
		}

		// Move to the next node
		currentDrawNumber = currentDrawNumber->_next;

	} while (ballValue < _drawRange);

	_totalValidCombinationCards = 0;

	//create the draw history list from file.

	_ordinalBranchStart = new ordinalBranch;
	_ordinalBranchStart->listNode = new ordinalListNode;
	initialize_ordinal_list(_ordinalBranchStart->listNode);
	_ordinalBranchStart->SampleSize = 0;
	_ordinalBranchStart->_previous = NULL;
	_ordinalBranchStart->_next = NULL;

	_totalNumbersDrawn = 0;
	_ordinalBranchTotalNodes = 1;
	_debugMode = true;
	
}
void Analyse::display_draw_statistics() 
{
	DrawStatisticList* currentDraw;
	currentDraw = _drawNumbersStart;
    std::cerr << "Draw Statistics (sorted by average):" << std::endl;
	while(currentDraw != nullptr)
	{
        std::cerr << "Draw Number: " << currentDraw->drawNumber
                  << " Total Drawn: " << currentDraw->totalTimesDrawn
                  << " Opportunities: " << currentDraw->opportunities
                  << " Average: " << currentDraw->average
				  << " Ordinal Chance: " << currentDraw->ordinalChance
                  << " Last Drawn: " << currentDraw->lastDrawn << std::endl;
		currentDraw = currentDraw->_next;
	}

}
void Analyse::display_ordinal_lists() {
	ordinalBranch* currentBranch;
	currentBranch = _ordinalBranchStart;
	int OrdinalLevel = 1;
	while(currentBranch != NULL)
	{
		std::cerr << "Ordinal Level "<<OrdinalLevel<<" sample size: "<<currentBranch->SampleSize<<" List:" << std::endl;
		ordinalListNode* currentOrdinal = currentBranch->listNode;
		while (currentOrdinal != nullptr) {
			std::cerr << "  Ordinal: " << currentOrdinal->ordinal
					<< " Average: " << currentOrdinal->average
					<< " Landed Total: " << currentOrdinal->landedTotal
					<< " Opportunities: " << currentOrdinal->opportunities << std::endl;
			currentOrdinal = currentOrdinal->_next;
		}
		std::cerr<<std::endl;
		currentBranch = currentBranch->_next;
		OrdinalLevel++;
	}
}
void Analyse::sort_draws_average()
{
    bool swapped;
    DrawStatisticList *ptr1;
    DrawStatisticList *lptr = nullptr;

    do {
        swapped = false;
        ptr1 = _drawNumbersStart;

        while (ptr1->_next != lptr) {
            if (ptr1->average > ptr1->_next->average) {
                std::swap(ptr1->totalTimesDrawn, ptr1->_next->totalTimesDrawn);
                std::swap(ptr1->drawNumber, ptr1->_next->drawNumber);
                std::swap(ptr1->isDrawn, ptr1->_next->isDrawn);
                std::swap(ptr1->opportunities, ptr1->_next->opportunities);
                std::swap(ptr1->average, ptr1->_next->average);
                std::swap(ptr1->lastDrawn, ptr1->_next->lastDrawn);
                swapped = true;
            }
            ptr1 = ptr1->_next;
        }
        lptr = ptr1;
    } while (swapped);
}
void Analyse::sort_ordinal_average(ordinalBranch*& Head)
{
    if (Head == nullptr) return;

    bool swapped;
    ordinalListNode *ptr1;
    ordinalListNode *lptr = nullptr;

    do {
        swapped = false;
        ptr1 = Head->listNode;

        while (ptr1->_next != lptr) {
            if (ptr1->average > ptr1->_next->average) {
                std::swap(ptr1->landedTotal, ptr1->_next->landedTotal);
                std::swap(ptr1->ordinal, ptr1->_next->ordinal);
                std::swap(ptr1->isDrawn, ptr1->_next->isDrawn);
                std::swap(ptr1->opportunities, ptr1->_next->opportunities);
                std::swap(ptr1->average, ptr1->_next->average);
                swapped = true;
            }
            ptr1 = ptr1->_next;
        }
        lptr = ptr1;
    } while (swapped);
}
void Analyse::record_ordinal_opportunity(int ordinance, ordinalBranch*& Node)
{
	ordinalBranch* currentBranch;
	ordinalListNode* currentListNode;
	currentBranch = Node;
	int listOrdinance = 1;
	currentListNode = currentBranch->listNode;
	while(currentListNode != nullptr)
	{
		if(currentListNode->ordinal == ordinance)
		{
			currentListNode->opportunities++;
			currentListNode->average = static_cast<double>(currentListNode->landedTotal) / static_cast<double>(currentListNode->opportunities);
			if(Node->_next != nullptr){
				record_ordinal_opportunity(listOrdinance, Node->_next);
			}
			return;
		}
		else 
		{
			currentListNode = currentListNode->_next;
			listOrdinance++;
		}
	}
}
void Analyse::analyse_all_draws()
{
    DrawStatisticList *listNumber;
    listNumber = _drawNumbersStart;
    int drawCardSlot = 0;
    int drawListLocation = 0;
    int ballNumber = 0;
    int ballsDrawn = 0;  
    string drawDate = "";
    string line;

    // Open the file with error checking
    ifstream file(_drawHistoryFile);
    if (!file.is_open()) {
        cerr << "[Error] Failed to open file: " << _drawHistoryFile << endl;
        return;
    }
	if (_debugMode)
    	cerr << "[Debug] Successfully opened file: " << _drawHistoryFile << endl;

    if (getline(file, line)) {
        if (_debugMode)
            cerr << "[Debug] Skipping header line: " << line << endl;
    }

    // Read each line from the file
    while (getline(file, line)) 
    {
        if (line.empty()) {
            cerr << "[Warning] Skipping empty line in file." << endl;
            continue;
        }
        stringstream ss(line);

        drawCardSlot = 0;

        // Read the draw date
        if (!getline(ss, drawDate, ',')) {
            cerr << "[Error] Failed to read draw date from line: " << line << endl;
            continue;
        }
		

        // Process each ball number in the draw
        while (getline(ss, line, ',')) // Use a temporary line variable to hold each ball number
        {
            try {
                ballNumber = stoi(line); // Convert the string to an integer
            } catch (const invalid_argument& e) {
                cerr << "[Error] Invalid ball number in line: " << line << endl;
                continue;
            }

            if (drawCardSlot >= _drawCardSize) {  // Assuming MAX_DRAW_SLOTS is defined
                cerr << "[Error] Exceeded maximum draw slots for date: " << drawDate << endl;
                break;
            }

            _lastDraw[drawCardSlot] = ballNumber;
            if(_debugMode)
            	cerr << "[Debug] Ball " << ballNumber << " drawn in slot " << drawCardSlot << endl;

            listNumber = _drawNumbersStart;
            drawListLocation = 1;

            while(listNumber != NULL)
            {
                if (!listNumber->isDrawn)
                {
                    if (listNumber->drawNumber == ballNumber)
                    {
						_totalNumbersDrawn++;
                        calculate_draw_event(listNumber);
                        if (_totalNumbersDrawn > _drawSeedPool)
                            calculate_ordinal_event(drawListLocation, _ordinalBranchStart);
                        
						if(_debugMode)
                        	cerr << "[Debug] Ball " << ballNumber << " matches DrawNumber at location " << drawListLocation << endl;
                    } 
                    else 
					{
                        listNumber->opportunities++;
						if (_totalNumbersDrawn > _drawSeedPool){
							record_ordinal_opportunity(drawListLocation, _ordinalBranchStart);
						}
                    }
                }
                listNumber = listNumber->_next;
                drawListLocation++;
            } 
            drawCardSlot++; 
        }
        
        // Reset IsDrawn flag for all DrawNumbers
        listNumber = _drawNumbersStart;
        while (listNumber != NULL)
        {
            listNumber->isDrawn = false;
            listNumber = listNumber->_next;
        }
        sort_draws_average();

        // If the first ordinal list is a stable size, perform sorting and clearing operations
        if (_totalNumbersDrawn > _drawSeedPool){
            sort_ordinal_lists();
        }
    }

    // Close the file
    file.close();
    cerr << "[Debug] Finished processing all draws." << endl;
}
void Analyse::correlate_data()
{
	ordinalBranch* currentBranch;
	ordinalListNode* currentListNode;
	currentBranch = _ordinalBranchStart;
	while (currentBranch->_next != nullptr){
		currentBranch = currentBranch->_next;
	}
	currentListNode = currentBranch->listNode;
	while ( currentListNode != nullptr)
	{
		propagate_statistical_tree(currentListNode->ordinal,currentListNode->average,currentBranch->_previous);
		currentListNode = currentListNode->_next;
	}
}
void Analyse::propagate_statistical_tree(int ordinance, double ordinalSum, ordinalBranch*& branchNode)
{
	ordinalListNode* currentListNode;
	currentListNode = branchNode->listNode;

	int listOrdinance = 1;
	while (listOrdinance < ordinance){
		currentListNode = currentListNode->_next;
		listOrdinance++; 
	}
	currentListNode->ordinalChance = ordinalSum;

	double ordinalSummation = currentListNode->average+ordinalSum;
	
	if( branchNode->_previous != nullptr){
		propagate_statistical_tree(currentListNode->ordinal,ordinalSummation,branchNode->_previous);
	}
	// we are at the base branch node which refferences the rank in the draw list sorted by averages.
	else {
		DrawStatisticList* drawList;
		drawList = _drawNumbersStart;
		// find the draw number based on the ordinal position in the sorted draw list. Big To Do here. :) 
		listOrdinance = 1;
		while(listOrdinance < currentListNode->ordinal){
			drawList = drawList->_next;
			listOrdinance++;
		}
		drawList->ordinalChance = ordinalSummation;
	}
}
void Analyse::sort_ordinal_lists()
{
	ordinalBranch *Current = _ordinalBranchStart;
	
	while(Current != NULL)
	{
		sort_ordinal_average(Current);
		Current = Current->_next;
	}

}
void Analyse::calculate_ordinal_event(int ordinance, ordinalBranch*& Node)
{
	ordinalListNode* currentListNode = Node->listNode;

	int NumberDrawn = 0;
	int DrawCardSlot = 0;
	int OrdinalListLocation = 1;
	while(currentListNode != nullptr)
	{
		if(currentListNode->ordinal == ordinance)
		{
			currentListNode->landedTotal++;
			currentListNode->opportunities++;
			currentListNode->average = static_cast<double>(currentListNode->landedTotal) / static_cast<double>(currentListNode->opportunities);
			Node->SampleSize++;
			if(Node->_next != nullptr){
				calculate_ordinal_event(OrdinalListLocation, Node->_next);
			}
			else if (Node->SampleSize > _ordinalSeedPool)
			{
				_ordinalBranchTotalNodes++;
				Node->_next = new ordinalBranch;
				Node->_next->SampleSize = 0;
				Node->_next->_next = NULL;
				Node->_next->_previous = Node;

				initialize_ordinal_list(Node->_next->listNode);
				calculate_ordinal_event(OrdinalListLocation, Node->_next);
			}
			return;
		}
		currentListNode = currentListNode->_next;
		OrdinalListLocation++;	
	}
}
void Analyse::initialize_ordinal_list(ordinalListNode*& Head)
{
    // Create a vector with numbers 1 to 49
    std::vector<int> ordinals(49);
    for (int i = 0; i < 49; ++i) {
        ordinals[i] = i + 1;
    }

    // Shuffle the vector to get a random permutation
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::shuffle(ordinals.begin(), ordinals.end(), std::default_random_engine(seed));

    // Initialize the head node
    Head = new ordinalListNode;
    ordinalListNode* currentList = Head;

    // Use the shuffled numbers to initialize the list
    for (int i = 0; i < 49; ++i) {
        currentList->ordinal = ordinals[i];
        currentList->average = 0.0;
        currentList->isDrawn = false;
        currentList->opportunities = 0;
		currentList->ordinalChance = 0.0;
        currentList->landedTotal = 0;

        if (i < 48) {
            currentList->_next = new ordinalListNode;
            if (!currentList->_next) {
                cerr << "[Error] Memory allocation failed at position " << (i + 1) << endl;
                break;  // Stop the loop if memory allocation fails
            }
            currentList = currentList->_next;
        } else {
            currentList->_next = nullptr;
        }
    }
}
void Analyse::calculate_draw_event(DrawStatisticList*& Number)
{
	Number->totalTimesDrawn++;
	Number->opportunities++;
	Number->average = static_cast<double>(Number->totalTimesDrawn) / static_cast<double>(Number->opportunities);
	Number->lastDrawn = _totalNumbersDrawn;
	Number->isDrawn = true;
}
bool Analyse::validate_draw_combination(Card PossibleCombinationCard)
{
	/*there are a set of statistical annomolies that relate to winning draws, 
	this function will invalidate any combination without these traits.
	It must have a Prime Number, an even and odd number, properly ranged in value,
	the sum is between a certain range.*/ 
	int Even = 0;
	int Summation = 0;
	int Ones = 0;
	int Tens = 0;
	int Twenties = 0;
	int Thirties = 0;
	int forties = 0;
	int Low = 0;
	//check for a prime number, invalid if not
	if (!prime_number_check(PossibleCombinationCard))
		return false;
	
    // there is a much better way im sure with some effort.
	for (int i = 0; i < 6; i++)
	{
		if (PossibleCombinationCard[i] % 2 == 0)
			Even++;
		Summation = Summation + PossibleCombinationCard[i];
		if (PossibleCombinationCard[i] < 10)
			Ones++;
		else if (PossibleCombinationCard[i] < 20)
			Tens++;
		else if (PossibleCombinationCard[i] < 30)
			Twenties++;
		else if (PossibleCombinationCard[i] < 40)
			Thirties++;
		else if (PossibleCombinationCard[i] < 50)
			forties++;
		if (PossibleCombinationCard[i] < 25)
			Low++;
	}
	if (Even < 2 || Even > 4)
		return false;
	if (Summation < 121 || Summation > 200)
		return false;
	if (Low < 2 || Low > 4)
		return false;
	if (Ones > 3 || Tens > 3 || Twenties > 3 || Thirties > 3 || forties > 3)
		return false;
	if (Ones && Tens && Twenties && Thirties && forties)
		return false;
	return true;
}
bool Analyse::prime_number_check(Card num)
{
	for (int i = 0; i < 6; i++)
	{
		for (int j = 0; j < 15; j++)
		{
			if (num[i] == _primeNumbers[j])
				return true;
		}
	}
	return false;
}
void Analyse::create_all_combinations()
{
	//This function creates every draw combination and validates them for use in this program.

	FILE *CombinationOutputFile = fopen(_combinationCollectionFile, "wb");
	int TotalGeneratedCombinations = 0;
    int TotalValidCombinations = 0;
	Card DrawCombination;
    string combinationLine;
	for (int t = 0; t < _drawCardSize; t++)
		DrawCombination[t] = 0;
	for (int DrawPosition1 = 1; DrawPosition1 <= 43; DrawPosition1++)
	{
		for (int DrawPosition2 = DrawPosition1 + 1; DrawPosition2 <= 44; DrawPosition2++)
		{
			for (int DrawPosition3 = DrawPosition2 + 1; DrawPosition3 <= 45; DrawPosition3++)
			{
				for (int DrawPosition4 = DrawPosition3 + 1; DrawPosition4 <= 46; DrawPosition4++)
				{
					for (int DrawPosition5 = DrawPosition4 + 1; DrawPosition5 <= 47; DrawPosition5++)
					{
						for (int DrawPosition6 = DrawPosition5 + 1; DrawPosition6 <= 48; DrawPosition6++)
						{
                            for (int DrawPosition7 = DrawPosition6 +1; DrawPosition7 <= 49; DrawPosition7++)
                            {
                                DrawCombination[0] = DrawPosition1;
                                DrawCombination[1] = DrawPosition2;
                                DrawCombination[2] = DrawPosition3;
                                DrawCombination[3] = DrawPosition4;
                                DrawCombination[4] = DrawPosition5;
                                DrawCombination[5] = DrawPosition6;
                                DrawCombination[6] = DrawPosition7;
                                TotalGeneratedCombinations++;
                                if (validate_draw_combination(DrawCombination)) {
                                    TotalValidCombinations++;

									combinationLine.clear();
                                    for (int j = 0; j < _drawCardSize; ++j) {
                                        combinationLine += to_string(DrawCombination[j]) + ' ';
                                    }
                                    combinationLine.pop_back(); // Remove the trailing space
                                    combinationLine += '\n';
                                     // Write the entire line to the file
                                    fprintf(CombinationOutputFile, "%s", combinationLine.c_str());
                                }
                            }
                        }
					}
				}
			}
		}
	}
    cerr << "Generated " << TotalGeneratedCombinations << ":" << '\n';
	fclose(CombinationOutputFile);
}
bool load_config(const string& configFilePath, Config& config) {
    ifstream configFile(configFilePath);
    if (!configFile.is_open()) {
        cerr << "Error opening config file: " << configFilePath << endl;
        return false;
    }

    string line;
    while (getline(configFile, line)) {
        stringstream ss(line);
        string key, value;
        if (getline(ss, key, '=') && getline(ss, value)) {
            if (key == "combinationCollectionFile") {
                config.combinationCollectionFile = value;
            } else if (key == "drawHistoryFile") {
                config.drawHistoryFile = value;
            } else if (key == "debugMode") {
                config.debugMode = (value == "true");
			}
        }
    }
    configFile.close();
    return true;
}

int main() {
    Config config;
    string configFilePath = "configs";

    cerr << "Enter config file path (default: configs): ";
    string userInput;
    getline(cin, userInput);

    if (!userInput.empty()) {
        configFilePath = userInput;
    }

    if (!load_config(configFilePath, config)) {
        cerr << "Using default settings." << endl;
    }

if (config.debugMode) {
        std::cerr << "Combinations file: " << config.combinationCollectionFile << std::endl;
        std::cerr << "Draw history file: " << config.drawHistoryFile << std::endl;
        std::cerr << "Debug mode: " << (config.debugMode ? "Enabled" : "Disabled") << std::endl;
    }

    Analyse drawData;
    drawData._debugMode = config.debugMode;

    // Fault tolerance for strncpy
    if (config.combinationCollectionFile.size() >= sizeof(drawData._combinationCollectionFile)) {
        std::cerr << "Error: combinationCollectionFile is too long!" << std::endl;
        return 1;
    }
    if (config.drawHistoryFile.size() >= sizeof(drawData._drawHistoryFile)) {
        std::cerr << "Error: drawHistoryFile is too long!" << std::endl;
        return 1;
    }

    strncpy(drawData._combinationCollectionFile, config.combinationCollectionFile.c_str(), sizeof(drawData._combinationCollectionFile) - 1);
    drawData._combinationCollectionFile[sizeof(drawData._combinationCollectionFile) - 1] = '\0'; // Ensure null termination

    strncpy(drawData._drawHistoryFile, config.drawHistoryFile.c_str(), sizeof(drawData._drawHistoryFile) - 1);
    drawData._drawHistoryFile[sizeof(drawData._drawHistoryFile) - 1] = '\0'; // Ensure null termination

    if (config.debugMode) {
        std::cerr << "Combination file path set to: " << drawData._combinationCollectionFile << std::endl;
        std::cerr << "Draw history file path set to: " << drawData._drawHistoryFile << std::endl;
    }

    drawData.init_all();

    // Load combinations from file or create them if loading fails
    FILE* combinationFile = fopen(drawData._combinationCollectionFile, "r");
    if (!combinationFile) {
        if (config.debugMode) {
            std::cerr << "Error opening combination file: " << drawData._combinationCollectionFile << " (" << strerror(errno) << ")" << std::endl;
            std::cerr << "Creating new combinations..." << std::endl;
        }
        drawData.create_all_combinations();
    } else {
        if (config.debugMode) {
            std::cerr << "Combination file opened successfully: " << drawData._combinationCollectionFile << std::endl;
        }
        fclose(combinationFile);
    }

    // Run the draw engine
    drawData.analyse_all_draws();
	drawData.correlate_data();
	drawData.display_draw_statistics();
	drawData.display_ordinal_lists();
    return 0;
}
