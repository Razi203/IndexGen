# DNA Codebook Generator

## How to Use the Project

### Step 1: Compilation

You will need a C++ compiler that supports C++17 (like g++) and the `make` build tool. Navigate to the project directory in your terminal and run the `make` command. This will compile all source files and create an executable file named `IndexGen`.

```bash
# Navigate to the project directory
cd /path/to/IndexGen

# Compile the project
make
```

### Step 2: Running the Generator

The application is configured and run entirely through command-line arguments.

```bash
# Run the generator with default settings
./IndexGen

# Run with custom parameters
./IndexGen --lenStart 12 --lenEnd 15 --editDist 5 --method VTCode --vt_a 1 --vt_b 2
```

The program will create a new directory with a timestamped name (e.g., `2023-10-27_10-30`) to store the output files and progress. You can specify a custom directory using the `--dir` flag.

### Step 3: Command-Line Arguments

Run ```./IndexGen -h``` or ```./IndexGen --help``` for the full list of arguments

### Step 4: Resuming an Interrupted Job

If a long-running job is stopped, you can resume it from the last saved checkpoint. Use the `--resume` flag and specify the directory containing the progress files with the `--dir` flag.

```bash
# Example: Resume the job located in the '2023-10-27_10-30' directory
./IndexGen --resume --dir 2023-10-27_10-30
```

-----

## Extending with New Methods

The application uses a **Strategy Pattern**, making it easy to add new candidate generation methods. To add a new method called `NEW_METHOD`, follow these steps.

### Step 1: Define the Method Identifier

Add a unique identifier for your new method to the `GenerationMethod` enum.

  * **File**: `IndexGen.hpp`
  * **Action**: Add `NEW_METHOD` to the `enum class`.

<!-- end list -->

```cpp
enum class GenerationMethod
{
    LINEAR_CODE,
    RANDOM,
    ALL_STRINGS,
    .
    .
    .
    NEW_METHOD // <-- Add your new method here
};
```

### Step 2: Create a New Constraints Struct

Create a `struct` to hold any parameters specific to your new method. This struct must inherit from `GenerationConstraints`.

  * **File**: `IndexGen.hpp`
  * **Action**: Define `NewMethodConstraints`.

<!-- end list -->

```cpp
struct NewMethodConstraints : public GenerationConstraints
{
    // Add any parameters your method needs
    int new_parameter_1;
    double new_parameter_2;

    // Create a constructor for easy initialization
    NewMethodConstraints(int p1, double p2)
        : GenerationConstraints(), new_parameter_1(p1), new_parameter_2(p2) {}
};
```

### Step 3: Implement the Generation Logic

Add a new `case` to the `switch` statement in the `Candidates` function to call your generation logic.

  * **File**: `Candidates.cpp`
  * **Action**: Add a `case` for `NEW_METHOD` inside the `Candidates` function.

<!-- end list -->

```cpp
// In the Candidates function...
switch (params.method)
{
    // ... (existing cases)

    case GenerationMethod::NEW_METHOD: {
        auto* constraints = dynamic_cast<NewMethodConstraints*>(params.constraints.get());
        if (!constraints) {
            throw std::runtime_error("Invalid constraints for NEW_METHOD.");
        }
        // Call your new generation function with its specific parameters
        unfiltered = GenNewMethodStrings(params.codeLen, constraints->new_parameter_1, constraints->new_parameter_2);
        break;
    }
}
```

### Step 4: Update Utility Functions (I/O)

To ensure your new method's parameters can be **saved, loaded, and printed** correctly, you must update the I/O functions in `Utils.cpp`.

  * **File**: `Utils.cpp`
  * **Action**: Add a `case` for `NEW_METHOD` to all relevant `switch` statements.

#### 4.1. Serialization (Saving and Loading)

Update `ParamsToFile` to write your new parameters to the progress file and `FileToParams` to read them back.

  * In `ParamsToFile`:
    ```cpp
    // ... inside the switch statement
    case GenerationMethod::NEW_METHOD: {
        auto* constraints = dynamic_cast<NewMethodConstraints*>(params.constraints.get());
        if (constraints) {
            output_file << constraints->new_parameter_1 << '\n';
            output_file << constraints->new_parameter_2 << '\n';
        }
        break;
    }
    ```
  * In `FileToParams`:
    ```cpp
    // ... inside the switch statement
    case GenerationMethod::NEW_METHOD: {
        int p1;
        double p2;
        input_file >> p1;
        input_file >> p2;
        params.constraints = std::make_unique<NewMethodConstraints>(p1, p2);
        break;
    }
    ```

#### 4.2. Printing and Logging

Update the helper functions that print parameters to the console or log files. This is crucial for debugging and keeping records.

  * In `GenerationMethodToString`:
    ```cpp
    // ... inside the switch statement
    case GenerationMethod::NEW_METHOD:
        return "New Method Name";
    ```
  * In `PrintParamsToFile` and `PrintTestParams`:
    ```cpp
    // ... inside the switch statement
    case GenerationMethod::NEW_METHOD: {
        auto *constraints = dynamic_cast<NewMethodConstraints *>(params.constraints.get());
        if (constraints) {
            output_file << "New Parameter 1:\t\t" << constraints->new_parameter_1 << endl;
            output_file << "New Parameter 2:\t\t" << constraints->new_parameter_2 << endl;
        }
        break;
    }
    ```

### Step 5: Add Command-Line Options

Finally, make your new method configurable from the command line.

  * **File**: `IndexGen.cpp`
  * **Action**: Update the `configure_parser` and `main` functions.

<!-- end list -->

1.  **Add the options** in `configure_parser`:
    ```cpp
    void configure_parser(cxxopts::Options &options)
    {
        options.add_options()
            // ... (existing options)
            ("new_param_1", "Description for new parameter 1", cxxopts::value<int>()->default_value("0"))
            ("new_param_2", "Description for new parameter 2", cxxopts::value<double>()->default_value("0.0"));
    }
    ```
2.  **Handle the new method** in `main`. Add an `else if` block to parse the new parameters and create your custom constraints object.
    ```cpp
    // In main()...
    string method_str = result["method"].as<string>();

    if (method_str == "LinearCode") {
        // ...
    }
    // ... (other methods)
    else if (method_str == "NEW_METHOD")
    {
        params.method = GenerationMethod::NEW_METHOD;
        int p1 = result["new_param_1"].as<int>();
        double p2 = result["new_param_2"].as<double>();
        params.constraints = make_unique<NewMethodConstraints>(p1, p2);
        cout << "Using Generation Method: NEW_METHOD (p1=" << p1 << ", p2=" << p2 << ")" << endl;
    }
    else
    {
        cerr << "Error: Unknown generation method '" << method_str << "'." << endl;
        return 1;
    }
    ```