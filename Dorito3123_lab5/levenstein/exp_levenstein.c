#include "levenshtein.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static double nowSeconds(void)
{
    return (double)clock() / CLOCKS_PER_SEC;
}

static int minIntLocal(int a, int b)
{
    return (a < b) ? a : b;
}

static int minOfThreeLocal(int a, int b, int c)
{
    return minIntLocal(minIntLocal(a, b), c);
}

static int **buildEditTable(const char *s1, const char *s2)
{
    int len1 = (int)strlen(s1);
    int len2 = (int)strlen(s2);
    int **dp = (int **)malloc((size_t)(len1 + 1) * sizeof(int *));
    if (!dp)
        return NULL;
    for (int i = 0; i <= len1; i++)
    {
        dp[i] = (int *)malloc((size_t)(len2 + 1) * sizeof(int));
        if (!dp[i])
        {
            for (int k = 0; k < i; k++)
                free(dp[k]);
            free(dp);
            return NULL;
        }
    }
    for (int i = 0; i <= len1; i++)
        dp[i][0] = i;
    for (int j = 0; j <= len2; j++)
        dp[0][j] = j;
    for (int i = 1; i <= len1; i++)
    {
        for (int j = 1; j <= len2; j++)
        {
            if (s1[i - 1] == s2[j - 1])
                dp[i][j] = dp[i - 1][j - 1];
            else
                dp[i][j] = 1 + minOfThreeLocal(dp[i - 1][j], dp[i][j - 1], dp[i - 1][j - 1]);
        }
    }
    return dp;
}

static void freeEditTableLocal(int **dp, int rows)
{
    if (!dp)
        return;
    for (int i = 0; i < rows; i++)
        free(dp[i]);
    free(dp);
}

static void dpTableDemo(void)
{
    const char *s1 = "kitten";
    const char *s2 = "sitting";
    int **dp = buildEditTable(s1, s2);
    if (!dp)
    {
        printf("dp table demo: error\n");
        return;
    }
    printf("\n");
    printf("dp table demo\n");
    printf("%s -> %s\n", s1, s2);
    printEditTable(dp, s1, s2);
    EditResult *result = levenshteinWithOperations(s1, s2);
    if (result)
    {
        printf("distance = %d\n", result->distance);
        printOperations(result, s1, s2);
        printTransformation(result, s1, s2);
        freeEditResult(result);
    }
    freeEditTableLocal(dp, (int)strlen(s1) + 1);
}


static char *randomString(int len)
{
    static const char alphabet[] = "abcdefghijklmnopqrstuvwxyz";
    char *s = (char *)malloc((size_t)len + 1);
    if (!s)
        return NULL;
    for (int i = 0; i < len; i++)
        s[i] = alphabet[rand() % (int)(sizeof(alphabet) - 1)];
    s[len] = '\0';
    return s;
}

static int repeatsForLength(int len)
{
    if (len <= 10)
        return 5000;
    if (len <= 100)
        return 1000;
    if (len <= 1000)
        return 50;
    return 1;
}

static void benchmarkLengths(void)
{
    int lengths[] = {10, 100, 1000, 10000};
    printf("length,avg_time_s\n");
    for (int i = 0; i < 4; i++)
    {
        int len = lengths[i];
        int repeats = repeatsForLength(len);
        char *a = randomString(len);
        char *b = randomString(len);
        if (!a || !b)
        {
            free(a);
            free(b);
            printf("%d,\n", len);
            continue;
        }
        volatile int sink = 0;
        double t1 = nowSeconds();
        for (int j = 0; j < repeats; j++)
            sink += levenshteinDistance(a, b);
        double t2 = nowSeconds();
        printf("%d,%.9f\n", len, (t2 - t1) / repeats);
        (void)sink;
        free(a);
        free(b);
    }
}

static void typoCorrectionDemo(void)
{
    struct Pair
    {
        const char *wrong;
        const char *correct;
    };
    struct Pair tests[] =
    {
        {"recieve", "receive"},
        {"algoritm", "algorithm"},
        {"teh", "the"},
        {"speling", "spelling"},
        {"levenshtain", "levenshtein"}
    };
    int count = (int)(sizeof(tests) / sizeof(tests[0]));
    printf("\n");
    printf("typo examples\n");
    for (int i = 0; i < count; i++)
    {
        EditResult *result = levenshteinWithOperations(tests[i].wrong, tests[i].correct);
        if (!result)
        {
            printf("%s -> %s : error\n", tests[i].wrong, tests[i].correct);
            continue;
        }
        printf("%s -> %s\n", tests[i].wrong, tests[i].correct);
        printf("distance = %d\n", result -> distance);
        printOperations(result, tests[i].wrong, tests[i].correct);
        printTransformation(result, tests[i].wrong, tests[i].correct);
        printf("\n");
        freeEditResult(result);
    }
}

static void similarNamesDemo(void)
{
    char *nameDatabase[] =
    {
        "Anastasia", "Anastasiya", "Anna", "Ann",
        "Ekaterina", "Katerina", "Catherine", "Kathryn",
        "Dmitry", "Dimitri", "Alexander", "Alexandra",
        "Sofia", "Sonya", "Sonia", "Maria", "Mariya"
    };
    const int nameDatabaseSize = (int)(sizeof(nameDatabase) / sizeof(nameDatabase[0]));
    const char *queries[] = {"Anastasya", "Katerin", "Dmitriy", "Soniya"};
    const int queriesSize = (int)(sizeof(queries) / sizeof(queries[0]));
    printf("similar names\n");
    for (int i = 0; i < queriesSize; i++)
    {
        int found = 0;
        char **result = findSimilarWords(queries[i], nameDatabase, nameDatabaseSize, 2, &found);
        printf("%s ->\n", queries[i]);
        if (!result || found == 0)
        {
            printf("not found\n\n");
            freeSimilarWords(result, found);
            continue;
        }
        for (int j = 0; j < found; j++)
        {
            int dist = levenshteinDistance(queries[i], result[j]);
            printf("%s -> %s, distance = %d\n", queries[i], result[j], dist);
        }
        printf("\n");
        freeSimilarWords(result, found);
    }
}

int main(void)
{
    srand((unsigned)time(NULL));
    benchmarkLengths();
    dpTableDemo();
    typoCorrectionDemo();
    similarNamesDemo();
    return 0;
}