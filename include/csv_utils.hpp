#ifndef CSV_UTILS_HPP
#define CSV_UTILS_HPP

#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <stdexcept>
#include <iomanip>

struct Element
{
    std::string name;
    double currentPercentage;
    double targetPercentage;
    double currentWeight;
};

struct IterationData
{
    int iteration;
    std::vector<Element> elements;
    std::vector<std::pair<std::string, double>> additions;
    double totalWeight;
};

class ExcelCSVReader
{
public:
    static std::vector<std::vector<std::string>> readCSV(const std::string &filename)
    {
        std::vector<std::vector<std::string>> data;
        std::ifstream file(filename);

        if (!file.is_open())
        {
            throw std::runtime_error("Unable to open file: " + filename);
        }

        std::string line;
        while (std::getline(file, line))
        {
            std::vector<std::string> row;
            std::stringstream ss(line);
            std::string cell;

            while (std::getline(ss, cell, ','))
            {
                cell.erase(std::remove(cell.begin(), cell.end(), '"'), cell.end());
                cell.erase(0, cell.find_first_not_of(" \t"));
                cell.erase(cell.find_last_not_of(" \t") + 1);
                row.push_back(cell);
            }
            data.push_back(row);
        }

        return data;
    }
};

class CSVWriter
{
public:
    static void writeIterationData(const std::string &baseFilename,
                                   const std::vector<IterationData> &iterationsData)
    {
        // Create output directory if it doesn't exist
        std::filesystem::create_directories("output");

        // Write summary file
        writeSummaryFile(baseFilename, iterationsData);

        // Write individual iteration files
        for (const auto &iteration : iterationsData)
        {
            writeIterationFile(baseFilename, iteration);
        }
    }

private:
    static void writeSummaryFile(const std::string &baseFilename,
                                 const std::vector<IterationData> &iterationsData)
    {
        std::ofstream file("output/" + baseFilename + "_summary.csv");
        if (!file.is_open())
        {
            throw std::runtime_error("Unable to create summary file");
        }

        // Write header
        file << "Iteration,Element,Current %,Target %,Addition (kg),New Total Weight (kg)\n";

        // Write data for each iteration
        for (const auto &iter : iterationsData)
        {
            for (size_t i = 0; i < iter.elements.size(); ++i)
            {
                file << iter.iteration << ",";
                file << iter.elements[i].name << ",";
                file << std::fixed << std::setprecision(2);
                file << iter.elements[i].currentPercentage << ",";
                file << iter.elements[i].targetPercentage << ",";

                // Find if there was an addition for this element
                auto addition = std::find_if(iter.additions.begin(), iter.additions.end(),
                                             [&](const auto &p)
                                             { return p.first == iter.elements[i].name; });

                if (addition != iter.additions.end())
                {
                    file << addition->second;
                }
                else
                {
                    file << "0.00";
                }
                file << "," << iter.totalWeight << "\n";
            }
        }
    }

    static void writeIterationFile(const std::string &baseFilename,
                                   const IterationData &iteration)
    {
        std::ofstream file("output/" + baseFilename + "_iteration_" +
                           std::to_string(iteration.iteration) + ".csv");
        if (!file.is_open())
        {
            throw std::runtime_error("Unable to create iteration file");
        }

        // Write headers
        file << "Element,Current %,Target %,Current Weight (kg),Addition (kg)\n";

        // Write data
        for (const auto &element : iteration.elements)
        {
            file << element.name << ",";
            file << std::fixed << std::setprecision(2);
            file << element.currentPercentage << ",";
            file << element.targetPercentage << ",";
            file << element.currentWeight << ",";

            // Find if there was an addition for this element
            auto addition = std::find_if(iteration.additions.begin(), iteration.additions.end(),
                                         [&](const auto &p)
                                         { return p.first == element.name; });

            if (addition != iteration.additions.end())
            {
                file << addition->second;
            }
            else
            {
                file << "0.00";
            }
            file << "\n";
        }

        // Write total weight at bottom
        file << "\nTotal Weight:," << iteration.totalWeight << " kg\n";
    }
};

#endif