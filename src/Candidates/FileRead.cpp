/**
 * @file FileRead.cpp
 * @brief Implementation of file reading logic for candidate generation.
 */

#include "Candidates/FileRead.hpp"
#include <algorithm> // for isspace, isdigit
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace std;

// Internal helper to process a single line
void process_line(const string &raw_line, int codeLen, vector<string> &result)
{
    if (raw_line.empty())
        return;

    // First remove all whitespace to get clean content
    string clean_line = "";
    for (char c : raw_line)
    {
        if (!isspace(c))
            clean_line += c;
    }

    if (clean_line.length() != (size_t)codeLen)
        return;

    string processed = "";
    bool valid = true;

    for (char c : clean_line)
    {
        if (isdigit(c))
        {
            if (c >= '0' && c <= '3')
            {
                processed += c;
            }
            else
            {
                valid = false;
                break;
            }
        }
        else
        {
            char upper = toupper(c);
            if (upper == 'A')
                processed += '0';
            else if (upper == 'C')
                processed += '1';
            else if (upper == 'G')
                processed += '2';
            else if (upper == 'T')
                processed += '3';
            else
            {
                valid = false;
                break;
            }
        }
    }

    if (valid)
    {
        result.push_back(processed);
    }
}

std::vector<std::string> ReadFileCandidates(const std::string &filename, int codeLen)
{
    std::vector<std::string> result;
    std::ifstream file(filename);

    if (!file.is_open())
    {
        throw std::runtime_error("Could not open input file: " + filename);
    }

    std::vector<std::string> preloaded_lines;
    std::string line;
    bool separator_found = false;

    // Peek at first 20 lines to detect format or find separator
    int peek_lines = 20;
    while (peek_lines-- > 0 && file.peek() != EOF && std::getline(file, line))
    {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (line.empty())
            continue;

        // Check for separator
        // Trim trailing whitespace for check
        std::string trimmed = line;
        while (!trimmed.empty() && isspace(trimmed.back()))
            trimmed.pop_back();

        bool potentially_separator = (trimmed.length() >= 3 && trimmed.find("===") == 0);
        if (potentially_separator)
        {
            bool all_eq = true;
            for (char c : trimmed)
                if (c != '=')
                    all_eq = false;
            if (all_eq)
            {
                separator_found = true;
                break;
            }
        }
        preloaded_lines.push_back(line);
    }

    // If separator NOT found, checks consistency of preloaded lines
    if (!separator_found)
    {
        bool looks_like_data = true;
        if (!preloaded_lines.empty())
        {
            size_t first_len = preloaded_lines[0].length();
            for (const auto &l : preloaded_lines)
            {
                if (l.length() != first_len)
                {
                    looks_like_data = false;
                    break;
                }
            }
        }

        if (looks_like_data)
        {
            for (const auto &l : preloaded_lines)
            {
                process_line(l, codeLen, result);
            }
        }
    }

    // Continue reading rest
    while (std::getline(file, line))
    {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (line.empty())
            continue;

        if (!separator_found)
        {
            // Still looking for separator
            // Trim trailing whitespace for check
            std::string trimmed = line;
            while (!trimmed.empty() && isspace(trimmed.back()))
                trimmed.pop_back();

            bool potentially_separator = (trimmed.length() >= 3 && trimmed.find("===") == 0);
            if (potentially_separator)
            {
                bool all_eq = true;
                for (char c : trimmed)
                    if (c != '=')
                        all_eq = false;
                if (all_eq)
                {
                    separator_found = true;
                    continue; // Data starts next line
                }
            }
        }
        else
        {
            // Separator found (or we decided it was data code path via peeking), process lines
            process_line(line, codeLen, result);
        }
    }

    return result;
}
