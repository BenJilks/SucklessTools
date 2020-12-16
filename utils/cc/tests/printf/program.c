
int printf(const char *msg, ...);

int main() 
{
    printf("Hello World!%c", 10);
    printf("%i %f %c %.6g%c", 42, 4.2, 'A', 4.2, 10);
    printf("%10i%c", 42, 10);
    printf("%10s%c", "Hello", 10);
}

