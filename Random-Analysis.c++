// Picks.cpp : Defines the entry point for the console application.
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
typedef char Card[DRAW_SIZE];
bool Debug=true;

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

class Picks
{
public:
	void DisplayDrawStatistics();
	void DisplayOrdinalLists();
	void Initialize();
	void SortOrdinalLists();
	void DrawEngine();
	void CreateCombinations();
	void CalculateOrdinalEvent(int,OrdinalNodes*&);
	void InitialStatisticalBase();
	bool CombinationHasPrimeNumber(Card);
	bool ValidateDrawCombination(Card);
	void Print();
	// bool LoadCombinationsList();
	void CalculateDrawEvent(DrawStatisticList*&);
	void Picker();
	void SortDrawNumbersAverage();
	void SortOrdinalAverage(OrdinalNodes*&);
	void FlagClear(OrdinalNodes*&);
	void InitOrdinalList(OrdinalList&);

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
    char DebugMode,PerformAnalysis,UseRootData;
	int DrawHistoryTotal;
	int total_picks;
	int TotalNumbersDrawn;
	int TotalValidCombinationCards;
	char DrawHistoryFile[50];
	char CombinationCollectionFile[50];
};

void Picks::Initialize()
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

	TotalValidCombinationCards = 0;

	//create the draw history list from file.

	OrdinalNodesStart = new OrdinalNodes;
	InitOrdinalList(OrdinalNodesStart->ListNode);
	OrdinalNodesStart->Previous = NULL;
	OrdinalNodesStart->Next = NULL;

	TotalNumbersDrawn = 0;

	
}
void Picks::DisplayDrawStatistics() 
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
void Picks::DisplayOrdinalLists() {
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
void Picks::SortDrawNumbersAverage()
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
void Picks::SortOrdinalAverage(OrdinalNodes*& Head)
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

void Picks::DrawEngine()
{
	DrawStatisticList *DrawNumber;
	DrawNumber = DrawNumbersStart;
	int NumberDrawn = 0;
	int DrawCardSlot = 0;
	int DrawListLocation = 0;
    char ball;
    int ballIndex;
    int ballsDrawn = 0;  

    string line;
    ifstream file(DrawHistoryFile);
    if (!file.is_open()) {
        cerr << "Error opening file: " << DrawHistoryFile << endl;
        return;
    }
    while (getline(file, line)) 
    {
        stringstream ss(line);
        DrawCardSlot = 0;
		while (ss >> ball)
		{
            LastDraw[DrawCardSlot] = ball;
            TotalNumbersDrawn++;
            DrawNumber = DrawNumbersStart;
            DrawListLocation = 0;
            while(DrawNumber != NULL)
            {
                if (!DrawNumber->IsDrawn)
                {
                    if(DrawNumber->DrawNumber == ball )
                    {
                        DrawNumber->IsDrawn = true;
						DrawNumber->Opportunities++;
                        CalculateDrawEvent(DrawNumber);
                        if(TotalNumbersDrawn > SeedPool)
                        {   
                            CalculateOrdinalEvent(DrawListLocation,OrdinalNodesStart);
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
        if(TotalNumbersDrawn > SeedPool)
        {
            SortDrawNumbersAverage();
            SortOrdinalLists();
            FlagClear(OrdinalNodesStart);
        }
	}
	file.close();
}

void Picks::FlagClear(OrdinalNodes*& Head)
{
	OrdinalNodes* Node = Head;
	while (Node != NULL)
	{
		Node->ListNode.IsDrawn = false;
		Node = Node->Next;
	}
}
void Picks::SortOrdinalLists()
{
	OrdinalNodes *Current = OrdinalNodesStart;
	
	while(Current != NULL)
	{
		SortOrdinalAverage(Current);
		FlagClear(Current);
		Current = Current->Next;
	}

}
void Picks::CalculateOrdinalEvent(int Postion, OrdinalNodes*& Node)
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
					InitOrdinalList(Node->ListNode);
					
				}
				CalculateOrdinalEvent(OrdinalListLocation, Node->Next);
				
			} else OrdinalListNode->Opportunities++;
		}
		OrdinalListNode = OrdinalListNode->Next;
		OrdinalListLocation++;
	}
}
void Picks::InitOrdinalList(OrdinalList& Head)
{
	OrdinalList* OrdinalListNode = &Head;
	
	int OrdinalSet = 0;
	do
	{
		if(Debug){
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
void Picks::CalculateDrawEvent(DrawStatisticList*& Number)
{
	Number->TotalTimesDrawn++;
	Number->Average = Number->TotalTimesDrawn / Number->Opportunities;
	Number->LastDrawn = TotalNumbersDrawn;
};
bool Picks::ValidateDrawCombination(Card PossibleCombinationCard)
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
	if (!CombinationHasPrimeNumber(PossibleCombinationCard))
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
bool Picks::CombinationHasPrimeNumber(Card num)
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
void Picks::CreateCombinations()
{
	//This function creates every draw combination and validates them for use in this program.

	FILE *CombinationOutputFile = fopen(CombinationCollectionFile, "wb");
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
                                if (ValidateDrawCombination(DrawCombination)) {
                                    TotalValidCombinations++;

                                    if (DebugMode) {
                                        cout << "Found combination " << TotalValidCombinations << ": ";
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

int main() {
    Picks picks;
    picks.Initialize();

    // Prompt for filename with default value
    strncpy(picks.CombinationCollectionFile, "./CombinationCollectionFile", sizeof(picks.CombinationCollectionFile) - 1);
    picks.CombinationCollectionFile[sizeof(picks.CombinationCollectionFile) - 1] = '\0'; // Ensure null termination

    cout << "Enter filename for combinations (default: ./CombinationCollectionFile): ";
    string userInput;
    cin.ignore();  // Clear the input buffer before getline
    getline(cin, userInput);

    if (!userInput.empty()) {
        strncpy(picks.CombinationCollectionFile, userInput.c_str(), sizeof(picks.CombinationCollectionFile) - 1);
        picks.CombinationCollectionFile[sizeof(picks.CombinationCollectionFile) - 1] = '\0'; // Ensure null termination
    }

    cout << "Enable debug mode? (y/n): ";
    cin >> userInput;
    picks.DebugMode = (userInput == "y");

    if (picks.DebugMode) {
        cout << "Perform statistical analysis? (y/n): ";
        cin >> userInput;
        picks.PerformAnalysis = (userInput == "y");

        cout << "Use root data? (y/n): ";
        cin >> userInput;
        picks.UseRootData = (userInput == "y");
    }

    // Load combinations from file or create them if loading fails
    FILE* combinationFile = fopen(picks.CombinationCollectionFile, "r");
    if (!combinationFile) {
        picks.CreateCombinations();
    }
	fclose(combinationFile);
     // Prompt for filename with default value
    strncpy(picks.DrawHistoryFile, "./extracted_draws649-10-31-2015.csv", sizeof(picks.DrawHistoryFile) - 1);
    picks.DrawHistoryFile[sizeof(picks.DrawHistoryFile) - 1] = '\0'; // Ensure null termination

    cout << "Enter filename for draw history (default: extracted_draws649-10-31-2015.csv): ";
    cin.ignore();  // Clear the input buffer before getline
    getline(cin, userInput);
    if (!userInput.empty()) {
        strncpy(picks.DrawHistoryFile, userInput.c_str(), sizeof(picks.DrawHistoryFile) - 1);
        picks.DrawHistoryFile[sizeof(picks.DrawHistoryFile) - 1] = '\0'; // Ensure null termination
    }


    // Run the draw engine
    picks.DrawEngine();

    return 0;
}

/*
void Picks::Picker()
{
	cout << "score count=" << score_count << '\n';
	cout << "diff count=" << DifferenceCount << '\n';
	cout << "sig_count=" << SigmaCount << '\n';
	cout << "avg count=" << AverageCount << '\n';
	int pos = 0;
	int i = 1;
	int y = 0;
	Card tmp;
	ValidCombinationList *tc;
	char ans = 'y';
	char pans = 'n';
	char show = 'n';
	bool valid = false;
	int set[49];
	for (int j = 0; j < 49; j++)
		set[j] = 0;
	cout << "first total combos=" << TotalValidCombinationCards + 1 << '\n';
	if (debug == 'y')
	{
		cout << "show set?\n";
		cin >> show;
	}
	tc = ValidCombinationsStart;
	while (tc != NULL)
	{
		for (int k = 0; k < 6; k++)
			NumberStatistics[((int)tc->ValidCombination[k]) - 1].rm++;
		tc = tc->Next;
	}
	// ReCull();
	if (show == 'y')
	{
		tc = ValidCombinationsStart;
		while (tc != NULL)
		{
			for (int k = 0; k < 6; k++)
				set[((int)tc->ValidCombination[k]) - 1]++;
			tc = tc->Next;
		}
		for (int q = 0; q < 49; q++)
		{
			if (set[q] != 0)
				cout << q + 1 << " hit " << set[q] << '\n';
		}
	}
	while (!valid)
	{
		cout << "total combos=" << TotalValidCombinationCards + 1 << '\n';
		cout << "enter the position\n";
		cin >> pos;
		pos--;
		if ((pos >= 0 && pos <= TotalValidCombinationCards))
			valid = true;
		else
			cout << "\nInvalid responce, enter the position\n";
	}
	valid = false;
	while (true)
	{
		tc = ValidCombinationsStart;
		while (i < pos)
		{
			tc = tc->Next;
			i++;
		}
		cout << "Random pick = ";
		y = 0;
		while (picks[y] != 99)
		{
			if (debug == 'y')
				cout << (int)picks[y] << " ";
			y++;
		}
		cout << "\n--> ";
		for (int j = 0; j < 6; j++)
		{
			cout << (int)tc->ValidCombination[j] << ' ';
			tmp[j] = tc->ValidCombination[j];
			picks[y] = tc->ValidCombination[j];
			y++;
		}
		cout << '\n';
		// cout<<"another?"<<'\n';
		i = 1;
		/*cin>>ans;
		if(ans!='y')
			break;
		if (Trim(tmp))
		{
			valid = false;
			while (!valid)
			{
				cout << "total combos=" << TotalValidCombinationCards + 1 << '\n';
				cout << "enter the position\n";
				cin >> pos;
				pos--;
				if ((pos >= 0 && pos <= TotalValidCombinationCards))
					valid = true;
				else
					cout << "\nInvalid responce, enter the position\n";
			}
		}
		else
			return;
	}
}*/

/*bool Picks::LoadCombinationsList() 
{
    FILE* combinationFile = fopen(CombinationCollectionFile, "r");
    if (!combinationFile) {
        cerr << "Error opening file: " << CombinationCollectionFile << endl;
        return false;
    }

    ValidCombinationList* currentCard = new ValidCombinationList;
	ValidCombinationsStart = currentCard;
    TotalValidCombinationCards = 0;
	int combinationCounter = 0;
    char buffer[DRAW_SIZE * 3]; // Buffer to hold a line of the file

    while (fgets(buffer, sizeof(buffer), combinationFile)) {
        // Parse the line into individual numbers
        if (sscanf(buffer, "%d %d %d %d %d %d %d", 
            &currentCard->ValidCombination[0], 
            &currentCard->ValidCombination[1], 
            &currentCard->ValidCombination[2], 
            &currentCard->ValidCombination[3], 
            &currentCard->ValidCombination[4], 
            &currentCard->ValidCombination[5],
            &currentCard->ValidCombination[6]) != DRAW_SIZE) {
            cerr << "Error parsing line: " << buffer << endl;
            continue;
        }
		combinationCounter++;
        if (DebugMode) {
            cout << "Combination #"<<combinationCounter<< ' '<<"Loaded as ->";
            for (int i = 0; i < DRAW_SIZE; ++i) {
                cout << currentCard->ValidCombination[i] << ' ';
            }
            cout << endl;
        }

        TotalValidCombinationCards++;

        currentCard->Next = new ValidCombinationList;
        currentCard = currentCard->Next;
        currentCard->Next = NULL;
    }

    // Clean up the last unused node
    ValidCombinationList* temp = ValidCombinationsStart;
    while (temp->Next->Next != NULL) {
        temp = temp->Next;
    }
    delete temp->Next;
    temp->Next = NULL;
    fclose(combinationFile);

    if (DebugMode) {
        cout << "Total valid combinations loaded: " << TotalValidCombinationCards << endl;
    }

    if (TotalValidCombinationCards == 0) {
        return false;
    } else {
        return true;
    }
}*/








