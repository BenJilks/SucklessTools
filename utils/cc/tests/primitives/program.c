
int printf(const char *msg, ...);

int main() 
{
    char c = 'A';
    int i = 42;
    float f = 4.2;
    const char *str = "Hello, World!";
    const char *multi_string = 
        "This " "is " "made "
        "from " "multiple " "strings";

    printf("%c %i %f %s %s",
        (int)c, i, f, str, multi_string);
}

