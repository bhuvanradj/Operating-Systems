// forces a pagefault exception by dereferencing kernel page from user;
int main() {
    int* a = (0x400000);
    int b = *a;
    return 0;
}
