typedef char mystring[256];
const int MAX = 100;
const float PI = 3.14159;
const char NL = '\n';
const string MSG = "hello world";
enum color
{
    red = 1,
    green,
    blue
};

int hex_val = 0XFF;
int oct_val = 0O77;
int bin_val = 0B1010;
int dec_val = 255;
float f1 = 3.14;
float f2 = 1.5E2;
float f3 = .5e-3;

int g_count = 0;
float g_ratio = 1.0;

int add(int a, int b)
{
    int r;
    r = a + b;
    return r;
}

int main()
{
    int a;
    int b;
    int sum;
    int i;
    float f;
    char c;
    int arr[10];

    cin >> a >> b;
    cout << MSG;

    /* Κληση συναρτησης */
    sum = add(a, b);
    cout << sum;

    /* Cast int->float */
    f = a;

    /* Αριθμητικες πραξεις */
    sum = a + b;
    sum = a - b;
    sum = a * b;
    sum = a / b;
    sum = a % b;

    /* Συγκρισεις */
    if (a == b)
        sum = 1;
    if (a != b)
        sum = 2;
    if (a > b)
        sum = 3;
    if (a < b)
        sum = 4;
    if (a >= b)
        sum = 5;
    if (a <= b)
        sum = 6;
    if (a && b)
        sum = 7;
    if (a || b)
        sum = 8;
    if (!a)
        sum = 9;

    /* Αυξηση/μειωση */
    a++;
    a--;
    ++b;
    --b;

    /* while */
    i = 0;
    sum = 0;
    while (i < 10)
    {
        sum = sum + i;
        i++;
    }

    /* for */
    for (i = 0; i < 5; i++)
    {
        sum = sum + i;
    }

    /* if-else */
    if (sum > 100)
    {
        sum = 100;
    }
    else
    {
        sum = sum + 1;
    }

    /* Πινακας */
    arr[0] = 42;
    cout << arr[0];

    /* #1 Αποτιμηση σταθερων */
    sum = 2 + 3;
    sum = sum * 0;
    sum = sum + 0;
    sum = sum * 1;

    /* #4 Διαδοση αντιγραφου */
    a = 42;
    b = a;
    cout << b;

    /* #3 Αχρηστος κωδικας */
    if (0)
    {
        sum = 999;
    }
    while (0)
    {
        sum = 999;
    }

    return 0;
}