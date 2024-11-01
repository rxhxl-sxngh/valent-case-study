#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <iomanip>
#include <algorithm>

struct Element
{
    std::string name;
    double currentPercentage;
    double targetPercentage;
    double currentWeight;
};

class SteelGradeCalculator
{
private:
    double totalWeight;
    std::vector<Element> elements;
    // 0.1% tolerance (in the real world, some elements in the initial composition may not be wanted in the final spec)
    // As long as the difference is within this tolerance, we consider the composition to be acceptable.
    const double TOLERANCE = 0.001;

    void updatePercentages()
    {
        for (auto &element : elements)
        {
            element.currentPercentage = (element.currentWeight / totalWeight) * 100.0;
        }
    }

public:
    SteelGradeCalculator(double weight) : totalWeight(weight) {}

    void addElement(const std::string &name, double current, double target)
    {
        double currentWeight = (current / 100.0) * totalWeight;
        elements.push_back({name, current, target, currentWeight});
    }

    void calculateAdditions()
    {
        // Print initial composition
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

            for (auto &element : elements)
            {
                if (element.name == "Iron")
                    continue; // Skip iron as it's the balance

                double targetWeight = (element.targetPercentage / 100.0) * totalWeight;
                if (element.currentWeight < targetWeight)
                {
                    double addition = targetWeight - element.currentWeight;

                    // Add a small buffer to account for future dilution
                    addition *= 1.05;

                    if (addition > 0.01)
                    { // Only add if the addition is significant
                        std::cout << "Add " << addition << " kg of " << element.name << "\n";
                        element.currentWeight += addition;
                        totalWeight += addition;
                        changesNeeded = true;
                    }
                }
            }

            // Update iron weight to maintain mass balance
            auto &iron = *std::find_if(elements.begin(), elements.end(),
                                       [](const Element &e)
                                       { return e.name == "Iron"; });
            iron.currentWeight = iron.currentWeight; // Iron weight stays the same

            // Recalculate all percentages
            updatePercentages();
        }

        // Print final composition
        std::cout << "\nFinal composition after additions:\n";
        for (const auto &element : elements)
        {
            std::cout << element.name << ": "
                      << element.currentPercentage << "% "
                      << "(Target: " << element.targetPercentage << "%)\n";
        }

        std::cout << "\nTotal weight added: " << (totalWeight - originalWeight) << " kg\n";
        std::cout << "Final batch weight: " << totalWeight << " kg\n";
    }

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
    // Initialize calculator with initial batch weight (whatever the weight of the ladle is)
    SteelGradeCalculator calculator(1000);

    // Add elements with initial and target percentages for 316 SS
    calculator.addElement("Chromium", 14.79, 17.00);
    calculator.addElement("Nickel", 2.00, 12.00);
    calculator.addElement("Molybdenum", 0.50, 2.50);
    calculator.addElement("Carbon", 0.01, 0.08);
    calculator.addElement("Manganese", 1.00, 2.00);
    calculator.addElement("Phosphorus", 0.03, 0.05);
    calculator.addElement("Sulfur", 0.02, 0.03);
    calculator.addElement("Silicon", 0.60, 0.75);
    calculator.addElement("Nitrogen", 0.05, 0.10);
    calculator.addElement("Iron", 81.00, 65.49);

    // Calculate required additions
    calculator.calculateAdditions();

    return 0;
}