
int printf(const char *msg, ...);

int main() 
{
    char c = 'A';
    int i = 42;
    float f = 4.2;
    const char *str = "Hello, World!";

    printf("%c %i %f %s",
        (int)c, i, f, str);
}

