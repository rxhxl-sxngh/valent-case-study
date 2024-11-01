#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <iomanip>
#include "include/csv_utils.hpp"

class SteelGradeCalculator
{
private:
    double totalWeight;
    std::vector<Element> elements;
    std::vector<IterationData> iterationsData;
    // 1% tolerance (in the real world, some elements in the initial composition may not be wanted in the final spec)
    // As long as the difference is within this tolerance, I consider the composition to be acceptable.
    const double TOLERANCE = 0.01;

    void updatePercentages()
    {
        for (auto &element : elements)
        {
            element.currentPercentage = (element.currentWeight / totalWeight) * 100.0;
        }
    }

    double parsePercentage(const std::string &str)
    {
        std::string cleaned = str;
        cleaned.erase(std::remove(cleaned.begin(), cleaned.end(), '%'), cleaned.end());
        return std::stod(cleaned);
    }

public:
    SteelGradeCalculator(double weight) : totalWeight(weight) {}

    // load steel composition data from a CSV file
    void loadFromCSV(const std::string &filename)
    {
        try
        {
            auto data = ExcelCSVReader::readCSV(filename);

            if (data.size() < 2)
            {
                throw std::runtime_error("CSV file must contain at least a header row and one data row");
            }

            elements.clear();
            iterationsData.clear();

            int nameCol = -1, currentCol = -1, targetCol = -1;
            for (size_t i = 0; i < data[0].size(); i++)
            {
                std::string header = data[0][i];
                std::transform(header.begin(), header.end(), header.begin(), ::tolower);

                if (header.find("element") != std::string::npos)
                    nameCol = i;
                else if (header.find("initial") != std::string::npos)
                    currentCol = i;
                else if (header.find("final") != std::string::npos)
                    targetCol = i;
            }

            if (nameCol == -1 || currentCol == -1 || targetCol == -1)
            {
                throw std::runtime_error("Required columns not found in CSV");
            }

            for (size_t i = 1; i < data.size(); i++)
            {
                try
                {
                    std::string name = data[i][nameCol];
                    double current = parsePercentage(data[i][currentCol]);
                    double target = parsePercentage(data[i][targetCol]);
                    double currentWeight = (current / 100.0) * totalWeight;

                    elements.push_back({name, current, target, currentWeight});
                }
                catch (const std::exception &e)
                {
                    std::cerr << "Warning: Skipping invalid row " << i << ": " << e.what() << std::endl;
                }
            }

            // Store initial state
            iterationsData.push_back({0, elements, {}, totalWeight});

            std::cout << "Successfully loaded " << elements.size() << " elements from " << filename << std::endl;
        }
        catch (const std::exception &e)
        {
            throw std::runtime_error("Error loading CSV file: " + std::string(e.what()));
        }
    }

    // main algorithm to calculate additions
    void calculateAdditions()
    {
        std::cout << "\nInitial composition before additions:\n";
        for (const auto &element : elements)
        {
            std::cout << element.name << ": "
                      << element.currentPercentage << "% "
                      << "(Target: " << element.targetPercentage << "%)\n";
        }

        std::cout << "\nCalculating additions for " << totalWeight << " kg batch:\n";
        std::cout << std::fixed << std::setprecision(2);

        double originalWeight = totalWeight;
        bool changesNeeded = true;
        int iterations = 0;
        const int MAX_ITERATIONS = 10;

        while (changesNeeded && iterations < MAX_ITERATIONS)
        {
            changesNeeded = false;
            iterations++;

            std::vector<std::pair<std::string, double>> currentIterationAdditions;

            // First, check if any elements are above target and calculate required dilution
            double maxDilutionNeeded = 0.0;
            for (const auto &element : elements)
            {
                if (element.name == "Iron")
                    continue;

                if (element.currentPercentage > element.targetPercentage)
                {
                    // Calculate how much the total mass needs to increase to dilute this element
                    double currentMass = element.currentWeight;
                    double desiredPercentage = element.targetPercentage / 100.0;
                    double requiredTotalMass = currentMass / desiredPercentage;
                    double dilutionNeeded = requiredTotalMass - totalWeight;
                    maxDilutionNeeded = std::max(maxDilutionNeeded, dilutionNeeded);
                }
            }

            // If dilution is needed, add iron first
            if (maxDilutionNeeded > TOLERANCE)
            {
                std::cout << "Add " << maxDilutionNeeded << " kg of Iron for dilution\n";
                auto &iron = *std::find_if(elements.begin(), elements.end(),
                                           [](const Element &e)
                                           { return e.name == "Iron"; });
                iron.currentWeight += maxDilutionNeeded;
                totalWeight += maxDilutionNeeded;
                currentIterationAdditions.push_back({"Iron", maxDilutionNeeded});
                changesNeeded = true;
                updatePercentages();
            }

            // Then handle elements below target
            for (auto &element : elements)
            {
                if (element.name == "Iron")
                    continue;

                double targetWeight = (element.targetPercentage / 100.0) * totalWeight;
                if (element.currentWeight < targetWeight)
                {
                    double addition = targetWeight - element.currentWeight;
                    addition *= 1.05; // Add 5% extra to account for future dilution

                    if (addition > TOLERANCE)
                    {
                        std::cout << "Add " << addition << " kg of " << element.name << "\n";
                        element.currentWeight += addition;
                        totalWeight += addition;
                        changesNeeded = true;
                        currentIterationAdditions.push_back({element.name, addition});
                    }
                }
            }

            updatePercentages();

            // Check if any element is still outside tolerance
            for (const auto &element : elements)
            {
                if (element.name == "Iron")
                    continue;

                if (std::abs(element.currentPercentage - element.targetPercentage) > TOLERANCE)
                {
                    changesNeeded = true;
                }
            }

            // Store iteration data
            iterationsData.push_back({iterations, elements, currentIterationAdditions, totalWeight});
        }

        std::cout << "\nFinal composition after additions:\n";
        for (const auto &element : elements)
        {
            std::cout << element.name << ": "
                      << element.currentPercentage << "% "
                      << "(Target: " << element.targetPercentage << "%)\n";
        }

        std::cout << "\nTotal weight added: " << (totalWeight - originalWeight) << " kg\n";
        std::cout << "Final batch weight: " << totalWeight << " kg\n";

        // Write all iteration data to CSV files
        CSVWriter::writeIterationData("steel_additions", iterationsData);
        std::cout << "\nDetailed results have been written to the 'output' directory.\n";
    }

    // verify that the final composition matches the target composition
    bool verifyComposition()
    {
        for (const auto &element : elements)
        {
            if (std::abs(element.currentPercentage - element.targetPercentage) > TOLERANCE)
            {
                return false;
            }
        }
        return true;
    }
};

int main()
{
    try
    {
        SteelGradeCalculator calculator(1000);
        calculator.loadFromCSV("steel_composition.csv");
        calculator.calculateAdditions();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}