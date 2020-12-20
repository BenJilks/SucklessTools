
int printf(const char *msg, ...);

void to_int(int i, unsigned u, char c, float f, double d)
{
    int i2i = (int)i;
    int u2i = (int)u;
    int c2i = (int)c;
    int f2i = (int)f;
    int d2i = (int)d;
    printf("%i %i %i %i %i%c", 
        i2i, u2i, c2i, f2i, d2i, 10);
}

void to_unsigned(int i, unsigned u, char c, float f, double d)
{
    unsigned i2u = (unsigned)i;
    unsigned u2u = (unsigned)u;
    unsigned c2u = (unsigned)c;
    unsigned f2u = (unsigned)f;
    unsigned d2u = (unsigned)d;
    printf("%u %u %u %u %u%c", 
        i2u, u2u, c2u, f2u, d2u, 10);
}

void to_char(int i, unsigned u, char c, float f, double d)
{
    char i2c = (char)i;
    char u2c = (char)u;
    char c2c = (char)c;
    char f2c = (char)f;
    char d2c = (char)d;
    printf("%i %i %i %i %i%c", 
        (int)i2c, (int)u2c, (int)c2c, (int)f2c, (int)d2c, 10);
}

void to_unsigned_char(int i, unsigned u, char c, float f, double d)
{
    unsigned char i2uc = (unsigned char)i;
    unsigned char u2uc = (unsigned char)u;
    unsigned char c2uc = (unsigned char)c;
    unsigned char f2uc = (unsigned char)f;
    unsigned char d2uc = (unsigned char)d;
    printf("%i %i %i %i %i%c", 
        (int)i2uc, (int)u2uc, (int)c2uc, (int)f2uc, (int)d2uc, 10);
}

void to_float(int i, unsigned u, char c, float f, double d)
{
    float i2f = (float)i;
    float u2f = (float)u;
    float c2f = (float)c;
    float f2f = (float)f;
    float d2f = (float)d;
    printf("%f %f %f %f %f%c", 
        i2f, u2f, c2f, f2f, d2f, 10);
}

void to_double(int i, unsigned u, char c, float f, double d)
{
    double i2d = (double)i;
    double u2d = (double)u;
    double c2d = (double)c;
    double f2d = (double)f;
    double d2d = (double)d;
    printf("%f %f %f %f %f%c", 
        i2d, u2d, c2d, f2d, d2d, 10);
}

int main() 
{
    int i = -42;
    unsigned u = (unsigned)42;
    char c = 'A';
    float f = 4.2;
    double d = (double)4.242;

    to_int(i, u, c, f, d);
    to_unsigned(i, u, c, f, d);
    to_char(i, u, c, f, d);
    to_unsigned_char(i, u, c, f, d);
    to_float(i, u, c, f, d);
    to_double(i, u, c, f, d);
    return 0;
}

