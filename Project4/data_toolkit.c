#include <stdio.h>
#include <stdlib.h>

//  GLOBAL DATASET 
int *dataset = NULL;
int size = 0;

//  FUNCTION DECLARATIONS 
void createDataset();
void displayDataset();
void computeSumAvg();
void findMinMax();
void filterDataset(int (*condition)(int));
void transformDataset(int (*operation)(int));
void sortDataset(int (*compare)(int, int));
void searchValue();
void saveToFile();
void loadFromFile();
void freeMemory();

//  CALLBACK FUNCTIONS 
int greaterThan20(int x) { return x > 20; }
int multiplyBy2(int x) { return x * 2; }
int ascending(int a, int b) { return a > b; }
int descending(int a, int b) { return a < b; }

//  CORE FUNCTIONS 

void createDataset() {
    printf("Enter number of elements: ");
    scanf("%d", &size);

    dataset = (int*) malloc(size * sizeof(int));
    if (dataset == NULL) {
        printf("Memory allocation failed!\n");
        return;
    }

    printf("Enter elements:\n");
    for (int i = 0; i < size; i++) {
        scanf("%d", &dataset[i]);
    }
}

void displayDataset() {
    if (dataset == NULL) {
        printf("Dataset is empty!\n");
        return;
    }

    printf("Dataset: ");
    for (int i = 0; i < size; i++) {
        printf("%d ", dataset[i]);
    }
    printf("\n");
}

void computeSumAvg() {
    if (dataset == NULL) return;

    int sum = 0;
    for (int i = 0; i < size; i++) sum += dataset[i];

    printf("Sum = %d\n", sum);
    printf("Average = %.2f\n", (float)sum / size);
}

void findMinMax() {
    if (dataset == NULL) return;

    int min = dataset[0], max = dataset[0];

    for (int i = 1; i < size; i++) {
        if (dataset[i] < min) min = dataset[i];
        if (dataset[i] > max) max = dataset[i];
    }

    printf("Min = %d, Max = %d\n", min, max);
}

void filterDataset(int (*condition)(int)) {
    if (dataset == NULL) return;

    printf("Filtered values: ");
    for (int i = 0; i < size; i++) {
        if (condition(dataset[i])) {
            printf("%d ", dataset[i]);
        }
    }
    printf("\n");
}

void transformDataset(int (*operation)(int)) {
    if (dataset == NULL) return;

    for (int i = 0; i < size; i++) {
        dataset[i] = operation(dataset[i]);
    }

    printf("Dataset transformed.\n");
}

void sortDataset(int (*compare)(int, int)) {
    if (dataset == NULL) return;

    for (int i = 0; i < size - 1; i++) {
        for (int j = 0; j < size - i - 1; j++) {
            if (compare(dataset[j], dataset[j + 1])) {
                int temp = dataset[j];
                dataset[j] = dataset[j + 1];
                dataset[j + 1] = temp;
            }
        }
    }

    printf("Dataset sorted.\n");
}

void searchValue() {
    if (dataset == NULL) return;

    int target;
    printf("Enter value to search: ");
    scanf("%d", &target);

    for (int i = 0; i < size; i++) {
        if (dataset[i] == target) {
            printf("Found at index %d\n", i);
            return;
        }
    }

    printf("Value not found.\n");
}

void saveToFile() {
    FILE *file = fopen("data.txt", "w");
    if (!file) {
        printf("Error opening file!\n");
        return;
    }

    fprintf(file, "%d\n", size);
    for (int i = 0; i < size; i++) {
        fprintf(file, "%d ", dataset[i]);
    }

    fclose(file);
    printf("Data saved to file.\n");
}

void loadFromFile() {
    FILE *file = fopen("data.txt", "r");
    if (!file) {
        printf("File not found!\n");
        return;
    }

    fscanf(file, "%d", &size);
    dataset = (int*) realloc(dataset, size * sizeof(int));

    for (int i = 0; i < size; i++) {
        fscanf(file, "%d", &dataset[i]);
    }

    fclose(file);
    printf("Data loaded from file.\n");
}

void freeMemory() {
    free(dataset);
}

// MAIN MENU

int main() {
    int choice;

    do {
        printf("\n--- Data Analysis Toolkit ---\n");
        printf("1. Create Dataset\n");
        printf("2. Display Dataset\n");
        printf("3. Sum & Average\n");
        printf("4. Min & Max\n");
        printf("5. Filter (>20)\n");
        printf("6. Transform (*2)\n");
        printf("7. Sort Ascending\n");
        printf("8. Sort Descending\n");
        printf("9. Search Value\n");
        printf("10. Save to File\n");
        printf("11. Load from File\n");
        printf("12. Exit\n");
        printf("Choice: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1: createDataset(); break;
            case 2: displayDataset(); break;
            case 3: computeSumAvg(); break;
            case 4: findMinMax(); break;
            case 5: filterDataset(greaterThan20); break;
            case 6: transformDataset(multiplyBy2); break;
            case 7: sortDataset(ascending); break;
            case 8: sortDataset(descending); break;
            case 9: searchValue(); break;
            case 10: saveToFile(); break;
            case 11: loadFromFile(); break;
            case 12: freeMemory(); printf("Exiting...\n"); break;
            default: printf("Invalid choice!\n");
        }

    } while (choice != 12);

    return 0;
}
