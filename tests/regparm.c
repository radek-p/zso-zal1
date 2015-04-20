__attribute__((regparm(3)))
int add(int a, int b, int c) {
	return a + b + c;
}

__attribute__((regparm(3)))
int xor(int a, int b, int c) {
	return a ^ b ^ c;
}
