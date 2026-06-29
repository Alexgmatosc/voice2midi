#include <JuceHeader.h>

int main (int argc, char* argv[])
{
    juce::UnitTestRunner runner;
    // Runs all registered juce::UnitTest subclasses
    runner.runAllTests();
    
    int failedCount = 0;
    for (int i = 0; i < runner.getNumResults(); ++i)
    {
        auto* result = runner.getResult(i);
        failedCount += result->failures;
    }
    
    return (failedCount > 0) ? 1 : 0;
}
