#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <bitset>
#include <vector>
#include <iomanip>
#include <unordered_map>
#include <algorithm>
#include <cctype>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <bitset>
#include <vector>
#include <iomanip>
#include <unordered_map>

/*

 * Diego De La Toba, Ian Sanchez
 * Program 3
 * 11/20/2023
 * Purpose:  implement a disassembler for the SIC/XE instruction set architecture.
 * The disassembler should be able to open an object code file, parse the records
 * in the object code file, and output the corresponding assembly code to an assembly
 * listing file. The assembly listingfile format can be seen in the sample assembly
 * listing file out.lst. 

 */
using namespace std;

// Define a struct for Symbol
struct Symbols {
    string name;
    string address;
    string flags;
};

// Define a struct to store literal information
struct Literals {
    string name;
    string litConst;
    int length;
    string address;
};

class Tools {
public:
    int subtract_hex(string hex1, string hex2) {
        // Convert strings to hexadecimal integers
        unsigned int dec1, dec2;
        stringstream ss;
        ss << hex << hex1;
        ss >> dec1;
        ss.str(""); // Clear contents
        ss.clear(); // Clear state
        ss << hex << hex2;
        ss >> dec2;

        // Subtract and return the result
        return dec1 - dec2;
    }

    void toUppercase(string &s) {
        for (size_t i = 0; i < s.length(); ++i) {
            s[i] = toupper(static_cast<unsigned char>(s[i]));
        }
    }
    //Method that will take in and N and I and interpret their OAT
    string oatDetector(char n, char i){
        string mode;

        if (i == '0' && n == '1') {
            mode = "@";
        } else if (i == '1' && n == '0') {
            mode = "#";
        } else if (i == '1' && n == '1') {
            mode = "";
        } else {
            mode = "";
        }

        return mode;
    }
    //find/update the loc that will be used in the program
    static int locFinder(int loc, int format, const string& instruc) {
        //checks for hex values for loc
        if (format == 2) {
            loc += 2;
            return loc;
        } else if (format == 3) {
            loc += 3;
            return loc;
        } else if (format == 4) {
            loc += 4;
            return loc;
        }

        return loc;
    }
    //When we have a format 2 instruction clear we need the correct letter so this map will automatically map number to letter
    string mapOpcodeToRegister(string opcode) {
        char mnemonic = opcode[2];
        std::unordered_map<char, std::string> registerMap = {
                {'0', "A"},
                {'1', "X"},
                {'2', "L"},
                {'3', "B"},
                {'4', "S"},
                {'5', "T"},
                {'6', "F"},
                {'8', "PC"},
                {'9', "SW"}
        };
        for (auto it = registerMap.begin(); it != registerMap.end(); ++it) {
            if (it->first == mnemonic){
                return it->second;
            }
        }
        return 0;
    }
    //Method that will take in and B, P, and X and interpret their TAAM
    string taamDetector(char b, char p, char x){
        string mode;

        if (p == '0' && b == '1') {
            mode = "base";
            if(x == '1'){
                mode = "base_indexed";
            }
        } else if (p == '1' && b == '0') {
            mode = "pc";
            if(x == '1'){
                mode = "pc_indexed";
            }
        } else if (p == '0' && b == '0') {
            mode = "absolute";
        } else {
            mode = "Invalid";
        }


        return mode;
    }
    //Will calculate the Target Address given mode, displacement, PC, B
    string calculateTA(string mode, string dispStr, int pc , string b , int x, const vector<Symbols>& symbolTable, const vector<Literals>& literalTable, const vector<Literals>& literalOrgTable) {
        int TA;
        int disp = stoi(dispStr, nullptr, 16);
        int base = stoi(b, nullptr, 16);
        int xInt;

        char myChar = x; // Replace 'A' with the character you want
        int asciiValue = static_cast<int>(myChar);
        if (myChar == 48) {
            xInt = 0;
        } else {
            xInt = 1;
        }
        string xRep = to_string(asciiValue);


        if(mode == "pc") {
            TA = disp + pc;
        } else if(mode == "base") {
            TA = disp + base;
        } else if(mode == "pc_indexed") {
            TA = disp + pc;
        } else if(mode == "base_indexed") {
            TA = base + disp;
        } else {
            TA = disp;
        }

        stringstream ss;
        ss << hex << setw(4) << setfill('0') << TA;
        string wow = ss.str();

        for (char& c : wow) {
            if (c >= 'a' && c <= 'f') {
                c = toupper(c);
            }
        }

        //for pc-register addressing mode, we must prune off the first, since we are working with only 3 hex digits for our TA
        if (wow.substr(0, 1) == "1" && (mode == "pc" || mode == "pc_indexed")) {
            wow = "0" + wow.substr(1, 3);
        }

        for(int i = 0; i < symbolTable.size(); i++){
            if (wow == (symbolTable[i].address).substr(2,5) && symbolTable[i].name != "FIRST"){
                return symbolTable[i].name;
            }
        }
        for(int i = 0; i < literalOrgTable.size(); i++){
            if (wow == (literalOrgTable[i].address)){
                return literalOrgTable[i].litConst;
            }
        }

        for(int i = 0; i < literalTable.size(); i++){
            if (wow == (literalTable[i].address).substr(2,5)){
                //if((literalTable[i].litConst).substr(0,1) == "="){
                //    return literalTable[i].litConst;
                //}
                return literalTable[i].name;
            }
        }

        return wow.substr(3);
    }
    //Converets a decimal in string into its binary form, "numBits" is used to determine how many
    //bits will be outputed for example extractBits(4,3) outputs 100 and extractBits(4,2) outputs 00
    static std::string extractBits(const std::string& input, int numBits) {
        int decimalNumber;

        if (input.size() == 1) {
            decimalNumber = hexCharToDecimal(input[0]);
        } else {
            decimalNumber = std::stoi(input, nullptr, 16);
        }

        std::bitset<32> binaryRepresentation(decimalNumber);
        std::string binaryString = binaryRepresentation.to_string();

        if (numBits >= binaryString.size()) {
            return binaryString;
        } else {
            return binaryString.substr(binaryString.size() - numBits);
        }
    }


private:
    //converts char into hex decimal format
    static int hexCharToDecimal(char hexChar) {
        if (hexChar >= '0' && hexChar <= '9') {
            return hexChar - '0';
        } else if (hexChar >= 'A' && hexChar <= 'F') {
            return hexChar - 'A' + 10;
        } else if (hexChar >= 'a' && hexChar <= 'f') {
            return hexChar - 'a' + 10;
        }
        return -1; // Invalid character
    }
};


int main(int argc, char * argv[]) {
    //assign arg file name to variable
    //string inputFileName = argv[1];
    //open file
    ifstream inputObj(argv[1]);
    ifstream inputSym(argv[2]);
    //create outfile
    ofstream outFile("out.lst");
    //Intialize all 'tools'
    Tools tool;
    //Initialize all vectors
    vector<string> progStarts;
    vector<string> progEnds;
    vector<string> words;
    vector <Symbols> symbolTable;
    vector <Literals> literalTable;
    vector <Literals> literalOrgTable;
    //ints
    int wordsCount = 0;
    int mask = 0xFC;
    int number;
    int currentOp;
    int symbolTracker = 0;
    int litOrgTracker = 0;
    int currLocInt;
    //strings
    string word;
    string progName;
    string tempMnemonics;
    string progEnd;
    string nextLoc;
    //bools
    bool reached = false;
    //chars
    char n, i, x, b, p, e;
    const static string ops[] = {
            "18", "58", "90", "40", "B4", "28",
            "88", "A0", "24", "64", "9C", "C4",
            "C0", "F4", "3C", "30", "34", "38",
            "48", "00", "68", "50", "70", "08",
            "6C", "74", "04", "D0", "20", "60",
            "98", "C8", "44", "D8", "AC", "4C",
            "A4", "A8", "F0", "EC", "0C", "78",
            "54", "80", "D4", "14", "7C", "E8",
            "84", "10", "1C", "5C", "94", "B0",
            "E0", "F8", "2C", "B8", "DC"
    };
    const static string mnemonics[] = {
            "ADD", "ADDF", "ADDR", "AND", "CLEAR", "COMP",
            "COMPF", "COMPR", "DIV", "DIVF", "DIVR", "FIX",
            "FLOAT", "HIO", "J", "JEQ", "JGT", "JLT",
            "JSUB", "LDA", "LDB", "LDCH", "LDF", "LDL",
            "LDS", "LDT", "LDX", "LPS", "MUL", "MULF",
            "MULR", "NORM", "OR", "RD", "RMO", "RSUB",
            "SHIFTL", "SHIFTR", "SIO", "SSK", "STA", "STB",
            "STCH", "STF", "STI", "STL","STS", "STSW",
            "STT", "STX", "SUB", "SUBF", "SUBR", "SVC",
            "TD", "TIO", "TIX", "TIXR", "WD"
    };
    const static bool format2[] = {
            false,false,true,false,true,false,
            false,true,false,false,true,false,
            false,false,false,false,false,false,
            false,false,false,false,false,false,
            false,false,false,false,false,false,
            true,false,false,false,true,false,
            true,true,false,false,false,false,
            false,false,false,false,false,false,
            false,false,false,false,true,true,
            false,false,false,true,false
    };
    //check if a file was successfully opened, if not exit with return 1 and with error message.
    if (!inputObj)
    {
        cerr << "Error: File unable to open. Please try again." << endl;
        return 1;
    } else {
        cout<<"File 'obj' opened successfully."<<endl;
    }

    if (!inputSym)
    {
        cerr << "Error: File unable to open. Please try again." << endl;
        return 1;
    } else {
        cout<<"File 'sym' opened successfully."<<endl;
    }
    //check if output file was created, if not send an error.
    if (!outFile) {
        cerr << "Error: Unable to open 'out.lst' for writing." << endl;
    } else {
        cout << "File 'out.lst' created successfully." << endl;
    }

    while (getline(inputSym, word)) {
        stringstream LitAndSym(word);

        // Skip lines with "Symbol  Address Flags:" or "Name    Lit_Const  Length Address:"
        if (word == "Name    Lit_Const  Length Address:") {
            reached = true;
            continue;
        } else if (word.find("Symbol") != string::npos) {
            continue;
        } else if (!reached) {
            // Parse lines with symbols before reaching "Name    Lit_Const  Length Address:"
            string name, address, flags;
            if (LitAndSym >> name >> address >> flags) {
                Symbols symbol;
                symbol.name = name;
                symbol.address = address;
                symbol.flags = flags;
                symbolTable.push_back(symbol);
            }
        } else {
            // Parse lines with literals after reaching "Name    Lit_Const  Length Address:"
            string name, litConst, address;
            int length;
            Literals literal;

            // Check the first non-whitespace character
            size_t firstChar = word.find_first_not_of(' ');
            if (firstChar != string::npos && word[firstChar] == '=') {
                // If it's '=', treat the line as a literalOrgTable entry
                if (LitAndSym >> litConst >> length >> address) {
                    cout << "litorg added" << endl;
                    literal.name = "";
                    literal.litConst = litConst;
                    literal.length = length;
                    literal.address = address.substr(2,4);
                    literalOrgTable.push_back(literal);
                }
            } else {
                // Otherwise, treat the line as a literalTable entry
                if (LitAndSym >> name >> litConst >> length >> address) {
                    cout << "literal added" << endl;
                    literal.name = name;
                    literal.litConst = litConst;
                    literal.length = length;
                    literal.address = address;
                    literalTable.push_back(literal);
                }
            }
        }
    }
    //This will add in all object code into a vector and also read in important information such as the
    //name and starting address of the program
    while (getline(inputObj, word)) {
        if (word.empty()) {
            // Skip empty lines
            continue;
        }

        string recordType = word.substr(0, 1);
        if (recordType == "H") {
            progName = word.substr(1, 6);
            progEnd = word.substr(15, 4);

            string lengthProg = word.substr(7, 6);
            outFile << left << setw(5) << word.substr(9, 4);

            outFile << left << setw(8) << progName;

            outFile << left << setw(8) << "START";
            // Check if lengthProg is "000000" and write "0" or "1" accordingly
            if (lengthProg == "000000") {
                outFile << "0";
            } else {
                // Handle other cases if needed
                outFile << lengthProg;
            }

            outFile << endl;
        } else if (recordType == "T") {
            words.push_back(word); // Loads the vector with text records
            //words count is iterated to help us keep track of the amount we have
            wordsCount++;
            //loads vector pogStarts with the starting address of the text record
            progStarts.push_back(word.substr(3,4));

        }
        else{
            //adds in all exta lines of obj file
            words.push_back(word);
        }
    }
    //iterate through all text records starting with 'T'
    for(int currTextRec = 0; currTextRec < wordsCount; currTextRec++) {
        string BASE = "0000";
        stringstream currHexLoc;
        //Give the location to currHexLoc and then using stringsteam convert it to a string currLoc
        currHexLoc << setw(4) << words.at(currTextRec).substr(1, 6);
        string currLocStr = currHexLoc.str();
        //Since currLocStr is in hex we use string to int (stoi) and save a int representation of it to currLocInt


        currLocInt = stoi(currLocStr, 0, 16);

        //Now that code has analyzed the first 9 digits we want to start at 9
        int newStart = 9;
        //bools to assess with we have sym or literal
        bool inSym = false;
        bool inLit = false;
        bool inOrgLit = false;


        while (newStart < words.at(currTextRec).length()) {
            stringstream formatSymbol;
            stringstream formatLiteral;
            stringstream hexCheck;
            //formats hexCheck width 4, fill w zeros, base 16, with value currLocInt
            hexCheck << setw(4) << setfill('0') << hex << currLocInt;
            //assigns loc with string representation of hexCheck

            ///hexx
            string currLocStr = hexCheck.str();
            //string symLoc;
            //string litLoc;
            string NAME = " ";
            string pc;
            Literals litVal;
            Literals litOrgVal;

            //iterates through the symbolTable to find if hex is a match and will extract name and put in NAME
            for (int i = 0; i < symbolTable.size(); i++) {
                //formatSymbol will set the address if symbolTable(i).address to width 4
                formatSymbol << setw(4) << symbolTable.at(i).address.substr(2, 4);
                string tempLoc = formatSymbol.str();
                tool.toUppercase(currLocStr);
                if (tempLoc == currLocStr) {
                    NAME = symbolTable.at(i).name;
                    symbolTracker++;
                }
                //reset formatSymbol SS
                formatSymbol.str("");
            }

            //check for match in literalTable
            for (int litLength = 0; litLength < literalTable.size(); litLength++) {
                formatLiteral << literalTable.at(litLength).address;
                string tempLoc = formatLiteral.str();
                tempLoc = tempLoc.substr(2, 4);
                if (tempLoc == currLocStr) {
                    litVal = literalTable.at(litLength);
                    inLit = true;
                }
                formatLiteral.str("");
            }

            //Check for match in litorg table
            for (int litOrgLength = 0; litOrgLength < literalOrgTable.size(); litOrgLength++) {
                string tempLoc = literalOrgTable.at(litOrgLength).address;
                if (tempLoc == currLocStr) {
                    litOrgVal = literalOrgTable.at(litOrgLength);
                    litOrgTracker++;
                    inOrgLit = true;
                }
                formatLiteral.str("");
            }
            if(inOrgLit){
                stringstream formatLitOrgLoc;
                formatLitOrgLoc << setw(4) << setfill('0') << hex << currLocInt;
                string tempLoc = formatLitOrgLoc.str();
                tool.toUppercase(tempLoc);
                //if we are not at the last text record and we have not reached all symbols then print out LITORG since we
                //have not reached the end of the program
                if((currTextRec < wordsCount - 1) || (symbolTracker < symbolTable.size())){
                    outFile << left << setw(13) << " ";
                    outFile << left << setw(8) << "LTORG";
                    outFile << left << setw(30) << " "<<endl;
                }
                //output loc first column, blank space, *, obj code, and update the next curr loc accordingly
                outFile << left << setw(5) << tempLoc;
                outFile << left << setw(8) << " ";
                outFile << left << setw(8) << "*";
                outFile << left << setw(14) << litOrgVal.litConst;
                string tempObj = litOrgVal.litConst.substr(3, litOrgVal.length);
                outFile << left << setw(16) << tempObj << endl;
                currLocInt += litOrgVal.length / 2;
                newStart += litOrgVal.length;
                //set inOrgLit to false so we do not enter infinite loop.
                inOrgLit = false;
            }
                ///in lit
            else if (inLit) {

                stringstream formatLitLoc;
                formatLitLoc << setw(4) << setfill('0') << hex << currLocInt;
                string tempLoc = formatLitLoc.str();
                tool.toUppercase(tempLoc);

                //output loc first column, name of literal, BYTE, obj code, and update the next curr loc accordingly
                outFile << left << setw(5) << tempLoc;
                outFile << left << setw(8) << litVal.name;
                outFile << left << setw(8) << "BYTE";
                outFile << left << setw(14) << litVal.litConst;
                string tempObj = litVal.litConst.substr(2, litVal.length);
                outFile << left << setw(16) << tempObj << endl;
                currLocInt += litVal.length / 2;
                newStart += litVal.length;

                //set inLit to false so we do not enter infinite loop.
                inLit = false;
            } else {
                string hexLoc;
                stringstream locStream;
                locStream << setw(4) << setfill('0') << hex << currLocInt;
                hexLoc = locStream.str();
                tool.toUppercase(hexLoc);
                string extracted = words.at(currTextRec).substr(newStart, 2);

                // Convert the extracted substring to an integer
                number = std::stoi(extracted, 0, 16);

                //apply mask to extracted op in hex
                currentOp = number & mask;

                //revert back to string
                stringstream ss;
                ss << hex << uppercase << setw(2) << setfill('0') << currentOp;
                string formatted_currentOp = ss.str();

                //find opcode index in ops array
                int j = 0;
                while (ops[j] != formatted_currentOp) {
                    j++;
                }

                //PRINT INSTR
                //Store mnemonic
                tempMnemonics = mnemonics[j];

                //extract values for n i
                extracted = words.at(currTextRec)[newStart + 1];

                //convert extracted to binary for n and i
                string tempVals = tool.extractBits(extracted, 2);

                n = tempVals[0];
                i = tempVals[1];
                // extract 3rd index and convert extracted to binary for x, b, p, e
                extracted = words.at(currTextRec)[newStart + 2];

                tempVals = tool.extractBits(extracted, 4);
                x = tempVals[0];
                cout << x << endl;
                b = tempVals[1];
                p = tempVals[2];
                e = tempVals[3];
                //checks if format 2
                string currObj;
                if (format2[j]) {
                    outFile << left << setw(5) << hexLoc;
                    outFile << left << setw(8) << NAME;
                    outFile << left << setw(8) << mnemonics[j];
                    currObj = words.at(currTextRec).substr(newStart, 4);
                    //check if it is CLEAR
                    if (tempMnemonics == "CLEAR") {
                        outFile << left << setw(14) << tool.mapOpcodeToRegister(currObj);
                    } else {
                        outFile << left << setw(14) << " ";
                    }
                    outFile << left << setw(12) << currObj << endl;
                    newStart += 4;
                    currLocInt = tool.locFinder(currLocInt, 2, tempMnemonics);
                } else {
                    string currOat = tool.oatDetector(n, i);
                    string currTaam = tool.taamDetector(b, p, x);
                    //checks if format 3
                    if (e == '0') {
                        currObj = words.at(currTextRec).substr(newStart, 6);
                        outFile << left << setw(5) << hexLoc;
                        outFile << left << setw(8) << NAME;
                        outFile << left << setw(8) << mnemonics[j];
                        newStart += 6;

                        ///ian added this find out why
                        //int tSize = words.at(currTextRec).length();
                        //if (newStart >= tSize) {
                        //    pc = words.at(currTextRec + 1).substr(1, 6);
                        //    currLocInt = stoi(pc, nullptr, 16);
                        //} else {
                        currLocInt = tool.locFinder(currLocInt, 3, tempMnemonics);
                        //}
                        string ta = tool.calculateTA(currTaam, currObj.substr(currObj.length() - 3), currLocInt, BASE,
                                                     x, symbolTable, literalTable, literalOrgTable);
                        for (char &c: ta) {
                            if (c >= 'a' && c <= 'f') {
                                c = toupper(c);
                            }
                        }
                        //check if indexed and print target address accordingly
                        if(x == '0')outFile << left << setw(14) << (currOat + ta);
                        else if(x == '1') outFile << left << setw(14) << (currOat + ta + ",X");

                        //print out obj code
                        outFile << left << setw(16) << currObj << endl;

                        if (mnemonics[j] == "LDB") {
                            outFile << left << setw(13) << " ";
                            outFile << left << setw(8) << "BASE";
                            string preCap = tool.calculateTA(currTaam, currObj.substr(currObj.length() - 3), currLocInt,
                                                             BASE, x, symbolTable, literalTable, literalOrgTable);
                            for (char &c: preCap) {
                                if (c >= 'a' && c <= 'f') {
                                    c = toupper(c);
                                }
                            }
                            outFile << left << setw(16) << preCap << endl;
                        }
                    }
                        //checks if format 4
                    else if (e == '1') {
                        currObj = words.at(currTextRec).substr(newStart, 8);
                        outFile << left << setw(5) << hexLoc;
                        outFile << left << setw(8) << NAME;
                        outFile << left << setw(8) << ("+" + mnemonics[j]);
                        newStart += 8;

                        currLocInt = tool.locFinder(currLocInt, 4, tempMnemonics);

                        string ta = currObj.substr(4, 4);
                        //since now ta is not able to be converted to letters and not numbers as a string we save it before we use it
                        string tempTA = ta;
                        //iterate through symbol table and see if TA has a match if so update to the name
                        for (int i = 0; i < symbolTable.size(); i++) {
                            if (ta == (symbolTable[i].address).substr(2, 5) && symbolTable[i].name != "FIRST") {
                                ta = symbolTable[i].name;
                            }
                        }
                        //iterate through literal table and see if TA has a match if so update to the lit_const
                        for(int i = 0; i < literalOrgTable.size(); i++){
                            if (ta == (literalOrgTable[i].address)){
                                ta = literalOrgTable[i].litConst;
                            }
                        }
                        //check case where indexed
                        if(x == '0')outFile << left << setw(14) << (currOat + ta);
                        else if(x == '1') outFile << left << setw(14) << (currOat + ta + ",X");
                        outFile << left << setw(16) << currObj << endl;
                        //check case where we have ldb, if so print base below instruction
                        if (mnemonics[j] == "LDB") {
                            outFile << left << setw(13) << " ";
                            outFile << left << setw(8) << "BASE";
                            //tempTA is utlized and BASE is given address rather than a label
                            BASE = tempTA;
                            outFile << left << setw(16) << ta << endl;
                        }
                    }
                    //error message, niether if statement case was fullfilled
                    else {
                        outFile << "error";
                    }
                }
            }
        }
        int RESB;

        //once we print the text record we check for any extra symbols that need to be printed out before we continue
        //to the next text record

        //start by checking if we have not reached the last text record
        if(currTextRec + 1 < wordsCount) {

            //initialize string hex string that will be converted to hex longs. This is so we can check if one is greater than the other
            std::string nextStart = progStarts.at(currTextRec +1);
            std::string currLocc = symbolTable.at(symbolTracker).address.substr(2,4);

            unsigned long next = std::stoul(nextStart, nullptr, 16);

            //while there is still symbols left to explore in the table
            while(symbolTracker < symbolTable.size()){
                //current symbols location
                currLocc = symbolTable.at(symbolTracker).address.substr(2,4);
                unsigned long curr = std::stoul(currLocc, nullptr, 16);
                //if start if next text record is greater than loc of current symbol
                if(next > curr){
                    //if symboltracker is at the last index of the vector
                    if(symbolTracker == symbolTable.size()-1){
                        //RESB is set to the loc of the next text record start - the current symbols location. then is converted to decimal form
                        RESB = tool.subtract_hex(nextStart,symbolTable.at(symbolTracker).address.substr(2,4));
                    }
                    else{
                        string currNext = symbolTable.at(symbolTracker + 1).address.substr(2,4);
                        unsigned long nextLocInt = std::stoul(currNext, nullptr, 16);
                        //if the loc of the next sym is larger than the loc of the next text rec start
                        if(nextLocInt>next){
                            //RESB is set to the loc of the next text record start - the current symbols location. then is converted to decimal form
                            cout<< "subtracting " + nextStart + " with " + symbolTable.at(symbolTracker).address.substr(2,4)<<endl;
                            RESB = tool.subtract_hex(nextStart,symbolTable.at(symbolTracker).address.substr(2,4));
                        }

                        else{
                            //RESB is set to the loc of the next symbols location - the current symbols location. then is converted to decimal form
                            cout<< "subtracting " + currNext + " with " + symbolTable.at(symbolTracker).address.substr(2,4);
                            RESB = tool.subtract_hex(currNext,symbolTable.at(symbolTracker).address.substr(2,4));
                        }
                    }


                    outFile << left << setw(5) << symbolTable.at(symbolTracker).address.substr(2,4);
                    outFile << left << setw(8) << symbolTable.at(symbolTracker).name;
                    //if NextCurr is greater than next than we should do next - curr
                    outFile << left << setw(8) << "RESB";
                    outFile << left << setw(16) << RESB <<endl;
                }
                else{
                    break;
                }
                symbolTracker++;
            }
        }

    }
    //end of for loop

    //once we have exited the text record for loop we check if there are any remaining symbols that are yet to be outputed
    while(symbolTracker < symbolTable.size()){
        int RESB;
        //if we are at the last symbol in the table
        if(symbolTracker + 1 >= symbolTable.size()){
            //RESB is set to the loc of the programs end loc - the current symbols location. then is converted to decimal form
            RESB = tool.subtract_hex(progEnd, symbolTable.at(symbolTracker).address.substr(2,4));
        }
        //otherwise there are still more symbols that need to be printed after the one the program is currently on
        else{
            //RESB is set to the loc the next symbol - the current symbols location. then is converted to decimal form
            RESB = tool.subtract_hex(symbolTable.at(symbolTracker + 1).address.substr(2,4), symbolTable.at(symbolTracker).address.substr(2,4));
        }
        outFile << left << setw(5) << symbolTable.at(symbolTracker).address.substr(2,4);
        outFile << left << setw(8) << symbolTable.at(symbolTracker).name;
        outFile << left << setw(8) << "RESB";
        outFile << left << setw(16) << RESB <<endl;
        symbolTracker++;
    }
    //print ending line
    outFile << left << setw(13) << " " << "END     " << left << setw(13) << progName << endl;


    //close out file and input file
    cout<<"File Closed";
    outFile.close();
    inputObj.close();
    inputSym.close();
    return 0;

}
