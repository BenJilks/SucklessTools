
int printf(const char *msg, ...);

#define I  42
#define UI (unsigned)4026531840
#define C  (char)5
#define UC (unsigned char)240
#define F  (float)4.2
#define D  (double)4.242

#define OPERATION(op) \
    printf("I op I: %i ", I + I); \
    printf("%c", 10)

int main() 
{
    OPERATION(+);
}

