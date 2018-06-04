#include <stdio.h>
#include <math.h>

#define N 16
#define N2 136
#define N4 3876
#define N_E2 (N * N)
#define N_E3 (N * N * N)
#define N_E4 (N * N * N * N)

template<class T>
inline void swap(T & a, T & b) {
	T c = a;
	a = b;
	b = c;
}

template<class T>
inline T max(T & a, T & b, T & c, T & d, T & e, T & f) {
	T m1 = a > b ? a : b;
	T m2 = c > d ? c : d;
	T m3 = e > f ? e : f;
	m1 = m1 > m2 ? m1 : m2;
	return m1 > m3 ? m1 : m3;
}

static int ReloadConfigOnEachMove;

static int ExtensionCoefficient;
static int FutilityMargin[4];
static int RazoringMargin[4];

static short E1[N];
static short S1[N];

static short E2[N2];
static short S2[N2];
static short E2Max[N2];
static short S2Max[N2];

static short Score[N4];
static short Eval[N4];

void genEval() {
	int v[N_E4] = { -1 };

	for (int x = 0, i = 0; x < N; x++) {
		for (int y = 0; y < N; y++) {
			for (int z = 0; z < N; z++) {
				for (int w = 0; w < N; w++) {
					int a = x, b = y, c = z, d = w;
					if (b > a) swap(a, b);
					if (c > a) swap(a, c);
					if (d > a) swap(a, d);
					if (c > b) swap(c, b);
					if (d > b) swap(d, b);
					if (d > c) swap(d, c);

					v[i++] = a + b * N + c * N_E2 + d * N_E3;
				}
			}
		}
	}

	for (int i = 0, sum = N_E4; i < N_E4; i++)
		if (v[i] > -1) {
			for (int j = i + 1; j < N_E4; j++)
				if (v[i] == v[j]) {
					v[j] = -1;
					sum--;
				}
		}
	int count = 0;
	for (int i = 0; i < N_E4; i++) {
		if (v[i] > -1) v[i] = count++;
	}

	// (n + m - 1)! / ((n - 1)! * m!)
	auto comb = [](unsigned int n, unsigned int m) -> int {
		unsigned long long a = 1;
		for (unsigned int i = n; i < m + n; i++) a *= i;
		for (unsigned int i = 2; i <= m; i++) a /= i;
		return (int)a;
	};

	// y > x
#define I(x, y) ((y) * N + (x) - comb(y, 2))

	for (int i = 0; i < N; i++)
		for (int j = 0; j < N; j++)
			for (int m = 0; m < N; m++)
				for (int n = 0; n < N; n++) {
					int a = i, b = j, c = m, d = n;  // a > b > c > d
					if (b > a) swap(a, b);
					if (c > a) swap(a, c);
					if (d > a) swap(a, d);
					if (c > b) swap(c, b);
					if (d > b) swap(d, b);
					if (d > c) swap(d, c);

					int pcode = a + b * N + c * N_E2 + d * N_E3;
					pcode = v[pcode];

					Eval[pcode]  = E1[a] + E1[b] + E1[c] + E1[d];
					Score[pcode] = S1[a] + S1[b] + S1[c] + S1[d];

					Eval[pcode] +=
						  E2[I(a, b)] + E2[I(a, c)] + E2[I(a, d)]
						+ E2[I(b, c)] + E2[I(b, d)] + E2[I(c, d)];

					Score[pcode] +=
						  S2[I(a, b)] + S2[I(a, c)] + S2[I(a, d)]
						+ S2[I(b, c)] + S2[I(b, d)] + S2[I(c, d)];

					Eval[pcode] += max(
						E2Max[I(a, b)], E2Max[I(a, c)], E2Max[I(a, d)], 
					    E2Max[I(b, c)], E2Max[I(b, d)], E2Max[I(c, d)]
					);

					Score[pcode] += max(
						S2Max[I(a, b)], S2Max[I(a, c)], S2Max[I(a, d)], 
						S2Max[I(b, c)], S2Max[I(b, d)], S2Max[I(c, d)]
					);
				}
}

int main(int argc, char *argv[]) {
	if (argc != 3) return -1;

	FILE *fp;
	if (fopen_s(&fp, argv[1], "r")) return -2;

	fscanf_s(fp, "%d", &ReloadConfigOnEachMove);
	fscanf_s(fp, "%d", &ExtensionCoefficient);
	fscanf_s(fp, "%d%d%d%d", FutilityMargin, FutilityMargin + 1, FutilityMargin + 2, FutilityMargin + 3);
	fscanf_s(fp, "%d%d%d%d", RazoringMargin, RazoringMargin + 1, RazoringMargin + 2, RazoringMargin + 3);
	
	for (int i = 0; i < N; i++)
		fscanf_s(fp, "%hd", E1 + i);
	for (int i = 0; i < N; i++)
		fscanf_s(fp, "%hd", S1 + i);

	for (int i = 0; i < N2; i++)
		fscanf_s(fp, "%hd", E2 + i);
	for (int i = 0; i < N2; i++)
		fscanf_s(fp, "%hd", S2 + i);

	for (int i = 0; i < N2; i++)
		fscanf_s(fp, "%hd", E2Max + i);
	for (int i = 0; i < N2; i++)
		fscanf_s(fp, "%hd", S2Max + i);

	fclose(fp);

	genEval();

	if (fopen_s(&fp, argv[2], "w")) return -2;

	fprintf_s(fp, "Override: 1\n");
	fprintf_s(fp, "UseOpeningBook: 1\n");
	fprintf_s(fp, "ReloadConfigOnEachMove: %d\n", ReloadConfigOnEachMove);
	fprintf_s(fp, "ExtensionCoefficient: %d\n", ExtensionCoefficient);
	fprintf_s(fp, "FutilityPurningMargin: %d %d %d %d\n", FutilityMargin[0], 
		FutilityMargin[1], FutilityMargin[2], FutilityMargin[3]);
	fprintf_s(fp, "RazoringMargin: %d %d %d %d\n", RazoringMargin[0],
		RazoringMargin[1], RazoringMargin[2], RazoringMargin[3]);
	fprintf_s(fp, "SEBetaMargin: 3.0\n");

	fprintf_s(fp, "Eval:\n");
	for (int i = 0; i < N4; i++)
		fprintf_s(fp, i % N == (N - 1) ? "%6hd\n" : "%6hd", Eval[i]);
	fprintf_s(fp, "\nScore:\n");
	for (int i = 0; i < N4; i++)
		fprintf_s(fp, i % N == (N - 1) ? "%6hd\n" : "%6hd", Score[i]);

	fclose(fp);

	printf("config generated.\n");

	return 0;
}