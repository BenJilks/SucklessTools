
int bar()
{
    return 42;
}

int foo()
{
    return bar();
}

int main() 
{
    return foo();
}

