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
/* Struct to represent the data for a specific ordinal position in a draw probability list.
This list contains 49 elements, each element referencing a rank (ordinal) in another list
of 49 elements that are sorted by probability. The referenced list could be a list of draw
numbers or another list of ordinal positions (ordinal list).*/
    
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
/* end of struct

Detailed Explanation:
- This struct is part of a list of 49 elements. Each element represents an ordinal position
  in the draw probability list.
- The 'ordinal' field references a specific rank in another 49-element list, which is sorted
  by probability. This referenced list could either be the list of draw numbers or another ordinal list.
- The 'ordinalChance' field represents the cumulative probability that this ordinal position will hold a drawn number.
  It is calculated by summing the average probabilities (ordinal averages) of all ordinal list elements
  that reference this element, plus this element's own average. This means that each ordinal list is interconnected
  with others through the 'ordinalBranch' structure, where each element is referenced by an element in the _next listNode.
- The 'average' field indicates the probability that the drawn number is referenced by this ordinal.
  Essentially, it's the probability that this specific position in the list will contain the drawn number.
- The 'opportunities' field counts how many times this ordinal position had the opportunity to hold a drawn number.
  This provides a more detailed view of the draw events, treating each draw as a separate set of 7 random events instead of 1. */
};

struct ordinalBranch{
/* Struct to manage the interconnection between ordinal lists.
Each ordinal list can reference elements in other lists through the _next listNode,
and these references are managed by the 'ordinalBranch' structure. This structure
is part of a double-linked list and tracks the number of recorded events. When the number of
events (sampleSize) reaches a certain threshold (_ordinalSampleSize), a new ordinal list can be instantiated.*/


    ordinalListNode *listNode;  // Pointer to the head node of the ordinal list.
    int sampleSize;             // Number of times this List recorded an event (draw events or opportunities).
    ordinalBranch *_next;       // Pointer to the next branch in the double-linked list.
    ordinalBranch *_previous;   // Pointer to the previous branch in the double-linked list.
/*	end of struct

Detailed Explanation:
- The 'ordinalBranch' struct manages the connections between different ordinal lists and tracks the 
  number of recorded events within each list.
- 'listNode' points to the head of the current ordinal list, which is a list of 49 ordinalListNode elements.
- 'sampleSize' keeps track of the number of events (e.g., draws or opportunities) recorded in the current list.
  When this sample size reaches a predetermined threshold (_ordinalSampleSize), the system can instantiate a new ordinal list,
  effectively creating a new branch in the structure to continue tracking events without losing historical data.
- '_next' and '_previous' create a double-linked list structure, allowing traversal both forward and backward 
  through the chain of ordinal branches. This double-linking facilitates efficient navigation and management of 
  multiple interconnected lists, supporting complex probability analysis.
- When '_previous' is null, it indicates that we are at the first list, which references the draw number list directly.
- When '_next' is null, it indicates that we are at the last list in the chain, where the ordinals reference the _previous listNode.
  This position is significant because it marks the start of the summation process for calculating the cumulative probability
  (ordinalChance) across all interconnected lists. From this last list, the cumulative probability is computed by summing
  the relevant averages and propagating this information back through the chain of ordinal branches. */
};

/*	TODO: Future updates to the `ordinalListNode` and `ordinalBranch` structs:
- Consider introducing a child struct to encapsulate all calculated statistics (average, sigma, standard deviation, etc.).
- This child struct will help organize the data and make it easier to extend the functionality of statistical propagation.
- Each statistical metric (e.g., average, sigma, sd) should be updated consistently across the linked list of ordinal branches.
- The `propagate_statistical_tree` function will need to be expanded to handle these additional metrics.*/

struct Config {
/* Struct to manage the configuration settings for the analysis program.
This struct holds file paths for important data files and a flag for enabling or disabling debug mode.*/

    string combinationCollectionFile;  // Path to the file that stores the collection of combinations.
                                       // This file contains precomputed or collected combinations used in the analysis.
    string drawHistoryFile;            // Path to the file that stores the history of draws.
                                       // This CSV file contains the historical draw data in a specific order (e.g., new_draw_order.csv).
    bool debugMode;                    // Flag to enable or disable debug mode.
                                       // When set to true, additional debug information will be logged or displayed.

    /* Constructor to initialize the configuration with default values.
    - combinationCollectionFile is initialized to "./combinationCollectionFile.dat"
    - drawHistoryFile is initialized to "./new_draw_order.csv"
    - debugMode is initialized to false (debug mode off by default)*/
    Config() : combinationCollectionFile("./combinationCollectionFile.dat"),
               drawHistoryFile("./new_draw_order.csv"),
               debugMode(false) {}
};

class Analyse
{
public:
    // Displays statistics for all drawn numbers, including total times drawn, opportunities, and averages.
    void display_draw_statistics();

    // Displays the ordinal lists across all ordinal branches, showing each ordinal's position, average, and other statistics.
    void display_ordinal_lists();

    // Initializes all necessary data structures and settings for the analysis.
    // This includes setting up draw statistics lists, ordinal branches, and initial configurations.
    void init_all();

    // Sorts all ordinal lists within the linked ordinalBranch structure by average or other statistical metrics.
    void sort_ordinal_lists();

    // Analyzes all draw events from a historical draw file, updating statistics, recording opportunities, and sorting lists as needed.
    void analyse_all_draws();

    // Calculates and records the statistical event for a specific ordinal position within the ordinal branches.
    // This function propagates updates through the branches and creates new branches if necessary.
    void calculate_ordinal_event(int ordinance, ordinalBranch*&);

    // Records an opportunity for a specific ordinal position within the ordinal branches, 
    // particularly when the number wasn't drawn but could have been.
    void record_ordinal_opportunity(int ordinance, ordinalBranch*&);

    // Correlates data across ordinal branches, starting from the last branch and propagating statistical calculations (like averages) backwards.
    // Future versions may include more statistical metrics such as sigma and standard deviation.
    void correlate_data();

    // Calculates and records a draw event for a specific number, updating its statistics such as total times drawn and average.
    // Designed to be extendable for additional calculations in the future.
    void calculate_draw_event(DrawNumberStatistics*&);

    // Propagates statistical calculations through the ordinal branches, updating nodes and transferring final sums to the draw numbers.
    // Handles recursive propagation and ensures the correct statistical metrics are updated.
    void propagate_statistical_tree(int ordinance, double ordinalSum, ordinalBranch*&);

    // Sorts the linked list of draw statistics based on the average value of each draw number.
    // Uses a bubble sort algorithm and is designed to be easily extended to sort by other metrics.
    void sort_draws_average();

    // Sorts the ordinal list within a given ordinal branch by average or other statistical metrics.
    // Currently sorts by average but can be extended to include other criteria.
    void sort_ordinal_average(ordinalBranch*&);

    // Initializes a linked list of ordinalListNode elements in sequential order, setting up the ordinal positions and default statistics.
    // Memory allocation is checked, and the list is terminated properly.
    void initialize_ordinal_list(ordinalListNode*&);

    // Loads the configuration from a file into the provided Config object.
    // Returns true if the configuration is successfully loaded, false otherwise.
    bool load_config(const string& configFilePath, Config&);


	bool validate_draw_combination(Card);
	bool prime_number_check(Card);
	void create_all_combinations();



	struct ValidCombinationList{

		char _validCombination[_drawCardSize];
		struct ValidCombinationList *_next;
	};
	std::vector<ValidCombinationList> _validCombination;

	
   // Pointer to the start of the linked list that holds statistics for each draw number.
    // This list keeps track of various statistics like total times drawn, opportunities, and averages.
    DrawNumberStatistics *_drawNumbersStart;

    // Pointer to the start of the linked list of ordinal branches.
    // Each branch contains a list of ordinal positions and related statistical data.
    ordinalBranch *_ordinalBranchStart;

    // Stores the last draw event in the form of a Card object, which may contain information about the drawn numbers.
    // This variable holds the most recent set of numbers drawn.
    Card _lastDraw;

    // Pointer to the start of the linked list that holds valid combinations.
    // This list contains valid combinations of numbers or other relevant data, typically used for analysis.
    ValidCombinationList *_validCombinationsStart;

    // Tracks the total number of draw events that have been processed.
    // This counter is incremented each time a new draw is processed and is used for various calculations.
    int _totalDrawEvents;

    // Tracks the total number of valid combination cards that have been processed.
    // This variable is incremented as valid combinations are identified and added to the list.
    int _totalValidCombinationCards;

    // Tracks the total number of ordinal branch nodes in the linked list.
    // This counter helps in managing and navigating the structure of ordinal branches.
    int _ordinalBranchTotalNodes;

    // Holds the file path for the draw history file, which contains past draw events.
    // This file is used to load historical draw data for analysis.
    char _drawHistoryFile[50];

    // Holds the file path for the combination collection file, which contains combinations to be analyzed.
    // This file is used to load a collection of combinations for validation and further analysis.
    char _combinationCollectionFile[50];

    // Boolean flag to enable or disable debug mode.
    // When set to true, additional debug information is output to help trace the program's execution.
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

void Analyse::display_draw_statistics() {
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

void Analyse::record_ordinal_opportunity(int ordinance, ordinalBranch*& Node){
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

void Analyse::analyse_all_draws(){
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

void Analyse::correlate_data(){
/* Function to correlate data across the ordinal branches, starting from the last branch and moving backward.
This function propagates statistical calculations (currently the average) from the last ordinal branch
back through the previous branches. Future expansions will likely include other statistical metrics
such as sigma, standard deviation, and others, requiring updates to the data structures.*/

    ordinalBranch* currentBranch;        // Pointer to traverse the ordinal branches.
    ordinalListNode* currentListNode;    // Pointer to traverse the ordinal list nodes within a branch.
    currentBranch = _ordinalBranchStart;

    // Traverse to the last ordinal branch in the linked list.
    while (currentBranch->_next != nullptr){
        currentBranch = currentBranch->_next;
    }

    // Start at the last branch's list and iterate through its ordinal list nodes.
    currentListNode = currentBranch->listNode;
    while (currentListNode != nullptr)
    {
        // Propagate the average from the current list node to the previous branches.
        // Currently, this only handles the average, but future versions will need to handle
        // other statistical calculations (e.g., sigma, standard deviation).
        propagate_statistical_tree(currentListNode->ordinal, currentListNode->average, currentBranch->_previous);

        // Move to the next node in the ordinal list.
        currentListNode = currentListNode->_next;
    }
}

void Analyse::propagate_statistical_tree(int ordinance, double ordinalSum, ordinalBranch*& branchNode){
/* Function to propagate statistical calculations (currently ordinal sum) through the ordinal branches.
This function works recursively, starting from a given branch node and moving backward through the linked list
of branches until it reaches the base branch node, which references the draw list sorted by averages.
At each level, the function updates the ordinal chance with the cumulative sum of averages.*/

    ordinalListNode* currentListNode;  // Pointer to traverse the ordinal list nodes within the current branch.
    currentListNode = branchNode->listNode;  // Start with the head of the ordinal list in the given branch.

    int listOrdinance = 1;  // Counter to track the position within the ordinal list.
    
    // Traverse to the ordinal list node that corresponds to the given ordinance.
    while (listOrdinance < ordinance){
        currentListNode = currentListNode->_next;  // Move to the next node.
        listOrdinance++;  // Increment the position counter.
    }

    // Update the ordinalChance of the current list node with the cumulative sum passed to the function.
    currentListNode->ordinalChance = ordinalSum;

    // Calculate the new cumulative sum by adding the current node's average to the sum passed down.
    double ordinalSummation = currentListNode->average + ordinalSum;
    
    // If there is a previous branch, recursively propagate the cumulative sum further back.
    if (branchNode->_previous != nullptr){
        propagate_statistical_tree(currentListNode->ordinal, ordinalSummation, branchNode->_previous);
    }
    // If we've reached the base branch node (no previous branch), transfer the final cumulative sum to the draw numbers.
    else {
        DrawNumberStatistics* drawList;  // Pointer to traverse the draw list.
        drawList = _drawNumbersStart;  // Start at the head of the draw list.

        // Traverse to the draw number that corresponds to the final ordinance position.
        listOrdinance = 1;
        while(listOrdinance < currentListNode->ordinal){
            drawList = drawList->_next;  // Move to the next draw number in the list.
            listOrdinance++;
        }

        // Update the ordinalChance of the draw number with the final cumulative sum.
        drawList->ordinalChance = ordinalSummation;
    }
/* end of function
Explanation of the Function:
1. The function begins at a specific branch node and uses the given ordinance (position) to locate the 
   corresponding ordinal list node.
2. Once the target node is found, the function updates its ordinalChance field with the cumulative sum
   passed down from previous levels.
3. The function then adds the current node's average to the cumulative sum and continues propagating this
   updated sum backward through the previous branches using a recursive call.
4. When the function reaches the base branch node (where there is no previous branch), it transfers the
   final cumulative sum to the corresponding draw number's ordinalChance field, effectively summarizing
   the statistical data through the entire structure.

5. The function is currently designed to handle the propagation of a single statistical metric (ordinal sum),
    but can be extended to include additional metrics (e.g., sigma, standard deviation) as needed.

6. This process is essential for calculating the overall likelihood of a draw number being drawn, based on
   its ordinal position in various sorted lists.*/
}

void Analyse::sort_ordinal_lists(){
/* Function to sort all ordinal lists within the linked ordinalBranch structure.
Currently, this function sorts each list by the average values, but it is designed 
with the intention to support sorting by other statistical metrics in the future.*/

    ordinalBranch *Current = _ordinalBranchStart;  // Start at the head of the ordinal branch list.
    
    // Traverse through each ordinal branch in the linked list.
    while(Current != NULL)
    {
        // Sort the current ordinal list by average values.
        // TODO: Extend the sorting mechanism to allow sorting by other statistical metrics.
        sort_ordinal_average(Current);

        // Move to the next ordinal branch in the list.
        Current = Current->_next;
    }
/* end of function
Explanation of the Function:
1. The function starts by pointing to the first ordinal branch in the linked list (indicated by `_ordinalBranchStart`).
2. It then iterates through each ordinal branch in the list, using a loop that continues until the end of the list is reached (`Current != NULL`).
3. For each ordinal branch, the function calls `sort_ordinal_average()`, which sorts the ordinal list nodes within that branch by their average values.
4. After sorting the current branch's list, the function advances to the next branch in the list by updating the `Current` pointer.
5. The function is designed to be easily extendable. Future versions could include additional sorting functions, such as:
   - `sort_ordinal_by_sigma(Current)`: Sort by the sigma (standard deviation) of the values.
   - `sort_ordinal_by_median(Current)`: Sort by the median of the values.
   - `sort_ordinal_by_custom_metric(Current, customMetricFunction)`: Sort by a custom metric defined by a function pointer or lambda expression.*/
}



void Analyse::calculate_ordinal_event(int ordinance, ordinalBranch*& Node){
/* Function to calculate and record the statistical event for a specific ordinal position (ordinance).
This function updates the statistical data (e.g., landed total, opportunities, average) for a drawn number 
within the ordinal lists. It propagates these updates through the hierarchy of ordinal branches.
If a branch's sample size exceeds a predefined threshold, a new ordinal branch is created.*/

    // Start at the head of the ordinal list in the current branch.
    ordinalListNode* currentListNode = Node->listNode;

    int OrdinalListLocation = 1; // Counter to track the location within the ordinal list.
    
    // Traverse the ordinal list to find the node corresponding to the given ordinance.
    while(currentListNode != nullptr)
    {
        if(currentListNode->ordinal == ordinance)
        {
            // Update the statistical data for the node where the ordinal matches.
            currentListNode->landedTotal++; // Increment the count of times this ordinal position has been landed on.
            currentListNode->opportunities++; // Increment the number of opportunities for this ordinal position.
            
            // Recalculate the average for this ordinal position.
            currentListNode->average = static_cast<double>(currentListNode->landedTotal) 
                                       / static_cast<double>(currentListNode->opportunities);
            
            // Increment the sample size for the current branch.
            Node->sampleSize++;

            // If there is a next branch in the sequence, recursively update the corresponding ordinal node.
            if(Node->_next != nullptr){
                calculate_ordinal_event(OrdinalListLocation, Node->_next);
            }
            // If there is no next branch and the sample size exceeds the threshold, create a new branch.
            else if (Node->sampleSize > _ordinalSampleSize)
            {
                _ordinalBranchTotalNodes++; // Track the total number of ordinal branches.
                
                // Create a new ordinal branch.
                Node->_next = new ordinalBranch;
                Node->_next->sampleSize = 0;
                Node->_next->_next = nullptr;
                Node->_next->_previous = Node;

                // Initialize the new ordinal list in the new branch.
                initialize_ordinal_list(Node->_next->listNode);

                // Recursively update the new branch with the current ordinal event.
                calculate_ordinal_event(OrdinalListLocation, Node->_next);
            }

            // Exit the function immediately, as there is no need to continue further.
            return;
        }

        // Move to the next node in the ordinal list.
        currentListNode = currentListNode->_next;
        OrdinalListLocation++; // Increment the location counter.
    }
/*	end of function
Explanation of the Function:
1. The function starts by traversing the ordinal list within the current branch to find the node that matches the given ordinance (position).
2. Once the matching node is found, the function updates its statistical data, including:
   - Incrementing the `landedTotal` (number of times the position has been landed on).
   - Incrementing the `opportunities` (number of chances the position had to be drawn).
   - Recalculating the `average`, which is the ratio of `landedTotal` to `opportunities`.
3. The function then increments the `sampleSize` for the current branch.
4. If the current branch has a subsequent branch in the sequence, the function recursively calls itself to update the corresponding node in the next branch.
5. If there is no next branch and the `sampleSize` exceeds the predefined threshold (`_ordinalSampleSize`), the function creates a new ordinal branch:
   - A new `ordinalBranch` is instantiated, linked to the current branch.
   - The new branch's ordinal list is initialized, and the function recursively updates the new branch with the current ordinal event.
6. This function ensures that the statistical data is propagated through all relevant branches, accurately reflecting the impact of the drawn number across the entire structure.
7. This setup is designed to handle dynamic growth in the number of branches as more draw events are processed, keeping the analysis structure scalable and adaptable.
TODO: Consider extending this function to handle additional statistical metrics (e.g., sigma, standard deviation, etc.) and update the comments accordingly.*/
}

void Analyse::initialize_ordinal_list(ordinalListNode*& Head){
/* Function to initialize a linked list of 49 ordinalListNode elements in sequential order.
Each node in the list represents an ordinal position and is initialized with default values. */

    // Step 1: Initialize the head node of the linked list.
    Head = new ordinalListNode;
    if (!Head) {
        cerr << "[Error] Memory allocation failed for the head node." << endl;
        return;
    }

    // Step 2: Pointer to traverse and build the list.
    ordinalListNode* currentList = Head;

    // Step 3: Initialize each node in the list with sequential ordinal values.
    for (int i = 1; i <= 49; ++i) {
        currentList->ordinal = i;             // Assign ordinal value sequentially from 1 to 49.
        currentList->average = 0.0;           // Initialize average to 0.0.
        currentList->isDrawn = false;         // Initialize isDrawn to false.
        currentList->opportunities = 0;       // Initialize opportunities to 0.
        currentList->ordinalChance = 0.0;     // Initialize ordinalChance to 0.0.
        currentList->landedTotal = 0;         // Initialize landedTotal to 0.

        // Step 4: Create the next node if we're not at the last element.
        if (i < 49) {
            currentList->_next = new ordinalListNode;
            if (!currentList->_next) {
                cerr << "[Error] Memory allocation failed at position " << i << endl;
                break;  // Stop the loop if memory allocation fails.
            }
            currentList = currentList->_next; // Move to the next node in the list.
        } else {
            // Step 5: Set the last node's _next pointer to nullptr to indicate the end of the list.
            currentList->_next = nullptr;
        }
    }
/* end of function
Explanation of the Function:
1. **Head Node Initialization:**
   - The function starts by creating and initializing the head node of the linked list.
   - Memory allocation is checked to ensure successful creation. If allocation fails, an error is reported and the function exits.
2. **Sequential Initialization:**
   - A loop runs from 1 to 49, initializing each `ordinalListNode` with a sequential ordinal value and default statistics.
   - The fields `average`, `isDrawn`, `opportunities`, `ordinalChance`, and `landedTotal` are all initialized to their default values.
3. **Linked List Construction:**
   - For each node, the function allocates memory for the next node in the list, linking them together sequentially.
   - If memory allocation fails at any point, the function outputs an error message and stops further processing.
4. **End of the List:**
   - The last node's `_next` pointer is set to `nullptr`, indicating the end of the linked list.
**Advantages of the Refactored Function:**
- **Simplicity:** The function is straightforward and easy to understand, as it initializes the list in a simple, sequential order without unnecessary complexity.
- **Clarity:** The removal of random generation simplifies the function's purpose, making it clear that the list is ordered sequentially.
- **Efficiency:** This approach avoids the overhead of shuffling, making the function more efficient for cases where a simple ordered list is sufficient.*/
}

void Analyse::calculate_draw_event(DrawNumberStatistics*& Number){
/* Function to record a draw event for a specific number.
This function updates key statistics for the number being drawn, such as the total times drawn,
draw opportunities, and average. It also marks the number as drawn and records when it was last drawn.
The function is designed to be extendable, allowing for additional calculations to be added in the future. */

    // Increment the total number of times this number has been drawn.
    Number->totalTimesDrawn++;

    // Increment the number of opportunities this number had to be drawn.
    Number->drawOpportunities++;

    // Recalculate the average based on the updated totals.
    // The average is the ratio of total times drawn to the number of opportunities.
    Number->average = static_cast<double>(Number->totalTimesDrawn) / static_cast<double>(Number->drawOpportunities);

    // Record the event number when this number was last drawn.
    // This value is set to the current total draw events in the system.
    Number->lastDrawn = _totalDrawEvents;

    // Mark the number as drawn in the current draw.
    Number->isDrawn = true;

/* TODO: Extend this function to include additional statistical calculations in the future.
Potential future calculations might include:
- Update sigma (standard deviation) based on the new draw event.
- Calculate and update any custom metrics that need to be tracked for each draw.
- Log or record additional metadata related to the draw event, such as draw date or position in the draw.*/
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
