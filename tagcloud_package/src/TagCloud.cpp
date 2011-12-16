/*
Portions of this software Copyright (c) 2011 by Vertica, an HP
Company.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

- Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.

- Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.


THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <string>
#include <map>
#include <list>
#include <set>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <queue>
#include <ctype.h>
#include <limits>
#include <stdlib.h>

#include "Vertica.h"

using namespace Vertica;

const size_t MaxWordLen = 25;
const size_t WordRange = 10;
const size_t MaxStringLen = 64000;

class WordFreqCalc
{
public:
    WordFreqCalc(const std::string &target);
    void updateWordFreq(const std::string &line);
    const std::map<std::string, double> &getWordFreq() { return wordFreq; }

private:
    // function words should not be considered, such as prepositions: 'of', 'and', 'a', 'an', etc
    std::set<std::string> funcWords;

    // weighted average of frequency a word appears close to the target word
    std::map<std::string, double> wordFreq;

    const std::string &target;
};

WordFreqCalc::WordFreqCalc(const std::string &target)
: target(target)
{
    std::istringstream ss("quot lt gt ref amp apos http www com html htm org url name title index domain link comment diff prev otherlinks page file first last user jpg cite php oldid there also be is was are were able to not can could dare have has had may might must need ought shall should will would a all an another any both each either every her his its my neither no other our per some that the their these this those whatever whichever your accordingly after albeit although and as because before both but consequently either for hence however if neither nevertheless nor once or since so than that then thence therefore tho' though thus till unless until when whenever where whereas wherever whether while whilst yet all another any anybody anyone anything both each either everybody everyone everything few he her hers herself him himself his it its itself many me mine myself neither no_one nobody none nothing one other ours ourselves several she some somebody someone something such that theirs them themselves these they this those us we what whatever which whichever who whoever whom whomever whose you yours yourself yourselves all another any both certain each either enough few fewer less little loads lots many more most much neither no none part several some various aboard about above absent according across after against ahead along alongside amid amidst among amongst anti around as aside astraddle astride at bar barring before behind below beneath beside besides between beyond but by circa concerning considering despite due during except excepting excluding failing following for from given in including inside into less like minus near near next notwithstanding of off on onto opposite out outside over past pending per pertaining regarding respecting round save saving since than through throughout thru till toward towards under underneath unlike until unto upon versus via with within without");
    while (ss) {
        std::string buf;
        ss >> buf;
        funcWords.insert(buf);
    }
}

void WordFreqCalc::updateWordFreq(const std::string &line)
{
    std::list<std::string> prevWords;
    bool afterTarget = false; // whether we've seen target within WordRange
    size_t posAfterTarget = 0;

    // skip if the string doesn't have the target word
    if (line.find(target) == std::string::npos)
        return;

    // transform into lower case, and ignore all non-letter characters
    std::string newline = line;
    for (size_t i = 0; i < newline.size(); ++i) {
        if (::isalpha(newline[i]))
            newline[i] = ::tolower(newline[i]);
        else
            newline[i] = ' ';
    }

    std::istringstream ss(newline);
    while (ss) {
        std::string word;
        ss >> word;

        // ignore too long or too short words
        if (word.size() > MaxWordLen || word.size() <= 2)
            continue;

        // skip function words
        if (funcWords.count(word) > 0)
            continue;

        // found the target word
        if (word == target) {
            afterTarget = true;
            posAfterTarget = 0;

            // update the frequencies of each previous words
            size_t distance = 1;
            std::list<std::string>::const_reverse_iterator rit;
            for (rit = prevWords.rbegin(); rit != prevWords.rend(); ++rit) {
                wordFreq[*rit] += 1/(double)distance;
                ++distance;
            }

            prevWords.clear();
            continue;
        }

        // keep track this word, with limited memory
        prevWords.push_back(word);
        while (prevWords.size() > WordRange)
            prevWords.pop_front();

        // for words closely after the target words, update their frequencies as well
        if (afterTarget) {
            ++posAfterTarget;
            wordFreq[word] += 1/(double)posAfterTarget;
            if (posAfterTarget >= WordRange)
                afterTarget = false;
        }
    }
}

class RelevantWords : public TransformFunction
{
    virtual void processPartition(ServerInterface &srvInterface, PartitionReader &input_reader, PartitionWriter &output_writer)
    {
        const VString &arg0 = input_reader.getStringRef(0);
        const std::string &target = arg0.str();

        WordFreqCalc wordFreqCalc(target);

        // compute the relevant words and their weights/frequencies
        do {
            const VString &line = input_reader.getStringRef(1);
            if (line.isNull()) continue;
            wordFreqCalc.updateWordFreq(line.str());
        } while (input_reader.next());

        // generate output from the computed map
        const std::map<std::string, double> &wordFreq = wordFreqCalc.getWordFreq();
        std::map<std::string, double>::const_iterator it;
        for (it = wordFreq.begin(); it != wordFreq.end(); ++it) {
            output_writer.setFloat(0, it->second);
            VString &word = output_writer.getStringRef(1);
            word.copy(it->first);
            output_writer.next();
        }
    }
};

class RelevantWordsFactory : public TransformFunctionFactory
{
    virtual TransformFunction *createTransformFunction(ServerInterface &srvInterface)
    { return vt_createFuncObj(srvInterface.allocator, RelevantWords); }

    virtual void getReturnType(ServerInterface &srvInterface, const SizedColumnTypes &input_types, SizedColumnTypes &output_types)
    {
        output_types.addFloat("weight");
        output_types.addVarchar(MaxWordLen, "word");
    }

    virtual void getPrototype(ServerInterface &srvInterface, ColumnTypes &argTypes, ColumnTypes &returnType)
    {
        argTypes.addVarchar(); // the key word
        argTypes.addVarchar(); // the column containing text corpus

        returnType.addFloat();
        returnType.addVarchar();
    }

};

RegisterFactory(RelevantWordsFactory);


class RelevantWordsNoLoad : public TransformFunction
{
    virtual void processPartition(ServerInterface &srvInterface, PartitionReader &input_reader, PartitionWriter &output_writer)
    {
        const VString &arg0 = input_reader.getStringRef(0);
        const std::string &target = arg0.str();

        const VString &arg1 = input_reader.getStringRef(1);
        const std::string &filename = arg1.str();
        std::ifstream infile(filename.c_str(), std::ios::in);
        if (!infile.good())
            vt_report_error(0, "Could not open file %s", filename.c_str());

        WordFreqCalc wordFreqCalc(target);

        const size_t BLK_SIZE_BYTE = 64*1024; // 64k
        char buf[BLK_SIZE_BYTE];
        while (infile.good()) {
            infile.read(buf, BLK_SIZE_BYTE);
            wordFreqCalc.updateWordFreq(buf);
        }

        // generate output from the computed map
        const std::map<std::string, double> &wordFreq = wordFreqCalc.getWordFreq();
        std::map<std::string, double>::const_iterator it;
        for (it = wordFreq.begin(); it != wordFreq.end(); ++it) {
            output_writer.setFloat(0, it->second);
            VString &word = output_writer.getStringRef(1);
            word.copy(it->first);
            output_writer.next();
        }
    }
};

class RelevantWordsNoLoadFactory : public TransformFunctionFactory
{
    virtual TransformFunction *createTransformFunction(ServerInterface &srvInterface)
    { return vt_createFuncObj(srvInterface.allocator, RelevantWordsNoLoad); }

    virtual void getReturnType(ServerInterface &srvInterface, const SizedColumnTypes &input_types, SizedColumnTypes &output_types)
    {
        output_types.addFloat("weight");
        output_types.addVarchar(MaxWordLen, "word");
    }

    virtual void getPrototype(ServerInterface &srvInterface, ColumnTypes &argTypes, ColumnTypes &returnType)
    {
        argTypes.addVarchar(); // the word
        argTypes.addVarchar(); // file name of the text corpus

        returnType.addFloat();
        returnType.addVarchar();
    }

};

RegisterFactory(RelevantWordsNoLoadFactory);


struct RenderWord
{
    RenderWord(const std::string &word, int fontsize, const std::string &color)
    : word(word), fontsize(fontsize), color(color)
    { }

    std::string word;
    int fontsize;
    std::string color;
};

int getFontSize(double w_max, double w_min, double w)
{
    const int font_max = 50;
    const int font_min = 10;
    return font_max * (w - w_min) / (w_max - w_min) + font_min;
}

bool compare_random(const RenderWord &a, const RenderWord &b)
{
    return rand() % 2 == 0;
}

class GenerateTagCloud : public TransformFunction
{
    virtual void processPartition(ServerInterface &srvInterface, PartitionReader &input_reader, PartitionWriter &output_writer)
    {
        const VString &arg2 = input_reader.getStringRef(2);
        const std::string &filename = arg2.str();
        std::ofstream outfile(filename.c_str(), std::ios::out | std::ios::trunc);
        if (!outfile.good())
            vt_report_error(0, "Could not open file %s for output", filename.c_str());

        std::map<std::string, double> wordFreq;

        // populate the word requency map, and compute necessary parameters to get font size later
        double w_min = std::numeric_limits<double>::max();
        double w_max = std::numeric_limits<double>::min();
        do {
            double weight = input_reader.getFloatRef(0);
            const VString &word = input_reader.getStringRef(1);
            if (word.isNull()) continue;
            wordFreq[word.str()] = weight;
            w_min = std::min(w_min, weight);
            w_max = std::max(w_max, weight);
        } while (input_reader.next());

        // some predefined color used in outputed HTML
        std::vector<std::string> colors;
        colors.push_back("red");
        colors.push_back("blue");
        colors.push_back("orange");
        colors.push_back("green");
        colors.push_back("black");

        // randomly generate color, and assign the font size according to their weight
        std::list<RenderWord> renderList;
        std::map<std::string, double>::const_iterator it;
        for (it = wordFreq.begin(); it != wordFreq.end(); ++it) {
            int fz = getFontSize(w_max, w_min, it->second);
            const std::string &color = colors[rand() % colors.size()];
            renderList.push_back(RenderWord(it->first, fz, color));
        }
        // sort by random to shuffle positions of the words
        renderList.sort(compare_random);

        // generate output
        const size_t NumWordsPerLine = 10;
        size_t nword = 0;
        std::list<RenderWord>::const_iterator iter;
        std::ostringstream oss;
        for (iter = renderList.begin(); iter != renderList.end(); ++iter) {
            // get a new line
            if (nword % NumWordsPerLine == 0) oss << "<p class=\"tag_cloud\"></p>";

            oss << "<span style=\"font-size: " << iter->fontsize << "px; color: "
                << iter->color << "\">"
                << iter->word << "</span>";
            ++nword;
        }

        // write the output to file
        outfile << oss.str();
        VString &word = output_writer.getStringRef(0);
        word.copy("HTML file generated!");
        output_writer.next();
    }
};

class GenerateTagCloudFactory : public TransformFunctionFactory
{
    virtual TransformFunction *createTransformFunction(ServerInterface &srvInterface)
    { return vt_createFuncObj(srvInterface.allocator, GenerateTagCloud); }

    virtual void getReturnType(ServerInterface &srvInterface, const SizedColumnTypes &input_types, SizedColumnTypes &output_types)
    {
        output_types.addVarchar(MaxStringLen, "HTML generate status");
    }

    virtual void getPrototype(ServerInterface &srvInterface, ColumnTypes &argTypes, ColumnTypes &returnType)
    {
        argTypes.addFloat();   // weight of the word
        argTypes.addVarchar(); // the word
        argTypes.addVarchar(); // filename of the generated .html file

        returnType.addVarchar(); // return the status
    }

};

RegisterFactory(GenerateTagCloudFactory);
