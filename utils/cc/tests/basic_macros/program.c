
int printf(const char *msg, ...);

#define FORTY_TWO 42

#define FUNC(x) \
    x * x

#define MID_EXPRESSION_FUNC_WITHIN_FUNC(x) \
    FUNC(x) + FUNC(x)

#define ARGUMENTS_WITHIN_STRING(x) \
    "This should not replace x, as it's in a string"

int main()
{
    printf("%i %i %i %s%c", 
        FORTY_TWO,
        FUNC(FORTY_TWO),
        MID_EXPRESSION_FUNC_WITHIN_FUNC(FORTY_TWO),
        ARGUMENTS_WITHIN_STRING(oops),
        10);
}

