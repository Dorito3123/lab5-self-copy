#include "levenshtein.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const int DEFAULT_BUFFER_CAPACITY = 16;
static const int INITIAL_WORDS_CAPACITY = 8;
static const int COUNT_FOR_FIND_MIN = 1e9;

static int minInt(int a, int b)
{
    return (a < b) ? a : b;
}

static int minOfThree(int a, int b, int c)
{
    return minInt(minInt(a, b), c);
}


static int **allocateTable(int rows, int cols)
{
    if (rows <= 0 || cols <= 0)
        return NULL;
    int **table = (int **)malloc((size_t)rows * sizeof(int *));
    if (!table)
        return NULL;
    for (int i = 0; i < rows; i++)
    {
        table[i] = (int *)malloc((size_t)cols * sizeof(int));
        if (!table[i])
        {
            for (int j = 0; j < i; j++)
                free(table[j]);
            free(table);
            return NULL;
        }
    }
    return table;
}

static void freeTable(int **table, int rows)
{
    if (!table)
        return;
    for (int i = 0; i < rows; i++)
        free(table[i]);
    free(table);
}

// Для отладки — возвращает имя операции
const char *operationName(OperationType type)
{
    switch (type)
    {
        case OP_NONE:    return "NONE";
        case OP_INSERT:  return "INSERT";
        case OP_DELETE:  return "DELETE";
        case OP_REPLACE: return "REPLACE";
        default:         return "UNKNOWN";
    }
}


int levenshteinDistance(const char *s1, const char *s2)
{
    if (!s1 || !s2)
        return -1;
    int len1 = (int)strlen(s1);
    int len2 = (int)strlen(s2);
    int **dp = allocateTable(len1 + 1, len2 + 1);
    if (!dp)
        return -1;
    for (int i = 0; i <= len1; i++)
        dp[i][0] = i;
    for (int j = 0; j <= len2; j++)
        dp[0][j] = j;
    for (int i = 1; i <= len1; i++)
    {
        for (int j = 1; j <= len2; j++)
        {
            if (s1[i - 1] == s2[j - 1])
            {
                dp[i][j] = dp[i - 1][j - 1];
            }
            else
            {
                dp[i][j] = 1 + minOfThree(dp[i - 1][j], dp[i][j - 1], dp[i - 1][j - 1]);
            }
        }
    }
    int answer = dp[len1][len2];
    freeTable(dp, len1 + 1);
    return answer;
}


static void reverseTypes(OperationType *arr, int n)
{
    int left = 0, right = n - 1;
    while (left < right)
    {
        OperationType tmp = arr[left];
        arr[left] = arr[right];
        arr[right] = tmp;
        left++; right--;
    }
}

EditResult *levenshteinWithOperations(const char *s1, const char *s2)
{
    if (!s1 || !s2)
        return NULL;
    int len1 = (int)strlen(s1);
    int len2 = (int)strlen(s2);
    int **dp = allocateTable(len1 + 1, len2 + 1);
    if (!dp)
        return NULL;
    for (int i = 0; i <= len1; i++)
        dp[i][0] = i;
    for (int j = 0; j <= len2; j++)
        dp[0][j] = j;
    for (int i = 1; i <= len1; i++)
    {
        for (int j = 1; j <= len2; j++)
        {
            if (s1[i - 1] == s2[j - 1])
            {
                dp[i][j] = dp[i - 1][j - 1];
            }
            else
            {
                dp[i][j] = 1 + minOfThree(dp[i - 1][j], dp[i][j - 1], dp[i - 1][j - 1]);
            }
        }
    }
    EditResult *result = (EditResult *)malloc(sizeof(EditResult));
    if (!result)
    {
        freeTable(dp, len1 + 1);
        return NULL;
    }
    result -> distance = dp[len1][len2];
    result -> operationCount = 0;
    if (result -> distance > 0)
    {
        result -> operations = malloc((size_t)result -> distance * sizeof(*result -> operations));
        if (!result -> operations)
        {
            free(result);
            freeTable(dp, len1 + 1);
            return NULL;
        }
    }
    else
    {
        result -> operations = NULL;
    }

    int capacity = len1 + len2 + 2;
    OperationType *steps_reverse = (OperationType *)malloc((size_t)capacity * sizeof(OperationType));
    if (!steps_reverse)
    {
        freeEditResult(result);
        freeTable(dp, len1 + 1);
        return NULL;
    }
    int step_count = 0;
    int i = len1, j = len2;
    while (i > 0 || j > 0)
    {
        if (i > 0 && j > 0 && s1[i - 1] == s2[j - 1] && dp[i][j] == dp[i - 1][j - 1])
        {
            steps_reverse[step_count++] = OP_NONE;
            i--; j--;
            continue;
        }
        if (i > 0 && j > 0 && dp[i][j] == dp[i - 1][j - 1] + 1)
        {
            steps_reverse[step_count++] = OP_REPLACE;
            i--; j--;
        }
        else
            if (i > 0 && dp[i][j] == dp[i - 1][j] + 1)
            {
                steps_reverse[step_count++] = OP_DELETE;
                i--;
            }
            else
                if (j > 0 && dp[i][j] == dp[i][j - 1] + 1)
                {
                    steps_reverse[step_count++] = OP_INSERT;
                    j--;
                }
                else
                {
                    int best = COUNT_FOR_FIND_MIN;
                    OperationType pick = OP_NONE;
                    if (i > 0 && j > 0)
                    {
                        int cost;
                        if (s1[i - 1] == s2[j - 1])
                        {
                            cost = dp[i - 1][j - 1];
                        }
                        else
                        {
                            cost = dp[i - 1][j - 1] + 1;
                        }
                        if (cost < best)
                        {
                            best = cost;
                            if (s1[i - 1] == s2[j - 1])
                            {
                                pick = OP_NONE;
                            }
                            else
                            {
                                pick = OP_REPLACE;
                            }
                        }
                    }
                    if (i > 0)
                    {
                        int cost = dp[i - 1][j] + 1;
                        if (cost < best)
                        {
                            best = cost;
                            pick = OP_DELETE;
                        }
                    }
                    if (j > 0)
                    {
                        int cost = dp[i][j - 1] + 1;
                        if (cost < best)
                        {
                            best = cost;
                            pick = OP_INSERT;
                        }
                    }
                    steps_reverse[step_count++] = pick;
                    if (pick == OP_NONE || pick == OP_REPLACE)
                    {
                        i--;
                        j--;
                    }
                    else
                        if (pick == OP_DELETE)
                            i--;
                        else
                            j--;
                }
    }

    reverseTypes(steps_reverse, step_count);

    int i1 = 0, j1 = 0, pos = 0, out = 0;
    for (int i = 0; i < step_count; i++) 
    {
        OperationType operation_type = steps_reverse[i];
        if (operation_type == OP_NONE)
        {
            i1++; j1++; pos++;
            continue;
        }
        EditOperation operation;
        operation.type = operation_type;
        operation.position = pos;
        operation.oldChar = '\0';
        operation.newChar = '\0';
        if (operation_type == OP_REPLACE)
        {
            operation.oldChar = s1[i1];
            operation.newChar = s2[j1];
            i1++; j1++; pos++;
        }
        else
            if (operation_type == OP_DELETE)
            {
                operation.oldChar = s1[i1];
                i1++;
            }
            else
                if (operation_type == OP_INSERT)
                {
                    operation.newChar = s2[j1];
                    j1++; pos++;
                }
        if (out < result -> distance)
        {
            result -> operations[out++] = operation;
        }
    }
    result -> operationCount = out;
    free(steps_reverse);
    freeTable(dp, len1 + 1);
    return result;
}


void printEditTable(int **dp, const char *s1, const char *s2)
{
    if (!dp || !s1 || !s2)
        return;
    int len1 = (int)strlen(s1);
    int len2 = (int)strlen(s2);
    printf("      \"\"");
    for (int j = 0; j < len2; j++)
        printf("%4c", s2[j]);
    printf("\n");
    for (int i = 0; i <= len1; i++)
    {
        if (i == 0)
            printf("  \"\" ");
        else
            printf("%4c ", s1[i - 1]);
        for (int j = 0; j <= len2; j++)
            printf("%4d", dp[i][j]);
        printf("\n");
    }
}

void printOperations(EditResult *result, const char *s1, const char *s2)
{
    (void)s1;
    (void)s2;
    if (!result)
        return;
    if (result -> operationCount == 0)
    {
        printf("операции: -\n");
        return;
    }
    printf("операции:\n");
    for (int i = 0; i < result -> operationCount; i++)
    {
        EditOperation operation = result -> operations[i];
        if (operation.type == OP_INSERT)
        {
            printf("%d вставить '%c' в %d\n", i + 1, operation.newChar, operation.position);
        }
        else
            if (operation.type == OP_DELETE)
            {
                printf("%d удалить '%c' в %d\n", i + 1, operation.oldChar, operation.position);
            }
            else
                if (operation.type == OP_REPLACE)
                {
                    printf("%d заменить '%c' на '%c' в %d\n", i + 1, operation.oldChar, operation.newChar, operation.position);
                }
                else
                {
                    printf("%d. %s\n", i + 1, operationName(operation.type));
                }
    }
}


static char *strDuplicate(const char *s)
{
    if (!s) return NULL;
    size_t n = strlen(s);
    char *copy = (char *)malloc(n + 1);
    if (!copy) return NULL;
    memcpy(copy, s, n + 1);
    return copy;
}


static int ensureCapacity(char **buffer, int *capacity, int need)
{
    if (need <= *capacity)
        return 1;
    int new_capacity;
    if (*capacity > 0)
        new_capacity = *capacity;
    else
        new_capacity = DEFAULT_BUFFER_CAPACITY;
    while (new_capacity < need)
        new_capacity *= 2;
    char *new_buffer = (char *)realloc(*buffer, (size_t)new_capacity);
    if (!new_buffer) return 0;
    *buffer = new_buffer;
    *capacity = new_capacity;
    return 1;
}


void printTransformation(EditResult *result, const char *s1, const char *s2)
{
    (void)s2;
    if (!result || !s1)
        return;
    char *current = strDuplicate(s1);
    if (!current)
        return;
    int len = (int)strlen(current);
    int capacity = len + 1;
    if (result -> operationCount == 0)
    {
        printf("преобразование %s\n", current);
        free(current);
        return;
    }
    for (int i = 0; i < result -> operationCount; i++)
    {
        EditOperation operation = result -> operations[i];
        char *before = strDuplicate(current);
        if (!before)
            break;
        if (operation.type == OP_REPLACE)
        {
            if (operation.position >= 0 && operation.position < len)
            {
                current[operation.position] = operation.newChar;
            }
        }
        else
            if (operation.type == OP_DELETE)
            {
                if (operation.position >= 0 && operation.position < len)
                {
                    memmove(current + operation.position, current + operation.position + 1, (size_t)(len - operation.position));
                    len--;
                }
            }
            else
            if (operation.type == OP_INSERT)
            {
                if (operation.position >= 0 && operation.position <= len)
                {
                    if (!ensureCapacity(&current, &capacity, len + 2))
                    {
                        free(before);
                        break;
                    }
                    memmove(current + operation.position + 1, current + operation.position, (size_t)(len - operation.position + 1));
                    current[operation.position] = operation.newChar;
                    len++;
                }
            }
        if (operation.type == OP_REPLACE)
        {
            printf("%d заменить '%c' на '%c' в %d, %s -> %s\n", i + 1, operation.oldChar, operation.newChar, operation.position, before, current);
        }
        else
            if (operation.type == OP_DELETE)
            {
            printf("%d удалить '%c' в %d, %s -> %s\n", i + 1, operation.oldChar, operation.position, before, current);
            }
            else
                if (operation.type == OP_INSERT)
                {
                    printf("%d вставить '%c' в %d, %s -> %s\n", i + 1, operation.newChar, operation.position, before, current);
                }
        free(before);
    }
    free(current);
}

char **findSimilarWords(const char *word, char **dictionary, int dictSize,
                        int maxDistance, int *resultCount)
{
    if (resultCount)
        *resultCount = 0;
    if (!word || !dictionary || dictSize <= 0 || maxDistance < 0 || !resultCount)
        return NULL;
    int capacity = INITIAL_WORDS_CAPACITY;
    int count = 0;
    char **out = (char **)malloc((size_t)capacity * sizeof(char *));
    if (!out)
        return NULL;
    for (int i = 0; i < dictSize; i++)
    {
        const char *dict_word = dictionary[i];
        if (!dict_word)
            continue;
        int distance = levenshteinDistance(word, dict_word);
        if (distance >= 0 && distance <= maxDistance)
        {
            if (count == capacity)
            {
                capacity *= 2;
                char **tmp = (char **)realloc(out, (size_t)capacity * sizeof(char *));
                if (!tmp)
                {
                    freeSimilarWords(out, count);
                    return NULL;
                }
                out = tmp;
            }
            out[count] = strDuplicate(dict_word);
            if (!out[count])
            {
                freeSimilarWords(out, count);
                return NULL;
            }
            count++;
        }
    }
    if (count == 0)
    {
        free(out);
        return NULL;
    }
    *resultCount = count;
    return out;
}

void freeSimilarWords(char **words, int count)
{
    if (!words)
        return;
    for (int i = 0; i < count; i++)
        free(words[i]);
    free(words);
}


void freeEditResult(EditResult *result)
{
    if (!result)
        return;
    free(result -> operations);
    free(result);
}
