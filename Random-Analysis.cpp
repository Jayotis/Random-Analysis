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

// Define constants related to Lotto 649.
const int _drawRange = 49;		// The range of numbers that can be drawn (1-49).
const int _drawCardSize = 7;		// The number of numbers drawn in each draw (6 + 1 bonus).
const int _drawSampleSize = 500;		// A certain amount of draws that produce a somewhat stable(numbers don't move around wild) list. 
const int _ordinalSampleSize = 500;		// same as above but for the ordinal lists.
char _primeNumbers[15] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47};
typedef int Card[_drawCardSize];

// Define a struct to hold statistics for each number.
struct DrawNumberStatistics{

	int totalTimesDrawn;		// How many times this number has been drawn.
	int drawNumber;				// The number itself.
	bool isDrawn;				// Flag indicating if the number was drawn.
	double ordinalChance;		// The summation of all the ordinal averages that point to the postion this number is in on the draw list.
	int drawOpportunities; 			// Each attempt to draw this number from the avaliable balls. 
	double average;				// the average as times drawn over total opportunities.
	int lastDrawn;				// The amount of draws that have past since it was last drawn.
	DrawNumberStatistics *_next;
};

struct ordinalListNode {
// Struct to represent the data for a specific ordinal position in a draw probability list.
// This list contains 49 elements, each element referencing a rank (ordinal) in another list
// of 49 elements that are sorted by probability. The referenced list could be a list of draw
// numbers or another list of ordinal positions (ordinal list).
    
    int landedTotal;     // The total number of times a number has landed in this specific ordinal position.
    int ordinal;         // The rank or position in the referenced list (another 49-element list).
                         // This ordinal points to a specific rank in a list sorted by probability.
    int opportunities;   // The number of times this ordinal position had the chance to hold a drawn number.
                         // It counts the draw events where this position could have been selected.
    double ordinalChance;// The cumulative probability that this ordinal position will hold a drawn number.
                         // This is calculated by summing the average probabilities (ordinal averages) of
                         // all ordinal list elements that reference this element, plus this element's own average.
    double average;      // The probability that the drawn number was referenced by this ordinal.
                         // In other words, this is the probability that this ordinal position holds the drawn number.
    bool isDrawn;        // Flag indicating whether the number was actually drawn in this position during the current draw sequence.
    
    // Pointer to the next node in the linked list, representing the next ordinal position.
    ordinalListNode *_next;
// Detailed Explanation:
// - This struct is part of a list of 49 elements. Each element represents an ordinal position
//   in the draw probability list.
// - The 'ordinal' field references a specific rank in another 49-element list, which is sorted
//   by probability. This referenced list could either be the list of draw numbers or another ordinal list.
// - The 'ordinalChance' field represents the cumulative probability that this ordinal position will hold a drawn number.
//   It is calculated by summing the average probabilities (ordinal averages) of all ordinal list elements
//   that reference this element, plus this element's own average. This means that each ordinal list is interconnected
//   with others through the 'ordinalBranch' structure, where each element is referenced by an element in the _next listNode.
// - The 'average' field indicates the probability that the drawn number is referenced by this ordinal.
//   Essentially, it's the probability that this specific position in the list will contain the drawn number.
// - The 'opportunities' field counts how many times this ordinal position had the opportunity to hold a drawn number.
//   This provides a more detailed view of the draw events, treating each draw as a separate set of 7 random events instead of 1.
};
struct ordinalBranch{
// Struct to manage the interconnection between ordinal lists.
// Each ordinal list can reference elements in other lists through the _next listNode,
// and these references are managed by the 'ordinalBranch' structure. This structure
// is part of a double-linked list and tracks the number of recorded events. When the number of
// events (sampleSize) reaches a certain threshold (_ordinalSampleSize), a new ordinal list can be instantiated.


    ordinalListNode *listNode;  // Pointer to the head node of the ordinal list.
    int sampleSize;             // Number of times this List recorded an event (draw events or opportunities).
    ordinalBranch *_next;       // Pointer to the next branch in the double-linked list.
    ordinalBranch *_previous;   // Pointer to the previous branch in the double-linked list.
// Detailed Explanation:
// - The 'ordinalBranch' struct manages the connections between different ordinal lists and tracks the 
//   number of recorded events within each list.
// - 'listNode' points to the head of the current ordinal list, which is a list of 49 ordinalListNode elements.
// - 'sampleSize' keeps track of the number of events (e.g., draws or opportunities) recorded in the current list.
//   When this sample size reaches a predetermined threshold (_ordinalSampleSize), the system can instantiate a new ordinal list,
//   effectively creating a new branch in the structure to continue tracking events without losing historical data.
// - '_next' and '_previous' create a double-linked list structure, allowing traversal both forward and backward 
//   through the chain of ordinal branches. This double-linking facilitates efficient navigation and management of 
//   multiple interconnected lists, supporting complex probability analysis.
//
// - When '_previous' is null, it indicates that we are at the first list, which references the draw number list directly.
// - When '_next' is null, it indicates that we are at the last list in the chain, where the ordinals reference the _previous listNode.
//   This position is significant because it marks the start of the summation process for calculating the cumulative probability
//   (ordinalChance) across all interconnected lists. From this last list, the cumulative probability is computed by summing
//   the relevant averages and propagating this information back through the chain of ordinal branches.
};

// Struct to manage the configuration settings for the analysis program.
// This struct holds file paths for important data files and a flag for enabling or disabling debug mode.
struct Config {
    string combinationCollectionFile;  // Path to the file that stores the collection of combinations.
                                       // This file contains precomputed or collected combinations used in the analysis.
    string drawHistoryFile;            // Path to the file that stores the history of draws.
                                       // This CSV file contains the historical draw data in a specific order (e.g., new_draw_order.csv).
    bool debugMode;                    // Flag to enable or disable debug mode.
                                       // When set to true, additional debug information will be logged or displayed.

    // Constructor to initialize the configuration with default values.
    // - combinationCollectionFile is initialized to "./combinationCollectionFile.dat"
    // - drawHistoryFile is initialized to "./new_draw_order.csv"
    // - debugMode is initialized to false (debug mode off by default)
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
	void calculate_draw_event(DrawNumberStatistics*&);
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
	std::vector<DrawNumberStatistics*> _drawStatistics;

	DrawNumberStatistics *_drawNumbersStart;
	ordinalBranch *_ordinalBranchStart;
	Card _lastDraw;
	ValidCombinationList *_validCombinationsStart;
	int _drawHistoryTotal;
	int _totalPicks;
	int _totalDrawEvents;
	int _totalValidCombinationCards;
	int _ordinalBranchTotalNodes;
	char _drawHistoryFile[50];
	char _combinationCollectionFile[50];
	bool _debugMode;
};


void Analyse::init_all() {
// Function to initialize all necessary data structures and settings for the analysis.

    // Initialize the linked list for draw number statistics.
    _drawNumbersStart = new DrawNumberStatistics;
    if (!_drawNumbersStart) {
        cerr << "[Error] Failed to allocate memory for _drawNumbersStart." << endl;
        return; // Consider more graceful error handling in production code.
    }

    DrawNumberStatistics *currentDrawNumber = _drawNumbersStart;
    int ballValue = 0;

    // Loop to initialize each draw number's statistics.
    while (ballValue < _drawRange) {
        // Initialize the current node's data.
        currentDrawNumber->totalTimesDrawn = 0;
        currentDrawNumber->drawNumber = ballValue + 1; // Assign draw number.
        currentDrawNumber->isDrawn = false;
        currentDrawNumber->drawOpportunities = 0;
        currentDrawNumber->average = 0.0;
        currentDrawNumber->lastDrawn = 0;
        currentDrawNumber->ordinalChance = 0.0;

        ballValue++;

        // Check if we've reached the last draw number.
        if (ballValue == _drawRange) {
            currentDrawNumber->_next = nullptr; // End the list.
            break;
        }

        // Allocate memory for the next node in the list.
        currentDrawNumber->_next = new DrawNumberStatistics;
        if (!currentDrawNumber->_next) {
            cerr << "[Error] Failed to allocate memory for the next DrawNumberStatistics node." << endl;
            currentDrawNumber->_next = nullptr;
            break; // Exit the loop if memory allocation fails.
        }

        // Move to the next node.
        currentDrawNumber = currentDrawNumber->_next;
    }

    // Initialize other necessary members.
    _totalValidCombinationCards = 0; // Initialize the count of valid combination cards.

    // Initialize the ordinal branch structure.
    _ordinalBranchStart = new ordinalBranch;
    // Set initial values for the ordinal branch.
    _ordinalBranchStart->sampleSize = 0;
    _ordinalBranchStart->_previous = nullptr;
    _ordinalBranchStart->_next = nullptr;
	
    _ordinalBranchStart->listNode = new ordinalListNode;
    // Initialize the first ordinal list.
    initialize_ordinal_list(_ordinalBranchStart->listNode);

    // Initialize other relevant counters and flags.
    _totalDrawEvents = 0;
    _ordinalBranchTotalNodes = 1;
    _debugMode = true;
}

void Analyse::display_draw_statistics() 
{
/* Function to display the draw statistics for each number.
 This function iterates through the linked list of draw statistics, 
 starting from _drawNumbersStart, and prints out the relevant information 
 for each draw number.*/

    // Start with the first draw number in the linked list.
	DrawNumberStatistics* currentDraw;
	currentDraw = _drawNumbersStart;

    // Print the header for the statistics display.
    std::cerr << "Draw Statistics (sorted by average):" << std::endl;

    // Iterate through the linked list of draw statistics.
	while(currentDraw != nullptr)
	{
        // Output the statistics for the current draw number.
        std::cerr << "Draw Number: " << currentDraw->drawNumber          // The draw number being reported.
                  << " Total Drawn: " << currentDraw->totalTimesDrawn    // The total number of times this number has been drawn.
                  << " Opportunities: " << currentDraw->drawOpportunities// The number of opportunities this number had to be drawn.
                  << " Average: " << currentDraw->average                // The average position of this number in all draws.
				  << " Ordinal Chance: " << currentDraw->ordinalChance    // The calculated chance of this number being drawn in its ordinal position.
                  << " Last Drawn: " << currentDraw->lastDrawn << std::endl; // The last draw number in which this number was drawn.

        // Move to the next draw number in the linked list.
		currentDraw = currentDraw->_next;
	}
}

void Analyse::display_ordinal_lists() {
// Function to display the ordinal lists for each branch in the ordinal branch structure.
// This function iterates through the linked list of ordinal branches, starting from _ordinalBranchStart,
// and prints out the relevant information for each ordinal list at each level.

    // Start with the first ordinal branch in the linked list.
    ordinalBranch* currentBranch;
    currentBranch = _ordinalBranchStart;
    int OrdinalLevel = 1; // Track the rank of the ordinal branch list (starting from 1).

    // Iterate through the linked list of ordinal branches.
    while(currentBranch != NULL)
    {
        // Print the level and sample size for the current ordinal branch.
        std::cerr << "Ordinal Level " << OrdinalLevel << " sample size: " << currentBranch->sampleSize << " List:" << std::endl;
        
        // Start with the first ordinal list node in the current branch.
        ordinalListNode* currentOrdinal = currentBranch->listNode;

        // Iterate through the linked list of ordinal list nodes in the current branch.
        while (currentOrdinal != nullptr) {
            // Output the statistics for the current ordinal.
            std::cerr << "  Ordinal: " << currentOrdinal->ordinal          // The ordinal position being reported.
                    << " Average: " << currentOrdinal->average            // The average probability for this ordinal position.
                    << " Landed Total: " << currentOrdinal->landedTotal   // The total number of times a number has landed in this ordinal position.
                    << " Opportunities: " << currentOrdinal->opportunities << std::endl; // The number of opportunities this ordinal position had.

            // Move to the next ordinal list node in the current branch.
            currentOrdinal = currentOrdinal->_next;
        }

        std::cerr << std::endl; // Print a newline for readability between levels.

        // Move to the next ordinal branch in the linked list.
        currentBranch = currentBranch->_next;
        OrdinalLevel++; // Increment the ordinal level counter.
    }
}


void Analyse::sort_draws_average() {
/* Function to sort the linked list of draw statistics based on the 'average' field.
   This function uses a modified bubble sort algorithm to arrange the draw numbers
   in ascending order of their average draw position. */

    bool swapped; // Flag to track if any swaps were made during the iteration.
    DrawNumberStatistics *ptr1; // Pointer used to traverse the list.
    DrawNumberStatistics *lptr = nullptr; // Pointer to mark the end of the unsorted portion of the list.

    // Repeat the sorting process until no swaps are made (i.e., the list is sorted).
    do {
        swapped = false;
        ptr1 = _drawNumbersStart; // Start from the beginning of the list.

        // Traverse the list until the last sorted element (lptr).
        while (ptr1->_next != lptr) {
            // Compare the average of the current node with the next node.
            if (ptr1->average > ptr1->_next->average) {
                // If the current node's average is greater, swap the entire contents of the nodes.
                std::swap(ptr1->totalTimesDrawn, ptr1->_next->totalTimesDrawn);
                std::swap(ptr1->drawNumber, ptr1->_next->drawNumber);
                std::swap(ptr1->isDrawn, ptr1->_next->isDrawn);
                std::swap(ptr1->drawOpportunities, ptr1->_next->drawOpportunities);
                std::swap(ptr1->average, ptr1->_next->average);
                std::swap(ptr1->lastDrawn, ptr1->_next->lastDrawn);
				std::swap(ptr1->ordinalChance, ptr1->_next->ordinalChance);
                
                swapped = true; // Indicate that a swap was made.
            }
            ptr1 = ptr1->_next; // Move to the next node in the list.
        }

        // Mark the end of the sorted portion of the list.
        lptr = ptr1;

    // Continue the process as long as swaps are being made.
    } while (swapped);
}

void Analyse::sort_ordinal_average(ordinalBranch*& Head) {
/* Function to sort the ordinal list within a given ordinal branch based on the 'average' field.
This function uses a modified bubble sort algorithm to arrange the ordinal list nodes
in ascending order of their average probability values. */

    // If the head of the ordinal branch is null, there is nothing to sort.
    if (Head == nullptr) return;

    bool swapped; // Flag to track if any swaps were made during the iteration.
    ordinalListNode *ptr1; // Pointer used to traverse the list.
    ordinalListNode *lptr = nullptr; // Pointer to mark the end of the unsorted portion of the list.

    // Repeat the sorting process until no swaps are made (i.e., the list is sorted).
    do {
        swapped = false;
        ptr1 = Head->listNode; // Start from the beginning of the ordinal list.

        // Traverse the list until the last sorted element (lptr).
        while (ptr1->_next != lptr) {
            // Compare the average of the current node with the next node.
            if (ptr1->average > ptr1->_next->average) {
                // If the current node's average is greater, swap the entire contents of the nodes.
                std::swap(ptr1->landedTotal, ptr1->_next->landedTotal);
                std::swap(ptr1->ordinal, ptr1->_next->ordinal);
                std::swap(ptr1->isDrawn, ptr1->_next->isDrawn);
                std::swap(ptr1->opportunities, ptr1->_next->opportunities);
                std::swap(ptr1->average, ptr1->_next->average);
                std::swap(ptr1->ordinalChance, ptr1->_next->ordinalChance); // Swap ordinalChance as well.
                
                swapped = true; // Indicate that a swap was made.
            }
            ptr1 = ptr1->_next; // Move to the next node in the list.
        }

        // Mark the end of the sorted portion of the list.
        lptr = ptr1;

    // Continue the process as long as swaps are being made.
    } while (swapped);
}


void Analyse::record_ordinal_opportunity(int ordinance, ordinalBranch*& Node)
{
/* Function to record an opportunity for a specific ordinal position (rank) in the linked ordinal branches.
This is a recursive function that takes a rank position (ordinance) of a sorted list and increments the 
opportunities count for that position if it could have been drawn but wasn't. 
The function then propagates this opportunity recording through all linked ordinal branches, 
ensuring that every relevant ordinal list node is updated across all levels.

This function is called from within analyse_all_draws().*/

    // Initialize pointers to traverse the current branch and its ordinal list.
    ordinalBranch* currentBranch = Node;          // Start with the provided branch (Node).
    ordinalListNode* currentListNode = currentBranch->listNode;  // Start with the head of the ordinal list.
    int listOrdinance = 1;  // This counter tracks the position within the current list.

    // Traverse through the ordinal list nodes in the current branch.
    while(currentListNode != nullptr)
    {
        // Check if the current node's ordinal matches the target ordinance.
        if(currentListNode->ordinal == ordinance)
        {
            // This ordinal position corresponds to the one that could have been drawn but wasn't.
            // Increment the opportunities count for this ordinal position.
            currentListNode->opportunities++;

            // Update the average probability for this ordinal position.
            // The average is recalculated as the ratio of the times this ordinal has landed
            // to the number of opportunities it has had.
            currentListNode->average = static_cast<double>(currentListNode->landedTotal) 
                                      / static_cast<double>(currentListNode->opportunities);

            // If there is a subsequent ordinal branch in the linked list (_next is not nullptr),
            // recursively call this function to propagate the opportunity recording.
            if(Node->_next != nullptr){
                record_ordinal_opportunity(listOrdinance, Node->_next);
            }

            // Exit the recursion once the opportunity is recorded and propagated.
            return;
        }
        else 
        {
            // Move to the next ordinal list node in the current branch.
            currentListNode = currentListNode->_next;
            listOrdinance++; // Increment the list ordinance counter to reflect the position.
        }
    }
}



void Analyse::analyse_all_draws()
{
/* Function to analyze all draw events from a historical draw file.
This function processes each draw in the file, updates the draw statistics,
records ordinal opportunities, and sorts the draw and ordinal lists as needed.
TODO: Refactor this function into smaller functions for better clarity and maintainability.
Suggested function breakdown:
1. A function to read and process the draw history file line by line.
2. A function to process each individual draw (ball number by ball number).
3. A function to record opportunities for unmatched numbers.
4. A function to update statistics and calculate probabilities for matched numbers.
5. A function to reset flags and sort lists after processing each draw.*/

    DrawNumberStatistics *listNumber;  // Pointer to traverse the linked list of draw statistics.
    listNumber = _drawNumbersStart;    // Start from the head of the draw numbers list.
    int drawCardSlot = 0;              // Counter for the position within the current draw.
    int drawListLocation = 0;          // Location in the draw statistics list.
    int ballNumber = 0;                // The current ball number being processed.
    int ballsDrawn = 0;                // Count of balls drawn in a draw (not used in this snippet).
    string drawDate = "";              // Variable to store the date of the draw.
    string line;                       // String to hold each line read from the file.

    // Open the draw history file with error checking.
    ifstream file(_drawHistoryFile);
    if (!file.is_open()) {
        cerr << "[Error] Failed to open file: " << _drawHistoryFile << endl;
        return;
    }
    if (_debugMode)
        cerr << "[Debug] Successfully opened file: " << _drawHistoryFile << endl;

    // Skip the header line of the CSV file if present.
    if (getline(file, line)) {
        if (_debugMode)
            cerr << "[Debug] Skipping header line: " << line << endl;
    }

    // TODO: Consider refactoring this loop into a separate function: process_draw_line()
    // Read and process each line (draw event) from the file.
    while (getline(file, line)) 
    {
        if (line.empty()) {
            cerr << "[Warning] Skipping empty line in file." << endl;
            continue;
        }
        stringstream ss(line); // Use stringstream to parse the line.

        drawCardSlot = 0; // Reset the slot counter for each draw.

        // Read the draw date.
        if (!getline(ss, drawDate, ',')) {
            cerr << "[Error] Failed to read draw date from line: " << line << endl;
            continue;
        }
        
        // TODO: Consider refactoring this loop into a separate function: process_draw_number()
        // Process each ball number in the current draw.
        while (getline(ss, line, ',')) // Parse each ball number in the draw.
        {
            try {
                ballNumber = stoi(line); // Convert the string to an integer.
            } catch (const invalid_argument& e) {
                cerr << "[Error] Invalid ball number in line: " << line << endl;
                continue;
            }

            // Check if we exceed the expected number of slots for a draw.
            if (drawCardSlot >= _drawCardSize) {  // Assuming _drawCardSize is defined elsewhere.
                cerr << "[Error] Exceeded maximum draw slots for date: " << drawDate << endl;
                break;
            }

            // Record the ball number in the appropriate slot.
            _lastDraw[drawCardSlot] = ballNumber;
            if(_debugMode)
                cerr << "[Debug] Ball " << ballNumber << " drawn in slot " << drawCardSlot << endl;

            // Reset list traversal variables.
            listNumber = _drawNumbersStart; // Start from the beginning of the draw statistics list.
            drawListLocation = 1; // Location counter starts at 1.

            // TODO: Consider refactoring this loop into two separate functions:
            // 1. update_matched_number_statistics() - for when the number matches.
            // 2. record_opportunity_for_unmatched() - for when the number doesn't match.
            // Traverse the draw statistics list to update each draw number's stats.
            while(listNumber != NULL)
            {
                // If this number hasn't been drawn yet, process it.
                if (!listNumber->isDrawn)
                {
                    // Check if the current ball matches the draw number in the list.
                    if (listNumber->drawNumber == ballNumber)
                    {
                        _totalDrawEvents++; // Increment total draw events counter.
                        
                        // Perform draw event calculations for the matched number.
                        calculate_draw_event(listNumber);
                        
                        // If enough draws have occurred, calculate ordinal events.
                        if (_totalDrawEvents > _drawSampleSize)
                            calculate_ordinal_event(drawListLocation, _ordinalBranchStart);
                        
                        if(_debugMode)
                            cerr << "[Debug] Ball " << ballNumber << " matches DrawNumber at location " << drawListLocation << endl;
                    } 
                    else 
                    {
                        // For unmatched numbers, increment their opportunities.
                        listNumber->drawOpportunities++;
                        
                        // Record ordinal opportunities for unmatched numbers.
                        if (_totalDrawEvents > _drawSampleSize) {
                            record_ordinal_opportunity(drawListLocation, _ordinalBranchStart);
                        }
                    }
                }
                // Move to the next draw number in the list.
                listNumber = listNumber->_next;
                drawListLocation++;
            } 
            drawCardSlot++; // Move to the next slot in the draw.
        }
        
        // TODO: Refactor into a function to reset flags and sort the lists: reset_and_sort_after_draw()
        // Reset the isDrawn flag for all draw numbers for the next draw.
        listNumber = _drawNumbersStart;
        while (listNumber != NULL)
        {
            listNumber->isDrawn = false; // Reset the flag.
            listNumber = listNumber->_next;
        }

        // Sort the draw statistics list based on the updated averages.
        sort_draws_average();

        // If the total number of draw events exceeds the sample size, 
        // sort the ordinal lists and perform any necessary operations.
        if (_totalDrawEvents > _drawSampleSize){
            sort_ordinal_lists();
        }
    }

    // Close the file after processing all draws.
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
		DrawNumberStatistics* drawList;
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
			Node->sampleSize++;
			if(Node->_next != nullptr){
				calculate_ordinal_event(OrdinalListLocation, Node->_next);
			}
			else if (Node->sampleSize > _ordinalSampleSize)
			{
				_ordinalBranchTotalNodes++;
				Node->_next = new ordinalBranch;
				Node->_next->sampleSize = 0;
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

    // Shuffle the vector to get a random permutation. (This is all not needed, to change)
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
void Analyse::calculate_draw_event(DrawNumberStatistics*& Number)
{
	Number->totalTimesDrawn++;
	Number->drawOpportunities++;
	Number->average = static_cast<double>(Number->totalTimesDrawn) / static_cast<double>(Number->drawOpportunities);
	Number->lastDrawn = _totalDrawEvents;
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
