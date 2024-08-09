// Analyse.cpp : Defines the entry point for the console application.
//

#include <iostream>
#include <fstream>
#include <cmath>
#include <iomanip>
#include <cstring>
#include <vector>

#define _USE_MATH_DEFINES
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace std;


const int DrawRange = 49;
const int DRAW_SIZE = 7;
const int SeedPool = 500;
char PrimeNumbers[15] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47};
typedef int Card[DRAW_SIZE];
bool debug=false;

struct DrawStatisticList{

	int TotalTimesDrawn;
	char DrawNumber;
	bool IsDrawn;
	int Opportunities; //each attempt to draw this number from the avaliable balls. 
	double Average;
	int LastDrawn;
	DrawStatisticList *Next;
};

/*This represents the statistics of the Draw Statistics Lists being correct
	after sorting. A seed pool of Draw data is needed first. The Ordinal variable
	referrences the previous Node in the node list unless it is the first list,
	the first list references the DrawStatisticList */
struct OrdinalList{

	int LandedTotal;
	int Ordinal;
	int Opportunities;
	double Average;
	bool IsDrawn;
	OrdinalList *Next;
};
struct OrdinalNodes{

	OrdinalList ListNode;
	int SampleSize;  //Number of times this List recorded an event.
	OrdinalNodes *Next;
	OrdinalNodes *Previous; 
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
	void calculate_ordinal_event(int,OrdinalNodes*&);
	void InitialStatisticalBase();
	bool prime_number_check(Card);
	bool validate_draw_combination(Card);
	void Print();
	// bool LoadCombinationsList();
	void calculate_draw_event(DrawStatisticList*&);
	void Picker();
	void sort_draws_average();
	void sort_ordinal_average(OrdinalNodes*&);
	void clear_boolean_flags(OrdinalNodes*&);
	void init_ordinal_list(OrdinalList&);
	bool load_config(const string&, Config&);
	// Each OrdinalList is statistically represented once the lists are seeded.


	struct ValidCombinationList{

		char ValidCombination[DRAW_SIZE];
		struct ValidCombinationList *Next;
	};

	std::vector<ValidCombinationList> ValidCombination;
	std::vector<DrawStatisticList*> drawStatistics;

	DrawStatisticList *DrawNumbersStart;
	OrdinalNodes *OrdinalNodesStart;
	Card LastDraw;
	ValidCombinationList *ValidCombinationsStart;
    char PerformAnalysis,UseRootData;
	int DrawHistoryTotal;
	int total_picks;
	int totalNumbersDrawn;
	int totalValidCombinationCards;
	char drawHistoryFile[50];
	char combinationCollectionFile[50];
	bool debugMode;
};

void Analyse::init_all()
{
	DrawNumbersStart = new DrawStatisticList;
	DrawStatisticList *currentDrawNumber;
	currentDrawNumber = DrawNumbersStart;
	int ballValue = 0;
	do
	{
		currentDrawNumber->TotalTimesDrawn = 0;
		currentDrawNumber->DrawNumber = (ballValue + 1);
		currentDrawNumber->IsDrawn = false;
		currentDrawNumber->Opportunities = 0;
		currentDrawNumber->Average = 0.0;
		currentDrawNumber->LastDrawn = 0;
		ballValue++;
		if(ballValue == 49)
		{
			currentDrawNumber->Next = NULL;
			break;
		}
		currentDrawNumber->Next = new DrawStatisticList;
		currentDrawNumber = currentDrawNumber->Next;
	}while(ballValue < 49);

	totalValidCombinationCards = 0;

	//create the draw history list from file.

	OrdinalNodesStart = new OrdinalNodes;
	init_ordinal_list(OrdinalNodesStart->ListNode);
	OrdinalNodesStart->Previous = NULL;
	OrdinalNodesStart->Next = NULL;

	totalNumbersDrawn = 0;
	debugMode = true;
	
}
void Analyse::display_draw_statistics() 
{
    std::cout << "Draw Statistics (sorted by average):" << std::endl;
    for (const auto* stat : drawStatistics) {
        std::cout << "Draw Number: " << stat->DrawNumber
                  << " Total Drawn: " << stat->TotalTimesDrawn
                  << " Opportunities: " << stat->Opportunities
                  << " Average: " << stat->Average
                  << " Last Drawn: " << stat->LastDrawn << std::endl;
    }
}
void Analyse::display_ordinal_lists() {
	OrdinalNodes* currentOrdinal;
	currentOrdinal = OrdinalNodesStart;
	int OrdinalLevel = 1;
	while(currentOrdinal != NULL)
	{
		std::cout << "Ordinal Level "<<OrdinalLevel<<" Lists:" << std::endl;
		OrdinalList* current = &OrdinalNodesStart->ListNode;
		int listIndex = 0;
		while (current != nullptr) {
			std::cout << "Ordinal List " << listIndex++ << ":" << std::endl;
			std::cout << "  Ordinal: " << current->Ordinal
					<< " Average: " << current->Average
					<< " Landed Total: " << current->LandedTotal
					<< " Opportunities: " << current->Opportunities << std::endl;
			current = current->Next;
		}
		std::cout<<std::endl;
		currentOrdinal = currentOrdinal->Next;
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
        ptr1 = DrawNumbersStart;

        while (ptr1->Next != lptr) {
            if (ptr1->Average > ptr1->Next->Average) {
                std::swap(ptr1->TotalTimesDrawn, ptr1->Next->TotalTimesDrawn);
                std::swap(ptr1->DrawNumber, ptr1->Next->DrawNumber);
                std::swap(ptr1->IsDrawn, ptr1->Next->IsDrawn);
                std::swap(ptr1->Opportunities, ptr1->Next->Opportunities);
                std::swap(ptr1->Average, ptr1->Next->Average);
                std::swap(ptr1->LastDrawn, ptr1->Next->LastDrawn);
                swapped = true;
            }
            ptr1 = ptr1->Next;
        }
        lptr = ptr1;
    } while (swapped);
}
void Analyse::sort_ordinal_average(OrdinalNodes*& Head)
{
    if (Head == nullptr) return;

    bool swapped;
    OrdinalList *ptr1;
    OrdinalList *lptr = nullptr;

    do {
        swapped = false;
        ptr1 = &Head->ListNode;

        while (ptr1->Next != lptr) {
            if (ptr1->Average > ptr1->Next->Average) {
                std::swap(ptr1->LandedTotal, ptr1->Next->LandedTotal);
                std::swap(ptr1->Ordinal, ptr1->Next->Ordinal);
                std::swap(ptr1->IsDrawn, ptr1->Next->IsDrawn);
                std::swap(ptr1->Opportunities, ptr1->Next->Opportunities);
                std::swap(ptr1->Average, ptr1->Next->Average);
                swapped = true;
            }
            ptr1 = ptr1->Next;
        }
        lptr = ptr1;
    } while (swapped);
}
void Analyse::analyse_all_draws()
{
	DrawStatisticList *DrawNumber;
	DrawNumber = DrawNumbersStart;
	int NumberDrawn = 0;
	int DrawCardSlot = 0;
	int DrawListLocation = 0;
    int ball_num;
    int ballsDrawn = 0;  
	string draw_date = "";
    string line;

    ifstream file(drawHistoryFile);
    if (!file.is_open()) {
        cerr << "Error opening file: " << drawHistoryFile << endl;
        return;
    }
    while (getline(file, line)) 
    {
        stringstream ss(line);
        DrawCardSlot = 0;
		ss >> draw_date; // skip the date

		while (ss >> ball_num)
		{
            LastDraw[DrawCardSlot] = ball_num;
            totalNumbersDrawn++;
            DrawNumber = DrawNumbersStart;
            DrawListLocation = 0;
            while(DrawNumber != NULL)
            {
                if (!DrawNumber->IsDrawn)
                {
                    if(DrawNumber->DrawNumber == ball_num )
                    {
                        DrawNumber->IsDrawn = true;
						DrawNumber->Opportunities++;
                        calculate_draw_event(DrawNumber);
                        if(totalNumbersDrawn > SeedPool)
                        {   
                            calculate_ordinal_event(DrawListLocation,OrdinalNodesStart);
                        }
                        
                    } else DrawNumber->Opportunities++;  
                }
                DrawNumber=DrawNumber->Next;
                DrawListLocation++;
            } 
            DrawCardSlot++; 
		}
		
		DrawNumber = DrawNumbersStart;
        while(DrawNumber != NULL)
        {
            DrawNumber->IsDrawn = false;
            DrawNumber=DrawNumber->Next;
        }
		DrawCardSlot = 0;
        if(totalNumbersDrawn > SeedPool)
        {
            sort_draws_average();
            sort_ordinal_lists();
            clear_boolean_flags(OrdinalNodesStart);
        }
	}
	file.close();
}
void Analyse::clear_boolean_flags(OrdinalNodes*& Head)
{
	OrdinalNodes* Node = Head;
	while (Node != NULL)
	{
		Node->ListNode.IsDrawn = false;
		Node = Node->Next;
	}
}
void Analyse::sort_ordinal_lists()
{
	OrdinalNodes *Current = OrdinalNodesStart;
	
	while(Current != NULL)
	{
		sort_ordinal_average(Current);
		clear_boolean_flags(Current);
		Current = Current->Next;
	}

}
void Analyse::calculate_ordinal_event(int Postion, OrdinalNodes*& Node)
{

	OrdinalList* OrdinalListNode = &Node->ListNode;

	int NumberDrawn = 0;
	int DrawCardSlot = 0;
	int OrdinalListLocation = 0;
	while(OrdinalListNode != nullptr)
	{
		if (!OrdinalListNode->IsDrawn)
		{
			if(OrdinalListNode->Ordinal == Postion)
			{
				OrdinalListNode->IsDrawn = true;
				OrdinalListNode->LandedTotal++;
				OrdinalListNode->Opportunities++;
				OrdinalListNode->Average = OrdinalListNode->LandedTotal / OrdinalListNode->Opportunities;
				Node->SampleSize++;
				if(Node->Next == NULL && Node->SampleSize >= SeedPool)
				{
					Node->Next = new OrdinalNodes;
					//Node.Next->ListNode = new OrdinalList;
					init_ordinal_list(Node->ListNode);
					
				}
				calculate_ordinal_event(OrdinalListLocation, Node->Next);
				
			} else OrdinalListNode->Opportunities++;
		}
		OrdinalListNode = OrdinalListNode->Next;
		OrdinalListLocation++;
	}
}
void Analyse::init_ordinal_list(OrdinalList& Head)
{
	OrdinalList* OrdinalListNode = &Head;
	
	int OrdinalSet = 0;
	do
	{
		if(debug){
			cout<<OrdinalListNode->Ordinal<<'\n';
		}

		OrdinalListNode->Ordinal = OrdinalSet;
		OrdinalListNode->Average = 0.0;
		OrdinalListNode->IsDrawn = false;
		OrdinalListNode->Opportunities = 0;
		OrdinalListNode->LandedTotal = 0;
		OrdinalSet++;
		if(OrdinalSet == 49)
		{
			OrdinalListNode->Next = NULL;
			break;
		}
		OrdinalListNode->Next = new OrdinalList;
		OrdinalListNode = OrdinalListNode->Next;
	}while(OrdinalSet < 49);
}
void Analyse::calculate_draw_event(DrawStatisticList*& Number)
{
	Number->TotalTimesDrawn++;
	Number->Average = Number->TotalTimesDrawn / Number->Opportunities;
	Number->LastDrawn = totalNumbersDrawn;
};
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
			if (num[i] == PrimeNumbers[j])
				return true;
		}
	}
	return false;
}
void Analyse::create_all_combinations()
{
	//This function creates every draw combination and validates them for use in this program.

	FILE *CombinationOutputFile = fopen(combinationCollectionFile, "wb");
	int TotalGeneratedCombinations = 0;
    int TotalValidCombinations = 0;
	Card DrawCombination;
    string combinationLine;
	for (int t = 0; t < DRAW_SIZE; t++)
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

                                    if (debugMode) {
                                        cout << "Combination " << TotalValidCombinations << ": ";
                                        for (int j = 0; j < DRAW_SIZE; ++j) {
                                            cout << (int)DrawCombination[j] << ' ';
                                        }
                                        cout << '\n';
                                    }
									combinationLine.clear();
                                    for (int j = 0; j < DRAW_SIZE; ++j) {
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
    cout << "Generated " << TotalGeneratedCombinations << ":" << '\n';
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

    cout << "Enter config file path (default: configs): ";
    string userInput;
    getline(cin, userInput);

    if (!userInput.empty()) {
        configFilePath = userInput;
    }

    if (!load_config(configFilePath, config)) {
        cerr << "Using default settings." << endl;
    }

if (config.debugMode) {
        std::cout << "Combinations file: " << config.combinationCollectionFile << std::endl;
        std::cout << "Draw history file: " << config.drawHistoryFile << std::endl;
        std::cout << "Debug mode: " << (config.debugMode ? "Enabled" : "Disabled") << std::endl;
    }

    Analyse drawData;
    drawData.debugMode = config.debugMode;

    // Fault tolerance for strncpy
    if (config.combinationCollectionFile.size() >= sizeof(drawData.combinationCollectionFile)) {
        std::cerr << "Error: combinationCollectionFile is too long!" << std::endl;
        return 1;
    }
    if (config.drawHistoryFile.size() >= sizeof(drawData.drawHistoryFile)) {
        std::cerr << "Error: drawHistoryFile is too long!" << std::endl;
        return 1;
    }

    strncpy(drawData.combinationCollectionFile, config.combinationCollectionFile.c_str(), sizeof(drawData.combinationCollectionFile) - 1);
    drawData.combinationCollectionFile[sizeof(drawData.combinationCollectionFile) - 1] = '\0'; // Ensure null termination

    strncpy(drawData.drawHistoryFile, config.drawHistoryFile.c_str(), sizeof(drawData.drawHistoryFile) - 1);
    drawData.drawHistoryFile[sizeof(drawData.drawHistoryFile) - 1] = '\0'; // Ensure null termination

    if (config.debugMode) {
        std::cout << "Combination file path set to: " << drawData.combinationCollectionFile << std::endl;
        std::cout << "Draw history file path set to: " << drawData.drawHistoryFile << std::endl;
    }

    drawData.init_all();

    // Load combinations from file or create them if loading fails
    FILE* combinationFile = fopen(drawData.combinationCollectionFile, "r");
    if (!combinationFile) {
        if (config.debugMode) {
            std::cerr << "Error opening combination file: " << drawData.combinationCollectionFile << " (" << strerror(errno) << ")" << std::endl;
            std::cerr << "Creating new combinations..." << std::endl;
        }
        drawData.create_all_combinations();
    } else {
        if (config.debugMode) {
            std::cout << "Combination file opened successfully: " << drawData.combinationCollectionFile << std::endl;
        }
        fclose(combinationFile);
    }

    // Run the draw engine
    drawData.analyse_all_draws();

    return 0;
}
