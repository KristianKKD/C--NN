#include <neuralnetwork.hpp>
#include <library.hpp>
#include <map>
#include <unordered_set>
#include <algorithm>

string ReadFile(string path);
void ToLower(string& input);
void RemoveChar(string& text, char c);
string ReplaceAll(string input, string oldstring, string newstring);

using namespace std;
using nn = Neural::NeuralNetwork;

bool IsDelimiter(char c) {
    static const std::unordered_set<char> delimiters = {
        ' ', '\t', '\n', '\r', '\v', '\f',   // Whitespace characters
        ';', ',', '.', '!', '?', ':',        // Punctuation
        '"', '(', ')', '[', ']', '{', '}', // Brackets and quotes
        '_', '=', '+', '/', '\\',       // Arithmetic and separator characters
        '<', '>', '@', '#', '$', '%', '^', '&', '*', // Symbols
        '~', '`'  // Miscellaneous symbols
    };
    return delimiters.find(c) != delimiters.end();
}

map<string, int> IndexWords(string text) {
    map<string, int> wordHashmap;
    int mapSize = 0;
    text += " ";
    
    int lastStop = 0;
    for (int i = 0; i < text.size(); i++) {
        char c = text[i];

        if (IsDelimiter(c) || std::isdigit(c) || i == text.size()) {
            //get word
            string word = text.substr(lastStop, i-lastStop);
            lastStop = i + 1;
            
            if (word.size() == 0 || word.back() == '-' || word.front() == '-' || word.back() == '\'' || word.front() == '\'') //invalid word
                continue;
            //valid word

            //add to map
            auto pos = wordHashmap.find(word);
            if (pos == wordHashmap.end()) //doesn't exist in the map yet
                wordHashmap[word] = mapSize++; //just add the word
        }
    }

    return wordHashmap;
}

vector<string> GetNextNWords(string text, int pos, int n) {
    //we won't know if we are placed within a word
    //it's not worth going back to find the word so we need to find the next word after the next delimiter
    int lastStop = pos; 
    text += " ";

    vector<string> words;

    for (int i = pos; i < text.size(); i++) {
        char c = text[i];

        if (IsDelimiter(c) || std::isdigit(c) || i == text.size()) {
            //get word
            string word = text.substr(lastStop, i-lastStop);
            lastStop = i + 1;
            
            if (word.size() == 0 || word.back() == '-' || word.back() == '\'' || word.front() == '\'') //invalid word
                continue;
            //valid word

            words.push_back(word);
            if (n > 0 && words.size() >= n)
                break;
        }
    }

    return words;
}

#include <filesystem>

int main() {
    //read text, minor formatting
    string path = "C:\\Users\\KrabGor\\OneDrive\\Programming\\C  NN\\shakespeare.txt";
    string text = ReadFile(path);
    ToLower(text);
    text = ReplaceAll(text, "--", " ");

    if (text.size() <= 0)
        return 1;

    text = text;

    //find all words
    //convert them into an index (i.e. 1=the, 2=man)
    //replace the text with the indexed words (the,man = 1,2)
    map<string, int> indexedWords = IndexWords(text.substr(0, 5000)); ///////////////////////////////

    //convert the words to an easier to use format
    vector<string> words;
    for (const auto& pair : indexedWords)
        words.push_back(pair.first);

    int wordCount = words.size(); //0-1 in float is within this range
    Log("Found " + to_string(wordCount) + " unique words!");

    //ATTEMPT 2
    //setup training
    int batchSize = 3; //sentence size
    int epochCount = 100;
    
    //collect data for further processing
    vector<string> allDataWords = GetNextNWords(text, 0, -1); //get all words
    int datasetSize = allDataWords.size() - (allDataWords.size() % batchSize); //divisible by batchSize for batches
    Log("Dataset is " + to_string(datasetSize) + " words long!");

    //convert words to useable values for model
    vector<float> allDataConverted;
    for (int i = 0; i < datasetSize; i++)
        allDataConverted.push_back((float)indexedWords[allDataWords[i]]/wordCount);
   
   //setup batches for training
    float trainingPercentage = 0.8;
    int trainingBatchCount = std::round((datasetSize / batchSize) * trainingPercentage);
    
    //collect the data into batches
    Log("Creating batches...");
    vector<vector<float>> batches;
    for (int i = 0; i < datasetSize;) {
        vector<float> subset(allDataConverted.begin() + i, allDataConverted.begin() + i + batchSize);
        batches.push_back(subset);
        i += batchSize;
    }
    Log("Finished creating batches...");
    
    //shuffle the batches
    mt19937 g(Library::randomSeed);  // Mersenne Twister random number generator
    shuffle(batches.begin(), batches.end(), g);

    //split the batches into training and verification
    vector<vector<float>> trainingBatches;
    vector<vector<float>> verificationBatches;
    trainingBatches.assign(batches.begin(), batches.begin() + trainingBatchCount);
    verificationBatches.assign(batches.begin() + trainingBatchCount, batches.end());

    //build network
    int hiddenNodesPerLayer = 2;
    int hiddenLayers = 2;
    Neural::NeuralNetwork* net = new Neural::NeuralNetwork(batchSize, batchSize, hiddenLayers, hiddenNodesPerLayer);
    Log("Built network with " + to_string(hiddenLayers) + " layers!");

    net->PrintNetwork();
    return 0;

    float* inputsArr = new float[batchSize];
    float* outputsArr = new float[batchSize];

    //train
    for (int epoch = 0; epoch < epochCount; epoch++) {
        Log("Epoch: " + to_string(epoch));

        for (int batchIndex = 0; batchIndex < batches.size(); batchIndex++) {
            //Log("Batch: " + to_string(batchIndex));

            int randInputCount = std::max(1, (int)round(Library::RandomValue() * batchSize) - 1);
            for (int i = 0; i < batchSize; i++)
                inputsArr[i] = ((i < randInputCount) ? batches[batchIndex][i] : 0); //if index is above randomCount, insert empty 'word', otherwise insert word from batch

            net->FeedForward(inputsArr, outputsArr);
            //net->BackpropogateLearn(outputs, batchSize, targets, batchSize);
        }
    }
    
    //test the network manually
    string line = "";
    while (line != "exit") {
        Log("Enter input:");
        getline(cin, line);

        //collect inputs into a useable format
        vector<string> uiWords = GetNextNWords(line, 0, -1);
        for (int i = 0; i < min((int)uiWords.size(), (int)batchSize); i++) {
            Log(uiWords[i] + " - " + to_string((float)indexedWords[uiWords[i]]/wordCount));
            inputsArr[i] = (float)indexedWords[uiWords[i]] / wordCount;
        }

        net->FeedForward(inputsArr, outputsArr);

        for (int i = uiWords.size(); i < batchSize; i++) 
            std::cout << words[std::round(outputsArr[i] * wordCount)] << " ";
        std::cout << endl;
    }

    //free memory
    delete[] inputsArr;
    delete[] outputsArr;
    delete net;

    return 0;
}