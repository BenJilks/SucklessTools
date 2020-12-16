
int printf(const char *msg, ...);

int main() 
{
    // Signed int
    int i = -42;
    printf("%i%c", i, 10);
    if (i > 0)
        return 1;

    // Unsigned int
    unsigned u = (unsigned)i;
    printf("%u%c", u, 10);
    if (u < 0)
        return 1;

    // Char
    char c = (char)(255 + 2);
    printf("%i%c", (int)c, 10);

    // Float
    float f = 4.2;
    printf("%.6g%c", f, 10);
    
    // Double
    double d = (double)4.242;
    printf("%.6g%c", d, 10);

    // Cast to int
    int c2i = (int)c;
    int f2i = (int)f;
    int d2i = (int)d;
    printf("%i %i %i%c", c2i, f2i, d2i, 10);

    return 0;
}

