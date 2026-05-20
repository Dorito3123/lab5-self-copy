#ifndef LEVENSHTEIN_H
#define LEVENSHTEIN_H

typedef enum {
    OP_NONE,
    OP_INSERT,
    OP_DELETE,
    OP_REPLACE
} OperationType;

typedef struct {
    OperationType type;
    int position;
    char oldChar;
    char newChar;
} EditOperation;

typedef struct {
    int distance;
    EditOperation *operations;
    int operationCount;
} EditResult;

int levenshteinDistance(const char *s1, const char *s2);
EditResult *levenshteinWithOperations(const char *s1, const char *s2);

void printEditTable(int **dp, const char *s1, const char *s2);
void printOperations(EditResult *result, const char *s1, const char *s2);
void printTransformation(EditResult *result, const char *s1, const char *s2);

char **findSimilarWords(const char *word, char **dictionary, int dictSize,
                        int maxDistance, int *resultCount);
void freeSimilarWords(char **words, int count);
void freeEditResult(EditResult *result);

const char *operationName(OperationType type);

#endif
